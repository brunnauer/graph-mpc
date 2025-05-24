add_executable(test_shuffle_new
    test_shuffle_new.cpp
    ${TEST_SOURCES}
    ${TEST_HEADERS})
target_link_libraries(test_shuffle_new PUBLIC Boost::system Boost::program_options EMPTool GMP GraphMPC_setup GraphMPC_lib)
target_include_directories(test_shuffle_new PUBLIC ${TEST_INCLUDE_DIRS})
