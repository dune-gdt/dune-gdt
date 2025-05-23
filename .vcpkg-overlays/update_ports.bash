#!/usr/bin/env bash

set -euo pipefail

# Ensure the script can be called from any directory
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$(cd "$SCRIPT_DIR/../" && pwd)

cd "$REPO_ROOT"

# Ensure the tmp directory exists
mkdir -p tmp

# Source the submodule info from module_list.bash
source "$REPO_ROOT/deps/module_list.bash"

# Pre-defined map of additional dependencies for specific ports
declare -A PORT_DEPENDENCIES

PORT_DEPENDENCIES[dune-alugrid]="dune-grid"
PORT_DEPENDENCIES[dune-uggrid]="dune-common"
PORT_DEPENDENCIES[dune-grid]="dune-common dune-geometry"
PORT_DEPENDENCIES[dune-geometry]="dune-common"
PORT_DEPENDENCIES[dune-grid-glue]="dune-grid"
PORT_DEPENDENCIES[dune-istl]="dune-common"
PORT_DEPENDENCIES[dune-localfunctions]="dune-geometry"
PORT_DEPENDENCIES[dune-testtools]="dune-common"

# Define features and their dependencies
declare -A PORT_FEATURES
PORT_FEATURES[dune-grid]="alberta:Support for Alberta grid implementation;uggrid:Support for UG grid implementation"

# Define feature dependencies
declare -A FEATURE_DEPENDENCIES
FEATURE_DEPENDENCIES[dune-grid,alberta]="alberta"
FEATURE_DEPENDENCIES[dune-grid,uggrid]="dune-uggrid"
# Define the submodule information
# Iterate over all submodules from the associative array
for submodule_name in "${!SUBMODULE_INFO_HASH[@]}"; do
    submodule_dir="deps/$submodule_name"
    # Create the overlay port directory
    mkdir -p .vcpkg-overlays/ports/$submodule_name

    # Only use information from module_list.bash
    git_hash="${SUBMODULE_INFO_HASH[$submodule_name]}"
    url="${SUBMODULE_INFO_URL[$submodule_name]}"

    # Try to extract version from dune.module in the remote repo
    version="0.0.1"
    tmp_clone_dir="tmp/port_clone_$submodule_name"
    [[ -d "$tmp_clone_dir" ]] || git clone "$url" "$tmp_clone_dir" &> /dev/null
    pushd "$tmp_clone_dir" &> /dev/null
    git fetch --all &>/dev/null
    git checkout "$git_hash" &>/dev/null
    if [ -f "dune.module" ]; then
        found_version=$(grep -E '^Version: ' "dune.module" | head -n1 | sed 's/Version: *//')
        if [ -n "$found_version" ]; then
            echo "Found version: $found_version for $submodule_name"
            version="$found_version"
        fi
    else
        echo "No version found in dune.module for $submodule_name, using default version: $version"
    fi
    popd &> /dev/null

    homepage="$url"

    # Prepare dependencies JSON array
    dependencies='        {
            "name": "vcpkg-cmake",
            "host": true
        },
        {
            "name": "vcpkg-cmake-config",
            "host": true
        }'

    # Add extra dependencies if present in the map
    if [[ -n "${PORT_DEPENDENCIES[$submodule_name]:-}" ]]; then
        for dep in ${PORT_DEPENDENCIES[$submodule_name]}; do
            dependencies="$dependencies,
        {\"name\": \"$dep\"}"
        done
    fi

    # Prepare features section if present
    features=""
    if [[ -n "${PORT_FEATURES[$submodule_name]:-}" ]]; then
        features=',
    "features": {'
        IFS=';' read -ra feature_list <<< "${PORT_FEATURES[$submodule_name]}"
        for feature_desc in "${feature_list[@]}"; do
            feature_name="${feature_desc%%:*}"
            feature_description="${feature_desc#*:}"
            features="$features
        \"$feature_name\": {            \"description\": \"$feature_description\""
            if [[ -n "${FEATURE_DEPENDENCIES[$submodule_name,$feature_name]:-}" ]]; then
                features="$features,
            \"dependencies\": ["
                IFS=' ' read -ra feat_deps <<< "${FEATURE_DEPENDENCIES[$submodule_name,$feature_name]}"
                first_dep=true
                for feat_dep in "${feat_deps[@]}"; do
                    if [ "$first_dep" = true ]; then
                        first_dep=false
                    else
                        features="$features,"
                    fi
                    features="$features
                {\"name\": \"$feat_dep\"}"
                done
                features="$features
            ]"
            fi
            features="$features
        },"
        done
        features="${features%,}
    }"
    fi

    # Create vcpkg.json
    cat > ".vcpkg-overlays/ports/$submodule_name/vcpkg.json" << EOF
{
    "name": "$submodule_name",
    "version": "$version",
    "description": "Local overlay port for $submodule_name",
    "homepage": "$homepage",
    "dependencies": [
$dependencies
    ]$features
}
EOF

    # Create portfile.cmake (install COPYING as copyright if present)
    cat > ".vcpkg-overlays/ports/$submodule_name/portfile.cmake" << EOF
set(VCPKG_BUILD_TYPE release)

vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL "$url"
    REF $git_hash
)

vcpkg_cmake_configure(
    SOURCE_PATH "\${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTING=OFF
        -DCMAKE_DISABLE_FIND_PACKAGE_MPI=TRUE
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/$submodule_name)

file(REMOVE_RECURSE "\${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "\${CURRENT_PACKAGES_DIR}/debug/share")

if(EXISTS "\${SOURCE_PATH}/COPYING")
    file(INSTALL "\${SOURCE_PATH}/COPYING" DESTINATION "\${CURRENT_PACKAGES_DIR}/share/\${PORT}" RENAME "copyright")
else()
    file(WRITE "\${CURRENT_PACKAGES_DIR}/share/\${PORT}/copyright" "No license file found in the source repository. Please check the source code for license information.")
endif()
EOF
    pre-commit run cmake-format --files .vcpkg-overlays/ports/$submodule_name/* &> /dev/null || \
        pre-commit run clang-format --files .vcpkg-overlays/ports/$submodule_name/* &> /dev/null || true
    echo "Created overlay port for $submodule_name"
done
