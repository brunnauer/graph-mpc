#pragma once

#include "../src/graphmpc/mp_protocol.h"

class PiMProtocol : public MPProtocol {
   public:
    PiMProtocol(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> &network) : MPProtocol(conf, network) {}

    void pre_mp() override {}

    void apply() override {}

    void post_mp() override {}
};