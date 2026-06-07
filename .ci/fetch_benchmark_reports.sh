#!/usr/bin/env bash
#
# Fetch the published nanobench JSON reports from the orphan `benchmarks` branch into a local
# directory, so that the documentation build (the `benchmark-plots` Sphinx directive) can render
# them. The destination directory ends up holding the per-run report directories directly, e.g.
#
#   <dest>/20260607-095800-abc1234/burgers__1d__explicit__fv.json
#   <dest>/latest/...
#
# Usage: fetch_benchmark_reports.sh <dest-dir> [remote-url] [branch]
#
# If the branch does not exist yet (no benchmark run has happened on `main`), the script exits 0
# without creating anything; the docs then render a placeholder note instead of failing.
set -euo pipefail

dest="${1:?usage: $0 <dest-dir> [remote-url] [branch]}"
remote="${2:-https://github.com/${GITHUB_REPOSITORY:-dune-gdt/dune-gdt}.git}"
branch="${3:-benchmarks}"

if ! git ls-remote --exit-code --heads "${remote}" "${branch}" >/dev/null 2>&1; then
  echo "benchmark branch '${branch}' not found at ${remote}; skipping (docs will show a placeholder)"
  exit 0
fi

tmp="$(mktemp -d)"
trap 'rm -rf "${tmp}"' EXIT
git clone --depth 1 --branch "${branch}" "${remote}" "${tmp}"

mkdir -p "${dest}"
if [ -d "${tmp}/reports" ]; then
  cp -r "${tmp}/reports/." "${dest}/"
  echo "copied benchmark reports from ${branch} into ${dest}"
else
  echo "no reports/ directory on branch '${branch}'; nothing to copy"
fi
