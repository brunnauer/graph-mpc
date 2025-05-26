set(TEST_SOURCES 
    ${PROJECT_SOURCE_DIR}/src/compaction.cpp
    ${PROJECT_SOURCE_DIR}/src/random_generators.cpp
    ${PROJECT_SOURCE_DIR}/src/sharing.cpp
    ${PROJECT_SOURCE_DIR}/src/shuffle.cpp
    ${PROJECT_SOURCE_DIR}/src/sorting.cpp
)

set(TEST_HEADERS 
    ${PROJECT_SOURCE_DIR}/src/utils/types.h 
    ${PROJECT_SOURCE_DIR}/src/random_generators.h 
    ${PROJECT_SOURCE_DIR}/src/shuffle.h
    ${PROJECT_SOURCE_DIR}/src/sorting.h
    ${PROJECT_SOURCE_DIR}/src/perm.h
    ${PROJECT_SOURCE_DIR}/src/graph.h
    ${PROJECT_SOURCE_DIR}/src/sharing.h
)

set(TEST_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/setup)

add_subdirectory(test_perm)
add_subdirectory(test_shuffle)
add_subdirectory(test_sharing)
add_subdirectory(test_graph)
add_subdirectory(test_sort)
add_subdirectory(test_sw_perm)

file(COPY ../certs DESTINATION ${PROJECT_BINARY_DIR}/tests/test_shuffle)
file(COPY ../certs DESTINATION ${PROJECT_BINARY_DIR}/tests/test_shuffle_new)
file(COPY ../certs DESTINATION ${PROJECT_BINARY_DIR}/tests/test_sw_perm)
file(COPY ../certs DESTINATION ${PROJECT_BINARY_DIR}/tests/test_sharing)
file(COPY ../certs DESTINATION ${PROJECT_BINARY_DIR}/tests/test_sort)
file(COPY ../certs DESTINATION ${PROJECT_BINARY_DIR}/tests/test_sw_perm)
