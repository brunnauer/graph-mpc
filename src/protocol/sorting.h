#pragma once

#include "../utils/perm.h"
#include "../utils/protocol_config.h"
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

struct SwitchPermPreprocessing_Dealer {
    std::vector<Ring> pi_B_0;
    std::vector<Ring> pi_shares_P1;
    std::vector<Ring> omega_B_0;
    std::vector<Ring> omega_shares_P1;
    std::vector<Ring> merged_B_0;
    std::vector<Ring> merged_B_1;
    std::vector<Ring> sigma_0_p;
    std::vector<Ring> sigma_1;

    std::tuple<std::vector<Ring>, std::vector<Ring>> to_vals() {
        std::vector<Ring> vals_P0;
        std::vector<Ring> vals_P1;

        vals_P0.insert(vals_P0.end(), pi_B_0.begin(), pi_B_0.end());
        vals_P0.insert(vals_P0.end(), omega_B_0.begin(), omega_B_0.end());
        vals_P0.insert(vals_P0.end(), merged_B_0.begin(), merged_B_0.end());
        vals_P0.insert(vals_P0.end(), sigma_0_p.begin(), sigma_0_p.end());

        vals_P1.insert(vals_P1.end(), pi_shares_P1.begin(), pi_shares_P1.end());
        vals_P1.insert(vals_P1.end(), omega_shares_P1.begin(), omega_shares_P1.end());
        vals_P1.insert(vals_P1.end(), merged_B_1.begin(), merged_B_1.end());
        vals_P1.insert(vals_P1.end(), sigma_1.begin(), sigma_1.end());

        return {vals_P0, vals_P1};
    }
};

struct SwitchPermPreprocessing {
    ShufflePre pi_share;      // size: n / 2n
    ShufflePre omega_share;   // size: n / 2n
    ShufflePre merged_share;  // size: n / 2n
};

namespace sort {

/**
 * ----- F_get_sort -----
 */

Permutation get_sort(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                     std::vector<std::vector<Ring>> &bit_shares);

SortPreprocessing get_sort_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, size_t n_bits);

SortPreprocessing_Dealer get_sort_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n, size_t n_bits);

SortPreprocessing get_sort_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, size_t n_bits, std::vector<Ring> &vals, size_t &idx);

Permutation get_sort_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                              std::vector<std::vector<Ring>> &bit_shares, SortPreprocessing &preproc);

/**
 * ----- F_sort_iteration -----
 */
Permutation sort_iteration(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &perm,
                           std::vector<Ring> &bit_vec_share);

SortIterationPreprocessing sort_iteration_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE);

SortIterationPreprocessing_Dealer sort_iteration_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n);

SortIterationPreprocessing sort_iteration_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals, size_t &idx);

Permutation sort_iteration_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &perm,
                                    std::vector<Ring> &bit_shares, SortIterationPreprocessing &preproc);

/**
 * ----- F_apply_perm -----
 */
std::vector<Ring> apply_perm(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &perm,
                             std::vector<Ring> &input_share);

std::vector<Ring> apply_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &perm,
                                      ShufflePre &perm_share, std::vector<Ring> &input_share);

/**
 * ----- F_reverse_perm -----
 */
std::vector<Ring> reverse_perm(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &perm,
                               std::vector<Ring> &input_share);

std::vector<Ring> reverse_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &perm,
                                        ShufflePre &pi, std::vector<Ring> &unshuffle_B, std::vector<Ring> &input_share);

/**
 * ----- F_sw_perm -----
 */
std::vector<Ring> switch_perm(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &p1,
                              Permutation &p2, std::vector<Ring> &input_share);

SwitchPermPreprocessing switch_perm_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE);

SwitchPermPreprocessing_Dealer switch_perm_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n);

SwitchPermPreprocessing switch_perm_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals, size_t &idx);

std::vector<Ring> switch_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &p1,
                                       Permutation &p2, ShufflePre &pi, ShufflePre &omega, ShufflePre &merged, std::vector<Ring> &input_share);

};  // namespace sort