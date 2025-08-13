#pragma once

#include <random>
#include <tuple>
#include <vector>

#include "../setup/utils.h"
#include "../utils/preprocessings.h"
#include "../utils/random_generators.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

namespace mul {
void preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv, bool binary = false,
                bool save_to_disk = false);

std::vector<Ring> evaluate(Party id, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, std::vector<Ring> x, std::vector<Ring> y,
                           bool binary = false, bool save_to_disk = false);

};  // namespace mul
