#pragma once

#include <random>
#include <tuple>
#include <vector>

#include "../utils/preprocessings.h"
#include "../utils/random_generators.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

namespace mul {
void preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv, bool binary = false,
                bool ssd = false);

std::vector<Ring> evaluate(Party id, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, std::vector<Ring> x, std::vector<Ring> y,
                           bool binary = false, bool ssd = false);

};  // namespace mul
