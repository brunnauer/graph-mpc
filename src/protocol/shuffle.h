#pragma once

#include <omp.h>

#include "../utils/perm.h"
#include "../utils/random_generators.h"
#include "../utils/types.h"
#include "io/comm.h"

/**
 * Contains the preprocessing of a secure shuffle
 * After preprocessing, B is set and pi_1_p for P1
 * The size after preprocessing is therefore: n / 2n for P0 / P1
 * If save is set to true, then pi_0, pi_0_p resp. pi_1 are set for P0 resp. P1 and also R for both parties (for repeating the same shuffle later)
 */
struct ShufflePre {
    Permutation pi_0;
    Permutation pi_1;
    Permutation pi_0_p;
    Permutation pi_1_p;
    std::vector<Ring> B;
    std::vector<Ring> R;
    bool save;
};

namespace shuffle {

ShufflePre get_shuffle_compute(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &shared_secret_D0, std::vector<Ring> &shared_secret_D1, bool save);

ShufflePre get_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, bool save);

std::vector<Ring> get_repeat(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, ShufflePre &perm_share);

std::vector<Ring> get_unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, ShufflePre &perm_share);

ShufflePre get_merged_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, ShufflePre &pi_share,
                              ShufflePre &omega_share);

std::tuple<ShufflePre, std::vector<Ring>, std::vector<Ring>> get_shuffle_1(Party id, RandomGenerators &rngs, size_t n);

ShufflePre get_shuffle_2(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> vals, size_t &idx, bool save);

std::tuple<std::vector<Ring>, std::vector<Ring>> get_unshuffle_1(Party id, RandomGenerators &rngs, size_t n, ShufflePre &perm_share);

std::vector<Ring> get_unshuffle_2(Party id, size_t n, std::vector<Ring> &vals, size_t &idx);

std::tuple<ShufflePre, std::vector<Ring>, std::vector<Ring>, std::vector<Ring>, std::vector<Ring>> get_merged_shuffle_1(Party id, RandomGenerators &rngs,
                                                                                                                        size_t n, ShufflePre &pi_share,
                                                                                                                        ShufflePre &omega_share);

ShufflePre get_merged_shuffle_2(Party id, size_t n, std::vector<Ring> &vals, size_t &idx);

// std::vector<Ring> shuffle_1(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &input_share, ShufflePre &perm_share);

// std::vector<Ring> shuffle_1(Party id, RandomGenerators &rngs, size_t n, Permutation &input_share, ShufflePre &perm_share);

// std::vector<Ring> shuffle_2(Party id, RandomGenerators &rngs, std::vector<Ring> &vec_A, ShufflePre &perm_share, size_t n);

// std::vector<Ring> unshuffle_1(Party id, RandomGenerators &rngs, ShufflePre &shuffle_share, std::vector<Ring> &input_share, size_t n);

// std::vector<Ring> unshuffle_2(Party id, ShufflePre &shuffle_share, std::vector<Ring> vec_t, std::vector<Ring> &B, size_t n);

std::vector<Ring> shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, std::vector<Ring> &input_share, ShufflePre &perm_share,
                          size_t n, size_t BLOCK_SIZE);

Permutation shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, Permutation &input_share, ShufflePre &perm_share, size_t n,
                    size_t BLOCK_SIZE);

void prepare_repeat(ShufflePre &perm_share, std::vector<Ring> &repeat_B);

std::vector<Ring> unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, ShufflePre &shuffle_share, std::vector<Ring> &B,
                            std::vector<Ring> &input_share, size_t n, size_t BLOCK_SIZE);

Permutation unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, ShufflePre &shuffle_share,
                      std::vector<Ring> &B, Permutation &input_share);

};  // namespace shuffle