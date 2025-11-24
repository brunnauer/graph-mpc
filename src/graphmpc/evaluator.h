#pragma once

#include "../utils/graph.h"
#include "../utils/structs.h"
#include "circuit.h"
#include "storage.h"

class Evaluator {
   public:
    Evaluator(ProtocolConfig &conf, Storage *data, std::shared_ptr<io::NetIOMP> network, Graph &g)
        : data(data), initialized(false), id(conf.id), size(conf.size), nodes(conf.nodes), rngs(&conf.rngs), network(network) {
        if (id != D) {
            size_t idx = 0;

            for (size_t i = 0; i < g.src_bits.size(); ++i) {
                input_to_val[idx] = g.src_bits[i];
                idx++;
            }
            for (size_t i = 0; i < g.dst_bits.size(); ++i) {
                input_to_val[idx] = g.dst_bits[i];
                idx++;
            }

            input_to_val[idx] = g.src;
            idx++;
            input_to_val[idx] = g.dst;
            idx++;
            input_to_val[idx] = g.isV_inv;
            idx++;
            input_to_val[idx] = g.data;
            idx++;

            for (size_t i = 0; i < g.data_parallel.size(); ++i) {
                input_to_val[idx] = g.data_parallel[i];
                idx++;
            }
        }
    }

    void run(Circuit *circ);

    const std::vector<Ring> result();

   private:
    std::vector<Ring> data_recv;
    std::vector<Ring> mul_vals;
    std::vector<Ring> and_vals;
    std::vector<Ring> shuffle_vals;
    std::vector<Ring> reveal_vals;

    std::unordered_map<size_t, std::vector<Ring>> input_to_val;
    std::vector<Ring> output;
    Storage *data;

    std::vector<std::vector<Ring>> wires;
    std::vector<size_t> waiting;
    bool initialized;

    Party id;
    size_t size, nodes;
    RandomGenerators *rngs;

    std::shared_ptr<io::NetIOMP> network;

    size_t mul_idx = 0;
    size_t and_idx = 0;
    size_t shuffle_idx = 0;
    size_t reveal_idx = 0;

    void evaluate_send(std::vector<std::shared_ptr<Gate>> &layer);

    void online_communication();

    void evaluate_recv(std::vector<std::shared_ptr<Gate>> &layer);

    std::vector<Ring> read_online(std::vector<Ring> &buffer, size_t n_elems, size_t &idx);

    void update_wires(size_t &in1_idx, size_t &in2_idx) {
        waiting[in1_idx]--;
        waiting[in2_idx]--;
        if (waiting[in1_idx] == 0) std::vector<Ring>().swap(wires[in1_idx]);  // Free memory
        if (waiting[in2_idx] == 0) std::vector<Ring>().swap(wires[in2_idx]);  // Free memory
    }

    void update_wire(size_t &in1_idx) {
        waiting[in1_idx]--;
        if (waiting[in1_idx] == 0) std::vector<Ring>().swap(wires[in1_idx]);  // Free memory
    }

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
};