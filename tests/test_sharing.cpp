#include <cassert>

#include "../setup/setup.h"
#include "../src/utils/random_generators.h"
#include "../src/utils/sharing.h"

void test_sharing(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t BLOCK_SIZE) {
    std::cout << "------ test_sharing ------" << std::endl << std::endl;
    json output_data;

    network->init();

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
    Graph g;
    g.add_list_entry(0, 0, 1, 1);
    g.add_list_entry(1, 1, 1, 2);
    g.add_list_entry(2, 2, 1, 3);
    g.add_list_entry(3, 0, 1, 4);
    g.add_list_entry(0, 1, 0, 0);
    g.add_list_entry(1, 2, 0, 0);
    g.add_list_entry(2, 0, 0, 0);
    g.add_list_entry(3, 2, 0, 0);

    std::cout << "Initial graph: " << std::endl;
    auto src_bits = to_bits(g.src, 32);
    g.src = from_bits(src_bits, g.size);
    g.print();

    SecretSharedGraph shared_graph = share::random_share_graph(id, rngs, 32, g);
    Graph reconstructed_graph = share::reveal_graph(id, network, 32, shared_graph);

    std::cout << "Reconstructed graph: " << std::endl;
    reconstructed_graph.print();

    assert(g.src == reconstructed_graph.src);
    assert(g.dst == reconstructed_graph.dst);
    assert(g.isV == reconstructed_graph.isV);
    assert(g.payload == reconstructed_graph.payload);
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