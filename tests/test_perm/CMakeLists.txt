add_executable(test_perm
    main.cpp
    ${TEST_SOURCES}
    ${TEST_HEADERS})
target_link_libraries(test_perm PUBLIC Boost::system EMPTool NTL GMP)
target_include_directories(test_perm PUBLIC ${TEST_INCLUDE_DIRS})

add_test(test_perm, test_perm)