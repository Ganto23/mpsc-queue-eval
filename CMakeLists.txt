cmake_minimum_required(VERSION 3.15)
project(mpsc_queue_eval VERSION 1.0)

# Enforce strict C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# HFT & Bare-Metal Compiler Flags
# -O3: Maximum speed optimization
# -mcpu=cortex-a76: Generates assembly specifically tuned for the Pi 5
# -Wall -Wextra -Wpedantic: Catch all warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mcpu=cortex-a76 -Wall -Wextra -Wpedantic")

# Add the pthread library (required for std::thread and pthread_setaffinity_np)
find_package(Threads REQUIRED)

# --- Google Benchmark Setup ---
include(FetchContent)

set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable internal benchmark tests" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable installation" FORCE)

FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.8.3
)
FetchContent_MakeAvailable(googlebenchmark)

# --- Executables ---

# 1. Your general playground (if you still want to compile main.cpp to test things)
add_executable(main_app main.cpp)
target_include_directories(main_app PRIVATE include)
target_link_libraries(main_app PRIVATE Threads::Threads)

# 2. The Heavyweight Benchmark Executable
add_executable(mpsc_bench benchmarks/main_bench.cpp)
target_include_directories(mpsc_bench PRIVATE include)

# We link benchmark::benchmark (not benchmark_main, since we use BENCHMARK_MAIN() macro)
# AND we must link Threads::Threads so our queue threads actually compile
target_link_libraries(mpsc_bench PRIVATE benchmark::benchmark Threads::Threads)