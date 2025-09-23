#include <algorithm>
#include <cassert>

#include "../setup/comm.h"
#include "../setup/utils.h"
#include "../src/examples/pi_r.h"
#include "../src/utils/graph.h"

void test_pi_r(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file, Graph &g) {
    std::cout << "------ test_pi_r ------" << std::endl << std::endl;
    bool ssd = true;
    auto network = std::make_shared<io::NetIOMP>(net_conf, ssd);

    const size_t n_vertices = 5;
    const size_t n_iterations = 2;
    std::vector<Ring> weights(n_iterations);
    size_t n_bits = std::ceil(std::log2(n_vertices + 2));
    n = 17;
    PiRProtocol prot(id, rngs, network, n, n_bits, n_vertices, n_iterations, ssd);

    /*
  Graph instance:
  v1 - v2
  ||   ||
  v3   v4 - v5

  Which in list form is (kind of random order here for testing):
  (1,1,1)
  (2,2,1)
  (1,2,0)
  (4,5,0)
  (2,1,0)
  (1,3,0)
  (1,3,0)
  (3,1,0)
  (5,5,1)
  (4,4,1)
  (3,3,1)
  (3,1,0)
  (2,4,0)
  (4,2,0)
  (2,4,0)
  (5,4,0)
  (4,2,0)
  */
    if (id == P0) {
        g.add_list_entry(1, 1, 1);
        g.add_list_entry(2, 2, 1);
        g.add_list_entry(1, 2, 0);
        g.add_list_entry(4, 5, 0);
        g.add_list_entry(2, 1, 0);
        g.add_list_entry(1, 3, 0);
        g.add_list_entry(1, 3, 0);
        g.add_list_entry(3, 1, 0);
        g.add_list_entry(5, 5, 1);
    }
    if (id == P1) {
        g.add_list_entry(4, 4, 1);
        g.add_list_entry(3, 3, 1);
        g.add_list_entry(3, 1, 0);
        g.add_list_entry(2, 4, 0);
        g.add_list_entry(4, 2, 0);
        g.add_list_entry(2, 4, 0);
        g.add_list_entry(5, 4, 0);
        g.add_list_entry(4, 2, 0);
    }
    Graph g_shared = g.share_subgraphs(id, rngs, network, n_bits);
    g_shared._n_vertices = 5;  // For helper
    std::cout << "Graph size: " << g_shared.size() << std::endl;
    std::cout << "Nodes: " << g_shared.n_vertices() << std::endl;
    network->sync();

    Graph g_rev = g_shared.reveal(id, network);
    if (id != D) g_rev.print();

    prot.run(g_shared, weights, TEST, true);

    /* Preprocessing communication assertions */
    // if (id == D) {
    ///* n_elems * 4 Bytes per element */
    // size_t comm_expected_pre = 4 + PI_K_COMM_PRE(n, n_bits, n_iterations);
    // size_t comm_actual_pre = prot.comm_pre();
    // assert(comm_expected_pre == comm_actual_pre);
    //}

    ///* Evaluation communication assertions */
    // if (id != D) {
    // size_t comm_expected_eval = PI_K_COMM_ONLINE(n, n_bits, n_iterations);
    // size_t comm_actual_eval = prot.comm_eval();
    // assert(comm_expected_eval == comm_actual_eval);
    //}

    auto res_g = g_shared.reveal(id, network);

    if (id != D) {
        res_g.print();

        assert(res_g._data[0] == 4);
        assert(res_g._data[1] == 5);
        assert(res_g._data[2] == 3);
        assert(res_g._data[3] == 4);
        assert(res_g._data[4] == 3);
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
        setup::run_test(opts, test_pi_r);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}