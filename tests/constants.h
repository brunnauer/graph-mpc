#pragma once

#include <stdio.h>

/**
 * List of functions that return the specified communication costs in #Ring-elements
 */

/* Total offline communication for Dealer*/
/* Multiplication, Compaction, etc. */
constexpr const size_t mul_comm_pre(size_t n) { return n; }         // n: shares_P1
constexpr const size_t compaction_comm_pre(size_t n) { return n; }  // n: shares_P1
constexpr const size_t eqz_comm_pre(size_t n) { return 5 * n; }     // n_layer * n: shares_P1
constexpr const size_t B2A_comm_pre(size_t n) { return n; }         // n: shares_P1

/* Shuffle */
constexpr const size_t shuffle_comm_pre(size_t n) { return 3 * n; }         // 3n: B_0, B_1, pi_1_p
constexpr const size_t unshuffle_comm_pre(size_t n) { return 2 * n; }       // 2n: B_0, B_1
constexpr const size_t merged_shuffle_comm_pre(size_t n) { return 4 * n; }  // 4n: sigma_0_p, sigma_1, B_0, B_1

/* Sort */
constexpr const size_t sort_comm_pre(size_t n, size_t n_bits) {
    return n_bits * compaction_comm_pre(n) + (n_bits - 1) * (shuffle_comm_pre(n) + unshuffle_comm_pre(n));
}  // n_bits * compaction + (n_bits - 1) * (shuffle + unshuffle)

constexpr const size_t apply_perm_comm_pre(size_t n) { return shuffle_comm_pre(n); }
constexpr const size_t reverse_perm_comm_pre(size_t n) { return shuffle_comm_pre(n) + unshuffle_comm_pre(n); }

/* Online communication costs per party */
/* Multiplication, Compaction, etc. */
constexpr const size_t mul_comm_online(size_t n) { return 2 * n; }                   // 2n: n xa's, n yb's
constexpr const size_t compaction_comm_online(size_t n) { return 2 * n; }            // 2n: n xa's, n yb's
constexpr const size_t eqz_comm_online(size_t n) { return 5 * mul_comm_online(n); }  // 5 * 2n: 5 * (n xa's, n yb's)
constexpr const size_t B2A_comm_online(size_t n) { return 2 * n; }                   // 2n: n xa's, n yb's

/* Shuffle */
constexpr const size_t shuffle_comm_online(size_t n) { return n; }         // n: t_0/1A
constexpr const size_t unshuffle_comm_online(size_t n) { return n; }       // n: t_0/1
constexpr const size_t merged_shuffle_comm_online(size_t n) { return n; }  // n: t_0/1

/* Sort */
constexpr const size_t sort_comm_online(size_t n, size_t n_bits) {
    return n_bits * compaction_comm_online(n) + (n_bits - 1) * (shuffle_comm_online(n) + n + shuffle_comm_online(n) + unshuffle_comm_online(n));
}

constexpr const size_t apply_perm_comm_online(size_t n) { return shuffle_comm_online(n) + n + shuffle_comm_online(n); }
constexpr const size_t reverse_perm_comm_online(size_t n) { return shuffle_comm_online(n) + n + unshuffle_comm_online(n); }