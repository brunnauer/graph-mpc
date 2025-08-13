#include <cassert>
#include <random>

#include "../setup/comm.h"
#include "../setup/utils.h"
#include "../src/graphmpc/message_passing.h"
#include "../src/mp_protocol.h"
#include "../src/utils/permutation.h"

void test_mp(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file) {
    std::cout << "------ test_mp ------" << std::endl << std::endl;
    auto network = std::make_shared<io::NetIOMP>(net_conf, true);

    /**
     *      0 == 1
     *       \  //
     *         2
     */

    n = 8;
    const size_t n_iterations = 2;
    size_t n_bits = std::ceil(std::log2(n));  // Optimization
    std::cout << "n_bits: " << n_bits << std::endl;

    Graph g = Graph::parse(input_file);
    // Graph g;
    // g.add_list_entry(0, 0, 1, 1);
    // g.add_list_entry(1, 1, 1, 2);
    // g.add_list_entry(2, 2, 1, 3);
    // g.add_list_entry(0, 1, 0, 0);
    // g.add_list_entry(1, 0, 0, 0);
    // g.add_list_entry(2, 1, 0, 0);
    // g.add_list_entry(2, 0, 0, 0);
    // g.add_list_entry(2, 1, 0, 0);

    std::vector<Ring> weights(n_iterations);
    Graph g_shared = g.secret_share(id, rngs, network, n_bits, P0);

    MPProtocol mp(id, rngs, network, n, n_bits, n_iterations, weights, true);
    mp.run(g_shared, TEST);

    /* Preprocessing communication assertions */
    if (id == D) {
        size_t expected_comm_pre = MP_COMM_PRE(n, n_bits) + 4;
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

    /* Correctness assertions */
    auto res_g = g_shared.reveal(id, network);

    if (id != D) {
        res_g.print();
        assert(res_g._data[0] == 18);
        assert(res_g._data[1] == 21);
        assert(res_g._data[2] == 3);
        assert(res_g._data[3] == 0);
        assert(res_g._data[4] == 0);
        assert(res_g._data[5] == 0);
        assert(res_g._data[6] == 0);
        assert(res_g._data[7] == 0);
    }
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Run a simple test for message-passing.");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_test(opts, test_mp);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}