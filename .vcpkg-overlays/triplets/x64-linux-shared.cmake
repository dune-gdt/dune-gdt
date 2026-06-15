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

# openblas: its DYNAMIC_ARCH AVX512 kernels fail to compile as a shared build with gcc-13 ("inlining failed in call to
# 'always_inline' '_mm512_add_pd': target specific option mismatch"). openblas is plain C/Fortran with no C++
# vague-linkage globals, so a static copy does not cause the at-exit double free the dynamic linkage is meant to avoid;
# build it static to dodge the shared-build compile error.
if(PORT STREQUAL "openblas")
  set(VCPKG_LIBRARY_LINKAGE static)
endif()
