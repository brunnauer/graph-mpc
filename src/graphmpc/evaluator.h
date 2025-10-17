#pragma once

#include "../utils/graph.h"
#include "../utils/structs.h"
#include "circuit.h"
#include "storage.h"

class Evaluator {
   public:
    Evaluator(ProtocolConfig &conf, Storage *store, std::shared_ptr<io::NetIOMP> network)
        : store(store),
          id(conf.id),
          size(conf.size),
          nodes(conf.nodes),
          depth(conf.depth),
          bits(conf.bits),
          rngs(&conf.rngs),
          ssd(conf.ssd),
          network(network) {}

    void run(Circuit *circ, Graph &g);

   private:
    std::vector<Ring> data_recv;
    std::vector<Ring> mul_vals;
    std::vector<Ring> and_vals;
    std::vector<Ring> shuffle_vals;
    std::vector<Ring> reveal_vals;

    std::vector<Ring> input_to_val;
    std::vector<Ring> output;
    Storage *store;
    std::vector<Ring> wires;

    Party id;
    size_t size, nodes, depth, bits;
    RandomGenerators *rngs;
    bool ssd;

    std::shared_ptr<io::NetIOMP> network;

    long long comm_ms = 0;
    long long sr_ms = 0;

    void set_input(Graph &g);

    void evaluate_send(std::vector<std::shared_ptr<Function>> &layer);

    void online_communication();

    void evaluate_recv(std::vector<std::shared_ptr<Function>> &layer);

    std::vector<Ring> read_online(std::vector<Ring> &buffer, size_t n_elems);
};