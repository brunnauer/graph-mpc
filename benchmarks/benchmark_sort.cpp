#include <cassert>
#include <random>

#include "../setup/setup.h"
#include "../src/protocol/sort.h"
#include "../src/utils/perm.h"

void benchmark(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t BLOCK_SIZE, size_t repeat, size_t n_vertices,
               bool save_output, std::string save_file) {
    json output_data;

    /* Setting up the input vector */
    std::vector<Ring> input_vector(n);

    for (size_t i = 0; i < n; i++) {
        input_vector[i] = rand() % n;
    }

    std::vector<std::vector<Ring>> bits(sizeof(Ring) * 8);
    std::vector<std::vector<Ring>> bit_shares(sizeof(Ring) * 8);

    for (size_t i = 0; i < sizeof(Ring) * 8; ++i) {
        bits[i].resize(n);
        bit_shares[i].resize(n);
        for (size_t j = 0; j < n; ++j) {
            bits[i][j] = (input_vector[j] & (1 << i)) >> i;
        }
    }

    for (size_t i = 0; i < bit_shares.size(); ++i) {
        bit_shares[i] = share::random_share_secret_vec_2P(id, rngs, bits[i]);
    }

    network->init();

    for (size_t r = 0; r < repeat; ++r) {
        std::cout << "--- Repetition " << r + 1 << " ---" << std::endl;

        StatsPoint start_pre(*network);
        auto sort_preproc = sort::get_sort_preprocess(id, rngs, network, n, bit_shares.size());
        StatsPoint end_pre(*network);

        auto rbench_pre = end_pre - start_pre;
        output_data["benchmarks_pre"].push_back(rbench_pre);
        size_t bytes_sent_pre = 0;
        for (const auto &val : rbench_pre["communication"]) {
            bytes_sent_pre += val.get<int64_t>();
        }
        std::cout << "setup time: " << rbench_pre["time"] << " ms" << std::endl;
        std::cout << "setup sent: " << bytes_sent_pre << " bytes" << std::endl;

        StatsPoint start(*network);
        auto sort_share = sort::get_sort_evaluate(id, rngs, network, n, bit_shares, sort_preproc);
        StatsPoint end(*network);

        auto rbench = end - start;
        output_data["benchmarks"].push_back(rbench);

        size_t bytes_sent = 0;
        for (const auto &val : rbench["communication"]) {
            bytes_sent += val.get<int64_t>();
        }

        std::cout << "online time: " << rbench["time"] << " ms" << std::endl;
        std::cout << "online sent: " << bytes_sent << " bytes" << std::endl;

        std::cout << std::endl;
    }

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

    bpo::options_description cmdline("Benchmark the sorting protocol.");
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