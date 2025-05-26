add_library(GraphMPC_lib STATIC)
target_sources(GraphMPC_lib 
    PRIVATE 
    utils/types.h
    random_generators.cpp 
    sharing.h
    io/tls_net_io_channel.h
    io/netmp.h
    io/comm.h
    sorting.h
    shuffle.cpp
    compaction.h
    protocol_config.h
    # Add further files if necessary
)
target_link_libraries(GraphMPC_lib PUBLIC OpenSSL::SSL EMPTool NTL GMP)

target_include_directories(GraphMPC_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(GraphMPC_src)
target_sources(GraphMPC_src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(GraphMPC_src PUBLIC GraphMPC_lib)