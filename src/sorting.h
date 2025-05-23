#pragma once

#include "compaction.h"
#include "perm.h"
#include "protocol_config.h"
#include "sharing.h"
#include "shuffle.h"

namespace sort {

std::vector<Row> sort_bit_vec(ProtocolConfig &conf, std::vector<Row> &bit_vec_share) {
    Shuffle shuffle(conf, 1);

    /* Set wire to input*/
    if (bit_vec_share.size() != conf.n_rows) throw std::invalid_argument("Input share has wrong size.");

    /* Generate secret shared compaction perm */
    Permutation perm_y = compaction::get_compaction(conf, bit_vec_share);
    /* Shuffle it */
    shuffle.set_input(perm_y);
    shuffle.run();

    /* Reveal shuffled perm and compute inverse (due to way permutation works) */
    std::vector<Row> revealed = shuffle.reveal();
    Permutation perm_shuffled(revealed);
    // perm_shuffled = perm_shuffled.inverse();

    /* Shuffle input using the previous order */
    shuffle.set_input(bit_vec_share);
    shuffle.repeat();
    std::vector<Row> input_shuffled = shuffle.get_output();

    std::vector<Row> sorted_share = perm_shuffled(input_shuffled);
    return sorted_share;
}

Permutation sort_iteration(ProtocolConfig &conf, Permutation &perm, std::vector<Row> &bit_vec_share) {
    Shuffle shuffle(conf, 1);

    if (bit_vec_share.size() != conf.n_rows) throw std::invalid_argument("Input share has wrong size.");

    /* Shuffle it */
    shuffle.set_input(perm);
    shuffle.run();

    /* Reveal shuffled perm */
    std::vector<Row> revealed = shuffle.reveal();
    Permutation perm_open(revealed);

    /* Shuffle input using the previous order */
    shuffle.set_input(bit_vec_share);
    shuffle.repeat();
    std::vector<Row> input_shuffled = shuffle.get_output();
    auto input_shuffled_revealed = share::reveal(conf, input_shuffled);

    /* Apply revealed permutation to shuffled input */
    std::vector<Row> sorted_share = perm_open(input_shuffled);
    auto sorted_share_revealed = share::reveal(conf, sorted_share);

    /* Compute compaction to stable sort bit_vec_share */
    Permutation sigma_p = compaction::get_compaction(conf, sorted_share);
    auto sigma_p_vec = sigma_p.get_perm_vec();
    std::vector<Row> next_vec = perm_open.inverse()(sigma_p_vec);

    /* Unshuffle next */
    shuffle.set_input(next_vec);
    shuffle.unshuffle();
    return Permutation(shuffle.get_output());
}

Permutation get_sort(ProtocolConfig &conf, std::vector<std::vector<Row>> &bit_shares) {
    /* Compute compaction of x_0 */
    Permutation sigma = compaction::get_compaction(conf, bit_shares[0]);
    auto vec = sigma.get_perm_vec();
    auto rev = share::reveal(conf, vec);

    /* Proceed sorting with x_1, x_2, ... */
    size_t n_bits = sizeof(Row) * 8;
    for (size_t i = 1; i < n_bits; ++i) {
        auto bits = share::reveal(conf, bit_shares[i]);
        sigma = sort_iteration(conf, sigma, bit_shares[i]);
        auto vec2 = sigma.get_perm_vec();
        auto rev2 = share::reveal(conf, vec2);
        int x = 2; /* Dummy */
    }
    return sigma;
}

};  // namespace sort