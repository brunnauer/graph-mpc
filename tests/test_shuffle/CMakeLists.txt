add_executable(test_shuffle
    main.cpp
    ${TEST_SOURCES}
    ${TEST_HEADERS})
target_link_libraries(test_shuffle PUBLIC Boost::system EMPTool NTL GMP)
target_include_directories(test_shuffle PUBLIC ${TEST_INCLUDE_DIRS})

add_test(test_shuffle, test_shuffle)