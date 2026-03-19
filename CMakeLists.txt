cmake_minimum_required(VERSION 3.15)
project(mpsc_queue_eval VERSION 1.0)

# Enforce strict C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# HFT & Bare-Metal Compiler Flags
# -O3: Maximum speed optimization
# -mcpu=cortex-a76: Tells GCC to generate assembly specifically tuned for the Pi 5's architecture
# -Wall -Wextra -Wpedantic: Catch all warnings (essential for dangerous lock-free code)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mcpu=cortex-a76 -Wall -Wextra -Wpedantic")

# Add the pthread library (required for Linux threading and affinity)
find_package(Threads REQUIRED)

# Define our main benchmarking executable
add_executable(mpsc_bench main.cpp)

# Link threads to the executable
target_link_libraries(mpsc_bench PRIVATE Threads::Threads)



include(FetchContent)

set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable internal benchmark tests" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable installation" FORCE)

FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.8.3
)
FetchContent_MakeAvailable(googlebenchmark)

# Add your benchmark executable
add_executable(queue_bench benchmarks/main_bench.cpp)

# Link the benchmark library and your include folder
target_link_libraries(queue_bench benchmark::benchmark_main)
target_include_directories(queue_bench PRIVATE include)