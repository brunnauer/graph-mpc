add_library(GraphMPC_lib STATIC)
target_sources(GraphMPC_lib 
    PRIVATE 
    utils/bits.h
    utils/comm.h
    utils/graph.h
    utils/permutation.h
    utils/random_generators.h
    utils/sharing.cpp
    utils/shuffle_preproc.h
    utils/stats.cpp
    utils/types.h
    io/disk.h
    io/netmp.h
    graphmpc/add_weights.h
    graphmpc/add.h
    graphmpc/bit2a.h
    graphmpc/compaction.h
    graphmpc/construct_payload.h
    graphmpc/deduplication.h
    graphmpc/equals_zero.h
    graphmpc/flip.h
    graphmpc/function.h
    graphmpc/gather.h
    graphmpc/merged_shuffle.h
    graphmpc/message_passing.cpp
    graphmpc/mp_protocol.cpp
    graphmpc/mul.h
    graphmpc/permute.h
    graphmpc/propagate.h
    graphmpc/reveal.h
    graphmpc/shuffle.h
    graphmpc/shuffle_repeat.h
    graphmpc/sort.cpp
    graphmpc/unshuffle.h
    graphmpc/update.h
    setup/cmdline.cpp
    setup/configs.h
    setup/input-sharing/input_client.h
    setup/input-sharing/input_server.h
    # Add further files if necessary
)
target_link_libraries(GraphMPC_lib PUBLIC OpenSSL::SSL EMPTool SOCKET GMP OpenMP::OpenMP_CXX nlohmann_json::nlohmann_json Boost::system Boost::program_options)
target_include_directories(GraphMPC_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(GraphMPC_lib PRIVATE cxx_std_17)

include_directories("opt/homebrew/opt/libomp/include")
link_directories("/opt/homebrew/opt/libomp/lib/")

add_executable(launch_client setup/input-sharing/launch_client.cpp)
target_link_libraries(launch_client GraphMPC_lib Boost::system Boost::program_options)
target_include_directories(launch_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
