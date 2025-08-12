#pragma once

#include <omp.h>

#include <tuple>
#include <vector>

#include "../graphmpc/mul.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

namespace clip {
/**
 * ----- F_eqz -----
 */
// std::vector<Ring> equals_zero(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, std::vector<Ring> &input_share);

// std::vector<Ring> equals_zero_two(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, std::vector<Ring> &input_share);

void equals_zero_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv);

std::vector<Ring> equals_zero_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, MPPreprocessing &preproc,
                                       std::vector<Ring> &input_share);
/**
 * ----- F_B2A -----
 */

// std::vector<Ring> B2A(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, std::vector<Ring> &input_share);

void B2A_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv);

std::vector<Ring> B2A_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                               std::vector<Ring> &input_share);
/**
 * ----- Flip -----
 */
std::vector<Ring> flip(Party id, std::vector<Ring> &input_share);
};  // namespace clip