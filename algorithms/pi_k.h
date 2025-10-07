#pragma once

#include "../src/graphmpc/mp_protocol.h"

class PiKProtocol : public MPProtocol {
   public:
    PiKProtocol(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> &network) : MPProtocol(conf, network) {}

    void pre_mp() override {
        add_deduplication();
        // deduplication_preprocess(id, rngs, network, size, bits, preproc, recv_shuffle, recv_mul, ssd); }
    }

    void apply() override { add_update(w.mp_data, g._data); }

    void post_mp() override {}
};