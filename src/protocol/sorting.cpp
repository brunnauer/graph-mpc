#include "sorting.h"

/**
 * ----- F_get_sort -----
 */
Permutation sort::get_sort(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                           std::vector<std::vector<Ring>> &bit_shares) {
    /* Compute compaction of x_0 */
    Permutation sigma = compaction::get_compaction(id, rngs, network, n, BLOCK_SIZE, bit_shares[0]);
    /* Proceed sorting with x_1, x_2, ... */
    size_t n_bits = bit_shares.size();
    for (size_t i = 1; i < n_bits; ++i) {
        sigma = sort_iteration(id, rngs, network, n, BLOCK_SIZE, sigma, bit_shares[i]);
    }
    return sigma;
}

SortPreprocessing sort::get_sort_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                            size_t n_bits) {
    SortPreprocessing preproc;

    /* Get one compaction preprocessing */
    preproc.triples = compaction::preprocess(id, rngs, network, n, BLOCK_SIZE);

    /* Compute the preprocessing for n_bits-1 sort_iteratinos */
    for (size_t i = 0; i < n_bits - 1; ++i) {
        SortIterationPreprocessing sort_iteration_pre;

        ShufflePre perm_share = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
        std::vector<Ring> unshuffle_B = shuffle::get_unshuffle(id, rngs, network, n, BLOCK_SIZE, perm_share);
        auto triples = compaction::preprocess(id, rngs, network, n, BLOCK_SIZE);

        sort_iteration_pre.perm_share = perm_share;
        sort_iteration_pre.unshuffle_B = unshuffle_B;
        sort_iteration_pre.triples = triples;

        preproc.sort_iteration_pre.push_back(sort_iteration_pre);
    }

    return preproc;
}

SortPreprocessing_Dealer sort::get_sort_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n, size_t n_bits) {
    SortPreprocessing_Dealer preproc;

    preproc.comp_shares_P1 = compaction::preprocess_Dealer(id, rngs, n);

    for (size_t i = 0; i < n_bits - 1; ++i) {
        SortIterationPreprocessing_Dealer sort_iteration_pre;
        auto [perm_share, B0, shares_P1] = shuffle::get_shuffle_1(id, rngs, n);
        sort_iteration_pre.B0 = B0;
        sort_iteration_pre.shares_P1 = shares_P1;

        auto [unshuffle_B0, unshuffle_B1] = shuffle::get_unshuffle_1(id, rngs, n, perm_share);
        sort_iteration_pre.unshuffle_B0 = unshuffle_B0;
        sort_iteration_pre.unshuffle_B1 = unshuffle_B1;

        auto comp_shares_P1 = compaction::preprocess_Dealer(id, rngs, n);
        sort_iteration_pre.comp_vals_P1 = comp_shares_P1;

        preproc.sort_iteration_pre.push_back(sort_iteration_pre);
    }
    return preproc;
}

SortPreprocessing sort::get_sort_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, size_t n_bits, std::vector<Ring> &vals, size_t &idx) {
    SortPreprocessing preproc;

    auto triples = compaction::preprocess_Parties(id, rngs, n, vals, idx);
    preproc.triples = triples;

    for (size_t i = 0; i < n_bits - 1; ++i) {
        SortIterationPreprocessing sort_iteration_pre;

        ShufflePre shuffle_share = shuffle::get_shuffle_2(id, rngs, n, vals, idx, true);
        auto triple = compaction::preprocess_Parties(id, rngs, n, vals, idx);
        std::vector<Ring> unshuffle_B = shuffle::get_unshuffle_2(id, n, vals, idx);

        sort_iteration_pre.perm_share = shuffle_share;
        sort_iteration_pre.triples = triple;
        sort_iteration_pre.unshuffle_B = unshuffle_B;
        preproc.sort_iteration_pre.push_back(sort_iteration_pre);
    }

    return preproc;
}

