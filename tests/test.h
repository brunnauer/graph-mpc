#pragma once

#include <nlohmann/json.hpp>

#include "../src/graphmpc/circuit.h"
#include "../src/graphmpc/evaluator.h"
#include "../src/graphmpc/preprocessor.h"
#include "../src/graphmpc/storage.h"
#include "../src/setup/cmdline.h"
#include "../src/utils/stats.h"

using json = nlohmann::json;

class Test {
   public:
    Test(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> network) : id(conf.id), conf(conf), network(network) {}

    Circuit *circ;
    Preprocessor *preproc;
    Evaluator *eval;

    Graph g;

    Party id;
    ProtocolConfig conf;
    std::shared_ptr<io::NetIOMP> network;
    json output_data;

    virtual Circuit *create_circuit() = 0;

    virtual Graph create_graph() = 0;

    virtual void run_assertions(Graph &result) = 0;

    void run(bool parallel = false) {
        circ = create_circuit();
        g = create_graph();

        auto io = Storage(conf, circ);
        preproc = new Preprocessor(conf, &io, network);
        eval = new Evaluator(conf, &io, network);

        network->sync();
        size_t bytes_sent_pre = 0;
        size_t bytes_sent_eval = 0;

        /* Preprocessing */
        StatsPoint start_pre(*network);
        preproc->run(circ);
        StatsPoint end_pre(*network);

        auto rbench_pre = end_pre - start_pre;
        output_data["benchmarks_pre"].push_back(rbench_pre);

        for (const auto &val : rbench_pre["communication"]) {
            bytes_sent_pre += val.get<int64_t>();
        }
        std::cout << "preprocessing time: " << rbench_pre["time"] << " ms" << std::endl;
        std::cout << "preprocessing sent: " << bytes_sent_pre << " bytes" << std::endl;

        /* Network Sync */
        network->sync();

        /* Evaluation */
        StatsPoint start_online(*network);
        eval->run(circ, g);
        StatsPoint end_online(*network);

        auto rbench = end_online - start_online;
        output_data["benchmarks"].push_back(rbench);

        for (const auto &val : rbench["communication"]) {
            bytes_sent_eval += val.get<int64_t>();
        }
        std::cout << "online time: " << rbench["time"] << " ms" << std::endl;
        std::cout << "online sent: " << bytes_sent_eval << " bytes" << std::endl;

        network->sync();

        output_data["stats"] = {{"peak_virtual_memory", peakVirtualMemory()}, {"peak_resident_set_size", peakResidentSetSize()}};

        std::cout << "--- Overall Statistics ---\n";
        for (const auto &[key, value] : output_data["stats"].items()) {
            std::cout << key << ": " << value << "\n";
        }
        std::cout << std::endl;

        auto result = g.reveal(id, network);
        run_assertions(result);
    }
};