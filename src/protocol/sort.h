#pragma once

#include "../utils/perm.h"
#include "../utils/random_generators.h"
#include "../utils/sharing.h"
#include "compaction.h"
#include "shuffle.h"

struct SortIterationPreprocessing_Dealer {
    std::vector<Ring> B0;
    std::vector<Ring> shares_P1;
    std::vector<Ring> comp_vals_P1;
    std::vector<Ring> unshuffle_B0;
    std::vector<Ring> unshuffle_B1;

    std::tuple<std::vector<Ring>, std::vector<Ring>> to_vals() {
        std::vector<Ring> vals_P0, vals_P1;
        vals_P0.insert(vals_P0.end(), B0.begin(), B0.end());
        vals_P0.insert(vals_P0.end(), unshuffle_B0.begin(), unshuffle_B0.end());
        vals_P1.insert(vals_P1.end(), shares_P1.begin(), shares_P1.end());
        vals_P1.insert(vals_P1.end(), comp_vals_P1.begin(), comp_vals_P1.end());
        vals_P1.insert(vals_P1.end(), unshuffle_B1.begin(), unshuffle_B1.end());

        return {vals_P0, vals_P1};
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

struct SortPreprocessing_Dealer {
    std::vector<Ring> comp_shares_P1;
    std::vector<SortIterationPreprocessing_Dealer> sort_iteration_pre;

    std::tuple<std::vector<Ring>, std::vector<Ring>> to_vals() {
        std::vector<Ring> vals_P0;
        std::vector<Ring> vals_P1;

        for (auto &elem : comp_shares_P1) {
            vals_P1.push_back(elem);
        }

        for (size_t i = 0; i < sort_iteration_pre.size(); ++i) {
            auto [sort_it_vals_P0, sort_it_vals_P1] = sort_iteration_pre[i].to_vals();
            vals_P0.insert(vals_P0.end(), sort_it_vals_P0.begin(), sort_it_vals_P0.end());
            vals_P1.insert(vals_P1.end(), sort_it_vals_P1.begin(), sort_it_vals_P1.end());
        }

        return {vals_P0, vals_P1};
    }
};

/**
 * Preprocessing for performing one sort in the online phase
 * Total size: 3n + (n_bits-1) * (5n resp. 6n)
 */
struct SortPreprocessing {
    std::vector<std::tuple<Ring, Ring, Ring>> triples;           // size: 3n
    std::vector<SortIterationPreprocessing> sort_iteration_pre;  // size: (n_bits-1) * (5n resp. 6n)
};

namespace sort {

/**
 * ----- F_get_sort -----
 */

Permutation get_sort(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, std::vector<std::vector<Ring>> &bit_shares);

SortPreprocessing get_sort_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t n_bits);

SortPreprocessing_Dealer get_sort_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n, size_t n_bits);

SortPreprocessing get_sort_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, size_t n_bits, std::vector<Ring> &vals, size_t &idx);

Permutation get_sort_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, std::vector<std::vector<Ring>> &bit_shares,
                              SortPreprocessing &preproc);

/**
 * ----- F_sort_iteration -----
 */
Permutation sort_iteration(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                           std::vector<Ring> &bit_vec_share);

SortIterationPreprocessing sort_iteration_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n);

SortIterationPreprocessing_Dealer sort_iteration_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n);

SortIterationPreprocessing sort_iteration_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals, size_t &idx);

Permutation sort_iteration_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                                    std::vector<Ring> &bit_shares, SortIterationPreprocessing &preproc);

};  // namespace sort