#include "../algorithms/pi_r.h"
#include "benchmark.h"

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptionsBenchmark());

    bpo::options_description cmdline("Benchmark the secure computation of the reach score.");
    cmdline.add(prog_opts);
    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        auto conf = setup::setupProtocol(opts);
        auto b_conf = setup::setupBenchmark(opts);
        auto network = setup::setupNetwork(opts);

        auto circuit = PiRCircuit(conf);
        auto benchmark = Benchmark(conf, b_conf, &circuit, network);
        benchmark.run(true);

    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}

// void benchmark(BenchmarkConfig &bench_conf, NetworkConfig &net_conf) {
///* Initialize network connection */
// auto network = std::make_shared<io::NetIOMP>(net_conf, bench_conf.ssd);

// PiRProtocol prot(id, rngs, network, n, n_bits, n_vertices, depth, ssd, save_output, output_data, save_file, repeat);

///* Construct and share graph */
// Graph g;
// if (id == P0) {
// for (size_t i = 0; i < n_vertices / 2; i++) g.add_list_entry(i + 1, i + 1, 1);
// for (size_t i = 0; i < (n - n_vertices) / 2; i++) g.add_list_entry(1, 2, 0);
//}
// if (id == P1) {
// for (size_t i = n_vertices / 2; i < n_vertices; i++) g.add_list_entry(i + 1, i + 1, 1);
// for (size_t i = (n - n_vertices) / 2; i < n - n_vertices; i++) g.add_list_entry(1, 2, 0);
//}
// Graph g_shared = g.share_subgraphs(id, rngs, network, n_bits);

// prot.run(g_shared, weights, BENCHMARK, true);
//}
