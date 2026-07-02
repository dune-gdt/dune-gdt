#!/usr/bin/env bash
#
# Frees up disk space on the GitHub-hosted ubuntu-26.04 runner by removing preinstalled
# toolchains this project never uses. The image ships with several large SDKs (Android,
# .NET, Haskell, Swift, CodeQL) intended for other projects' CI -- see
# https://github.com/actions/runner-images/blob/main/images/ubuntu/Ubuntu2604-Readme.md
# for the full inventory. Together they occupy tens of GB that our vcpkg-from-source and
# LTO wheel builds would rather have.
#
# Intended to run once, right after checkout, in disk-hungry jobs (the vcpkg/coverage
# build, the wheel build, the benchmarks build). Safe to call unconditionally: every
# removal is guarded by an existence check, so a path missing on a future image version
# is silently skipped rather than failing the job.

set -uo pipefail

remove_path() {
  local path="$1"
  if [ -e "${path}" ]; then
    local size
    size="$(sudo du -sh "${path}" 2>/dev/null | cut -f1)"
    echo "removing ${path} (${size:-unknown size})"
    sudo rm -rf "${path}"
  fi
}

before_kib="$(df --output=avail -B1K / | tail -n1)"

# Android SDK + NDK: by far the largest single item on the image.
remove_path "${ANDROID_HOME:-/usr/local/lib/android}"

# .NET SDKs (several major versions preinstalled side by side).
remove_path /usr/share/dotnet
remove_path "${HOME}/.dotnet"

# Haskell/GHC toolchain.
remove_path /opt/ghc
remove_path /usr/local/.ghcup

# Swift toolchain.
remove_path /usr/share/swift

# CodeQL bundle (this repo has no code-scanning workflow).
remove_path "${RUNNER_TOOL_CACHE:-/opt/hostedtoolcache}/CodeQL"

after_kib="$(df --output=avail -B1K / | tail -n1)"
freed_mib=$(( (after_kib - before_kib) / 1024 ))
echo "freed approximately ${freed_mib} MiB (root filesystem avail: $((before_kib / 1024)) MiB -> $((after_kib / 1024)) MiB)"
