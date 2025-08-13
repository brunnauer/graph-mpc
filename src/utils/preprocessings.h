#pragma once

#include <queue>
#include <tuple>
#include <vector>

#include "permutation.h"
#include "types.h"

class ShufflePre {
   public:
    ShufflePre() = default;

    ShufflePre(Permutation &pi_0, Permutation &pi_1, Permutation &pi_0_p, Permutation &pi_1_p, std::vector<Ring> &B, std::vector<Ring> &R)
        : pi_0(pi_0), pi_1(pi_1), pi_0_p(pi_0_p), pi_1_p(pi_1_p), B(B), R(R) {};

    Permutation pi_0;
    Permutation pi_1;
    Permutation pi_0_p;
    Permutation pi_1_p;
    std::vector<Ring> B;
    std::vector<Ring> R;

    std::vector<Ring> serialize() {
        std::vector<Ring> result;
        if (pi_0.not_null()) {
            auto vec = pi_0.get_perm_vec();
            result.insert(result.end(), vec.begin(), vec.end());
        }
        if (pi_1.not_null()) {
            auto vec = pi_1.get_perm_vec();
            result.insert(result.end(), vec.begin(), vec.end());
        }
        if (pi_0_p.not_null()) {
            auto vec = pi_0_p.get_perm_vec();
            result.insert(result.end(), vec.begin(), vec.end());
        }
        if (pi_1_p.not_null()) {
            auto vec = pi_1_p.get_perm_vec();
            result.insert(result.end(), vec.begin(), vec.end());
        }
        result.insert(result.end(), B.begin(), B.end());
        result.insert(result.end(), R.begin(), R.end());
        return result;
    }

    static ShufflePre deserialize(std::vector<Ring> &input, size_t n) {
        std::vector<Ring> _pi_0 = std::vector<Ring>(input.begin(), input.begin() + n);
        std::vector<Ring> _pi_1 = std::vector<Ring>(input.begin() + n, input.begin() + 2 * n);
        std::vector<Ring> _pi_0_p = std::vector<Ring>(input.begin() + 2 * n, input.begin() + 3 * n);
        std::vector<Ring> _pi_1_p = std::vector<Ring>(input.begin() + 3 * n, input.begin() + 4 * n);
        std::vector<Ring> B = std::vector<Ring>(input.begin() + 4 * n, input.begin() + 5 * n);
        std::vector<Ring> R = std::vector<Ring>(input.begin() + 5 * n, input.begin() + 6 * n);

        Permutation pi_0(_pi_0);
        Permutation pi_1(_pi_1);
        Permutation pi_0_p(_pi_0_p);
        Permutation pi_1_p(_pi_1_p);
        return {pi_0, pi_1, pi_0_p, pi_1_p, B, R};
    }
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