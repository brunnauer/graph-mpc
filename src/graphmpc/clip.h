#pragma once

#include <omp.h>

#include <tuple>
#include <vector>

#include "../graphmpc/mul.h"
#include "../utils/sharing.h"

namespace clip {
/**
 * ----- F_eqz -----
 */
void equals_zero_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv,
                            bool ssd = false);

std::vector<Ring> equals_zero_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, MPPreprocessing &preproc,
                                       std::vector<Ring> &input_share, bool ssd = false);
/**
 * ----- F_B2A -----
 */

void B2A_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv, bool ssd = false);

std::vector<Ring> B2A_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                               std::vector<Ring> &input_share, bool ssd = false);

/**
 * ----- Flip -----
 */
std::vector<Ring> flip(Party id, std::vector<Ring> &input_share);
};  // namespace clip