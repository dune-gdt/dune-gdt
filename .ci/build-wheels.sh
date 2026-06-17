#!/bin/bash -l
set -exuo pipefail

WHEEL_DIR="${DUNE_SRC_DIR}/${WHEELDIR_RELATIVE}"
# Python version might have qoutes around it
PYTHON_BIN="python${PYTHON_VERSION//\"/}"

export CCACHE_DIR="${WHEEL_DIR}/cache"
# Hash compiler contents rather than mtime/path so the cache stays valid even
# when the pinned manylinux image is refreshed (a fresh pull changes file
# timestamps but not the compiler binary).
export CCACHE_COMPILERCHECK=content
# The release bindings are template- and LTO-heavy; their object cache outgrows
# ccache's 5 GB default. The runner volume has ample headroom (volume=150gb).
export CCACHE_MAXSIZE=10G
# Create final wheel dir, but not tmp here
mkdir -p "${WHEEL_DIR}/final" || true

# Create a temporary directory for wheel building
TMP_WHEEL_DIR=$(mktemp -d -p "${WHEEL_DIR}" tmp.XXXXXXXXXX)

cleanup() {
    rm -rf "${TMP_WHEEL_DIR}"
}
trap cleanup EXIT

# otherwise versioneer fails on mounted source directories in CI
git config --global --add safe.directory "${DUNE_SRC_DIR}"
# vcpkg runs `git fetch` as root inside .vcpkg-root/downloads/git-tmp, but when
# .vcpkg-root is restored from the actions/cache build-tree cache it is owned by
# the runner user -> git aborts with "fatal: detected dubious ownership" (exit
# 128) on the first dependency fetch. Trust all repo dirs; the container is
# ephemeral and single-purpose, so disabling the owner check here is safe.
git config --global --add safe.directory '*'

yum install -y curl zip unzip tar ccache

# some of our vcpkg deps don't build with cmake 4 yet
# the container's cmake is managed with pipx already
pipx install --force "cmake<4"

"${PYTHON_BIN}" -m venv "${WHEEL_DIR}/venv"
# shellcheck disable=SC1091
. "${WHEEL_DIR}/venv/bin/activate"
"${PYTHON_BIN}" -m pip install auditwheel wheel build
cd "${DUNE_SRC_DIR}"

# --- vcpkg github fetch: diagnose + authenticate -----------------------------
# vcpkg pulls several dune-* deps via `git fetch` from github.com during configure.
# On the RunsOn/AWS fleet these fail (exit 128) on the first real network fetch
# while succeeding from developer machines -- consistent with GitHub throttling
# unauthenticated git from cloud IPs. Probe reachability before vcpkg runs. The
# output is tee'd to a host-visible file because the job log is only retrievable
# as a tail, and vcpkg's verbose bootstrap pushes this early output out of that
# window; the workflow "Dump" step cats this file on failure.
PROBE_LOG="${WHEEL_DIR}/github-probe.log"
{
  echo "=== github fetch probe: dune-mirrors/dune-geometry (in-container) ==="
  echo "git: $(git --version 2>&1)"
  echo "GITHUB_TOKEN present: $([ -n "${GITHUB_TOKEN:-}" ] && echo yes || echo no)"
  echo "--- unauthenticated git ls-remote (first refs) ---"
  git ls-remote https://github.com/dune-mirrors/dune-geometry.git 2>&1 | head -3 || true
  curl -sS -o /dev/null -w 'curl info/refs http=%{http_code}\n' \
    "https://github.com/dune-mirrors/dune-geometry.git/info/refs?service=git-upload-pack" 2>&1 || true
  echo "===================================================================="
} 2>&1 | tee "${PROBE_LOG}"
# Authenticate github fetches when a token is available so vcpkg uses the (much
# higher) authenticated rate limit. set +x so the token is never echoed; then
# re-probe through the authenticated path to confirm it took effect.
set +x
if [ -n "${GITHUB_TOKEN:-}" ]; then
  git config --global \
    url."https://x-access-token:${GITHUB_TOKEN}@github.com/".insteadOf \
    "https://github.com/"
  set -x
  { echo "--- authenticated git ls-remote (first refs) ---"
    git ls-remote https://github.com/dune-mirrors/dune-geometry.git 2>&1 | head -3 || true
    echo "====================================================================" ; } 2>&1 | tee -a "${PROBE_LOG}"
else
  set -x
fi

# Route every compile through ccache via the compiler launcher rather than
# PATH symlinks: the manylinux container puts gcc-toolset ahead of /usr/local/bin
# in PATH, so ccache symlinks there are never reached. Passing the launcher as a
# cache variable (-D) also forces CMake to regenerate the Ninja rules when a
# cached build/wheelbuilder-release tree is restored. Mirrors the build-linux job.
# CMAKE_CXX_SCAN_FOR_MODULES=OFF: the manylinux image only ships ccache 3.7.7
# (EPEL), which predates C++20 module scanning and corrupts the P1689 scan
# command (-fmodules-ts -fdeps-format=p1689r5 -MD) -> "cc1plus: error: to
# generate dependencies you must specify either '-M' or '-MM'". dune-gdt does
# not use C++20 modules, so disabling the scan is safe and also drops a
# redundant build step. (build-linux keeps scanning on; its apt ccache is 4.x.)
cmake --preset wheelbuilder-release -DDXT_DONT_LINK_PYTHON_LIB=1 \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_SCAN_FOR_MODULES=OFF
cmake --build --preset wheelbuilder-release --target bindings -- -j "$(nproc --ignore 1)" -l "$(nproc --ignore 1)"

DUNE_BUILD_DIR="${DUNE_SRC_DIR}/build/wheelbuilder-release"
"${PYTHON_BIN}" -m pip wheel "${DUNE_BUILD_DIR}/python/xt/" -w "${TMP_WHEEL_DIR}"
# xt is an exact-version dependency of gdt -> needs `--find-links`
"${PYTHON_BIN}" -m pip install "${TMP_WHEEL_DIR}"/dune_xt*whl
"${PYTHON_BIN}" -m pip wheel "${DUNE_BUILD_DIR}/python/gdt/" -w "${TMP_WHEEL_DIR}" --find-links "${TMP_WHEEL_DIR}/"
# Bundle external shared libraries into the wheels
"${PYTHON_BIN}" -m auditwheel repair --plat "${PLATFORM}" "${TMP_WHEEL_DIR}"/*xt*.whl -w "${WHEEL_DIR}/final"
"${PYTHON_BIN}" -m auditwheel repair --plat "${PLATFORM}" "${TMP_WHEEL_DIR}"/*gdt*.whl -w "${WHEEL_DIR}/final"

deactivate
ccache -s
# cleanup will be called automatically by trap