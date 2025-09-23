#include "../setup/comm.h"
#include "../setup/utils.h"
#include "../src/examples/pi_r.h"
#include "../src/utils/graph.h"

void benchmark(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, size_t repeat, size_t n_vertices, bool save_output,
               std::string save_file, bool ssd, std::string input_file, Graph &g) {
    size_t n_bits = std::ceil(std::log2(n_vertices + 2));
    size_t n_iterations = 0;
    std::vector<Ring> weights(n_iterations);

    /* Initialize network connection */
    auto network = std::make_shared<io::NetIOMP>(net_conf, ssd);

    PiRProtocol prot(id, rngs, network, n, n_bits, n_vertices, n_iterations, ssd, save_output, save_file);

    /* Construct and share graph */
    if (id == P0) {
        for (size_t i = 0; i < n_vertices / 2; i++) g.add_list_entry(i + 1, i + 1, 1);
        for (size_t i = 0; i < (n - n_vertices) / 2; i++) g.add_list_entry(1, 2, 0);
    }
    if (id == P1) {
        for (size_t i = n_vertices / 2; i < n_vertices; i++) g.add_list_entry(i + 1, i + 1, 1);
        for (size_t i = (n - n_vertices) / 2; i < n - n_vertices; i++) g.add_list_entry(1, 2, 0);
    }
    Graph g_shared = g.share_subgraphs(id, rngs, network, n_bits);
    network->sync();

    for (size_t r = 0; r < repeat; ++r) {
        std::cout << "--- Repetition " << r + 1 << " ---" << std::endl;
        prot.run(g_shared, weights, BENCHMARK);
    }
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark the secure computation of the reach score.");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_benchmark(opts, benchmark);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}