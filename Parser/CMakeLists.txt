cmake_minimum_required(VERSION 3.5)

project(superExpr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
file(GLOB_RECURSE ALL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../sql_parse/sql-parser/src/*.cpp")

set(LIB_SOURCES "")
foreach(source_file ${ALL_SOURCES})
    file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}/../sql_parse/sql-parser/src" ${source_file})
    if(NOT rel_path MATCHES "test" AND NOT rel_path MATCHES "benchmark")
        list(APPEND LIB_SOURCES ${source_file})
    endif()
endforeach()

add_library(SQL_LIB STATIC ${LIB_SOURCES})
target_include_directories(SQL_LIB PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../sql_parse/sql-parser/src")

add_executable(superExpr  second_version.cpp)
target_link_libraries(superExpr PRIVATE SQL_LIB)

include(GNUInstallDirs)
install(TARGETS superExpr SQL_LIB
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

