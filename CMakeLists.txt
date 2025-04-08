cmake_minimum_required(VERSION 3.10)

project(GraphMPC VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(GMP REQUIRED)
find_package(Boost 1.72.0 COMPONENTS unit_test_framework program_options system REQUIRED)
find_package(emp-tool REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(NTL 11.0.0 REQUIRED)

if(EMP-TOOL_FOUND)
    add_library(EMPTool INTERFACE)
    target_link_libraries(EMPTool INTERFACE ${EMP-TOOL_LIBRARIES})
    target_include_directories(EMPTool INTERFACE ${EMP-TOOL_INCLUDE_DIRS})
endif()

include(CTest)
enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(benchmarks)