# Overlay of the upstream vcpkg lapack-reference port, kept verbatim except for the LAPACKE build below (the same
# arrangement the mpfr overlay in this directory uses). Refresh it against upstream when the vcpkg baseline in
# ../../../vcpkg.json moves; it was taken from baseline d015e31e90838a4c9dfa3eed45979bc70d9357fc.
#
# This file and vcpkg.json are the only ones that differ from that baseline. FindLAPACK.cmake,
# vcpkg-cmake-wrapper.cmake.in, usage and the three patches are byte-identical copies, so a refresh can replace them
# outright; only the changes marked below have to be re-applied by hand.
#
# Why we carry it: dune-xt needs LAPACKE (Dune::XT::Common::Lapacke, dune/xt/common/lapacke.{hh,cc}), and nothing in
# the manifest provides it. Upstream leaves LAPACKE off ("LAPACKE should be its own PORT" in the TODO below) and
# installs no headers at all, and the other BLAS/LAPACK provider we depend on -- openblas -- is configured
# -DBUILD_WITHOUT_LAPACK=ON by its own port, so libopenblas carries no LAPACKE_* symbols either. Reference LAPACK
# already builds here with a Fortran compiler, so enabling its in-tree LAPACKE is a single option.
#
#TODO: Features to add:
# USE_XBLAS??? extended precision blas. needs xblas
# LAPACKE should be its own PORT
# USE_OPTIMIZED_LAPACK (Probably not what we want. Does a find_package(LAPACK): probably for LAPACKE only builds _> own port?)
# LAPACKE_WITH_TMG Build LAPACKE with tmglib routines
if(EXISTS "${CURRENT_INSTALLED_DIR}/share/clapack/copyright")
    message(FATAL_ERROR "Can't build ${PORT} if clapack is installed. Please remove clapack:${TARGET_TRIPLET}, and try to install ${PORT}:${TARGET_TRIPLET} again.")
endif()

include(vcpkg_find_fortran)
# Upstream sets VCPKG_POLICY_EMPTY_INCLUDE_FOLDER here because it installs no headers. We build LAPACKE, which
# installs lapacke.h and friends, so the policy is deliberately dropped: an empty include folder now means the
# LAPACKE build silently did not happen, and vcpkg should fail instead of shipping a headerless package.
set(VCPKG_POLICY_ALLOW_OBSOLETE_MSVCRT enabled)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO  "Reference-LAPACK/lapack"
    REF "v${VERSION}"
    SHA512 9749976d773830eb635498611c7f1247af8dece23fe8c08446243aa39bdcc20dd35fdc670345643cd1ec6828e379d5c2152009817e0b486c10fd89a06602e0fb
    HEAD_REF master
    PATCHES
        cmake-config.patch
        fix_prefix.patch
        implicit-link.patch
)

if(NOT VCPKG_TARGET_IS_WINDOWS)
    set(ENV{FFLAGS} "$ENV{FFLAGS} -fPIC") # should come from toolchain
endif()

vcpkg_check_features(OUT_FEATURE_OPTIONS OPTIONS
    FEATURES
        cblas   CBLAS
        cblas   BUILD_INDEX64_EXT_API
        noblas  USE_OPTIMIZED_BLAS
        noblas  CMAKE_REQUIRE_FIND_PACKAGE_BLAS
)

set(VCPKG_CRT_LINKAGE_BACKUP ${VCPKG_CRT_LINKAGE})
vcpkg_find_fortran(FORTRAN_CMAKE) # provides VCPKG_USE_INTERNAL_Fortran

