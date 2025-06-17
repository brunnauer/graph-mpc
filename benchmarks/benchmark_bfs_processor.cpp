#include <algorithm>

#include "../setup/setup.h"
#include "../src/processor.h"
#include "../src/utils/graph.h"

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
    output_data["benchmarks_pre"] = json::array();
    output_data["benchmarks"] = json::array();

    std::cout << "--- Details ---\n";
    for (const auto &[key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    Party party = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);
    const size_t BLOCK_SIZE = 1000000;
    const size_t n_iterations = 1;

    Graph g(vec_size);
    std::vector<Ring> src, dst, isV, payload;
    for (size_t i = 0; i < nodes; ++i) {
        src.push_back(i);
        dst.push_back(i);
        isV.push_back(1);
        payload.push_back(0);
    }
    for (size_t i = nodes; i < vec_size; ++i) {
        Ring rand = std::rand() % nodes;
        src.push_back(rand);
        dst.push_back((rand + 1) % nodes);
        isV.push_back(0);
        payload.push_back(0);
    }
    payload[0] = 1;

    g.src = src;
    g.dst = dst;
    g.isV = isV;
    g.payload = payload;

    Processor proc(party, rngs, network, g.size, BLOCK_SIZE);
    SecretSharedGraph g_shared = share::random_share_graph(party, rngs, g);
    proc.set_graph(g_shared);

    for (size_t r = 0; r < repeat; ++r) {
        std::cout << "--- Repetition " << r + 1 << " ---" << std::endl;

        StatsPoint start_pre(*network);
        proc.run_mp_preprocessing(n_iterations);
        StatsPoint end_pre(*network);
        network->sync();

        auto rbench = end_pre - start_pre;
        output_data["benchmarks"].push_back(rbench);

        size_t bytes_sent = 0;
        for (const auto &val : rbench["communication"]) {
            bytes_sent += val.get<int64_t>();
        }

        std::cout << "preprocessing time: " << rbench["time"] << " ms" << std::endl;
        std::cout << "preprocessing sent: " << bytes_sent << " bytes" << std::endl;

        StatsPoint start(*network);
        proc.run_mp_evaluation(n_iterations, nodes);
        StatsPoint end(*network);

        rbench = end - start;
        output_data["benchmarks"].push_back(rbench);

        bytes_sent = 0;
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

    exit(0);
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark a simple test for shuffling and unshuffling");
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