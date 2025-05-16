add_executable(test_graph
    test_graph.cpp
    ${TEST_SOURCES}
    ${TEST_HEADERS})
target_link_libraries(test_graph PUBLIC Boost::system Boost::program_options EMPTool GMP GraphMPC_lib)
target_include_directories(test_graph PUBLIC ${TEST_INCLUDE_DIRS})
