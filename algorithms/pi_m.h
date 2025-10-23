#pragma once

#include "../src/graphmpc/circuit.h"

class PiMCircuit : public Circuit {
   public:
    PiMCircuit(ProtocolConfig &conf) : Circuit(conf) {
        build();
        level_order();
    }

    void pre_mp() override {}

    size_t apply(size_t &data_old, size_t &data_new) override { return data_new; }

    size_t post_mp(size_t &data) override { return data; }
};