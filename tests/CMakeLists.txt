set(TEST_SOURCES 
    ${PROJECT_SOURCE_DIR}/src/prng/csprng.cpp 
    ${PROJECT_SOURCE_DIR}/src/utils/types.h 
    ${PROJECT_SOURCE_DIR}/src/random_generators.cpp
    ${PROJECT_SOURCE_DIR}/src/shuffle.cpp
)

set(TEST_HEADERS 
    ${PROJECT_SOURCE_DIR}/src/prng/duthomhas/csprng.hpp
    ${PROJECT_SOURCE_DIR}/src/random_generators.h 
    ${PROJECT_SOURCE_DIR}/src/shuffle.h
    ${PROJECT_SOURCE_DIR}/src/perm.h
)

set(TEST_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/tests)


add_subdirectory(test_perm)
add_subdirectory(test_shuffle)