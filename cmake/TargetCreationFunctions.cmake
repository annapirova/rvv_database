include(CMakeParseArguments)


MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${curdir}/${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

function(create_project_lib TARGET)
    if(NOT ARGN)
        set(INCLUDE_PATH "${CMAKE_SOURCE_DIR}/include")
    else()
        set(INCLUDE_PATH ${ARGN})
    endif()

    file(GLOB_RECURSE TARGET_SRC "*.c*")
    file(GLOB_RECURSE TARGET_HD "*.h*")
    add_library(${TARGET} STATIC ${TARGET_SRC} ${TARGET_HD})

    target_include_directories(${TARGET} PUBLIC "${INCLUDE_PATH}")

    get_property ( LIB_LIST GLOBAL PROPERTY LIBS_P)
    list(APPEND LIB_LIST ${TARGET})
    set_property ( GLOBAL PROPERTY LIBS_P ${LIB_LIST})
endfunction()

include(CMakeParseArguments)

function(create_project_lib_advanced TARGET)
    # WITH_SQL - флаг
    # INCLUDES - ключевое слово, после которого можно передать список путей
    set(options WITH_SQL)
    set(oneValueArgs)
    set(multiValueArgs INCLUDES)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_INCLUDES)
        set(INCLUDE_PATHS "${CMAKE_SOURCE_DIR}/include")
    else()
        set(INCLUDE_PATHS ${ARG_INCLUDES})
    endif()

    file(GLOB_RECURSE TARGET_SRC "*.c*")
    file(GLOB_RECURSE TARGET_HD "*.h*")
    add_library(${TARGET} STATIC ${TARGET_SRC} ${TARGET_HD})

    target_include_directories(${TARGET} PUBLIC ${INCLUDE_PATHS})

    if(ARG_WITH_SQL AND WITH_SQLPARSER)
        target_link_libraries(${TARGET} PUBLIC sqlparser)
        add_dependencies(${TARGET} run_sqlparser_makefile)
    endif()

    get_property(LIB_LIST GLOBAL PROPERTY LIBS_P)
    list(APPEND LIB_LIST ${TARGET})
    set_property(GLOBAL PROPERTY LIBS_P "${LIB_LIST}")
endfunction()


function(create_executable_project TARGET)
    file(GLOB_RECURSE TARGET_SRC "*.c*")
    file(GLOB_RECURSE TARGET_HD "*.h*")
    add_executable(${TARGET} ${TARGET_SRC} ${TARGET_HD})

    target_include_directories(${TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/include")
    
    get_property(LIB_LIST GLOBAL PROPERTY LIBS_P)
    
    if(WITH_SQLPARSER)
        list(APPEND LIB_LIST sqlparser) 
        add_dependencies(${TARGET} run_sqlparser_makefile)
    endif()
    
    target_link_libraries(${TARGET} PRIVATE ${LIB_LIST})
endfunction()


function(create_executable_test_project TARGET)
    file(GLOB_RECURSE TARGET_SRC "*.c*")
    file(GLOB_RECURSE TARGET_HD "*.h*")
    
    add_executable(${TARGET} ${TARGET_SRC} ${TARGET_HD})

    target_include_directories(${TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/include")

    get_property(LIB_LIST GLOBAL PROPERTY LIBS_P)
    
    target_link_libraries(${TARGET} PRIVATE 
        gtest gtest_main ${LIB_LIST}
        $<$<BOOL:${WITH_SQLPARSER}>:sqlparser>
    )

    if(WITH_SQLPARSER)
        add_dependencies(${TARGET} run_sqlparser_makefile)
    endif()

    add_test(NAME ${TARGET} COMMAND ${TARGET})
endfunction()



function(create_executable_benchmark_project TARGET)
    file(GLOB_RECURSE SOURCES "*.c*" "*.h*")
    
    add_executable(${TARGET} ${SOURCES})

    target_include_directories(${TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/include")

    get_property(LIB_LIST GLOBAL PROPERTY LIBS_P)
    
    target_link_libraries(${TARGET} PRIVATE 
        benchmark::benchmark 
        benchmark::benchmark_main 
        ${LIB_LIST}
        $<$<BOOL:${WITH_SQLPARSER}>:sqlparser>
    )

    if(WITH_SQLPARSER)
        add_dependencies(${TARGET} run_sqlparser_makefile)
    endif()
endfunction()

