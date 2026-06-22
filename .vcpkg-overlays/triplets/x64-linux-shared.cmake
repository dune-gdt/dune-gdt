# Mostly-dynamic Linux triplet used by the shared-library presets (debug/release).
#
# The dune libraries are built shared (BUILD_SHARED_LIBS=ON) so the Python bindings can resolve their symbols at
# runtime. For that to be safe, the vcpkg dependencies must be shared too: a static dependency would otherwise be
# embedded in both the shared dune libraries and every test binary, duplicating its vague-linkage globals and crashing
# the C++ tests with "free(): double free detected" at exit. Hence dynamic linkage by default.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

# Build every dependency in a single (release) configuration instead of vcpkg's default debug+release pair.
#
# By default vcpkg installs both variants side by side: release into <triplet>/lib and debug into <triplet>/debug/lib. A
# Debug consumer build (CMAKE_BUILD_TYPE=Debug) then straddles both trees -- per-config imported targets (e.g. gmpxx via
# dune/mpfr) resolve to debug/lib while module-mode finds (e.g. find_package(LAPACK)) resolve to the release lib.
# Because libgmpxx.so.4 and liblapack.so.3 carry the same SONAME in *both* directories, CMake cannot order the RPATH
# (debug/lib must precede lib for gmpxx, lib must precede debug/lib for lapack) and emits the harmless-but-noisy "Cannot
# generate a safe runtime search path ... cycle in the constraint graph" warning for every target.
#
# Pinning the dependencies to a single configuration removes the debug/lib tree entirely, so there is nothing to
# mis-order and the warnings disappear. Release is chosen over debug because we never step through dependency internals;
# this also roughly halves the dependency build time. Our own dune/dune-gdt libraries are still built with the
# consumer's CMAKE_BUILD_TYPE, and Debug<->Release is ABI-compatible here (no _GLIBCXX_DEBUG), which the current build
# already relies on by linking the release lapack into the Debug build.
set(VCPKG_BUILD_TYPE release)

# openblas: its DYNAMIC_ARCH AVX512 kernels fail to compile as a shared build with gcc-13 ("inlining failed in call to
# 'always_inline' '_mm512_add_pd': target specific option mismatch"). openblas is plain C/Fortran with no C++
# vague-linkage globals, so a static copy does not cause the at-exit double free the dynamic linkage is meant to avoid;
# build it static to dodge the shared-build compile error.
if(PORT STREQUAL "openblas")
  set(VCPKG_LIBRARY_LINKAGE static)
endif()
