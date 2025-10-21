#include "../algorithms/pi_k.h"
#include "benchmark.h"

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptionsBenchmark());

    bpo::options_description cmdline("Benchmark the secure computation of the Truncated Katz Score.");
    cmdline.add(prog_opts);
    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        auto conf = setup::setupProtocol(opts);
        auto b_conf = setup::setupBenchmark(opts);
        auto network = setup::setupNetwork(opts);

        auto circuit = PiKCircuit(conf);
        circuit.build();
        circuit.level_order();

        Graph g = Graph::benchmark_graph(conf, network);

        auto benchmark = Benchmark(conf, b_conf, &circuit, network, g);
        benchmark.run();

    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}
