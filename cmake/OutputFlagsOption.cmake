function(status MESSEGE FLAG)
    if (FLAG)
      message("${MESSEGE} YES")
    else()
      message("${MESSEGE} NO")
    endif()
endfunction()


message ("======================= RVV ItLab database build option ========================= ")

message ("-----------------------  compile options    ------------------------ ")

if(DEFINED CMAKE_CXX_STANDARD AND CMAKE_CXX_STANDARD)
  message("C++ standard        :" ${CMAKE_CXX_STANDARD})
endif()
message("C++ Compiler        :" ${CMAKE_CXX_COMPILER})
message("C Compiler          :" ${CMAKE_C_COMPILER})

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  message("C++ flags (Debug)   :" ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_DEBUG})
  message("C   flags (Debug)   :" ${CMAKE_C_FLAGS} " " ${CMAKE_C_FLAGS_DEBUG})
else()
  message("C++ flags (Release) :" ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_RELEASE})
  message("C   flags (Release) :" ${CMAKE_C_FLAGS} " " ${CMAKE_C_FLAGS_RELEASE})
endif()

message ("------------------------ having components -------------------------- ")

status("HAVE_AVX512       : " ${HAVE_AVX512})
status("HAVE_RVV_1P0      : " ${HAVE_RVV_1P0})

message ("---------------------- compiling components ------------------------- ")

status("Build the scalar library                : " ${BUILD_SCALAR})
status("Build the avx512 library                : " ${BUILD_AVX512})
status("Build the RVV    library                : " ${BUILD_RVV})
status("Build the tests for library             : " ${BUILD_TESTS})
status("Build the performance tests for library : " ${BUILD_BENCHMARK})
status("Build sql parser          				: " ${BUILD_SQLPARSER})

message ("======================= RVVMV build option ========================= ")
