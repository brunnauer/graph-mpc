#include "../algorithms/pi_k.h"
#include "benchmark.h"

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptionsBenchmark());

    bpo::options_description cmdline("Benchmark the secure computation of the reach score.");
    cmdline.add(prog_opts);
    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        auto network = setup::setupNetwork(opts);
        auto conf = setup::setupProtocol(opts);

        auto protocol = PiKProtocol(conf, network);
        auto benchmark = Benchmark(opts, &protocol, network);

        benchmark.run(true);

    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}
