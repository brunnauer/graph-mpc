#include <algorithm>
#include <cassert>

#include "../../setup/setup.h"
#include "../../src/protocol/message_passing.h"
#include "../../src/utils/perm.h"

void test_bfs(const bpo::variables_map &opts) {
    std::cout << "------ test_bfs ------" << std::endl << std::endl;

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
    ProtocolConfig conf(party, rngs, network, 25, 1000000);

    /**
     *                      10        8
     *                      /\     /\   /\
     *                      |      |     |
     *     0 <=> 6 -> 5 ->  4  <=> 2 <=> 1 <- 7
     *                     / /\
     *                    \/  \
     *                    9    3
     */

    Graph g;
    g.size = 25;
    g.src = std::vector<Row>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 1, 1, 2, 2, 2, 3, 4, 4, 4, 5, 6, 6, 7});
    g.dst = std::vector<Row>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 6, 2, 8, 1, 4, 8, 4, 2, 9, 10, 4, 0, 5, 1});
    g.isV = std::vector<Row>({1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    g.payload = std::vector<Row>({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    if (pid != D) g.print();

    SecretSharedGraph g_shared = share::random_share_graph(conf, g);

    auto preproc = mp::run_preprocess(conf, 4);
    mp::run_evaluate(conf, g_shared, 4, 11, preproc);

    auto res_g = share::reveal_graph(conf, g_shared);
    for (auto &elem : res_g.payload) elem = std::min(elem, (Row)1);

    if (pid != D) res_g.print();

    if (pid != D) {
        /* Source node should still be discovered */
        assert(res_g.payload[0] == 1);
        /* Discovered nodes */
        assert(res_g.payload[2] == 1);
        assert(res_g.payload[4] == 1);
        assert(res_g.payload[5] == 1);
        assert(res_g.payload[6] == 1);
        assert(res_g.payload[9] == 1);
        assert(res_g.payload[10] == 1);
        /* Undiscovered nodes */
        assert(res_g.payload[1] == 0);
        assert(res_g.payload[3] == 0);
        assert(res_g.payload[7] == 0);
        assert(res_g.payload[8] == 0);
    }

    exit(0);
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark a simple test for shuffling and unshuffling");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        test_bfs(opts);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}