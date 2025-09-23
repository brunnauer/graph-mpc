add_library(GraphMPC_lib STATIC)
target_sources(GraphMPC_lib 
    PRIVATE 
    message_passing.h
    examples/pi_k.h
    examples/pi_m.h
    examples/pi_r.h
    utils/bits.h
    utils/graph.h
    utils/preprocessings.h
    utils/permutation.h
    utils/random_generators.h
    utils/sharing.cpp
    utils/types.h
    io/netmp.h
    io/disk.h
    graphmpc/clip.cpp
    graphmpc/compaction.cpp
    graphmpc/deduplication.cpp
    graphmpc/message_passing.cpp
    graphmpc/mul.cpp
    graphmpc/shuffle.cpp
    graphmpc/sort.cpp
    # Add further files if necessary
)
target_link_libraries(GraphMPC_lib PUBLIC OpenSSL::SSL EMPTool SOCKET GMP OpenMP::OpenMP_CXX nlohmann_json::nlohmann_json)
target_include_directories(GraphMPC_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(GraphMPC_lib PRIVATE cxx_std_17)

include_directories("opt/homebrew/opt/libomp/include")
link_directories("/opt/homebrew/opt/libomp/lib/")


#add_executable(GraphMPC_src)
#target_sources(GraphMPC_src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
#target_link_libraries(GraphMPC_src PUBLIC GraphMPC_lib)