Permutation sort::get_sort_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                    std::vector<std::vector<Ring>> &bit_shares, SortPreprocessing &preproc) {
    auto triples = preproc.triples;
    Permutation sigma = compaction::evaluate(id, rngs, network, n, BLOCK_SIZE, triples, bit_shares[0]);

    size_t n_bits = bit_shares.size();
    for (size_t i = 1; i < n_bits; ++i) {
        auto sort_iteration_pre = preproc.sort_iteration_pre[i - 1];
        sigma = sort_iteration_evaluate(id, rngs, network, n, BLOCK_SIZE, sigma, bit_shares[i], sort_iteration_pre);
    }
    return sigma;
}

/**
 * ----- F_sort_iteration -----
 */

Permutation sort::sort_iteration(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &perm,
                                 std::vector<Ring> &bit_vec_share) {
    /* Shuffle perm */
    ShufflePre perm_share = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, perm, perm_share, n, BLOCK_SIZE);

    /* Reveal shuffled perm */
    Permutation perm_open = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);

    /* Shuffle input using the previous order */
    std::vector<Ring> input_shuffled = shuffle::shuffle(id, rngs, network, bit_vec_share, perm_share, n, BLOCK_SIZE);

    /* Apply revealed permutation to shuffled input */
    std::vector<Ring> sorted_share = perm_open(input_shuffled);

    /* Compute compaction to stable sort bit_vec_share */
    Permutation sigma_p = compaction::get_compaction(id, rngs, network, n, BLOCK_SIZE, sorted_share);
    auto sigma_p_vec = sigma_p.get_perm_vec();
    std::vector<Ring> next_perm = perm_open.inverse()(sigma_p_vec);

    /* Unshuffle next */
    std::vector<Ring> B = shuffle::get_unshuffle(id, rngs, network, n, BLOCK_SIZE, perm_share);
    return Permutation(shuffle::unshuffle(id, rngs, network, perm_share, B, next_perm, n, BLOCK_SIZE));
}

SortIterationPreprocessing sort::sort_iteration_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n,
                                                           size_t BLOCK_SIZE) {
    SortIterationPreprocessing preproc;

    ShufflePre perm_share = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    auto triples = compaction::preprocess(id, rngs, network, n, BLOCK_SIZE);
    std::vector<Ring> unshuffle_B = shuffle::get_unshuffle(id, rngs, network, n, BLOCK_SIZE, perm_share);

    preproc.perm_share = perm_share;
    preproc.triples = triples;
    preproc.unshuffle_B = unshuffle_B;

    return preproc;
}

SortIterationPreprocessing_Dealer sort::sort_iteration_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n) {
    SortIterationPreprocessing_Dealer preproc;
    auto [perm_share, B0, shares_P1] = shuffle::get_shuffle_1(id, rngs, n);
    auto comp_vals_P1 = compaction::preprocess_Dealer(id, rngs, n);
    auto [unshuffle_B0, unshuffle_B1] = shuffle::get_unshuffle_1(id, rngs, n, perm_share);
    preproc.B0 = B0;
    preproc.shares_P1 = shares_P1;
    preproc.comp_vals_P1 = comp_vals_P1;
    preproc.unshuffle_B0 = unshuffle_B0;
    preproc.unshuffle_B1 = unshuffle_B1;
    return preproc;
}

SortIterationPreprocessing sort::sort_iteration_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals, size_t &idx) {
    SortIterationPreprocessing preproc;
    preproc.perm_share = shuffle::get_shuffle_2(id, rngs, n, vals, idx, true);
    preproc.triples = compaction::preprocess_Parties(id, rngs, n, vals, idx);
    preproc.unshuffle_B = shuffle::get_unshuffle_2(id, n, vals, idx);
    return preproc;
}

