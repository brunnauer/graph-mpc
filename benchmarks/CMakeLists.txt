set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/benchmarks)

set(benchbin "")

macro(add_benchmark source_name)
    add_executable(${source_name} ${source_name}.cpp benchmark.cpp)
    if(MSVC)
        target_compile_options(${source_name} PRIVATE /W4 /WX)
    else()
        target_compile_options(${source_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    target_link_libraries(${source_name} Boost::system Boost::program_options GraphMPC_lib EMPTool NTL GMP)
    list(APPEND benchbin ${source_name})
endmacro()

add_benchmark(shuffle)

add_custom_target(benchmarks)
add_dependencies(benchmarks ${benchbin})

#file(COPY ../certs DESTINATION ${PROJECT_BINARY_DIR}/benchmarks)
#file(COPY ../run_test.sh DESTINATION ${PROJECT_BINARY_DIR}/benchmarks)
#file(COPY ../run_LAN_benchmarks.sh DESTINATION ${PROJECT_BINARY_DIR}/benchmarks)
#file(COPY ../run_WAN_benchmarks.sh DESTINATION ${PROJECT_BINARY_DIR}/benchmarks)
