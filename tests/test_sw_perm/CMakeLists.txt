add_executable(test_sw_perm
    test_sw_perm.cpp
    ${TEST_SOURCES}
    ${TEST_HEADERS})
target_link_libraries(test_sw_perm PUBLIC Boost::system Boost::program_options EMPTool GMP GraphMPC_setup GraphMPC_lib)
target_include_directories(test_sw_perm PUBLIC ${TEST_INCLUDE_DIRS})
