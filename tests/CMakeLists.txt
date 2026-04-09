set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)

set(testbin "")

macro(add_test source_name)
    add_executable(${source_name} ${source_name}.cpp)
    #if(MSVC)
        #target_compile_options(${source_name} PRIVATE /W4 /WX)
    #else()
        #target_compile_options(${source_name} PRIVATE -Wall -Wextra -Wpedantic)
    #endif()
    target_link_libraries(${source_name} Boost::system Boost::program_options GraphMPC_lib EMPTool GMP nlohmann_json::nlohmann_json)
    target_link_directories(${source_name} PRIVATE ../algorithms)
    target_sources(${source_name} PRIVATE test.h)
    list(APPEND testbin ${source_name})
endmacro()

add_test(test_perm)
add_test(test_pi_k)
add_test(test_pi_m)
add_test(test_pi_r)
add_test(test_pi_m_clients)

add_custom_target(tests)
add_dependencies(tests ${testbin})


file(COPY ../certs DESTINATION ${PROJECT_BINARY_DIR}/tests)
file(COPY ../config DESTINATION ${PROJECT_BINARY_DIR}/tests)
file(COPY ../example_graphs DESTINATION ${PROJECT_BINARY_DIR}/tests)
