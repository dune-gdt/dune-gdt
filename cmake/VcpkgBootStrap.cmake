# If VCPKG_ROOT is specified, we interpret this a desire to use an existing vcpkg installation.
if(DEFINED ENV{VCPKG_ROOT})
  set(VCPKG_ROOT $ENV{VCPKG_ROOT})
endif()
# If _VCPKG_ROOT_DIR is specified, we are being installed as a dependency of another package and want to use the same
# vcpkg.
if(DEFINED _VCPKG_ROOT_DIR)
  set(VCPKG_ROOT ${_VCPKG_ROOT_DIR})
endif()
if(DEFINED VCPKG_ROOT)
  if(EXISTS "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    message(STATUS "Using existing vcpkg (since VCPKG_ROOT was specified) from ${VCPKG_ROOT}")
    set(CMAKE_TOOLCHAIN_FILE
        "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "" FORCE)
    set(VCPKG_FOUND 1)
  else()
    message(
      WARNING "VCPKG_ROOT was specified (see below), but does not contain a working vcpk! "
              "Resorting to default setup below. "
              "In future invocations, consider to either not specify VCPKG_ROOT or point it to a working "
              "vcpkg installation!"
              "\nVCPKG_ROOT: '${VCPKG_ROOT}'")
    set(VCPKG_FOUND 0)
  endif()
else()
  set(VCPKG_FOUND 0)
endif()
# Since VCPKG_ROOT was not specified, we want to work with a local copy of vcpkg.
if(NOT ${VCPKG_FOUND})
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.vcpkg-root/scripts/buildsystems/vcpkg.cmake")
    # If .vcpkg-root exists, we assume we've set it up and want to reuse it.
    message(STATUS "Using existing vcpkg from .vcpkg-root")
    set(CMAKE_TOOLCHAIN_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/.vcpkg-root/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "" FORCE)
  else()
    # We require vcpkg and download it fresh.
    message(STATUS "Setting up vcpkg for re-use in .vcpkg-root (this may take some time)")
    include(FetchContent)

    # Make sure that GIT_TAG is aligned with vcpkgCommitId in:
    # https://github.com/arup-group/oasys-core-cmake/blob/main/.github/workflows/build_and_test.yml
    FetchContent_Declare(
      vcpkg
      GIT_REPOSITORY https://github.com/microsoft/vcpkg/
      GIT_TAG 2025.01.13
      SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/.vcpkg-root CMAKE_CACHE_ARGS
      -DVCPKG_BOOTSTRAP_OPTIONS:STRING="-disableMetrics")
    FetchContent_MakeAvailable(vcpkg)
    set(CMAKE_TOOLCHAIN_FILE
        "${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "" FORCE)
    message(STATUS "Setting up vcpkg for re-use in .vcpkg-root (this may take some time) - done")
  endif()
endif()
