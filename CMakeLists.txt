cmake_minimum_required(VERSION 3.10)

project(GraphMPC VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.72.0 COMPONENTS unit_test_framework program_options system REQUIRED)

include(CTest)
enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(benchmarks)