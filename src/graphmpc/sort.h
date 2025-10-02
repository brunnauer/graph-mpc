#pragma once

#include "../utils/permutation.h"
#include "../utils/preprocessings.h"
#include "../utils/sharing.h"
#include "compaction.h"
#include "shuffle.h"

namespace sort {

void get_sort_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, MPPreprocessing &preproc,
                         Party &recv_shuffle, Party &recv_mul, bool ssd = false);

void sort_iteration_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv_shuffle,
                               Party &recv_mul, bool ssd = false);

Permutation get_sort_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                              std::vector<std::vector<Ring>> &bit_shares, bool ssd = false);

Permutation sort_iteration_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, Permutation &perm,
                                    MPPreprocessing &preproc, std::vector<Ring> &bit_shares, bool ssd = false);

};  // namespace sort