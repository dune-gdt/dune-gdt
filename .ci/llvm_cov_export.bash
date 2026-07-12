#!/usr/bin/env bash
#
# Turns the raw llvm source-based coverage profiles written by an instrumented test run
# (clang22-release_coverage preset: -fprofile-instr-generate -fcoverage-mapping, with
# LLVM_PROFILE_FILE pointing into <builddir>/profraw/) into two lcov-format reports:
#
#   <builddir>/coverage-cpp.info           - with BRDA branch records (llvm-cov's true
#                                            branch coverage, no gcov artefacts)
#   <builddir>/coverage-cpp-lineonly.info  - the same report with all branch records
#                                            stripped (what an lcov-default upload would
#                                            look like: line coverage only, no partials)
#
# Both variants exist so the clang/lcov numbers can be compared against the current
# gcc/gcovr/Codecov evaluation (issue #314) with and without the "partials" dimension.
#
# Usage: llvm_cov_export.bash <builddir> <sourcedir> <llvm-major-version>

set -euo pipefail

builddir="$1"
sourcedir="$2"
llvm_version="$3"

profdata="llvm-profdata-${llvm_version}"
llvmcov="llvm-cov-${llvm_version}"
objdump="llvm-objdump-${llvm_version}"

cd "${builddir}"

shopt -s nullglob
profraw_files=(profraw/*.profraw)
if [[ ${#profraw_files[@]} -eq 0 ]]; then
  echo "error: no .profraw files under ${builddir}/profraw -- did the instrumented tests run" \
    "with LLVM_PROFILE_FILE set (ctest --preset=clang22-release_coverage)?" >&2
  exit 1
fi
echo "merging ${#profraw_files[@]} profraw file(s)"
# --failure-mode=all: a single corrupt profile (e.g. from a test killed mid-write by the
# ctest timeout) must not discard the whole run; only fail if nothing merges.
"${profdata}" merge -sparse --failure-mode=all -o coverage.profdata "${profraw_files[@]}"

# Every binary compiled with -fcoverage-mapping carries a __llvm_covmap section; llvm-cov
# hard-errors on objects without one, so filter the candidate set (test executables, the
# dune libraries and the Python binding modules all live under the build tree) down to
# instrumented ELF files first. vcpkg's dependency tree is built uninstrumented (default
# gcc) and CMake's compiler-probe artefacts live under CMakeFiles/, so both are pruned.
mapfile -t candidates < <(find . \
  \( -path ./vcpkg_installed -o -name CMakeFiles -o -path ./profraw \) -prune -false -o \
  -type f \( -perm -u+x -o -name '*.so*' \) -print | sort)
objects=()
for f in "${candidates[@]}"; do
  # cheap ELF magic check before the (slower) section scan
  [[ "$(head -c 4 "${f}" 2> /dev/null)" == $'\x7fELF' ]] || continue
  if "${objdump}" --section-headers "${f}" 2> /dev/null | grep -q '__llvm_covmap'; then
    objects+=("${f}")
  fi
done
if [[ ${#objects[@]} -eq 0 ]]; then
  echo "error: no instrumented binaries (with a __llvm_covmap section) found under ${builddir}" >&2
  exit 1
fi
echo "exporting coverage from ${#objects[@]} instrumented binaries"

# Mirror the gcovr call's --filter <sourcedir>/dune/: only our own sources, no vcpkg
# headers, no config.h from the build tree. llvm-cov takes one positional binary plus
# -object for the rest, and trailing positional SOURCES paths restrict the report
# (llvm-cov's regex engine has no negative lookahead, so -ignore-filename-regex cannot
# express "everything outside dune/").
object_args=()
for obj in "${objects[@]:1}"; do
  object_args+=(-object "${obj}")
done
"${llvmcov}" export -format=lcov -instr-profile=coverage.profdata -num-threads=4 \
  "${objects[0]}" "${object_args[@]}" "${sourcedir}/dune" > coverage-cpp.info

# BRF/BRH are per-record branch summaries, BRDA the branch data lines; stripping them
# yields the lcov-default (line-only) view. grep exits 1 on no matches only with -q, but
# guard anyway in case a future llvm-cov emits no branch records at all.
grep -vE '^(BRDA|BRF|BRH)' coverage-cpp.info > coverage-cpp-lineonly.info || true

echo "wrote ${builddir}/coverage-cpp.info and ${builddir}/coverage-cpp-lineonly.info"
