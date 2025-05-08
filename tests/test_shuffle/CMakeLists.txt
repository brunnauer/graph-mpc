add_executable(test_shuffle
    test_shuffle.cpp
    ${TEST_SOURCES}
    ${TEST_HEADERS})
target_link_libraries(test_shuffle PUBLIC Boost::system Boost::program_options EMPTool NTL GMP GraphMPC_setup GraphMPC_lib)
target_include_directories(test_shuffle PUBLIC ${TEST_INCLUDE_DIRS})
