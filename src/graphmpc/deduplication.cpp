#include "deduplication.h"

/* ----- Preprocessing ----- */
void deduplication_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, MPPreprocessing &preproc,
                              Party &recv, bool save_to_disk) {
    sort::get_sort_preprocess(id, rngs, network, n, n_bits + 1, preproc, recv, save_to_disk);

    for (size_t i = 0; i < n_bits; ++i) {
        sort::sort_iteration_preprocess(id, rngs, network, n, preproc, recv, save_to_disk);
    }

    auto shuffle_pre = shuffle::get_shuffle(id, rngs, network, n, recv);
    auto unshuffle = shuffle::get_unshuffle(id, rngs, network, n, shuffle_pre);
    preproc.shuffles.push(shuffle_pre);
    preproc.unshuffles.push(unshuffle);

    clip::equals_zero_preprocess(id, rngs, network, n - 1, preproc, recv, save_to_disk);  // src eqz
    clip::equals_zero_preprocess(id, rngs, network, n - 1, preproc, recv, save_to_disk);  // dst eqz
    mul::preprocess(id, rngs, network, n - 1, preproc, recv, false, save_to_disk);        // Multiplication
    mul::preprocess(id, rngs, network, n - 1, preproc, recv, false, save_to_disk);        // B2A

    preproc.deduplication = true;
}

/* ----- Evaluation ----- */
void deduplication_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Graph &g,
                            bool save_to_disk) {
    Permutation perm = sort::get_sort_evaluate(id, rngs, network, n, preproc, g.dst_order_bits, save_to_disk);
    preproc.dst_order = perm;

    for (size_t i = 0; i < g.src_bits().size(); ++i) {
        perm = sort::sort_iteration_evaluate(id, rngs, network, n, perm, preproc, g.src_bits()[i], save_to_disk);
    }

    Permutation src_dst_sort = perm;

    auto src = g.src();
    auto dst = g.dst();
    auto shuffle_pre = preproc.shuffles.front();
    preproc.shuffles.pop();
    auto perm_shuffled = shuffle::shuffle(id, rngs, network, n, shuffle_pre, src_dst_sort);
    auto perm_opened = share::reveal_perm(id, network, perm_shuffled);
    auto shuffled_src_share = shuffle::shuffle(id, rngs, network, n, shuffle_pre, src);
    auto shuffled_dst_share = shuffle::shuffle(id, rngs, network, n, shuffle_pre, dst);

    auto src_p = perm_opened(shuffled_src_share);
    auto dst_p = perm_opened(shuffled_dst_share);

    std::vector<Ring> src_dupl(n - 1);
    std::vector<Ring> dst_dupl(n - 1);

    for (size_t i = 1; i < n; ++i) {
        src_dupl[i - 1] = src_p[i] - src_p[i - 1];
        dst_dupl[i - 1] = dst_p[i] - dst_p[i - 1];
    }

    auto src_eqz = clip::equals_zero_evaluate(id, rngs, network, preproc, src_dupl, save_to_disk);
    auto dst_eqz = clip::equals_zero_evaluate(id, rngs, network, preproc, dst_dupl, save_to_disk);
    auto src_and_dst = mul::evaluate(id, network, n - 1, preproc, src_eqz, dst_eqz, true, save_to_disk);
    auto duplicates = clip::B2A_evaluate(id, rngs, network, n - 1, preproc, src_and_dst, save_to_disk);

    /* Reverse perm */
    duplicates.insert(duplicates.begin(), 0);  // first element is never a duplicate
    duplicates = perm_opened.inverse()(duplicates);
    auto duplicates_reversed = shuffle::unshuffle(id, rngs, network, n, preproc, shuffle_pre, duplicates);

    /* Append MSB */
    g.src_order_bits.push_back(duplicates_reversed);
    g.dst_order_bits.push_back(duplicates_reversed);
}

/* ----- Ad-Hoc Preprocessing ----- */
// void deduplication(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Graph &g) {
// std::vector<std::vector<Ring>> src_dst_bits;
// src_dst_bits.insert(src_dst_bits.end(), g.src_bits().begin(), g.src_bits().end());
// src_dst_bits.insert(src_dst_bits.end(), g.dst_bits().begin(), g.dst_bits().end());

// auto src = g.src();
// auto dst = g.dst();

// Permutation rho = sort::get_sort(id, rngs, network, n, src_dst_bits);

// auto shuffle_pre = preproc.shuffles.front();
// preproc.shuffles.pop();

// auto shuffled_rho = shuffle::shuffle(id, rngs, network, n, shuffle_pre, rho);
// auto clear_rho = share::reveal_perm(id, network, shuffled_rho);
// auto shuffled_src = shuffle::shuffle(id, rngs, network, n, shuffle_pre, src);
// auto shuffled_dst = shuffle::shuffle(id, rngs, network, n, shuffle_pre, dst);

// auto src_p = clear_rho(shuffled_src);
// auto dst_p = clear_rho(shuffled_dst);

// std::vector<Ring> src_dupl(n);
// std::vector<Ring> dst_dupl(n);

// src_dupl[0] = 1;
// dst_dupl[0] = 1;

// for (size_t i = 1; i < n; ++i) {
// src_dupl[i] = src_p[i] - src_p[i - 1];
// dst_dupl[i] = dst_p[i] - dst_p[i - 1];
//}

// auto src_eqz = clip::equals_zero(id, rngs, network, src_dupl);
// auto dst_eqz = clip::equals_zero(id, rngs, network, dst_dupl);

//// Missing preprocessing for multiplication !!
// auto src_and_dst = mul::evaluate_bin(id, network, n - 1, preproc, src_eqz, dst_eqz);
// auto duplicates = clip::B2A(id, rngs, network, src_and_dst);

///* Reverse perm */
// duplicates.insert(duplicates.begin(), 0);  // first element is never a duplicate
// duplicates = clear_rho.inverse()(duplicates);
// auto duplicates_reversed = shuffle::unshuffle(id, rngs, network, n, preproc, shuffle_pre, duplicates);

///* Append MSB */
// g.src_order_bits.push_back(duplicates_reversed);
// g.dst_order_bits.push_back(duplicates_reversed);
//}
