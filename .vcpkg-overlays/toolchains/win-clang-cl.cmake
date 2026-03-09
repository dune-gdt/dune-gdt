include("${CMAKE_CURRENT_LIST_DIR}/msvc_include.cmake")

# would make our msvc_include generation too complex to allow to set this automatically through the preset too
set(CMAKE_C_COMPILER_FRONTEND_VARIANT "MSVC")
set(CMAKE_CXX_COMPILER_FRONTEND_VARIANT "MSVC")

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/toolchains/msvc/Windows_Clang_toolchain.cmake")
