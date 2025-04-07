add_library(GraphMPC_lib STATIC)
target_sources(GraphMPC_lib 
    PRIVATE 
    prng/csprng.cpp
    utils/types.h
    random_generators.cpp 
    shuffle.cpp
    # Add further files if necessary
)

target_include_directories(GraphMPC_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(GraphMPC_src)
target_sources(GraphMPC_src PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(GraphMPC_src PRIVATE GraphMPC_lib)