add_library(GraphMPC_lib STATIC)
target_sources(GraphMPC_lib 
    PRIVATE 
    utils/types.h
    graph.h
    random_generators.cpp 
    sharing.cpp
    io/tls_net_io_channel.h
    io/netmp.h
    io/comm.h
    sorting.cpp
    shuffle.cpp
    compaction.cpp
    protocol_config.h
    message_passing.h
    # Add further files if necessary
)
target_link_libraries(GraphMPC_lib PUBLIC OpenSSL::SSL EMPTool NTL GMP)

target_include_directories(GraphMPC_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(GraphMPC_src)
target_sources(GraphMPC_src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(GraphMPC_src PUBLIC GraphMPC_lib)