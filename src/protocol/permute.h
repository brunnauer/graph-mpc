#pragma once

#include "io/comm.h"
#include "shuffle.h"
#include "utils/sharing.h"
#include "utils/types.h"

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
    ShufflePre pi;         // size: n / 2n
    ShufflePre omega;      // size: n / 2n
    ShufflePre merged;     // size: n / 2n
    Permutation pi_p1;     // size: n
    Permutation omega_p2;  // size: n
};

namespace permute {

/**
 * ----- F_apply_perm -----
 */
std::vector<Ring> apply_perm(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                             std::vector<Ring> &input_share);

std::vector<Ring> apply_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                                      ShufflePre &perm_share, std::vector<Ring> &input_share);

/**
 * ----- F_reverse_perm -----
 */
std::vector<Ring> reverse_perm(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                               std::vector<Ring> &input_share);

std::vector<Ring> reverse_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                                        ShufflePre &pi, std::vector<Ring> &unshuffle_B, std::vector<Ring> &input_share);

/**
 * ----- F_sw_perm -----
 */
std::vector<Ring> switch_perm(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &p1, Permutation &p2,
                              std::vector<Ring> &input_share);

SwitchPermPreprocessing switch_perm_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n);

SwitchPermPreprocessing_Dealer switch_perm_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n);

SwitchPermPreprocessing switch_perm_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals, size_t &idx);

std::vector<Ring> switch_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &p1, Permutation &p2,
                                       SwitchPermPreprocessing &preproc, std::vector<Ring> &input_share);

};  // namespace permute