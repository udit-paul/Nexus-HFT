cmake_minimum_required(VERSION 3.20)
project(nexus-hft VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add warnings and optimizations
if(MSVC)
    add_compile_options(/W4 /O2 /Oi /Ot /fp:fast)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -O3 -march=native)
endif()

# Include directories
include_directories(include)

# Subdirectories for testing and benchmarks
enable_testing()
add_subdirectory(tests)
add_subdirectory(benchmarks)

add_executable(nexus_engine_v2 
    src/main.cpp
    src/engine/order_book.cpp
    src/engine/matching_engine.cpp
)
target_include_directories(nexus_engine_v2 PRIVATE include)
