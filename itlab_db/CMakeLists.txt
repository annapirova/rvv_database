add_subdirectory(Table)
add_subdirectory(Parser)
#create_project_lib(itlab_db "${CMAKE_CURRENT_SOURCE_DIR}/include")

create_project_lib_advanced(test_lib 
    WITH_SQL 
    INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_include_directories(Table PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
