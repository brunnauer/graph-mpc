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

    std::vector<Ring> vtx_order;
    std::vector<Ring> src_order;
    std::vector<Ring> dst_order;
    std::vector<Ring> clear_shuffled_vtx_order;
    std::vector<Ring> clear_shuffled_src_order;
    std::vector<Ring> clear_shuffled_dst_order;

    size_t vtx_shuffle_idx;
    size_t src_shuffle_idx;
    size_t dst_shuffle_idx;
};

struct Inputs {
    std::vector<std::vector<Ring>> src_order_bits;
    std::vector<std::vector<Ring>> dst_order_bits;
    std::vector<Ring> isV_inv;
    std::vector<Ring> data;
};
