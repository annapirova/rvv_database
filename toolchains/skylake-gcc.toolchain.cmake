set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_C_COMPILER   gcc)

set(CMAKE_CXX_FLAGS_INIT "-m64 -march=skylake-avx512")
set(CMAKE_C_FLAGS_INIT   "-m64 -march=skylake-avx512")