#include "../setup/setup.h"
#include "../src/protocol/shuffle.h"
#include "../src/utils/random_generators.h"
#include "../src/utils/sharing.h"

void benchmark(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, size_t repeat, size_t n_vertices,
               bool save_output, std::string save_file) {
    json output_data;

    std::vector<Ring> input_table(n);
    for (size_t i = 0; i < n; ++i) input_table[i] = i;

    StatsPoint start_share(*network);
    auto share = share::random_share_secret_vec_2P(id, rngs, input_table);
    std::cout << "Sharing done." << std::endl;
    StatsPoint end_share(*network);
    network->sync();

    auto rbench = end_share - start_share;
    output_data["benchmarks"].push_back(rbench);

    size_t bytes_sent = 0;
    for (const auto &val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    std::cout << "share time: " << rbench["time"] << " ms" << std::endl;
    std::cout << "share sent: " << bytes_sent << " bytes" << std::endl;

    std::cout << std::endl;

    StatsPoint start_reveal(*network);
    auto revealed = share::reveal_vec(id, network, BLOCK_SIZE, share);
    std::cout << "Reveal done." << std::endl;
    StatsPoint end_reveal(*network);
    network->sync();

    rbench = end_reveal - start_reveal;
    output_data["benchmarks"].push_back(rbench);

    bytes_sent = 0;
    for (const auto &val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    std::cout << "reveal time: " << rbench["time"] << " ms" << std::endl;
    std::cout << "reveal sent: " << bytes_sent << " bytes" << std::endl;

    std::cout << std::endl;

    output_data["stats"] = {{"peak_virtual_memory", peakVirtualMemory()}, {"peak_resident_set_size", peakResidentSetSize()}};

    std::cout << "--- Overall Statistics ---\n";
    for (const auto &[key, value] : output_data["stats"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    if (save_output) {
        saveJson(output_data, save_file);
    }
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark a simple test for secret sharing a vector");
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