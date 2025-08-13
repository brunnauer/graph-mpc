#pragma once

#include <omp.h>

#include <cassert>
#include <memory>
#include <tuple>
#include <vector>

#include "../io/netmp.h"
#include "../utils/permutation.h"
#include "../utils/sharing.h"
#include "../utils/types.h"
#include "mul.h"

namespace compaction {

void preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv,
                bool save_to_disk = false);

Permutation evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, std::vector<Ring> &input_share,
                     bool save_to_disk = false);

Permutation get_compaction(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                           std::vector<Ring> &input_share, Party &recv);

};  // namespace compaction