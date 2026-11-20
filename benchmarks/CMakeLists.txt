add_executable(db_benchmarks
    hash_table_bench.cpp
)

target_include_directories(db_benchmarks PRIVATE
    "${CMAKE_SOURCE_DIR}/itlab_db/src"
)

target_link_libraries(db_benchmarks PRIVATE
    benchmark::benchmark_main
)
