#pragma once

#include "../src/graphmpc/mp_protocol.h"

class PiKProtocol : public MPProtocol {
   public:
    PiKProtocol(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> &network) : MPProtocol(conf, network) {}

    void add_compute_sorts() override {
        add_sort(g.src_order_bits, ctx.src_order, bits + 2);
        add_sort_iteration(ctx.dst_order, g.dst_order_bits[g.dst_order_bits.size() - 1], ctx.dst_order);
        add_sort_iteration(ctx.src_order, g.isV_inv, ctx.vtx_order);
    }

    void pre_mp() override { add_deduplication(); }

    void apply() override {}

    void post_mp() override {}
};