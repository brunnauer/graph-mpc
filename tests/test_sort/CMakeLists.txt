add_executable(test_sort
    test_sort.cpp
    ${TEST_SOURCES}
    ${TEST_HEADERS})
target_link_libraries(test_sort PUBLIC Boost::system Boost::program_options EMPTool GMP GraphMPC_setup GraphMPC_lib)
target_include_directories(test_sort PUBLIC ${TEST_INCLUDE_DIRS})
