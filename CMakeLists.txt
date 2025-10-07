cmake_minimum_required(VERSION 3.10)

project(GraphMPC VERSION 0.1 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if (APPLE)
    set(OpenMP_C "${CMAKE_C_COMPILER}")
    set(OpenMP_C_FLAGS "-Xclang -fopenmp -I/opt/homebrew/opt/libomp/include")
    set(OpenMP_C_LIB_NAMES "libomp")
    set(OpenMP_libomp_LIBRARY "omp")

    set(OpenMP_CXX "${CMAKE_CXX_COMPILER}")
    set(OpenMP_CXX_FLAGS "-Xclang -fopenmp -I/opt/homebrew/opt/libomp/include")
    set(OpenMP_CXX_LIB_NAMES "libomp")
    set(OpenMP_libomp_LIBRARY "omp")
else()
    set(OpenMP_CXX_FLAGS "-fopenmp")
endif()

find_package(OpenMP REQUIRED)
find_package(GMP REQUIRED)
find_package(Boost 1.72.0 COMPONENTS unit_test_framework program_options system REQUIRED)
find_package(emp-tool REQUIRED)
find_package(SOCKET REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json 3.9.0 REQUIRED)
find_package(NTL 11.0.0 REQUIRED)

if (APPLE)
    include_directories("/opt/homebrew/opt/libomp/include")
    link_directories("/opt/homebrew/opt/libomp/lib")
endif()

if(EMP-TOOL_FOUND)
    add_library(EMPTool INTERFACE)
    target_link_libraries(EMPTool INTERFACE ${EMP-TOOL_LIBRARIES})
    target_include_directories(EMPTool INTERFACE ${EMP-TOOL_INCLUDE_DIRS})
endif()

include(CTest)
enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
#add_subdirectory(benchmarks)