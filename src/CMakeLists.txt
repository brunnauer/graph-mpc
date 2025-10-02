add_library(GraphMPC_lib STATIC)
target_sources(GraphMPC_lib 
    PRIVATE 
    #examples/pi_k.h
    #examples/pi_m.h
    #examples/pi_r.h
    utils/bits.h
    utils/graph.h
    utils/preprocessings.h
    utils/permutation.h
    utils/random_generators.h
    utils/sharing.cpp
    utils/types.h
    utils/comm.h
    utils/stats.cpp
    io/netmp.h
    io/disk.h
    graphmpc/clip.cpp
    graphmpc/compaction.cpp
    graphmpc/deduplication.cpp
    graphmpc/message_passing.cpp
    graphmpc/mp_protocol.h
    graphmpc/mul.cpp
    graphmpc/shuffle.cpp
    graphmpc/sort.cpp
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
