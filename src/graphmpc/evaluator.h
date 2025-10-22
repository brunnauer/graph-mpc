#pragma once

#include "../utils/graph.h"
#include "../utils/structs.h"
#include "circuit.h"
#include "storage.h"

class Evaluator {
   public:
    Evaluator(ProtocolConfig &conf, Storage *store, std::shared_ptr<io::NetIOMP> network)
        : store(store), initialized(false), id(conf.id), size(conf.size), nodes(conf.nodes), rngs(&conf.rngs), network(network) {}

    void run(Circuit *circ, Graph &g);

   private:
    std::vector<Ring> data_recv;
    std::vector<Ring> mul_vals;
    std::vector<Ring> and_vals;
    std::vector<Ring> shuffle_vals;
    std::vector<Ring> reveal_vals;

    std::unordered_map<size_t, std::vector<Ring>> input_to_val;
    std::vector<Ring> output;
    Storage *store;

    std::vector<std::vector<Ring>> wires;
    std::vector<size_t> waiting;
    bool initialized;

    Party id;
    size_t size, nodes;
    RandomGenerators *rngs;

    std::shared_ptr<io::NetIOMP> network;

    long long comm_ms = 0;
    long long sr_ms = 0;

    void set_input(Graph &g);

    void evaluate_send(std::vector<std::shared_ptr<Function>> &layer);

    void online_communication();

    void evaluate_recv(std::vector<std::shared_ptr<Function>> &layer);

    std::vector<Ring> read_online(std::vector<Ring> &buffer, size_t n_elems);

    void init_waiting(Circuit *circ) {
        for (auto &layer : circ->get()) {
            for (auto &f : layer) {
                waiting[f->in1_idx]++;
                if (f->in2_idx != 0) {  // in2 set to zero means no in2
                    waiting[f->in2_idx]++;
                }
            }
        }
    }

    void update_wires(std::vector<std::shared_ptr<Function>> &layer) {
        for (auto &f : layer) {
            waiting[f->in1_idx]--;
            if (f->in2_idx != 0) {
                waiting[f->in2_idx]--;
            }
        }
#pragma omp parallel for
        for (size_t i = 0; i < waiting.size(); ++i) {
            if (waiting[i] == 0) {
                std::vector<Ring>().swap(wires[i]);  // Free memory
            }
        }
    }
};