cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
cmake_policy(SET CMP0135 NEW)

project(ITLAB_DB)

set (CMAKE_CONFIGURATION_TYPES "Debug;Release") 
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
option(WITH_BENCHMARK	   "Build google benchmark" OFF)
option(WITH_SQLPARSER	   "Build sqlparser" ON)


include(cmake/TargetCreationFunctions.cmake)
include(cmake/TestEnvironment.cmake)
include(cmake/OutputFlagsOption.cmake)

if(BUILD_SCALAR)
  add_compile_options(-DSP_M_SCALAR)
endif()

if(BUILD_AVX512)
  add_compile_options(-DSP_M_AVX512)
  add_compile_definitions(HAS_AVX512=1)
endif()

if(BUILD_RVV)
  if(HAVE_RVV_1P0)
    add_compile_options(-DSP_M_RVV_1P0)
	add_compile_definitions(HAS_RVV=1)
  endif()
endif()

if(WITH_SQLPARSER)
    add_compile_definitions(HAS_SQL_PARSER=1)
endif()

set_property(GLOBAL PROPERTY LIBS_P "")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/thirdparty/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/thirdparty/lib")

set(LOG_OUTPUT_DIR "${CMAKE_BINARY_DIR}/thirdparty/lib")
set(GOOGLETEST_OUTPUT_DIR "${CMAKE_BINARY_DIR}/thirdparty/lib" CACHE PATH "" FORCE)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

if(WITH_TESTS)
    enable_testing()
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory("thirdparty/googletest" EXCLUDE_FROM_ALL)
endif()

if(WITH_BENCHMARK)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    add_subdirectory("thirdparty/benchmark" EXCLUDE_FROM_ALL)
endif()

if (WITH_SQLPARSER)
	message("Configure sql-parser")
		add_custom_target(run_sqlparser_makefile
        COMMAND make -C "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/sql-parser" clean
        
        COMMAND ${CMAKE_COMMAND} -E env 
            make -C "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/sql-parser" 
                CC="${CMAKE_C_COMPILER}" 
                CXX="${CMAKE_CXX_COMPILER}" 
                CFLAGS="${CMAKE_C_FLAGS} -fPIC"      
                CXXFLAGS="${CMAKE_CXX_FLAGS} -fPIC"
                AR="${CMAKE_AR}"				
                static=yes
                
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/thirdparty/lib"
        COMMAND ${CMAKE_COMMAND} -E copy 
            "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/sql-parser/libsqlparser.a" 
            "${CMAKE_BINARY_DIR}/thirdparty/lib/libsqlparser.a"
        COMMENT "Building sql-parser via its own Makefile..."
        VERBATIM
    )

	add_library(sqlparser STATIC IMPORTED GLOBAL)
	set_target_properties(sqlparser PROPERTIES
        IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/thirdparty/lib/libsqlparser.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/sql-parser/src"
    )
endif()


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# build main library
add_subdirectory(itlab_db)
add_subdirectory(apps)

if (WITH_TESTS)
	add_subdirectory(tests)
endif()
