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

    set_target_properties(${TARGET} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endfunction()


#function(create_executable_project TARGET)
#    file(GLOB_RECURSE TARGET_SRC "*.c*")
#    file(GLOB_RECURSE TARGET_HD "*.h*")
#    add_executable(${TARGET} ${TARGET_SRC} ${TARGET_HD})

#    target_include_directories(${TARGET} PUBLIC "${CMAKE_SOURCE_DIR}/include")

#    get_property ( LIB_LIST GLOBAL PROPERTY LIBS_P)
#    target_link_libraries(${TARGET} ${LIB_LIST})
#	set_target_properties(${TARGET} PROPERTIES 
#		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
#endfunction()

function(create_executable_project TARGET)
    file(GLOB_RECURSE TARGET_SRC "*.c*")
    file(GLOB_RECURSE TARGET_HD "*.h*")
    add_executable(${TARGET} ${TARGET_SRC} ${TARGET_HD})

    target_include_directories(${TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/include")

    get_property(LIB_LIST GLOBAL PROPERTY LIBS_P)
    
    if(WITH_SQLPARSER)
        list(APPEND LIB_LIST sqlparser) 
    endif()

    target_link_libraries(${TARGET} ${LIB_LIST})

    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endfunction()



function(create_executable_test_project TARGET)
    file(GLOB_RECURSE TARGET_SRC "*.c*")
    file(GLOB_RECURSE TARGET_HD "*.h*")
    add_executable(${TARGET} ${TARGET_SRC} ${TARGET_HD})

    target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(${TARGET} PUBLIC "${CMAKE_SOURCE_DIR}/include")

    get_property ( LIB_LIST GLOBAL PROPERTY LIBS_P)
    target_link_libraries(${TARGET} gtest gtest_main ${LIB_LIST})

    add_test(NAME ${TARGET} COMMAND ${TARGET})
endfunction()

function(create_executable_benchmark_project TARGET)
    file(GLOB_RECURSE TARGET_SRC "*.c*")
    file(GLOB_RECURSE TARGET_HD "*.h*")
    add_executable(${TARGET} ${TARGET_SRC} ${TARGET_HD})

    target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(${TARGET} PUBLIC "${CMAKE_SOURCE_DIR}/include")

    get_property ( LIB_LIST GLOBAL PROPERTY LIBS_P)
    target_link_libraries(${TARGET} benchmark::benchmark benchmark::benchmark_main ${LIB_LIST})

endfunction()