Permutation sort::sort_iteration_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                          Permutation &perm, std::vector<Ring> &bit_shares, SortIterationPreprocessing &preproc) {
    ShufflePre perm_share = preproc.perm_share;
    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, perm, perm_share, n, BLOCK_SIZE);

    Permutation perm_open = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);

    std::vector<Ring> input_shuffled = shuffle::shuffle(id, rngs, network, bit_shares, perm_share, n, BLOCK_SIZE);

    std::vector<Ring> input_sorted = perm_open(input_shuffled);

    auto triples = preproc.triples;
    Permutation sigma_p = compaction::evaluate(id, rngs, network, n, BLOCK_SIZE, triples, input_sorted);

    auto sigma_p_vec = sigma_p.get_perm_vec();
    std::vector<Ring> next_perm = perm_open.inverse()(sigma_p_vec);

    auto unshuffle_B = preproc.unshuffle_B;
    Permutation next_perm_shuffled = Permutation(shuffle::unshuffle(id, rngs, network, perm_share, unshuffle_B, next_perm, n, BLOCK_SIZE));

    return next_perm_shuffled;
}

std::tuple<std::vector<Ring>, std::vector<Ring>> sort_iteration_evaluate_1(Party id, RandomGenerators &rngs, Permutation &perm,
                                                                           std::vector<Ring> &bit_vec_share, SortIterationPreprocessing &preproc, size_t n) {
    ShufflePre perm_share = preproc.perm_share;
    std::vector<Ring> vec_A_1 = shuffle::shuffle_1(id, rngs, n, perm, perm_share);
    std::vector<Ring> vec_A_2 = shuffle::shuffle_1(id, rngs, n, bit_vec_share, perm_share);
    return {vec_A_1, vec_A_2};
}

/**
 * ----- F_apply_perm -----
 */

std::vector<Ring> sort::apply_perm(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &perm,
                                   std::vector<Ring> &input_share) {
    /* Shuffle permutation and open it */
    ShufflePre perm_share = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    auto perm_shuffled = shuffle::shuffle(id, rngs, network, perm, perm_share, n, BLOCK_SIZE);
    auto perm_opened = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);

    /* Shuffle input_share with same perm */
    auto shuffled_input_share = shuffle::shuffle(id, rngs, network, input_share, perm_share, n, BLOCK_SIZE);

    /* Apply opened perm to input_share */
    return perm_opened(shuffled_input_share);
}

std::vector<Ring> sort::apply_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                            Permutation &perm, ShufflePre &perm_share, std::vector<Ring> &input_share) {
    auto perm_shuffled = shuffle::shuffle(id, rngs, network, perm, perm_share, n, BLOCK_SIZE);
    auto perm_opened = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);
    auto shuffled_input_share = shuffle::shuffle(id, rngs, network, input_share, perm_share, n, BLOCK_SIZE);
    return perm_opened(shuffled_input_share);
}

/**
 * ----- F_reverse_perm -----
 */
std::vector<Ring> sort::reverse_perm(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &perm,
                                     std::vector<Ring> &input_share) {
    ShufflePre pi = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    auto unshuffle_B = shuffle::get_unshuffle(id, rngs, network, n, BLOCK_SIZE, pi);

    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, perm, pi, n, BLOCK_SIZE);
    Permutation perm_opened = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);
    auto permuted_share = perm_opened.inverse()(input_share);

    auto result = shuffle::unshuffle(id, rngs, network, pi, unshuffle_B, permuted_share, n, BLOCK_SIZE);
    return result;
}

std::vector<Ring> sort::reverse_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                              Permutation &perm, ShufflePre &pi, std::vector<Ring> &unshuffle_B, std::vector<Ring> &input_share) {
    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, perm, pi, n, BLOCK_SIZE);
    Permutation perm_opened = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);
    auto permuted_share = perm_opened.inverse()(input_share);

    auto result = shuffle::unshuffle(id, rngs, network, pi, unshuffle_B, permuted_share, n, BLOCK_SIZE);
    return result;
}

/**
 * ----- F_sw_perm -----
 */

