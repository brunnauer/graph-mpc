#pragma once

#include <stdio.h>

#include "../src/utils/types.h"

/**
 * List of macros that compute the specified communication costs in #Ring-elements
 */

/* ----- Total offline communication for Dealer ----- */
/* Multiplication, Compaction, etc. */
#define MUL_COMM_PRE(n) (n)
#define COMPACTION_COMM_PRE(n) (n)
#define EQZ_COMM_PRE(n) (5 * (n))
#define B2A_COMM_PRE(n) (n)

/* Shuffle */
#define SHUFFLE_COMM_PRE(n) (3 * n)
#define UNSHUFFLE_COMM_PRE(n) (2 * n)
#define MERGED_SHUFFLE_COMM_PRE(n) (4 * n)

/* Sort */
#define SORT_COMM_PRE(n, n_bits) ((n_bits) * COMPACTION_COMM_PRE(n) + ((n_bits) - 1) * (SHUFFLE_COMM_PRE(n) + UNSHUFFLE_COMM_PRE(n)))
#define SORT_ITERATION_COMM_PRE(n) (SHUFFLE_COMM_PRE(n) + COMPACTION_COMM_PRE(n) + UNSHUFFLE_COMM_PRE(n))
#define APPLY_PERM_COMM_PRE(n) (SHUFFLE_COMM_PRE(n))
#define REVERSE_PERM_COMM_PRE(n) (SHUFFLE_COMM_PRE(n) + UNSHUFFLE_COMM_PRE(n))
#define SWITCH_PERM_COMM_PRE(n) (2 * SHUFFLE_COMM_PRE(n) + MERGED_SHUFFLE_COMM_PRE(n))

#define DEDUPLICATION_COMM_PRE(n, n_bits)                                                                                                                  \
    (SORT_COMM_PRE(n, (n_bits) + 1) + (n_bits) * SORT_ITERATION_COMM_PRE(n) + APPLY_PERM_COMM_PRE(n) + 2 * EQZ_COMM_PRE((n) - 1) + MUL_COMM_PRE((n) - 1) + \
     B2A_COMM_PRE((n) - 1) + UNSHUFFLE_COMM_PRE(n))

#define MP_COMM_PRE(n, n_bits) (2 * SORT_COMM_PRE(n, (n_bits) + 1) + SORT_ITERATION_COMM_PRE(n) + 3 * SHUFFLE_COMM_PRE(n) + 3 * MERGED_SHUFFLE_COMM_PRE(n))

#define BFS_COMM_PRE(n, n_bits, d)                                                                                                                         \
    (2 * SORT_COMM_PRE(n, (n_bits) + 1) + SORT_ITERATION_COMM_PRE(n) + APPLY_PERM_COMM_PRE(n) + 2 * SHUFFLE_COMM_PRE(n) + 3 * MERGED_SHUFFLE_COMM_PRE(n) + \
     EQZ_COMM_PRE(n) + B2A_COMM_PRE(n))

#define PI_K_COMM_PRE(n, n_bits, d)                                                                                                                           \
    (DEDUPLICATION_COMM_PRE(n, n_bits) + SORT_COMM_PRE(n, (n_bits) + 2) + 2 * SORT_ITERATION_COMM_PRE(n) + APPLY_PERM_COMM_PRE(n) + 2 * SHUFFLE_COMM_PRE(n) + \
     3 * MERGED_SHUFFLE_COMM_PRE(n))

/* ----- Online communication costs per party ----- */
/* Multiplication, Compaction, etc. */
#define MUL_COMM_ONLINE(n) (2 * (n))
#define COMPACTION_COMM_ONLINE(n) (2 * (n))
#define EQZ_COMM_ONLINE(n) (5 * MUL_COMM_ONLINE(n))
#define B2A_COMM_ONLINE(n) (2 * (n))

/* Shuffle */
#define SHUFFLE_COMM_ONLINE(n) (n)
#define UNSHUFFLE_COMM_ONLINE(n) (n)
#define MERGED_SHUFFLE_COMM_ONLINE(n) (n)

/* Sort */
#define SORT_COMM_ONLINE(n, n_bits) \
    ((n_bits) * COMPACTION_COMM_ONLINE(n) + ((n_bits) - 1) * (SHUFFLE_COMM_ONLINE(n) + (n) + SHUFFLE_COMM_ONLINE(n) + UNSHUFFLE_COMM_ONLINE(n)))

#define SORT_ITERATION_COMM_ONLINE(n) (2 * SHUFFLE_COMM_ONLINE(n) + (n) + COMPACTION_COMM_ONLINE(n) + UNSHUFFLE_COMM_ONLINE(n))

#define APPLY_PERM_COMM_ONLINE(n) (SHUFFLE_COMM_ONLINE(n) + (n) + SHUFFLE_COMM_ONLINE(n))

#define REVERSE_PERM_COMM_ONLINE(n) (SHUFFLE_COMM_ONLINE(n) + (n) + UNSHUFFLE_COMM_ONLINE(n))

#define FIRST_SWITCH_PERM_COMM_ONLINE(n) (2 * (2 * SHUFFLE_COMM_ONLINE(n) + (n)) + 2 * SHUFFLE_COMM_ONLINE(n))

#define SWITCH_PERM_COMM_ONLINE(n) (4 * SHUFFLE_COMM_ONLINE(n))

#define DEDUPLICATION_COMM_ONLINE(n, n_bits)                                                                                                          \
    (SORT_COMM_ONLINE(n, (n_bits) + 1) + (n_bits) * SORT_ITERATION_COMM_ONLINE(n) + 3 * SHUFFLE_COMM_ONLINE(n) + (n) + 2 * EQZ_COMM_ONLINE((n) - 1) + \
     MUL_COMM_ONLINE((n) - 1) + B2A_COMM_ONLINE((n) - 1) + UNSHUFFLE_COMM_ONLINE(n))

#define MP_COMM_ONLINE(n, n_bits, d)                                                                                                        \
    (2 * SORT_COMM_ONLINE(n, (n_bits) + 1) + SORT_ITERATION_COMM_ONLINE(n) + APPLY_PERM_COMM_ONLINE(n) + FIRST_SWITCH_PERM_COMM_ONLINE(n) + \
     ((d) - 1) * SWITCH_PERM_COMM_ONLINE(n))

#define BFS_COMM_ONLINE(n, n_bits, d)                                                                                                       \
    (2 * SORT_COMM_ONLINE(n, (n_bits) + 1) + SORT_ITERATION_COMM_ONLINE(n) + APPLY_PERM_COMM_ONLINE(n) + FIRST_SWITCH_PERM_COMM_ONLINE(n) + \
     ((d) - 1) * SWITCH_PERM_COMM_ONLINE(n) + EQZ_COMM_ONLINE(n) + B2A_COMM_ONLINE(n))

#define PI_K_COMM_ONLINE(n, n_bits, d)                                                                                                          \
    (DEDUPLICATION_COMM_ONLINE(n, n_bits) + SORT_COMM_ONLINE(n, (n_bits) + 2) + 2 * SORT_ITERATION_COMM_ONLINE(n) + APPLY_PERM_COMM_ONLINE(n) + \
     FIRST_SWITCH_PERM_COMM_ONLINE(n) + ((d) - 1) * SWITCH_PERM_COMM_ONLINE(n))