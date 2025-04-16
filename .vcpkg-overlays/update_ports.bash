#!/usr/bin/env bash

set -euo pipefail

# Ensure the script can be called from any directory
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$(cd "$SCRIPT_DIR/../" && pwd)

echo "DEBUG: REPO_ROOT is set to: '$REPO_ROOT'"
cd "$REPO_ROOT"

# Ensure the tmp directory exists
mkdir -p tmp

# Generate the list of submodules dynamically, filtering only those that are CMake-based projects
if ! git submodule foreach --quiet '([ -f CMakeLists.txt ] && echo $path) || true' > tmp/submodule_list.txt; then
    echo "Warning: Some submodules could not be processed. Continuing with the rest." >&2
fi

create_port_files() {
    local name=$1
    local source_path=$2
    local port_dir=".vcpkg-overlays/ports/$name"

    # Get the git hash of the submodule
    pushd "$source_path" > /dev/null || { echo "Failed to enter directory: $source_path"; return 1; }
    local git_hash=$(git rev-parse HEAD)
    popd > /dev/null

    # Extract project version from CMakeLists.txt (if available)
    local version="0.1.0"  # Default version
    if grep -q "project.*VERSION" "$source_path/CMakeLists.txt"; then
        version=$(grep "project.*VERSION" "$source_path/CMakeLists.txt" | sed -E 's/.*VERSION\s+([0-9]+\.[0-9]+\.[0-9]+).*/\1/')
    fi

    # Create vcpkg.json
    cat > "$port_dir/vcpkg.json" << EOF
{
    "name": "$name",
    "version": "$version",
    "description": "Local overlay port for $name",
    "homepage": "https://github.com/dune-community/$name",
    "dependencies": [
        {
            "name": "vcpkg-cmake",
            "host": true
        },
        {
            "name": "vcpkg-cmake-config",
            "host": true
        }
    ]
}
EOF

    # Find license file
    local license_file=""
    for file in LICENSE.md LICENSE COPYING COPYRIGHT LICENSE.txt COPYING.txt; do
        if [ -f "$source_path/$file" ]; then
            license_file="$file"
            break
        fi
    done

    # Create portfile.cmake
    cat > "$port_dir/portfile.cmake" << EOF
set(VCPKG_BUILD_TYPE release)

vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL "\${CMAKE_CURRENT_LIST_DIR}/../../../deps/$name"
    REF $git_hash
)

vcpkg_cmake_configure(
    SOURCE_PATH "\${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTING=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/$name)

file(REMOVE_RECURSE "\${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "\${CURRENT_PACKAGES_DIR}/debug/share")

EOF

    # Add license installation if a license file was found
    if [ -n "$license_file" ]; then
        cat >> "$port_dir/portfile.cmake" << EOF
file(
    INSTALL "\${SOURCE_PATH}/$license_file"
    DESTINATION "\${CURRENT_PACKAGES_DIR}/share/\${PORT}"
    RENAME copyright
)
EOF
    else
        # If no license file was found, create a placeholder
        cat >> "$port_dir/portfile.cmake" << EOF
# No license file found, creating a placeholder
file(WRITE "\${CURRENT_PACKAGES_DIR}/share/\${PORT}/copyright" "No license file found in the source repository. Please check the source code for license information.")
EOF
    fi

    echo "Created overlay port for $name"
}

# Process all submodules
while read submodule_dir; do
    submodule_name=$(basename "$submodule_dir")

    # Create the overlay port directory
    mkdir -p .vcpkg-overlays/ports/$submodule_name

    create_port_files "$submodule_name" "$submodule_dir"
done < tmp/submodule_list.txt