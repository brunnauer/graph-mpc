#pragma once

#include <omp.h>

#include <cassert>
#include <memory>
#include <tuple>
#include <vector>

#include "../io/comm.h"
#include "../utils/mul.h"
#include "../utils/perm.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

namespace compaction {

std::vector<std::tuple<Ring, Ring, Ring>> preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE);

std::vector<Ring> preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n);

std::vector<std::tuple<Ring, Ring, Ring>> preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &shares_P1, size_t &idx);

Permutation evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                     std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> &input_share);

std::vector<Ring> evaluate_1(Party id, size_t n, std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> &input_share);

Permutation evaluate_2(Party id, size_t n, std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> &vals, std::vector<Ring> &input_share);

Permutation get_compaction(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, std::vector<Ring> &input_share);

};  // namespace compaction