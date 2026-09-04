cmake_minimum_required(VERSION 3.20)

project(nexus-hft VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add warnings and optimizations
if(MSVC)
    add_compile_options(/W4 /O2 /Oi /Ot /fp:fast)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -O3)
endif()

# Include directories
include_directories(include)

# Optional testing and benchmarks
option(BUILD_TESTS "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Main executable
add_executable(nexus_engine_v2
    src/main.cpp
    src/engine/order_book.cpp
    src/engine/matching_engine.cpp
)

target_include_directories(nexus_engine_v2 PRIVATE include)