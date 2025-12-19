#pragma once

#include <memory>

#include "../utils/structs.h"
#include "gate.h"

class Circuit {
   public:
    Circuit(ProtocolConfig &conf)
        : n_shuffles(0),
          n_mults(0),
          n_wires(0),
          shuffle_idx(0),
          size(conf.size),
          nodes(conf.nodes),
          bits(conf.bits),
          depth(conf.depth),
          weights(conf.weights) {}

    std::vector<std::vector<std::shared_ptr<Gate>>> get() { return circ; }

    virtual void pre_mp();
    virtual SIMD_wire_id apply(SIMD_wire_id &data_old, SIMD_wire_id &data_new);
    virtual SIMD_wire_id post_mp(SIMD_wire_id &data);
    virtual void compute_sorts();  // Can be overwritten

    size_t n_shuffles;
    size_t n_mults;
    size_t n_wires;
    size_t shuffle_idx;

   protected:
    std::vector<std::vector<std::shared_ptr<Gate>>> circ;
    std::vector<std::shared_ptr<Gate>> gates;

    MPContext ctx;
    Inputs in;

    size_t size;
    size_t nodes;
    size_t bits;
    size_t depth;
    std::vector<Ring> weights;

    void build();

    void level_order();

    void set_inputs();

    void prepare_shuffles();

    SIMD_wire_id message_passing(SIMD_wire_id &data);

    SIMD_wire_id sort(std::vector<SIMD_wire_id> &bit_keys, SIMD_wire_id bits);

    SIMD_wire_id sort_iteration(SIMD_wire_id &perm, SIMD_wire_id &keys);

    /* ----- Single Functions ----- */

    SIMD_wire_id input();

    void output(SIMD_wire_id &input);

    SIMD_wire_id propagate_1(SIMD_wire_id &input);

    SIMD_wire_id propagate_2(SIMD_wire_id &input1, SIMD_wire_id &input2);

    SIMD_wire_id gather_1(SIMD_wire_id &input);

    SIMD_wire_id gather_2(SIMD_wire_id &input);

    SIMD_wire_id shuffle(SIMD_wire_id &input, SIMD_wire_id shuffle_idx);

    SIMD_wire_id unshuffle(SIMD_wire_id &input, SIMD_wire_id shuffle_idx);

    SIMD_wire_id merged_shuffle(SIMD_wire_id &input, SIMD_wire_id shuffle_idx, SIMD_wire_id pi_idx, SIMD_wire_id omega_idx);

    SIMD_wire_id compaction(SIMD_wire_id &input);

    SIMD_wire_id reveal(SIMD_wire_id &input);

    SIMD_wire_id permute(SIMD_wire_id &input, SIMD_wire_id &perm);

    SIMD_wire_id reverse_permute(SIMD_wire_id &input, SIMD_wire_id &perm);

    SIMD_wire_id _equals_zero(SIMD_wire_id &input, SIMD_wire_id size, SIMD_wire_id layer);

    SIMD_wire_id equals_zero(SIMD_wire_id &input, SIMD_wire_id size);

    SIMD_wire_id bit2A(SIMD_wire_id &input, SIMD_wire_id size);

    SIMD_wire_id deduplication_sub(SIMD_wire_id &input1);

    SIMD_wire_id deduplication_insert(SIMD_wire_id &input1);

    wire_id mul(wire_id &x, wire_id &y, bool binary = false);

    SIMD_wire_id mul_SIMD(SIMD_wire_id &x, SIMD_wire_id &y, bool binary = false);

    SIMD_wire_id mul_SIMD(SIMD_wire_id &x, SIMD_wire_id &y, SIMD_wire_id size, bool binary = false);

    SIMD_wire_id flip(SIMD_wire_id &input);

    wire_id add(wire_id &input1, wire_id &input2);

    SIMD_wire_id add_SIMD(SIMD_wire_id &input1, SIMD_wire_id &input2);

    wire_id sub(wire_id &input1, wire_id &input2);

    SIMD_wire_id sub_SIMD(SIMD_wire_id &input1, SIMD_wire_id &input2);

    wire_id add_const(wire_id &data, Ring val);

    SIMD_wire_id add_const_SIMD(SIMD_wire_id &data, Ring val);

    wire_id mul_const(wire_id &data, Ring val);

    SIMD_wire_id mul_const_SIMD(SIMD_wire_id &data, Ring val);

    SIMD_wire_id construct_data(std::vector<SIMD_wire_id> &parallel_data);

    SIMD_wire_id clip(SIMD_wire_id &data);
};