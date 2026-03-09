#!/usr/bin/env bash
# Script to pull latest upstream files from https://github.com/MarkSchofield/WindowsToolchain
# and rename them to match local naming conventions (replace '.' with '_' except for extension)
#
# Run this script from the cmake/toolchains/msvc/ directory.
set -euo pipefail

UPSTREAM_REPO="https://raw.githubusercontent.com/MarkSchofield/WindowsToolchain/main"

# List of files to pull from upstream (from repo root)
FILES=(
  "VSWhere.cmake"
  "Windows.Clang.toolchain.cmake"
  "Windows.Kits.cmake"
  "Windows.MSVC.toolchain.cmake"
)

# Map upstream filenames to local filenames (replace '.' with '_' except for extension)
function map_filename() {
  local fname="$1"
  local base="${fname%.*}"
  local ext="${fname##*.}"
  # Only replace if there is an extension
  if [[ "$base" != "$fname" ]]; then
    base="${base//./_}"
    echo "${base}.${ext}"
  else
    echo "${fname//./_}"
  fi
}

for src in "${FILES[@]}"; do
  dst=$(map_filename "$src")
  echo "Fetching $src -> $dst"
  curl -fsSL "$UPSTREAM_REPO/$src" -o "$dst"
  echo "Updated $dst"
done

# Fix filename references in the downloaded files to match our naming convention
echo "Patching filename references..."
for file in "${FILES[@]}"; do
  local_file=$(map_filename "$file")
  if [[ -f "$local_file" ]]; then
    # For each file in our list, replace references to the upstream naming with our naming
    for ref_file in "${FILES[@]}"; do
      upstream_name="$ref_file"
      local_name=$(map_filename "$ref_file")
      if [[ "$upstream_name" != "$local_name" ]]; then
        # Replace references in include statements and other file references
        sed -i "s|${upstream_name}|${local_name}|g" "$local_file"
      fi
    done
    echo "Patched references in $local_file"
  fi
done
