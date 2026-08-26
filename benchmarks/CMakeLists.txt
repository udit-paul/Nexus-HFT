include(FetchContent)

FetchContent_Declare(
  googlebenchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(bench_spsc_queue bench_spsc_queue.cpp ../src/concurrency/cpu_affinity.cpp)
target_include_directories(bench_spsc_queue PRIVATE ../include)
target_link_libraries(bench_spsc_queue benchmark::benchmark benchmark::benchmark_main)
if (UNIX AND NOT APPLE)
    target_link_libraries(bench_spsc_queue pthread)
endif()

# ── Matching Engine Benchmark ─────────────────────────────────────────────────
add_executable(bench_matching_engine
    bench_matching_engine.cpp
    ../src/engine/matching_engine.cpp
    ../src/engine/order_book.cpp
)
target_include_directories(bench_matching_engine PRIVATE ../include)
target_link_libraries(bench_matching_engine benchmark::benchmark benchmark::benchmark_main)
if (UNIX AND NOT APPLE)
    target_link_libraries(bench_matching_engine pthread)
endif()
