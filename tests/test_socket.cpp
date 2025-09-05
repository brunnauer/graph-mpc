#include "../setup/input-sharing/input_client.h"
#include "../setup/input-sharing/input_server.h"
#include "../setup/utils.h"
#include "../src/mp_protocol.h"
#include "../src/utils/graph.h"

void test_socket(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file, Graph &g) {
    bool save_to_disk = false;
    auto network = std::make_shared<io::NetIOMP>(net_conf, save_to_disk);

    std::vector<Ring> weights = {10000000, 100000, 1000, 1};
    const size_t n_vertices = 4;
    const size_t n_iterations = weights.size();
    size_t n_bits = std::ceil(std::log2(n_vertices + 2));
    n = 16;
    MPProtocol mp(id, rngs, network, n, n_bits, n_iterations, weights, save_to_disk);

    std::cout << "Graph size: " << g.size() << std::endl;
    std::cout << "Nodes: " << g.n_vertices() << std::endl;
    std::cout << "Bits: " << n_bits << std::endl;
    Graph g_rev = g.reveal(id, network);
    if (id != D) g_rev.print();

    network->sync();

    mp.run(g, TEST);

    auto res_g = g.reveal(id, network);

    if (id != D) {
        res_g.print();

        assert(res_g._data[0] == 20510023);  // 2 of length 1, 5 of length 2, 10 of length 3, 23 of length 4
        assert(res_g._data[1] == 30513025);  // 3 of length 1, 5 of length 2, 13 of length 3, 25 of length 4
        assert(res_g._data[2] == 20510023);  // 2 of length 1, 5 of length 2, 10 of length 3, 23 of length 4
        assert(res_g._data[3] == 10305013);  // 1 of length 1, 3 of length 2,  5 of length 3, 13 of length 4
    }

    return;
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Run a simple test for sorting.");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_test(opts, test_socket);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}