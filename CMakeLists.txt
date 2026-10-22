cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(SparseMatrix)

set (CMAKE_CONFIGURATION_TYPES "Debug;Release") 
set (CMAKE_CXX_STANDARD 17)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Find OpenMP support for C++
find_package(OpenMP REQUIRED)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

option(WITH_SCALAR_LIB     "Build the scalar library" ON)
option(WITH_AVX512_LIB     "Build the avx512 library" ON)
option(WITH_RVV_LIB        "Build the RVV library" ON)
option(WITH_TESTS		   "Build google test" ON)
option(WITH_BENCHMARK	   "Build google benchmark" ON)
option(WITH_SQLPARSER	   "Build sqlparser" ON)

include(cmake/TargetCreationFunctions.cmake)
include(cmake/TestEnvironment.cmake)
include(cmake/OutputFlagsOption.cmake)

if(BUILD_SCALAR)
  add_compile_options(-DSP_M_SCALAR)
endif()

if(BUILD_AVX512)
  add_compile_options(-DSP_M_AVX512)
endif()

if(BUILD_RVV)
  if(HAVE_RVV_1P0)
    add_compile_options(-DSP_M_RVV_1P0)
  endif()
endif()


add_subdirectory(src)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF)
set(HAVE_STD_REGEX 0)
set(HAVE_STEADY_CLOCK 0)
set(HAVE_POSIX_REGEX 0)
add_subdirectory("thirtdparty/googtest" EXCLUDE_FROM_ALL)
add_subdirectory("thirtdparty/benchmark" EXCLUDE_FROM_ALL)
add_subdirectory("thirtdparty/sql-parser" EXCLUDE_FROM_ALL)

enable_testing()

add_subdirectory(test_system)
