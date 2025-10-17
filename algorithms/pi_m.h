#pragma once

#include "../src/graphmpc/circuit.h"

class PiMCircuit : public Circuit {
   public:
    PiMCircuit(ProtocolConfig &conf) : Circuit(conf) {}

    void pre_mp() override {}

    std::vector<Ring> apply(std::vector<Ring> &data_vtx) override { return data_vtx; }

    std::vector<Ring> post_mp(std::vector<Ring> &data) override { return data; }
};