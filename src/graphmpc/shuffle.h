#pragma once

#include <omp.h>

#include <functional>
#include <random>
#include <stdexcept>

#include "../io/netmp.h"
#include "../utils/permutation.h"
#include "../utils/preprocessings.h"

namespace shuffle {

ShufflePre get_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, Party &recv);

void get_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv, bool ssd = false);

ShufflePre get_shuffle_compute(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &D0, std::vector<Ring> &D1, Party recv_larger_msg);

void get_unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, bool ssd = false);

ShufflePre get_merged_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &pi_share, ShufflePre &omega_share);

std::vector<Ring> shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &perm_share,
                          std::vector<Ring> &input_share);

Permutation shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &perm_share, Permutation &input_share);

std::vector<Ring> shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                          std::vector<Ring> &input_share, bool ssd = false, bool repeat = false);

Permutation shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Permutation &input_share,
                    bool ssd = false, bool repeat = false);

std::vector<Ring> shuffle_repeat(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                                 std::vector<Ring> &input_share, bool ssd, bool delete_preprocessing = false);

std::vector<Ring> unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &shuffle_pre,
                            std::vector<Ring> &unshuffle_B, std::vector<Ring> &input_share);

Permutation unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &shuffle_pre, std::vector<Ring> &unshuffle_B,
                      Permutation &input_share);

std::vector<Ring> unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                            std::vector<Ring> &input_share, bool ssd = false);

Permutation unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Permutation &input_share,
                      bool ssd = false);

};  // namespace shuffle