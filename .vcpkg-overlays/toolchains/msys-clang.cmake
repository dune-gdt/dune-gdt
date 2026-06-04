if(NOT X_VCPKG_LINUX_TOOLCHAIN_OVERRIDE_OSS_MSYS_CLANG)
  set(X_VCPKG_LINUX_TOOLCHAIN_OVERRIDE_OSS_MSYS_CLANG 1)

  set(MSYS2_ROOT "C:/msys64")
  set(MINGW_ROOT "${MSYS2_ROOT}/mingw64")

  set(CMAKE_C_COMPILER "${MINGW_ROOT}/bin/clang.exe")
  set(CMAKE_CXX_COMPILER "${MINGW_ROOT}/bin/clang++.exe")
  set(CMAKE_RC_COMPILER "${MINGW_ROOT}/bin/windres.exe")

  include(${CMAKE_CURRENT_LIST_DIR}/clang_include.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/try_compile_common.cmake)

endif()
