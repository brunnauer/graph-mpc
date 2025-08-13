#pragma once

#include "../utils/permutation.h"
#include "../utils/preprocessings.h"
#include "../utils/random_generators.h"
#include "../utils/sharing.h"
#include "../utils/types.h"
#include "compaction.h"
#include "shuffle.h"

namespace sort {

void get_sort_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, MPPreprocessing &preproc, Party &recv,
                         bool save_to_disk = false);

void sort_iteration_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv,
                               bool save_to_disk = false);

Permutation get_sort_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                              std::vector<std::vector<Ring>> &bit_shares, bool save_to_disk = false);

Permutation sort_iteration_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, Permutation &perm,
                                    MPPreprocessing &preproc, std::vector<Ring> &bit_shares, bool save_to_disk = false);

/**
 * ----- F_get_sort -----
 */

// Permutation get_sort(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, std::vector<std::vector<Ring>> &bit_shares);

/**
 * ----- F_sort_iteration -----
 */
// Permutation sort_iteration(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, Permutation &perm,
// std::vector<Ring> &bit_vec_share);

};  // namespace sort