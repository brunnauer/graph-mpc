#pragma once

#include <omp.h>

#include <functional>
#include <random>
#include <stdexcept>

#include "../io/netmp.h"
#include "../utils/permutation.h"
#include "../utils/preprocessings.h"
#include "../utils/random_generators.h"
#include "../utils/types.h"

namespace shuffle {

void get_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, bool save_to_disk = false);

ShufflePre get_shuffle_compute(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &D0, std::vector<Ring> &D1, Party recv_larger_msg);

ShufflePre get_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n);

ShufflePre get_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, Party &recv);

void get_unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &perm_share, MPPreprocessing &preproc);

std::vector<Ring> get_unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &perm_share);

ShufflePre get_merged_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &pi_share, ShufflePre &omega_share);

std::vector<Ring> shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &perm_share,
                          std::vector<Ring> &input_share);

std::vector<Ring> shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                          std::vector<Ring> &input_share);

Permutation shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Permutation &input_share);

Permutation shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &perm_share, Permutation &input_share);

std::vector<Ring> unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &shuffle_share, std::vector<Ring> &B,
                            std::vector<Ring> &input_share);

std::vector<Ring> unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, ShufflePre &shuffle_pre,
                            std::vector<Ring> &input_share);

Permutation unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, ShufflePre &shuffle_pre,
                      Permutation &input_share);

Permutation unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &shuffle_share, std::vector<Ring> &B,
                      Permutation &input_share);

};  // namespace shuffle