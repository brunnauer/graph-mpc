#include <cassert>

#include "../setup/utils.h"
#include "../src/utils/graph.h"
#include "../src/utils/random_generators.h"
#include "../src/utils/sharing.h"

void test_sharing(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file) {
    std::cout << "------ test_sharing ------" << std::endl << std::endl;
    json output_data;
    auto network = std::make_shared<io::NetIOMP>(net_conf, false);

    std::vector<Ring> share(n);
    std::vector<Ring> input_table(n);
    std::vector<Ring> reconstructed;

    for (size_t i = 0; i < n; ++i) {
        input_table[i] = i;
    }

    StatsPoint start_sharing(*network);
    share = share::random_share_secret_vec_2P(id, rngs, input_table);
    StatsPoint end_sharing(*network);

    auto rbench_sharing = end_sharing - start_sharing;
    output_data["benchmarks_pre"].push_back(rbench_sharing);
    size_t bytes_sent_sharing = 0;
    for (const auto &val : rbench_sharing["communication"]) {
        bytes_sent_sharing += val.get<int64_t>();
    }

    /* No communication when secret sharing a vector */
    assert(bytes_sent_sharing == 0);

    StatsPoint start_reveal(*network);
    reconstructed = share::reveal_vec(id, network, share);
    StatsPoint end_reveal(*network);

    auto rbench_reveal = end_reveal - start_reveal;
    output_data["benchmarks_pre"].push_back(rbench_reveal);
    size_t bytes_sent_reveal = 0;
    for (const auto &val : rbench_reveal["communication"]) {
        bytes_sent_reveal += val.get<int64_t>();
    }

    /* Each party sends its share during reveal */
    assert(bytes_sent_reveal == 4 * n);

    std::cout << "Final share: ";
    for (const auto &elem : share) {
        std::cout << elem << ", ";
    }
    std::cout << std::endl << std::endl;

    assert(input_table == reconstructed);

    std::cout << "Reconstructed: ";
    for (const auto &elem : reconstructed) {
        std::cout << elem << ", ";
    }
    std::cout << std::endl << std::endl;

    /* Test Graph Sharing */
    Graph g = Graph::parse(input_file);

    std::cout << "Personal graph: " << std::endl;
    g.print();

    Graph shared_graph = g.share_subgraphs(id, rngs, network, 32);
    Graph reconstructed_graph = shared_graph.reveal(id, network);

    std::cout << "Concatenated graph: " << std::endl;
    reconstructed_graph.print();

    // assert(2 * g.src.size() == reconstructed_graph.src.size());
    // assert(2 * g.dst.size() == reconstructed_graph.dst.size());
    // assert(2 * g.isV.size() == reconstructed_graph.isV.size());
    // assert(2 * g.payload.size() == reconstructed_graph.payload.size());

    // assert(shared_graph.src_bits.size() == 32);
    // assert(shared_graph.src_bits[0].size() == 2 * g.size);
    // assert(shared_graph.dst_bits[0].size() == 2 * g.size);
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark a simple test for shuffling and unshuffling");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_test(opts, test_sharing);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}