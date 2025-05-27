#include <algorithm>

#include "../setup/setup.h"
#include "../src/graph.h"
#include "../src/message_passing.h"

void benchmark(const bpo::variables_map &opts) {
    auto vec_size = opts["vec-size"].as<size_t>();

    size_t pid, nP, repeat, threads, shuffle_num, nodes;
    std::shared_ptr<io::NetIOMP> network = nullptr;
    uint64_t seeds_h[5];
    uint64_t seeds_l[5];
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
    ProtocolConfig conf(party, rngs, network, vec_size, 1000000);

    Graph g(vec_size);
    std::vector<Row> src, dst, isV, payload;
    for (size_t i = 0; i < nodes; ++i) {
        src.push_back(i);
        dst.push_back(i);
        isV.push_back(1);
        payload.push_back(0);
    }
    for (size_t i = nodes; i < vec_size; ++i) {
        Row rand = std::rand() % nodes;
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

    if (pid != D) g.print();

    for (size_t r = 0; r < repeat; ++r) {
        std::cout << "--- Repetition " << r + 1 << " ---" << std::endl;
        /*
                StatsPoint start_pre(*network);
                StatsPoint end_pre(*network);
                network->sync();

                auto rbench_pre = end_pre - start_pre;
                output_data["benchmarks_pre"].push_back(rbench_pre);
                size_t bytes_sent_pre = 0;
                for (const auto &val : rbench_pre["communication"]) {
                    bytes_sent_pre += val.get<int64_t>();
                }
                std::cout << "setup time: " << rbench_pre["time"] << " ms" << std::endl;
                std::cout << "setup sent: " << bytes_sent_pre << " bytes" << std::endl;
        */
        std::cout << "Sharing graph...";
        StatsPoint share_start(*network);
        SecretSharedGraph g_shared = share::random_share_graph(conf, g);
        StatsPoint share_end(*network);
        auto time = (share_end - share_start)["time"];
        std::cout << "Done. " << "time: " << time << std::endl << std::endl;

        StatsPoint start(*network);
        mp::run(conf, g_shared, 1, nodes);
        StatsPoint end(*network);

        std::cout << "Reconstructing graph...";
        StatsPoint reconstruct_start(*network);
        auto res_g = share::reconstruct_graph(conf, g_shared);
        StatsPoint reconstruct_end(*network);
        time = (reconstruct_end - reconstruct_start)["time"];
        std::cout << "Done. " << "time: " << time << std::endl << std::endl;

        for (auto &elem : res_g.payload) elem = std::min(elem, (Row)1);
        if (pid != D) res_g.print();

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