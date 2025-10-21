#pragma once

#include <memory>

#include "../utils/structs.h"
#include "function.h"

class Circuit {
   public:
    Circuit(ProtocolConfig &conf)
        : n_shuffles(0), n_unshuffles(0), n_mults(0), n_wires(0), size(conf.size), bits(conf.bits), depth(conf.depth), shuffle_idx(0), weights(conf.weights) {}

    std::vector<std::vector<std::shared_ptr<Function>>> get() { return circ; }

    void build();

    void level_order();

    virtual void pre_mp() = 0;
    virtual size_t apply(size_t &data_old, size_t &data_new) = 0;
    virtual size_t post_mp(size_t &data) = 0;
    virtual void compute_sorts();  // Can be overwritten

    size_t n_shuffles;
    size_t n_unshuffles;
    size_t n_mults;

    size_t n_wires;

   protected:
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

    size_t message_passing(size_t &data);

    size_t sort(std::vector<size_t> &bit_keys, size_t bits);

    size_t sort_iteration(size_t &perm, size_t &keys);

    /* ----- Single Functions ----- */

    size_t input();

    void output(size_t &input);

    size_t propagate_1(size_t &input);

    size_t propagate_2(size_t &input1, size_t &input2);

    size_t gather_1(size_t &input);

    size_t gather_2(size_t &input);

    size_t shuffle(size_t &input, size_t shuffle_idx);

    size_t unshuffle(size_t &input, size_t shuffle_idx);

    size_t merged_shuffle(size_t &input, size_t shuffle_idx, size_t pi_idx, size_t omega_idx);

    size_t compaction(size_t &input);

    size_t reveal(size_t &input);

    size_t permute(size_t &input, size_t &perm);

    size_t reverse_permute(size_t &input, size_t &perm);

    size_t equals_zero(size_t &input, size_t size, size_t layer);

    size_t bit2A(size_t &input, size_t size);

    size_t deduplication_sub(size_t &input1);

    size_t deduplication_insert(size_t &input1);

    size_t mul(size_t &x, size_t &y, bool binary = false);

    size_t mul(size_t &x, size_t &y, size_t size, bool binary = false);

    size_t flip(size_t &input);

    size_t add(size_t &input1, size_t &input2);

    size_t add_const(size_t &data, Ring val);

    size_t construct_data(std::vector<size_t> &parallel_data);
};