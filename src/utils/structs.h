#pragma once

#include <memory>
#include <vector>

#include "../io/disk.h"
#include "../setup/configs.h"
#include "shuffle_preproc.h"
#include "types.h"

struct MPContext {
    std::unordered_map<Party, std::vector<Ring>> preproc;
    std::vector<Ring> data_recv;
    std::vector<Ring> mult_vals;
    std::vector<Ring> and_vals;
    std::vector<Ring> shuffle_vals;
    std::vector<Ring> reveal_vals;

    size_t vtx_order;
    size_t src_order;
    size_t dst_order;
    size_t clear_shuffled_vtx_order;
    size_t clear_shuffled_src_order;
    size_t clear_shuffled_dst_order;

    size_t vtx_shuffle_idx;
    size_t src_shuffle_idx;
    size_t dst_shuffle_idx;
};

struct Inputs {
    std::vector<size_t> src_order_bits;
    std::vector<size_t> dst_order_bits;
    size_t src;
    size_t dst;
    size_t isV_inv;
    size_t data;
    std::vector<size_t> data_parallel;
};
