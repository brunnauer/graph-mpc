#pragma once

#include "../src/graphmpc/circuit.h"

class PiRCircuit : public Circuit {
   public:
    PiRCircuit(ProtocolConfig &conf) : Circuit(conf) { in.data_parallel.resize(conf.nodes); }

    void pre_mp() override {}

    size_t apply(size_t &data_old, size_t &data_new) override { return add(data_old, data_new); }

    size_t post_mp(size_t &data) override {
        for (size_t i = 0; i < in.data_parallel.size(); ++i) {
            in.data_parallel[i] = equals_zero(in.data_parallel[i], size, 0);
            in.data_parallel[i] = equals_zero(in.data_parallel[i], size, 1);
            in.data_parallel[i] = equals_zero(in.data_parallel[i], size, 2);
            in.data_parallel[i] = equals_zero(in.data_parallel[i], size, 3);
            in.data_parallel[i] = equals_zero(in.data_parallel[i], size, 4);
            in.data_parallel[i] = bit2A(in.data_parallel[i], size);
            in.data_parallel[i] = flip(in.data_parallel[i]);
        }

        return construct_data(in.data_parallel);
    }
};