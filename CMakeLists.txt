cmake_minimum_required(VERSION 3.15)
project(RingBuffer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release if no build type is specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(include)

find_package(Threads REQUIRED)

# Try system-installed benchmark first
find_package(benchmark QUIET)
if(NOT benchmark_FOUND)
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    
    # Force Release mode for Google Benchmark
    set(CMAKE_BUILD_TYPE_SAVED "${CMAKE_BUILD_TYPE}")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.1
    )
    FetchContent_MakeAvailable(benchmark)
    
    # Restore original build type
    set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE_SAVED}" CACHE STRING "Build type" FORCE)
    
    # Explicitly set Release flags for benchmark targets
    target_compile_definitions(benchmark PRIVATE NDEBUG)
    target_compile_definitions(benchmark_main PRIVATE NDEBUG)
endif()

add_executable(ringbuffer src/main.cc)
add_executable(ringbuffer_benchmark src/ringbuffer_benchmark.cc)

target_link_libraries(ringbuffer PRIVATE Threads::Threads)
target_link_libraries(ringbuffer_benchmark PRIVATE benchmark::benchmark Threads::Threads)

set_target_properties(ringbuffer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

set_target_properties(ringbuffer_benchmark PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ringbuffer PRIVATE
        -O3
        -march=native
        -mtune=native
        -Wall -Wextra -pedantic
    )

    target_compile_options(ringbuffer_benchmark PRIVATE
        -O3
        -march=native
        -mtune=native
        -Wall -Wextra -pedantic
    )
endif()