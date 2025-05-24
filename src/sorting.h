#pragma once

#include "compaction.h"
#include "perm.h"
#include "protocol_config.h"
#include "sharing.h"
#include "shuffle.h"
#include "shuffle_new.h"

namespace sort {

std::vector<Row> sort_bit_vec(ProtocolConfig &conf, std::vector<Row> &bit_vec_share) {
    if (bit_vec_share.size() != conf.n_rows) throw std::invalid_argument("Input share has wrong size.");

    PermShare perm_share = get_shuffle(conf);

    /* Generate secret shared compaction perm */
    Permutation perm_y = compaction::get_compaction(conf, bit_vec_share);
    /* Shuffle it */
    std::vector<Row> perm_shuffled = shuffle(conf, perm_y, perm_share, true);

    /* Reveal shuffled perm and compute inverse (due to way permutation works) */
    std::vector<Row> revealed = share::reveal(conf, perm_shuffled);
    Permutation perm_opened(revealed);

    /* Shuffle input using the previous order */
    std::vector<Row> input_shuffled = shuffle(conf, bit_vec_share, perm_share, false);

    std::vector<Row> sorted_share = perm_opened(input_shuffled);
    return sorted_share;
}

Permutation sort_iteration(ProtocolConfig &conf, Permutation &perm, std::vector<Row> &bit_vec_share) {
    if (bit_vec_share.size() != conf.n_rows) throw std::invalid_argument("Input share has wrong size.");

    /* Shuffle perm */
    PermShare perm_share = get_shuffle(conf);
    std::vector<Row> perm_shuffled = shuffle(conf, perm, perm_share, true);

    /* Reveal shuffled perm */
    std::vector<Row> revealed = share::reveal(conf, perm_shuffled);
    Permutation perm_open = Permutation(revealed);

    /* Shuffle input using the previous order */
    std::vector<Row> input_shuffled = shuffle(conf, bit_vec_share, perm_share, false);

    /* Apply revealed permutation to shuffled input */
    std::vector<Row> sorted_share = perm_open(input_shuffled);

    /* Compute compaction to stable sort bit_vec_share */
    Permutation sigma_p = compaction::get_compaction(conf, sorted_share);
    auto sigma_p_vec = sigma_p.get_perm_vec();
    std::vector<Row> next_vec = perm_open.inverse()(sigma_p_vec);

    /* Unshuffle next */
    return Permutation(unshuffle(conf, next_vec, perm_share));
}

Permutation get_sort(ProtocolConfig &conf, std::vector<std::vector<Row>> &bit_shares) {
    /* Compute compaction of x_0 */
    Permutation sigma = compaction::get_compaction(conf, bit_shares[0]);

    /* Proceed sorting with x_1, x_2, ... */
    size_t n_bits = sizeof(Row) * 8;
    for (size_t i = 1; i < n_bits; ++i) {
        sigma = sort_iteration(conf, sigma, bit_shares[i]);
    }
    return sigma;
}

std::vector<Row> apply_perm(ProtocolConfig &conf, Permutation &perm, std::vector<Row> &input_share) {
    /* Shuffle permutation and open it */
    PermShare perm_share = get_shuffle(conf);
    auto perm_shuffled = shuffle(conf, perm, perm_share, true);
    auto revealed = share::reveal(conf, perm_shuffled);
    Permutation perm_opened = Permutation(revealed);

    /* Shuffle input_share with same perm */
    auto shuffled_input_share = shuffle(conf, input_share, perm_share, false);

    /* Apply opened perm to input_share */
    return perm_opened(shuffled_input_share);
}

std::vector<Row> switch_perm(ProtocolConfig &conf, Permutation &p1, Permutation &p2, std::vector<Row> &input_share) {
    PermShare pi = get_shuffle(conf);
    PermShare omega = get_shuffle(conf);

    auto shuffled_p1 = shuffle(conf, p1, pi, true);
    auto shuffled_p2 = shuffle(conf, p2, omega, true);

    auto revealed_p1 = Permutation(share::reveal(conf, shuffled_p1)); /* pi(p1.get_perm_vec()) == (p1 * pi.inv) */
    auto revealed_p2 = Permutation(share::reveal(conf, shuffled_p2)); /* omega(p2.get_perm_vec()) == p2 * omega.inv */

    auto merged = get_merged_shuffle(conf, pi, omega); /* omega * pi.inv */

    auto permuted_input = revealed_p1.inverse()(input_share);
    auto permuted_input_shuffled = shuffle(conf, permuted_input, merged, false); /* (omega * pi.inv * pi * p1.inv)(input) == (omega * p1.inv)(input) */
    auto switched = revealed_p2(permuted_input_shuffled);                        /* (p2 * omega.inv * omega * p1.inv)(input) == (p2 * p1.inv)(input) */

    return switched;
}

};  // namespace sort