set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

find_program(RISCV_C_COMPILER NAMES riscv64-linux-gnu-gcc riscv64-unknown-linux-gnu-gcc REQUIRED)
find_program(RISCV_CXX_COMPILER NAMES riscv64-linux-gnu-g++ riscv64-unknown-linux-gnu-g++ REQUIRED)

set(CMAKE_C_COMPILER   ${RISCV_C_COMPILER})
set(CMAKE_CXX_COMPILER ${RISCV_CXX_COMPILER})

set(RISCV_FLAGS "-march=rv64gcv -mabi=lp64d")
set(CMAKE_C_FLAGS_INIT "${RISCV_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_INIT "${RISCV_FLAGS}" CACHE STRING "" FORCE)

# Путь поиска библиотек в GitHub Actions
set(CMAKE_FIND_ROOT_PATH /usr/riscv64-linux-gnu)