if("noblas" IN_LIST FEATURES)
    if("cblas" IN_LIST FEATURES)
        message(FATAL_ERROR "Feature 'noblas' cannot be used together with feature 'cblas'.")
    elseif(VCPKG_USE_INTERNAL_Fortran AND VCPKG_CRT_LINKAGE_BACKUP STREQUAL "static")
        # If openblas has been built with static crt linkage we cannot use it with gfortran.
        message(FATAL_ERROR "Feature 'noblas' cannot be used without supplying an external fortran compiler.")
    endif()
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${OPTIONS}
        ${FORTRAN_CMAKE}
        "-DTEST_FORTRAN_COMPILER=OFF"
        # the sole divergence from upstream: build the in-tree C interface, so that liblapacke and lapacke.h are
        # installed alongside liblapack (see the rationale at the top of this file)
        "-DLAPACKE=ON"
    MAYBE_UNUSED_VARIABLES
        CMAKE_REQUIRE_FIND_PACKAGE_BLAS
)

vcpkg_cmake_install()

# The version here is hacked due to a mistake in lapack. Should be 3.12.1 but is not
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/lapack-3.12.0)

# Building LAPACKE also installs a CMake package config of its own under lib/cmake/lapacke-<version>, which vcpkg's
# post-build checks reject for living outside share/<port>. Nothing consumes it here -- LAPACKE is located by
# cmake/modules/FindLAPACKE.cmake via find_library/find_path -- so drop it instead of running a second
# vcpkg_cmake_config_fixup, which would have to reach into the lapack config handled just above. The version is
# globbed rather than spelled out because upstream's LAPACK_VERSION disagrees with the port version (see the note on
# the fixup above).
file(GLOB LAPACKE_CMAKE_CONFIG_DIRS "${CURRENT_PACKAGES_DIR}/lib/cmake/lapacke-*"
     "${CURRENT_PACKAGES_DIR}/debug/lib/cmake/lapacke-*")
if(LAPACKE_CMAKE_CONFIG_DIRS)
    file(REMOVE_RECURSE ${LAPACKE_CMAKE_CONFIG_DIRS})
endif()

vcpkg_fixup_pkgconfig()

file(RENAME "${CURRENT_PACKAGES_DIR}/lib/pkgconfig/lapack.pc" "${CURRENT_PACKAGES_DIR}/lib/pkgconfig/lapack-reference.pc")
if(NOT VCPKG_BUILD_TYPE)
    file(RENAME "${CURRENT_PACKAGES_DIR}/debug/lib/pkgconfig/lapack.pc" "${CURRENT_PACKAGES_DIR}/debug/lib/pkgconfig/lapack-reference.pc")
endif()

if(NOT "noblas" IN_LIST FEATURES)
    file(RENAME "${CURRENT_PACKAGES_DIR}/lib/pkgconfig/blas.pc" "${CURRENT_PACKAGES_DIR}/lib/pkgconfig/blas-reference.pc")
    if(NOT VCPKG_BUILD_TYPE)
        file(RENAME "${CURRENT_PACKAGES_DIR}/debug/lib/pkgconfig/blas.pc" "${CURRENT_PACKAGES_DIR}/debug/lib/pkgconfig/blas-reference.pc")
    endif()
    if("cblas" IN_LIST FEATURES)
      file(RENAME "${CURRENT_PACKAGES_DIR}/lib/pkgconfig/cblas.pc" "${CURRENT_PACKAGES_DIR}/lib/pkgconfig/cblas-reference.pc")
      if(NOT VCPKG_BUILD_TYPE)
          file(RENAME "${CURRENT_PACKAGES_DIR}/debug/lib/pkgconfig/cblas.pc" "${CURRENT_PACKAGES_DIR}/debug/lib/pkgconfig/cblas-reference.pc")
      endif()
    endif()
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "static" BLA_STATIC)
configure_file("${CMAKE_CURRENT_LIST_DIR}/vcpkg-cmake-wrapper.cmake.in" "${CURRENT_PACKAGES_DIR}/share/${PORT}/wrapper/vcpkg-cmake-wrapper.cmake" @ONLY)
file(COPY "${CMAKE_CURRENT_LIST_DIR}/FindLAPACK.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
