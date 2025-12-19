#pragma once

#include "../src/graphmpc/circuit.h"

class PiRCircuit : public Circuit {
   public:
    PiRCircuit(ProtocolConfig &conf) : Circuit(conf) {
        in.data_parallel.resize(conf.nodes);
        build();
    }

    SIMD_wire_id apply(SIMD_wire_id &data_old, SIMD_wire_id &data_new) override { return add_SIMD(data_old, data_new); }

    SIMD_wire_id post_mp(SIMD_wire_id &data) override {
        SIMD_wire_id nodes = in.data_parallel.size();
        for (size_t i = 0; i < in.data_parallel.size(); ++i) {
            in.data_parallel[i] = clip(in.data_parallel[i]);
        }
        return construct_data(in.data_parallel);
    }
};