add_executable(test_sharing
    test_sharing.cpp
    ${TEST_SOURCES}
    ${TEST_HEADERS})
target_link_libraries(test_sharing PUBLIC Boost::system Boost::program_options EMPTool GMP GraphMPC_lib GraphMPC_setup)
target_include_directories(test_sharing PUBLIC ${TEST_INCLUDE_DIRS})
