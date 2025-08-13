#include <omp.h>

#include <algorithm>
#include <cassert>

#include "../setup/comm.h"
#include "../setup/utils.h"
#include "../src/graphmpc/deduplication.h"
#include "../src/mp_protocol.h"
#include "../src/utils/bits.h"
#include "../src/utils/graph.h"
#include "../src/utils/permutation.h"

void test_pi_k(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file) {
    std::cout << "------ test_pi_k ------" << std::endl << std::endl;
    auto network = std::make_shared<io::NetIOMP>(net_conf, true);

    std::vector<Ring> weights = {10000000, 100000, 1000, 1};
    const size_t n_vertices = 4;
    const size_t n_iterations = weights.size();
    size_t n_bits = std::ceil(std::log2(n_vertices + 2));
    n = 16;
    MPProtocol mp(id, rngs, network, n, n_bits, n_iterations, weights, true);

    /*
    Graph instance:
    v1 - v2
    || / ||
    v3   v4

    Which in list form is (kind of random order here for testing):
    (1,1,1) // 0
    (2,2,1) // 1
    (1,2,0) // 2
    (2,1,0) // 3
    (1,3,0) // 4
    (1,3,0) // 5
    (3,1,0) // 6
    (4,4,1) // 7
    (3,3,1) // 8
    (3,1,0) // 9
    (3,2,0) // 10
    (2,3,0) // 11
    (2,4,0) // 12
    (4,2,0) // 13
    (2,4,0) // 14
    (4,2,0) // 15
    */

    Graph g;
    if (id == P0) {
        g.add_list_entry(1, 1, 1);
        g.add_list_entry(2, 2, 1);
        g.add_list_entry(1, 2, 0);
        g.add_list_entry(2, 1, 0);
        g.add_list_entry(1, 3, 0);
        g.add_list_entry(1, 3, 0);
        g.add_list_entry(3, 1, 0);
        g.add_list_entry(4, 4, 1);
    }
    if (id == P1) {
        g.add_list_entry(3, 3, 1);
        g.add_list_entry(3, 1, 0);
        g.add_list_entry(3, 2, 0);
        g.add_list_entry(2, 3, 0);
        g.add_list_entry(2, 4, 0);
        g.add_list_entry(4, 2, 0);
        g.add_list_entry(2, 4, 0);
        g.add_list_entry(4, 2, 0);
    }
    Graph g_shared = g.share_subgraphs(id, rngs, network, n_bits);
    std::cout << "Graph size: " << g_shared.size() << std::endl;
    std::cout << "Nodes: " << g_shared.n_vertices() << std::endl;
    network->sync();

    Graph g_rev = g_shared.reveal(id, network);
    if (id != D) g_rev.print();

    mp.run(g_shared);

    /* Preprocessing communication assertions */
    if (id == D) {
        /* n_elems * 4 Bytes per element */
        size_t comm_expected_pre = 4 + PI_K_COMM_PRE(n, n_bits, n_iterations);
        size_t comm_actual_pre = mp.comm_pre();
        assert(comm_expected_pre == comm_actual_pre);
    }

    /* Evaluation communication assertions */
    if (id != D) {
        size_t comm_expected_eval = PI_K_COMM_ONLINE(n, n_bits, n_iterations);
        size_t comm_actual_eval = mp.comm_eval();
        assert(comm_expected_eval == comm_actual_eval);
    }

    auto res_g = g_shared.reveal(id, network);

    if (id != D) {
        res_g.print();

        assert(res_g._data[0] == 20510023);  // 2 of length 1, 5 of length 2, 10 of length 3, 23 of length 4
        assert(res_g._data[1] == 30513025);  // 3 of length 1, 5 of length 2, 13 of length 3, 25 of length 4
        assert(res_g._data[2] == 20510023);  // 2 of length 1, 5 of length 2, 10 of length 3, 23 of length 4
        assert(res_g._data[3] == 10305013);  // 1 of length 1, 3 of length 2,  5 of length 3, 13 of length 4
    }
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark a simple test for shuffling and unshuffling");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_test(opts, test_pi_k);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}