std::vector<Ring> sort::switch_perm(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &p1,
                                    Permutation &p2, std::vector<Ring> &input_share) {
    ShufflePre pi = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    ShufflePre omega = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);

    auto shuffled_p1 = shuffle::shuffle(id, rngs, network, p1, pi, n, BLOCK_SIZE);
    auto shuffled_p2 = shuffle::shuffle(id, rngs, network, p2, omega, n, BLOCK_SIZE);

    auto revealed_p1 = Permutation(share::reveal_perm(id, network, BLOCK_SIZE, shuffled_p1)); /* pi(p1.get_perm_vec()) == (p1 * pi.inv) */
    auto revealed_p2 = Permutation(share::reveal_perm(id, network, BLOCK_SIZE, shuffled_p2)); /* omega(p2.get_perm_vec()) == p2 * omega.inv */

    auto merged = shuffle::get_merged_shuffle(id, rngs, network, n, BLOCK_SIZE, pi, omega); /* omega * pi.inv */

    auto permuted_input = revealed_p1.inverse()(input_share);
    auto permuted_input_shuffled =
        shuffle::shuffle(id, rngs, network, permuted_input, merged, n, BLOCK_SIZE); /* (omega * pi.inv * pi * p1.inv)(input) == (omega * p1.inv)(input) */
    auto switched = revealed_p2(permuted_input_shuffled);                           /* (p2 * omega.inv * omega * p1.inv)(input) == (p2 * p1.inv)(input) */

    return switched;
}

SwitchPermPreprocessing sort::switch_perm_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE) {
    SwitchPermPreprocessing preproc;
    ShufflePre pi = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    ShufflePre omega = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    ShufflePre merged = shuffle::get_merged_shuffle(id, rngs, network, n, BLOCK_SIZE, pi, omega);

    preproc.pi_share = pi;
    preproc.omega_share = omega;
    preproc.merged_share = merged;

    return preproc;
}

SwitchPermPreprocessing_Dealer sort::switch_perm_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n) {
    SwitchPermPreprocessing_Dealer preproc;

    auto [pi_share, pi_B_0, pi_shares_P1] = shuffle::get_shuffle_1(id, rngs, n);
    auto [omega_share, omega_B_0, omega_shares_P1] = shuffle::get_shuffle_1(id, rngs, n);
    auto [merged_share, merged_B_0, merged_B_1, sigma_0_p, sigma_1] = shuffle::get_merged_shuffle_1(id, rngs, n, pi_share, omega_share);

    preproc.pi_B_0 = pi_B_0;
    preproc.pi_shares_P1 = pi_shares_P1;
    preproc.omega_B_0 = omega_B_0;
    preproc.omega_shares_P1 = omega_shares_P1;
    preproc.merged_B_0 = merged_B_0;
    preproc.merged_B_1 = merged_B_1;
    preproc.sigma_0_p = sigma_0_p;
    preproc.sigma_1 = sigma_1;

    return preproc;
}

SwitchPermPreprocessing sort::switch_perm_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals, size_t &idx) {
    SwitchPermPreprocessing preproc;

    ShufflePre pi_share = shuffle::get_shuffle_2(id, rngs, n, vals, idx, true);
    ShufflePre omega_share = shuffle::get_shuffle_2(id, rngs, n, vals, idx, true);
    ShufflePre merged_share = shuffle::get_merged_shuffle_2(id, n, vals, idx);

    preproc.pi_share = pi_share;
    preproc.omega_share = omega_share;
    preproc.merged_share = merged_share;
    return preproc;
}

std::vector<Ring> sort::switch_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                             Permutation &p1, Permutation &p2, ShufflePre &pi, ShufflePre &omega, ShufflePre &merged,
                                             std::vector<Ring> &input_share) {
    auto shuffled_p1 = shuffle::shuffle(id, rngs, network, p1, pi, n, BLOCK_SIZE);
    auto shuffled_p2 = shuffle::shuffle(id, rngs, network, p2, omega, n, BLOCK_SIZE);

    auto revealed_p1 = share::reveal_perm(id, network, BLOCK_SIZE, shuffled_p1);
    auto revealed_p2 = share::reveal_perm(id, network, BLOCK_SIZE, shuffled_p2);

    auto permuted_input = revealed_p1.inverse()(input_share);
    auto permuted_input_shuffled = shuffle::shuffle(id, rngs, network, permuted_input, merged, n, BLOCK_SIZE);
    auto switched = revealed_p2(permuted_input_shuffled);

    return switched;
}
