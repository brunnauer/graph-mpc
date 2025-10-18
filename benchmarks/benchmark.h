#pragma once

#include <nlohmann/json.hpp>

#include "../src/graphmpc/circuit.h"
#include "../src/graphmpc/evaluator.h"
#include "../src/graphmpc/preprocessor.h"
#include "../src/setup/cmdline.h"
#include "../src/utils/stats.h"

using json = nlohmann::json;

class Benchmark {
   public:
    Benchmark(ProtocolConfig &conf, BenchmarkConfig &b_conf, Circuit *circ, std::shared_ptr<io::NetIOMP> network)
        : conf(conf), circ(circ), io(conf, circ), network(network) {
        g = Graph::benchmark_graph(conf, network);

        preproc = new Preprocessor(conf, &io, network);
        eval = new Evaluator(conf, &io, network);

        input_file = b_conf.input_file;
        save_file = b_conf.save_file;
        repeat = b_conf.repeat;
        save_output = b_conf.save_output;

        output_data["details"] = {{"Party", conf.id},
                                  {"Size", conf.size},
                                  {"Nodes", conf.nodes},
                                  {"Depth", conf.depth},
                                  {"Bits", conf.bits},
                                  {"SSD Utilization", conf.ssd},
                                  {"Communication rounds", circ->get().size()},
                                  {"input file", input_file},
                                  {"save file", save_file},
                                  {"repeat", repeat},
                                  {"save output", save_output}};
        output_data["benchmarks_pre"] = json::array();
        output_data["benchmarks"] = json::array();
    }

    ProtocolConfig conf;
    Circuit *circ;
    Preprocessor *preproc;
    Evaluator *eval;
    Storage io;

    Graph g;

    std::shared_ptr<io::NetIOMP> network;
    std::string input_file, save_file;
    size_t repeat;
    bool save_output;
    json output_data;

    void run() {
        print();
        network->sync();
        for (size_t r = 0; r < repeat; ++r) {
            std::cout << "--- Repetition " << r + 1 << " ---" << std::endl;

            size_t bytes_sent_pre = 0;
            size_t bytes_sent_eval = 0;

            /* Preprocessing */
            StatsPoint start_pre(*network);
            preproc->run(circ);
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
            eval->run(circ, g);
            StatsPoint end_online(*network);

            auto rbench = end_online - start_online;
            output_data["benchmarks"].push_back(rbench);

            io.reset();
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