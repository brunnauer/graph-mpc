#include "sorting.h"

SortPreprocessing sort::get_sort_preprocess(ProtocolConfig &c, size_t n_bits) {
    SortPreprocessing preproc;

    /* Get compaction preprocessing */
    preproc.comp_triples = compaction::preprocess(c, n_bits);

    /* Compute the preprocessing for n_bits-1 shuffles */
    for (size_t i = 0; i < n_bits - 1; ++i) {
        PermShare perm_share = shuffle::get_shuffle(c);
        preproc.perm_share_vec.push_back(perm_share);
    }

    return preproc;
}

Permutation sort::sort_iteration_evaluate(ProtocolConfig &c, Permutation &perm, std::vector<Row> &bit_vec_share, SortPreprocessing &preproc) {
    /* Shuffle perm */
    PermShare perm_share = preproc.perm_share_vec[0];
    preproc.perm_share_vec.erase(preproc.perm_share_vec.begin());  // Immediately erase first element
    Permutation perm_shuffled = shuffle::shuffle(c, perm, perm_share, true);

    /* Reveal shuffled perm */
    Permutation perm_open = share::reveal_perm(c, perm_shuffled);

    /* Shuffle input vec using the previous perm_share */
    std::vector<Row> input_shuffled = shuffle::shuffle(c, bit_vec_share, perm_share, false);

    /* Apply shuffled permutation to shuffled input */
    std::vector<Row> input_sorted = perm_open(input_shuffled);

    /* Compute compaction to stable sort next input */
    auto [triple_a, triple_b, triple_c] = preproc.comp_triples[0];
    preproc.comp_triples.erase(preproc.comp_triples.begin());
    Permutation sigma_p = compaction::evaluate_one(c, triple_a, triple_b, triple_c, input_sorted);
    auto sigma_p_vec = sigma_p.get_perm_vec();
    std::vector<Row> next_perm = perm_open.inverse()(sigma_p_vec);

    Permutation next_perm_unshuffled = Permutation(shuffle::unshuffle(c, next_perm, perm_share));

    return next_perm_unshuffled;
}

Permutation sort::get_sort_evaluate(ProtocolConfig &c, std::vector<std::vector<Row>> &bit_shares, SortPreprocessing &preproc) {
    auto [triple_a, triple_b, triple_c] = preproc.comp_triples[0];
    /* Delete first element */
    preproc.comp_triples.erase(preproc.comp_triples.begin());

    Permutation sigma = compaction::evaluate_one(c, triple_a, triple_b, triple_c, bit_shares[0]);

    size_t n_bits = bit_shares.size();
    for (size_t i = 1; i < n_bits; ++i) {
        sigma = sort_iteration_evaluate(c, sigma, bit_shares[i], preproc);
    }
    return sigma;
}

Permutation sort::sort_iteration(ProtocolConfig &c, Permutation &perm, std::vector<Row> &bit_vec_share) {
    /* Shuffle perm */
    PermShare perm_share = shuffle::get_shuffle(c);
    Permutation perm_shuffled = shuffle::shuffle(c, perm, perm_share, true);

    /* Reveal shuffled perm */
    Permutation perm_open = share::reveal_perm(c, perm_shuffled);

    /* Shuffle input using the previous order */
    std::vector<Row> input_shuffled = shuffle::shuffle(c, bit_vec_share, perm_share, false);

    /* Apply revealed permutation to shuffled input */
    std::vector<Row> sorted_share = perm_open(input_shuffled);

    /* Compute compaction to stable sort bit_vec_share */
    Permutation sigma_p = compaction::get_compaction(c, sorted_share);
    auto sigma_p_vec = sigma_p.get_perm_vec();
    std::vector<Row> next_perm = perm_open.inverse()(sigma_p_vec);

    /* Unshuffle next */
    return Permutation(shuffle::unshuffle(c, next_perm, perm_share));
}

Permutation sort::get_sort(ProtocolConfig &conf, std::vector<std::vector<Row>> &bit_shares) {
    /* Compute compaction of x_0 */
    Permutation sigma = compaction::get_compaction(conf, bit_shares[0]);
    /* Proceed sorting with x_1, x_2, ... */
    size_t n_bits = bit_shares.size();
    for (size_t i = 1; i < n_bits; ++i) {
        sigma = sort_iteration(conf, sigma, bit_shares[i]);
    }
    return sigma;
}

std::vector<Row> sort::apply_perm(ProtocolConfig &c, Permutation &perm, std::vector<Row> &input_share) {
    /* Shuffle permutation and open it */
    PermShare perm_share = shuffle::get_shuffle(c);
    auto perm_shuffled = shuffle::shuffle(c, perm, perm_share, true);
    auto revealed = share::reveal_perm(c, perm_shuffled);
    Permutation perm_opened = Permutation(revealed);

    /* Shuffle input_share with same perm */
    auto shuffled_input_share = shuffle::shuffle(c, input_share, perm_share, false);

    /* Apply opened perm to input_share */
    return perm_opened(shuffled_input_share);
}

std::vector<Row> sort::switch_perm(ProtocolConfig &c, Permutation &p1, Permutation &p2, std::vector<Row> &input_share) {
    PermShare pi = shuffle::get_shuffle(c);
    PermShare omega = shuffle::get_shuffle(c);

    auto shuffled_p1 = shuffle::shuffle(c, p1, pi, true);
    auto shuffled_p2 = shuffle::shuffle(c, p2, omega, true);

    auto revealed_p1 = Permutation(share::reveal_perm(c, shuffled_p1)); /* pi(p1.get_perm_vec()) == (p1 * pi.inv) */
    auto revealed_p2 = Permutation(share::reveal_perm(c, shuffled_p2)); /* omega(p2.get_perm_vec()) == p2 * omega.inv */

    auto merged = shuffle::get_merged_shuffle(c, pi, omega); /* omega * pi.inv */

    auto permuted_input = revealed_p1.inverse()(input_share);
    auto permuted_input_shuffled = shuffle::shuffle(c, permuted_input, merged, false); /* (omega * pi.inv * pi * p1.inv)(input) == (omega * p1.inv)(input) */
    auto switched = revealed_p2(permuted_input_shuffled);                              /* (p2 * omega.inv * omega * p1.inv)(input) == (p2 * p1.inv)(input) */

    return switched;
}
