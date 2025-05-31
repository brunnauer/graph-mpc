#include <cassert>

#include "../../setup/setup.h"
#include "../../src/utils/random_generators.h"
#include "../../src/utils/sharing.h"

void test_sharing(const bpo::variables_map &opts) {
    std::cout << "------ test_sharing ------" << std::endl << std::endl;
    auto vec_size = opts["vec-size"].as<size_t>();

    size_t pid, nP, repeat, threads, shuffle_num, nodes;
    std::shared_ptr<io::NetIOMP> network = nullptr;
    uint64_t seeds_h[9];
    uint64_t seeds_l[9];
    json output_data;
    bool save_output;
    std::string save_file;

    setup::setupExecution(opts, pid, nP, repeat, threads, shuffle_num, nodes, network, seeds_h, seeds_l, save_output, save_file);
    output_data["details"] = {{"pid", pid},         {"num_parties", nP}, {"threads", threads},  {"seeds_h", seeds_h},
                              {"seeds_l", seeds_l}, {"repeat", repeat},  {"vec-size", vec_size}};

    std::cout << "--- Details ---\n";
    for (const auto &[key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    Party party = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);
    ProtocolConfig c(party, rngs, network, vec_size, 1000000);

    std::vector<Row> share(vec_size);
    std::vector<Row> input_table(vec_size);
    std::vector<Row> reconstructed;

    for (size_t i = 0; i < vec_size; ++i) {
        input_table[i] = i;
    }

    share = share::random_share_secret_vec_2P(c, input_table);
    reconstructed = share::reveal_vec(c, share);

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
    g.size = 8;
    std::vector<Row> src({0, 1, 2, 3, 0, 1, 2, 3});
    std::vector<Row> dst({0, 1, 2, 3, 1, 2, 0, 2});
    std::vector<Row> isV({1, 1, 1, 1, 0, 0, 0, 0});
    std::vector<Row> payload({1, 2, 3, 4, 0, 0, 0, 0});
    g.src = src;
    g.dst = dst;
    g.isV = isV;
    g.payload = payload;

    SecretSharedGraph shared_graph = share::random_share_graph(c, g);
    Graph reconstructed_graph = share::reveal_graph(c, shared_graph);

    std::cout << "Initial graph: " << std::endl;
    g.print();
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
        test_sharing(opts);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}