#pragma once

#include <nlohmann/json.hpp>

#include "../src/graphmpc/mp_protocol.h"
#include "../src/setup/cmdline.h"
#include "../src/utils/stats.h"

using json = nlohmann::json;

class Benchmark {
   public:
    Benchmark(bpo::variables_map &opts, MPProtocol *prot, std::shared_ptr<io::NetIOMP> network) : prot(prot), network(network) {
        auto conf = setup::setupBenchmark(opts);

        Graph g = prot->benchmark_graph();
        prot->set_input(g);
        network->sync();

        input_file = conf.input_file;
        save_file = conf.save_file;
        repeat = conf.repeat;
        save_output = conf.save_output;
        output_data = prot->details();
        output_data["details"].push_back({"input file", input_file});
        output_data["details"].push_back({"save file", save_file});
        output_data["details"].push_back({"repeat", repeat});
        output_data["details"].push_back({"save output", save_output});
        output_data["benchmarks_pre"] = json::array();
        output_data["benchmarks"] = json::array();
    }

    MPProtocol *prot;
    std::shared_ptr<io::NetIOMP> network;
    std::string input_file, save_file;
    size_t repeat;
    bool save_output;
    json output_data;

    void run(bool parallel = false) {
        /* Construct and share graph */
        print();
        prot->build();

        network->sync();
        for (size_t r = 0; r < repeat; ++r) {
            std::cout << "--- Repetition " << r + 1 << " ---" << std::endl;

            size_t bytes_sent_pre = 0;
            size_t bytes_sent_eval = 0;

            /* Preprocessing */
            StatsPoint start_pre(*network);
            prot->preprocess();
            StatsPoint end_pre(*network);

            /* Network Sync */
            network->sync();

            auto rbench_pre = end_pre - start_pre;
            output_data["benchmarks_pre"].push_back(rbench_pre);

            for (const auto &val : rbench_pre["communication"]) {
                bytes_sent_pre += val.get<int64_t>();
            }
            std::cout << "preprocessing time: " << rbench_pre["time"] << " ms" << std::endl;
            std::cout << "preprocessing sent: " << bytes_sent_pre << " bytes" << std::endl;

            /* Evaluation */
            StatsPoint start_online(*network);
            prot->evaluate();
            StatsPoint end_online(*network);

            auto rbench = end_online - start_online;
            output_data["benchmarks"].push_back(rbench);

            network->sync();

            for (const auto &val : rbench["communication"]) {
                bytes_sent_eval += val.get<int64_t>();
            }
            std::cout << "online time: " << rbench["time"] << " ms" << std::endl;
            std::cout << "online sent: " << bytes_sent_eval << " bytes" << std::endl << std::endl;
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

    virtual void print() {
        std::cout << "--- Benchmark Details ---\n";
        for (const auto &[key, value] : output_data["details"].items()) {
            std::cout << key << ": " << value << "\n";
        }
        std::cout << std::endl;
    }
};