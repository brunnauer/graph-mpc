add_executable(test_bfs
    test_bfs.cpp
    ${TEST_SOURCES}
    ${TEST_HEADERS})
target_link_libraries(test_bfs PUBLIC Boost::system Boost::program_options EMPTool GMP GraphMPC_setup GraphMPC_lib)
target_include_directories(test_bfs PUBLIC ${TEST_INCLUDE_DIRS})
