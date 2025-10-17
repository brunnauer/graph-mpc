#pragma once

#include "../src/graphmpc/circuit.h"

class PiKCircuit : public Circuit {
   public:
    PiKCircuit(ProtocolConfig &conf) : Circuit(conf) {}

    void compute_sorts() override {
        // ctx.src_order = add_sort(g.src_order_bits, bits + 2);
        // ctx.dst_order = add_sort_iteration(ctx.dst_order, g.dst_order_bits[g.dst_order_bits.size() - 1]);
        // ctx.vtx_order = add_sort_iteration(ctx.src_order, g.isV_inv);
    }

    void pre_mp() override {
        // add_deduplication();
    }

    std::vector<Ring> apply(std::vector<Ring> &data_vtx) override { return data_vtx; }

    std::vector<Ring> post_mp(std::vector<Ring> &data) override { return data; }
};