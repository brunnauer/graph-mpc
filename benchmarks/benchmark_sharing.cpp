#include "../setup/setup.h"
#include "../src/protocol/shuffle.h"
#include "../src/utils/protocol_config.h"
#include "../src/utils/sharing.h"

void benchmark(const bpo::variables_map &opts) {
    auto vec_size = opts["vec-size"].as<size_t>();

    size_t pid, nP, repeat, threads, shuffle_num, nodes;
    std::shared_ptr<io::NetIOMP> network = nullptr;
    uint64_t seeds_h[9];
    uint64_t seeds_l[9];
    json output_data;
    bool save_output;
    std::string save_file;

    setup::setupExecution(opts, pid, nP, repeat, threads, shuffle_num, nodes, network, seeds_h, seeds_l, save_output, save_file);
    output_data["details"] = {{"pid", pid},         {"num_parties", nP}, {"threads", threads},  {"seeds_h", seeds_h},
                              {"seeds_l", seeds_l}, {"repeat", repeat},  {"vec-size", vec_size}};
    output_data["benchmarks"] = json::array();

    std::cout << "--- Details ---\n";
    for (const auto &[key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    RandomGenerators rngs(seeds_h, seeds_l);
    Party party = (pid == 0 ? P0 : P1);
    ProtocolConfig conf(party, rngs, network, vec_size, 1000000);

    std::vector<Row> input_table(vec_size);
    for (size_t i = 0; i < vec_size; ++i) input_table[i] = i;

    StatsPoint start_share(*network);
    auto share = share::random_share_secret_vec_2P(conf, input_table);
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
    auto revealed = share::reveal_vec(conf, share);
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

    exit(0);
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark a simple test for secret sharing a vector");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        benchmark(opts);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}