#pragma once

#include "../src/graphmpc/mp_protocol.h"

class PiRProtocol : public MPProtocol {
   public:
    PiRProtocol(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> &network) : MPProtocol(conf, network) {}
    void pre_mp() override {}

    void apply() override {
        // Adding instead of overwriting
        // for (size_t i = 0; i < g.size(); ++i) {
        // g._data[i] += new_data[i];
        //}
    }

    void post_mp() override {
        // Clip
        // 1. EQZ
        // 2. Bit2A
        // 3. Flip
    }
};