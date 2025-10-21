#pragma once

#include "../src/graphmpc/circuit.h"

class PiKCircuit : public Circuit {
   public:
    PiKCircuit(ProtocolConfig &conf) : Circuit(conf) {}

    void compute_sorts() override {
        ctx.src_order = sort(in.src_order_bits, bits + 2);
        ctx.dst_order = sort_iteration(ctx.dst_order, in.dst_order_bits[in.dst_order_bits.size() - 1]);
        ctx.vtx_order = sort_iteration(ctx.src_order, in.isV_inv);
    }

    void pre_mp() override { /* Deduplication */
        ctx.dst_order = sort(in.dst_order_bits, bits + 1);
        auto deduplication_perm = ctx.dst_order;

        for (size_t i = 1; i < bits + 1; ++i) {
            deduplication_perm = sort_iteration(deduplication_perm, in.src_order_bits[i]);
        }

        deduplication_perm = shuffle(deduplication_perm, shuffle_idx);
        deduplication_perm = reveal(deduplication_perm);

        auto deduplication_src = shuffle(in.src, shuffle_idx);
        auto deduplication_dst = shuffle(in.dst, shuffle_idx);

        deduplication_src = permute(deduplication_src, deduplication_perm);
        deduplication_dst = permute(deduplication_dst, deduplication_perm);

        auto deduplication_src_dupl = deduplication_sub(deduplication_src);
        auto deduplication_dst_dupl = deduplication_sub(deduplication_dst);

        deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 0);
        deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 0);
        deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 1);
        deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 1);
        deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 2);
        deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 2);
        deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 3);
        deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 3);
        deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 4);
        deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 4);

        auto deduplication_duplicates = mul(deduplication_src_dupl, deduplication_dst_dupl, size - 1, true);
        deduplication_duplicates = bit2A(deduplication_duplicates, size - 1);
        deduplication_duplicates = deduplication_insert(deduplication_duplicates);

        deduplication_duplicates = reverse_permute(deduplication_duplicates, deduplication_perm);
        auto MSBs = unshuffle(deduplication_duplicates, shuffle_idx);
        in.src_order_bits.push_back(MSBs);
        in.dst_order_bits.push_back(MSBs);
        shuffle_idx++;
    }

    size_t apply(size_t &data_old, size_t &data_new) override { return data_new; }

    size_t post_mp(size_t &data) override { return data; }
};