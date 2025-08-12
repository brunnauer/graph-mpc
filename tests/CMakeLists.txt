set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)

set(testbin "")


macro(add_test source_name)
    add_executable(${source_name} ${source_name}.cpp)
    #if(MSVC)
        #target_compile_options(${source_name} PRIVATE /W4 /WX)
    #else()
        #target_compile_options(${source_name} PRIVATE -Wall -Wextra -Wpedantic)
    #endif()
    target_link_libraries(${source_name} Boost::system Boost::program_options GraphMPC_lib GraphMPC_setup EMPTool NTL GMP nlohmann_json::nlohmann_json)
    list(APPEND testbin ${source_name})
endmacro()

#add_test(test_bfs)
#add_test(test_deduplication)
add_test(test_eqz)
add_test(test_mp)
add_test(test_network)
add_test(test_perm)
add_test(test_pi_k)
add_test(test_pi_m)
add_test(test_sharing)
add_test(test_shuffle)
add_test(test_sort)

add_custom_target(tests)
add_dependencies(tests ${testbin})


file(COPY ../certs DESTINATION ${PROJECT_BINARY_DIR}/tests)