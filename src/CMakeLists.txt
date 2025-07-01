add_library(GraphMPC_lib STATIC)
target_sources(GraphMPC_lib 
    PRIVATE 
    utils/bits.h
    utils/types.h
    utils/graph.h
    utils/random_generators.cpp 
    utils/sharing.cpp
    utils/mul.cpp
    io/tls_net_io_channel.h
    io/netmp.h
    io/comm.h
    io/network_interface.cpp
    protocol/clip.cpp
    protocol/compaction.cpp
    protocol/deduplication.cpp
    protocol/message_passing.cpp
    protocol/permute.cpp
    protocol/shuffle.cpp
    protocol/sort.cpp
    # Add further files if necessary
)
target_link_libraries(GraphMPC_lib PUBLIC OpenSSL::SSL EMPTool NTL GMP OpenMP::OpenMP_CXX nlohmann_json::nlohmann_json)
target_include_directories(GraphMPC_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories("opt/homebrew/opt/libomp/include")
link_directories("/opt/homebrew/opt/libomp/lib/")


#add_executable(GraphMPC_src)
#target_sources(GraphMPC_src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
#target_link_libraries(GraphMPC_src PUBLIC GraphMPC_lib)
