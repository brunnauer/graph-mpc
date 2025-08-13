#include <omp.h>

#include <algorithm>
#include <cassert>

#include "../setup/comm.h"
#include "../setup/utils.h"
#include "../src/mp_protocol.h"
#include "../src/utils/permutation.h"

void test_pi_m(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file) {
    std::cout << "------ test_pi_m ------" << std::endl << std::endl;
    auto network = std::make_shared<io::NetIOMP>(net_conf, true);
    std::vector<Ring> weights = {10000000, 100000, 1000, 1};
    const size_t n_iterations = weights.size();
    size_t n_bits = std::ceil(std::log2(n + 2));
    MPProtocol mp(id, rngs, network, 16, n_bits, n_iterations, weights, true);
    /*
    Graph instance:
    v1 - v2
    || / ||
    v3   v4

    Which in list form is (kind of random order here for testing):
    (1,1,1)
    (2,2,1)
    (1,2,0)
    (2,1,0)
    (1,3,0)
    (1,3,0)
    (3,1,0)
    (4,4,1)
    (3,3,1)
    (3,1,0)
    (3,2,0)
    (2,3,0)
    (2,4,0)
    (4,2,0)
    (2,4,0)
    (4,2,0)
    */

    // Graph g = Graph::parse(input_file);
    Graph g;
    g.add_list_entry(1, 1, 1);
    g.add_list_entry(2, 2, 1);
    g.add_list_entry(1, 2, 0);
    g.add_list_entry(2, 1, 0);
    g.add_list_entry(1, 3, 0);
    g.add_list_entry(1, 3, 0);
    g.add_list_entry(3, 1, 0);
    g.add_list_entry(4, 4, 1);
    g.add_list_entry(3, 3, 1);
    g.add_list_entry(3, 1, 0);
    g.add_list_entry(3, 2, 0);
    g.add_list_entry(2, 3, 0);
    g.add_list_entry(2, 4, 0);
    g.add_list_entry(4, 2, 0);
    g.add_list_entry(2, 4, 0);
    g.add_list_entry(4, 2, 0);
    if (id != D) g.print();
    Graph g_shared = g.secret_share(id, rngs, network, n_bits, P0);

    mp.run(g_shared, TEST);

    /* Preprocessing communication assertions */
    if (id == D) {
        size_t expected_comm_pre = MP_COMM_PRE(g.size(), n_bits) + 4;
        size_t actual_comm_pre = mp.comm_pre();
        std::cout << "Expected: " << expected_comm_pre << std::endl;
        std::cout << "Actual: " << actual_comm_pre << std::endl;
        assert(expected_comm_pre == actual_comm_pre);
    }

    /* Evaluation communication assertions */
    if (id != D) {
        size_t expected_comm_eval = MP_COMM_ONLINE(n, n_bits, n_iterations);
        size_t actual_comm_eval = mp.comm_eval();
        std::cout << "Expected: " << expected_comm_eval << std::endl;
        std::cout << "Actual: " << actual_comm_eval << std::endl;
        assert(expected_comm_eval == actual_comm_eval);
    }

    auto res_g = g_shared.reveal(id, network);

    if (id != D) {
        res_g.print();
        assert(res_g._data[0] == 31030096);  // 3 of length 1, 10 of length 2, 30 of length 3,  96 of length 4
        assert(res_g._data[1] == 41036100);  // 4 of length 1, 10 of length 2, 36 of length 3, 100 of length 4
        assert(res_g._data[2] == 31030096);  // 3 of length 1, 10 of length 2, 30 of length 3,  96 of length 4
        assert(res_g._data[3] == 20820072);  // 2 of length 1,  8 of length 2, 20 of length 3,  72 of length 4
    }
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Run a simple test for computing the Multilayer-Katz-Score.");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_test(opts, test_pi_m);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}