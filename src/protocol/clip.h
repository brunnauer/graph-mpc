#pragma once

#include <omp.h>

#include <tuple>
#include <vector>

#include "../utils/mul.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

namespace clip {
/**
 * ----- F_eqz -----
 */
std::vector<Ring> equals_zero(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t BLOCK_SIZE, std::vector<Ring> &input_share);

std::vector<Ring> equals_zero_two(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                  std::vector<Ring> &input_share);

std::vector<std::tuple<Ring, Ring, Ring>> equals_zero_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n,
                                                                 size_t BLOCK_SIZE);

std::vector<Ring> equals_zero_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n);

std::vector<std::tuple<Ring, Ring, Ring>> equals_zero_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals, size_t &idx);

std::vector<Ring> equals_zero_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t BLOCK_SIZE,
                                       std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> &input_share);
/**
 * ----- F_B2A -----
 */

std::vector<Ring> B2A(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t BLOCK_SIZE, std::vector<Ring> &input_share);

std::vector<std::tuple<Ring, Ring, Ring>> B2A_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE);

std::vector<Ring> B2A_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n);

std::vector<std::tuple<Ring, Ring, Ring>> B2A_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals, size_t &idx);

std::vector<Ring> B2A_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t BLOCK_SIZE,
                               std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> &input_share);
/**
 * ----- Flip -----
 */
std::vector<Ring> flip(Party id, std::vector<Ring> &input_share);
};  // namespace clip