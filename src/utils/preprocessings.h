#pragma once

#include <queue>
#include <tuple>
#include <vector>

#include "permutation.h"
#include "types.h"

struct ShufflePre {
    Permutation pi_0;
    Permutation pi_1;
    Permutation pi_0_p;
    Permutation pi_1_p;
    std::vector<Ring> B;
    std::vector<Ring> R;
};

/**
 * Preprocessing for performing one sort iteration in the online phase
 * Total size after preprocessing: 5n / 6n for P0 / P1
 */
struct SortIterationPreprocessing {
    ShufflePre perm_share;                              // size: n/2n for P0/P1
    std::vector<std::tuple<Ring, Ring, Ring>> triples;  // size: 3n
    std::vector<Ring> unshuffle_B;                      // size: n
};

/**
 * Preprocessing for performing one sort in the online phase
 * Total size: 3n + (n_bits-1) * (5n resp. 6n)
 */
struct SortPreprocessing {
    std::vector<std::tuple<Ring, Ring, Ring>> triples;           // size: n_bits * 3n
    std::vector<SortIterationPreprocessing> sort_iteration_pre;  // size: (n_bits-1) * (5n resp. 6n)
};

struct MPPreprocessing {
    std::queue<std::tuple<Ring, Ring, Ring>> triples;
    std::queue<ShufflePre> shuffles;
    std::queue<std::vector<Ring>> unshuffles;

    /* Apply / Switch Perm*/
    ShufflePre vtx_order_shuffle;  // size: n resp. 2n
    ShufflePre src_order_shuffle;
    ShufflePre dst_order_shuffle;
    ShufflePre vtx_src_merge;
    ShufflePre src_dst_merge;
    ShufflePre dst_vtx_merge;

    /* Orders */
    Permutation src_order;
    Permutation dst_order;
    Permutation vtx_order;
    Permutation clear_shuffled_vtx_order;
    Permutation clear_shuffled_src_order;
    Permutation clear_shuffled_dst_order;

    bool deduplication = false;
};

template <typename T>
std::vector<T> extract(std::queue<T> &q, size_t n) {
    std::vector<T> result;
    result.reserve(n);  // Reserve space for efficiency
    while (n-- > 0 && !q.empty()) {
        result.push_back(q.front());
        q.pop();
    }
    return result;
}