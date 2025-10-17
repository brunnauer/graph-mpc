#pragma once

#include <memory>

#include "../utils/structs.h"
#include "function.h"

class Circuit {
   public:
    Circuit(ProtocolConfig &conf)
        : size(conf.size),
          bits(std::ceil(std::log2(conf.nodes + 2))),
          depth(conf.depth),
          n_wires(0),
          shuffle_idx(0),
          weights(conf.weights),
          n_shuffles(0),
          n_unshuffles(0),
          n_triples(0) {}

    std::vector<std::vector<std::shared_ptr<Function>>> get() { return circ; }

    void build();

    void level_order();

    virtual void pre_mp() = 0;
    virtual std::vector<Ring> apply(std::vector<Ring> &data_vtx) = 0;
    virtual std::vector<Ring> post_mp(std::vector<Ring> &data) = 0;
    virtual void compute_sorts();  // Can be overwritten

    size_t n_shuffles;
    size_t n_unshuffles;
    size_t n_triples;

    size_t n_wires;

   private:
    std::vector<std::vector<std::shared_ptr<Function>>> circ;
    std::vector<std::shared_ptr<Function>> f_queue;

    MPContext ctx;
    Inputs in;

    size_t size;
    size_t bits;
    size_t depth;
    size_t shuffle_idx;
    std::vector<Ring> weights;

    void set_inputs();

    void prepare_shuffles();

    std::vector<Ring> message_passing(std::vector<Ring> &data);

    std::vector<Ring> sort(std::vector<std::vector<Ring>> &bit_keys, size_t bits);

    std::vector<Ring> sort_iteration(std::vector<Ring> &perm, std::vector<Ring> &keys);

    /* ----- Single Functions ----- */

    std::vector<Ring> input();

    void output(std::vector<Ring> &input);

    std::vector<Ring> propagate_1(std::vector<Ring> &input);

    std::vector<Ring> propagate_2(std::vector<Ring> &input1, std::vector<Ring> &input2);

    std::vector<Ring> gather_1(std::vector<Ring> &input);

    std::vector<Ring> gather_2(std::vector<Ring> &input);

    std::vector<Ring> shuffle(std::vector<Ring> &input, size_t shuffle_idx);

    std::vector<Ring> unshuffle(std::vector<Ring> &input, size_t shuffle_idx);

    std::vector<Ring> merged_shuffle(std::vector<Ring> &input, size_t shuffle_idx, size_t pi_idx, size_t omega_idx);

    std::vector<Ring> compaction(std::vector<Ring> &input);

    std::vector<Ring> reveal(std::vector<Ring> &input);

    std::vector<Ring> permute(std::vector<Ring> &input, std::vector<Ring> &perm, bool inverse = false);

    std::vector<Ring> equals_zero(std::vector<Ring> &input, size_t size, size_t layer);

    std::vector<Ring> bit2A(std::vector<Ring> &input, size_t size);

    Ring sub(Ring &input1, Ring &input2);

    std::vector<Ring> mul(std::vector<Ring> &x, std::vector<Ring> &y, bool binary = false);

    std::vector<Ring> mul(std::vector<Ring> &x, std::vector<Ring> &y, size_t size, bool binary = false);

    std::vector<Ring> flip(std::vector<Ring> &input);

    std::vector<Ring> add(std::vector<Ring> &input1, std::vector<Ring> &input2);

    std::vector<Ring> add_const(std::vector<Ring> &data, Ring val);

    // void deduplication() {
    // ctx.dst_order = add_sort(g.dst_order_bits, bits + 1);
    // auto deduplication_perm = ctx.dst_order;

    // for (size_t i = 0; i < bits; ++i) {  // Was g.src_bits().size()
    // deduplication_perm = add_sort_iteration(deduplication_perm, g.src_bits[i]);
    //}

    // deduplication_perm = shuffle(deduplication_perm, shuffle_idx);
    // deduplication_perm = reveal(deduplication_perm);

    // auto deduplication_src = shuffle(g.src, shuffle_idx);
    // auto deduplication_dst = shuffle(g.dst, shuffle_idx);

    // deduplication_perm = permute(deduplication_src, deduplication_src);
    // deduplication_perm = permute(deduplication_dst, deduplication_dst);

    // auto deduplication_src_dupl = deduplication_sub(deduplication_src);
    // auto deduplication_dst_dupl = deduplication_sub(deduplication_dst);

    // deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 0);
    // deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 0);
    // deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 1);
    // deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 1);
    // deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 2);
    // deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 2);
    // deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 3);
    // deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 3);
    // deduplication_src_dupl = equals_zero(deduplication_src_dupl, size - 1, 4);
    // deduplication_dst_dupl = equals_zero(deduplication_dst_dupl, size - 1, 4);

    // auto deduplication_duplicates = mul(deduplication_src_dupl, deduplication_dst_dupl, size - 1, true);
    // deduplication_duplicates = bit2A(deduplication_duplicates, size - 1);
    // deduplication_duplicates = insert(deduplication_duplicates);

    // deduplication_duplicates = permute(deduplication_duplicates, deduplication_perm, true);
    // deduplication_duplicates = unshuffle(deduplication_duplicates, shuffle_idx);
    //// push_back(g.src_order_bits, deduplication_duplicates);
    //// push_back(g.dst_order_bits, deduplication_duplicates);
    // shuffle_idx++;
    //}

    // void add_clip() {
    //  for (size_t i = 0; i < w.mp_data_parallel.size(); ++i) {
    //  std::vector<Ring> &wire = w.data;
    //  if (w.mp_data_parallel.size() == 0) {
    //  wire = equals_zero(wire, size, 0);
    //  wire = equals_zero(wire, size, 1);
    //  wire = equals_zero(wire, size, 2);
    //  wire = equals_zero(wire, size, 3);
    //  wire = equals_zero(wire, size, 4);
    //  wire = bit2A(wire, size);
    //  wire = flip(wire);
    // } else {
    //  w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 0);
    //  w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 1);
    //  w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 2);
    //  w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 3);
    //  w.mp_data_parallel[i] = equals_zero(w.mp_data_parallel[i], size, 4);
    //  w.mp_data_parallel[i] = bit2A(w.mp_data_parallel[i], size);
    //  w.mp_data_parallel[i] = flip(w.mp_data_parallel[i]);
    // }
    // }
    //}
};