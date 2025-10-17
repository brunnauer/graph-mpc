#pragma once

#include "../src/graphmpc/circuit.h"

class PiRCircuit : public Circuit {
   public:
    PiRCircuit(ProtocolConfig &conf) : Circuit(conf) {
        // w.mp_data_parallel.resize(nodes);
        // for (size_t i = 0; i < nodes; ++i) {
        // w.mp_data_parallel[i].resize(size);
        // w.mp_data_parallel[i][i] = 1;  // Set index of node to 1
        // w.mp_data_parallel[i] = share::random_share_secret_vec_2P(id, rngs, w.mp_data_parallel[i]);
        //}
    }

    void pre_mp() override {}

    std::vector<Ring> apply(std::vector<Ring> &data_vtx) override {
        return data_vtx;
        // w.buf = add(w.data, w.buf);
        // w.buf = w.data;
    }

    std::vector<Ring> post_mp(std::vector<Ring> &data) override {
        // add_clip();
        // w.data = construct_payload(w.mp_data_parallel);
        return data;
    }
};