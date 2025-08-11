#include "sort.h"

void sort::get_sort_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, MPPreprocessing &preproc,
                               Party &recv) {
    mul::preprocess(id, rngs, network, n, preproc, recv);

    for (size_t i = 0; i < n_bits - 1; ++i) {
        sort_iteration_preprocess(id, rngs, network, n, preproc, recv);
    }
}

void sort::sort_iteration_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv) {
    ShufflePre shuffle_pre = shuffle::get_shuffle(id, rngs, network, n, recv);
    mul::preprocess(id, rngs, network, n, preproc, recv);
    std::vector<Ring> unshuffle_pre = shuffle::get_unshuffle(id, rngs, network, n, shuffle_pre);

    preproc.shuffles.push(shuffle_pre);
    preproc.unshuffles.push(unshuffle_pre);
}

Permutation sort::get_sort_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                                    std::vector<std::vector<Ring>> &bit_shares) {
    if (id == D) return Permutation(n);
    auto triples = extract(preproc.triples, n);
    Permutation sigma = compaction::evaluate(id, rngs, network, n, triples, bit_shares[0]);

    size_t n_bits = bit_shares.size();
    for (size_t i = 1; i < n_bits; ++i) {
        sigma = sort_iteration_evaluate(id, rngs, network, n, sigma, preproc, bit_shares[i]);
    }
    return sigma;
}

Permutation sort::sort_iteration_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, Permutation &perm,
                                          MPPreprocessing &preproc, std::vector<Ring> &bit_shares) {
    if (id == D) return Permutation(n);
    ShufflePre shuffle_share = preproc.shuffles.front();
    preproc.shuffles.pop();

    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, n, shuffle_share, perm);

    Permutation perm_open = share::reveal_perm(id, network, perm_shuffled);

    std::vector<Ring> input_shuffled = shuffle::shuffle(id, rngs, network, n, shuffle_share, bit_shares);
    std::vector<Ring> input_sorted = perm_open(input_shuffled);

    auto triples = extract(preproc.triples, n);
    Permutation sigma_p = compaction::evaluate(id, rngs, network, n, triples, input_sorted);

    auto sigma_p_vec = sigma_p.get_perm_vec();
    std::vector<Ring> next_perm = perm_open.inverse()(sigma_p_vec);

    auto unshuffle_share = preproc.unshuffles.front();
    preproc.unshuffles.pop();
    Permutation next_perm_shuffled = Permutation(shuffle::unshuffle(id, rngs, network, n, shuffle_share, unshuffle_share, next_perm));

    return next_perm_shuffled;
}

/* ----- Preprocessing ----- */
SortIterationPreprocessing sort::sort_iteration_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n) {
    SortIterationPreprocessing preproc;

    ShufflePre perm_share = shuffle::get_shuffle(id, rngs, network, n);
    auto triples = compaction::preprocess(id, rngs, network, n);
    std::vector<Ring> unshuffle_B = shuffle::get_unshuffle(id, rngs, network, n, perm_share);

    preproc.perm_share = perm_share;
    preproc.triples = triples;
    preproc.unshuffle_B = unshuffle_B;

    return preproc;
}

SortPreprocessing sort::get_sort_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits) {
    SortPreprocessing preproc;

    /* Get one compaction preprocessing */
    preproc.triples = compaction::preprocess(id, rngs, network, n);

    /* Compute the preprocessing for n_bits-1 sort_iteratinos */
    for (size_t i = 0; i < n_bits - 1; ++i) {
        SortIterationPreprocessing sort_iteration_pre = sort_iteration_preprocess(id, rngs, network, n);
        preproc.sort_iteration_pre.push_back(sort_iteration_pre);
    }

    return preproc;
}

/* ----- Evaluation ----- */
Permutation sort::sort_iteration_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, Permutation &perm,
                                          SortIterationPreprocessing &preproc, std::vector<Ring> &bit_shares) {
    if (id == D) return Permutation(n);
    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, n, preproc.perm_share, perm);

    Permutation perm_open = share::reveal_perm(id, network, perm_shuffled);

    std::vector<Ring> input_shuffled = shuffle::shuffle(id, rngs, network, n, preproc.perm_share, bit_shares);
    std::vector<Ring> input_sorted = perm_open(input_shuffled);

    auto triples = preproc.triples;
    Permutation sigma_p = compaction::evaluate(id, rngs, network, n, triples, input_sorted);

    auto sigma_p_vec = sigma_p.get_perm_vec();
    std::vector<Ring> next_perm = perm_open.inverse()(sigma_p_vec);

    auto unshuffle_B = preproc.unshuffle_B;
    Permutation next_perm_shuffled = Permutation(shuffle::unshuffle(id, rngs, network, n, preproc.perm_share, unshuffle_B, next_perm));

    return next_perm_shuffled;
}

Permutation sort::get_sort_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, SortPreprocessing &preproc,
                                    std::vector<std::vector<Ring>> &bit_shares) {
    if (id == D) return Permutation(n);
    Permutation sigma = compaction::evaluate(id, rngs, network, n, preproc.triples, bit_shares[0]);

    size_t n_bits = bit_shares.size();
    for (size_t i = 1; i < n_bits; ++i) {
        auto sort_iteration_pre = preproc.sort_iteration_pre[i - 1];
        sigma = sort_iteration_evaluate(id, rngs, network, n, sigma, sort_iteration_pre, bit_shares[i]);
    }
    return sigma;
}

/* ----- Ad-Hoc Preprocessing ----- */
/**
 * ----- F_get_sort -----
 */
Permutation sort::get_sort(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, std::vector<std::vector<Ring>> &bit_shares) {
    /* Compute compaction of x_0 */
    Permutation sigma = compaction::get_compaction(id, rngs, network, n, bit_shares[0]);
    /* Proceed sorting with x_1, x_2, ... */
    size_t n_bits = bit_shares.size();
    for (size_t i = 1; i < n_bits; ++i) {
        sigma = sort_iteration(id, rngs, network, n, sigma, bit_shares[i]);
    }
    return sigma;
}

/**
 * ----- F_sort_iteration -----
 */
Permutation sort::sort_iteration(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, Permutation &perm,
                                 std::vector<Ring> &bit_vec_share) {
    /* Shuffle perm */
    ShufflePre perm_share = shuffle::get_shuffle(id, rngs, network, n);
    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, n, perm_share, perm);

    /* Reveal shuffled perm */
    Permutation perm_open = share::reveal_perm(id, network, perm_shuffled);

    /* Shuffle input using the previous order */
    std::vector<Ring> input_shuffled = shuffle::shuffle(id, rngs, network, n, perm_share, bit_vec_share);

    /* Apply revealed permutation to shuffled input */
    std::vector<Ring> sorted_share = perm_open(input_shuffled);

    /* Compute compaction to stable sort bit_vec_share */
    Permutation sigma_p = compaction::get_compaction(id, rngs, network, n, sorted_share);
    auto sigma_p_vec = sigma_p.get_perm_vec();
    std::vector<Ring> next_perm = perm_open.inverse()(sigma_p_vec);

    /* Unshuffle next */
    std::vector<Ring> B = shuffle::get_unshuffle(id, rngs, network, n, perm_share);
    return Permutation(shuffle::unshuffle(id, rngs, network, n, perm_share, B, next_perm));
}
