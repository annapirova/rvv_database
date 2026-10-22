macro(assign_bool var)
     if(${ARGN})
         set(${var} ON)
     else()
         set(${var} OFF)
     endif()
endmacro()

assign_bool(BUILD_SCALAR (WITH_SCALAR_LIB))

message ("    check HAVE_AVX2")
try_compile(HAVE_AVX2 
            ${CMAKE_BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/cmake/checks/compile_avx2.cpp
            )

assign_bool(BUILD_AVX2 (HAVE_AVX2 AND WITH_AVX2_LIB))

message ("    check HAVE_AVX512")
try_compile(HAVE_AVX512 
            ${CMAKE_BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/cmake/checks/compile_avx512.cpp
            )

assign_bool(BUILD_AVX512 (HAVE_AVX512 AND WITH_AVX512_LIB))


message ("    check HAVE_RVV_0P7")
try_compile(HAVE_RVV_0P7
            ${CMAKE_BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/cmake/checks/compile_rvv_0p7.cpp
            )

message ("    check HAVE_RVV_1P0")
try_compile(HAVE_RVV_1P0
            ${CMAKE_BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/cmake/checks/compile_rvv_1p0.cpp
            )

message ("    check HAVE_MKL")
try_compile(HAVE_MKL
            ${CMAKE_BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/cmake/checks/compile_mkl.cpp
            )

assign_bool(BUILD_RVV ((HAVE_RVV_0P7 OR HAVE_RVV_1P0) AND WITH_RVV_LIB))
assign_bool(BUILD_ACCURACY_TESTS (WITH_ACCURACY_TESTS))
assign_bool(BUILD_PERF_TESTS (WITH_PERF_TESTS))
assign_bool(BUILD_CONVERT_TO_BIN (WITH_CONVERT_TO_BIN))
