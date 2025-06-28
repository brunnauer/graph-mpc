#include "permute.h"

/* ----- Preprocessing ----- */
SwitchPermPreprocessing permute::switch_perm_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE) {
    SwitchPermPreprocessing preproc;
    ShufflePre pi = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    ShufflePre omega = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    ShufflePre merged = shuffle::get_merged_shuffle(id, rngs, network, n, BLOCK_SIZE, pi, omega);

    preproc.pi = pi;
    preproc.omega = omega;
    preproc.merged = merged;

    return preproc;
}

SwitchPermPreprocessing_Dealer permute::switch_perm_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n) {
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

SwitchPermPreprocessing permute::switch_perm_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals, size_t &idx) {
    SwitchPermPreprocessing preproc;

    ShufflePre pi_share = shuffle::get_shuffle_2(id, rngs, n, vals, idx, true);
    ShufflePre omega_share = shuffle::get_shuffle_2(id, rngs, n, vals, idx, true);
    ShufflePre merged_share = shuffle::get_merged_shuffle_2(id, n, vals, idx);

    preproc.pi = pi_share;
    preproc.omega = omega_share;
    preproc.merged = merged_share;
    return preproc;
}

/* ----- Evaluation ----- */
std::vector<Ring> permute::apply_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                               Permutation &perm, ShufflePre &perm_share, std::vector<Ring> &input_share) {
    if (id == D) return std::vector<Ring>(n);
    auto perm_shuffled = shuffle::shuffle(id, rngs, network, perm, perm_share, n, BLOCK_SIZE);
    auto perm_opened = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);

    auto shuffled_input_share = shuffle::shuffle(id, rngs, network, input_share, perm_share, n, BLOCK_SIZE);
    return perm_opened(shuffled_input_share);
}

std::vector<Ring> permute::reverse_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                                 Permutation &perm, ShufflePre &pi, std::vector<Ring> &unshuffle_B, std::vector<Ring> &input_share) {
    if (id == D) return std::vector<Ring>(n);
    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, perm, pi, n, BLOCK_SIZE);
    Permutation perm_opened = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);
    auto permuted_share = perm_opened.inverse()(input_share);

    auto result = shuffle::unshuffle(id, rngs, network, pi, unshuffle_B, permuted_share, n, BLOCK_SIZE);
    return result;
}

std::vector<Ring> permute::switch_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                                Permutation &p1, Permutation &p2, SwitchPermPreprocessing &preproc, std::vector<Ring> &input_share) {
    if (id == D) return std::vector<Ring>(n);

    if (preproc.pi_p1.is_null()) {
        auto shuffled_p1 = shuffle::shuffle(id, rngs, network, p1, preproc.pi, n, BLOCK_SIZE);
        auto pi_p1 = share::reveal_perm(id, network, BLOCK_SIZE, shuffled_p1);
        preproc.pi_p1 = pi_p1;
    }

    if (preproc.omega_p2.is_null()) {
        auto shuffled_p2 = shuffle::shuffle(id, rngs, network, p2, preproc.omega, n, BLOCK_SIZE);
        auto omega_p2 = share::reveal_perm(id, network, BLOCK_SIZE, shuffled_p2);
        preproc.omega_p2 = omega_p2;
    }

    auto permuted_input = preproc.pi_p1.inverse()(input_share);
    auto permuted_input_shuffled = shuffle::shuffle(id, rngs, network, permuted_input, preproc.merged, n, BLOCK_SIZE);
    auto switched = preproc.omega_p2(permuted_input_shuffled);

    return switched;
}

/* ----- Ad-Hoc Preprocessing ----- */

/**
 * ----- F_apply_perm -----
 */
std::vector<Ring> permute::apply_perm(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &perm,
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

/**
 * ----- F_reverse_perm -----
 */
std::vector<Ring> permute::reverse_perm(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &perm,
                                        std::vector<Ring> &input_share) {
    ShufflePre pi = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    auto unshuffle_B = shuffle::get_unshuffle(id, rngs, network, n, BLOCK_SIZE, pi);

    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, perm, pi, n, BLOCK_SIZE);
    Permutation perm_opened = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);
    auto permuted_share = perm_opened.inverse()(input_share);

    auto result = shuffle::unshuffle(id, rngs, network, pi, unshuffle_B, permuted_share, n, BLOCK_SIZE);
    return result;
}

/**
 * ----- F_sw_perm -----
 */
std::vector<Ring> permute::switch_perm(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &p1,
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
