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
void preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv);

std::vector<std::tuple<Ring, Ring, Ring>> preprocess(Party id, RandomGenerators &rngs, std::vector<Ring> &D1, size_t &idx, size_t n);

std::vector<std::tuple<Ring, Ring, Ring>> preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n);

std::tuple<Ring, Ring, Ring> preprocess_one(Party id, RandomGenerators &rngs, std::vector<Ring> &D1, size_t &idx);

std::vector<Ring> evaluate(Party id, std::shared_ptr<io::NetIOMP> network, size_t n, std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> x,
                           std::vector<Ring> y);

std::vector<Ring> evaluate(Party id, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, std::vector<Ring> x, std::vector<Ring> y);

Ring evaluate_one(Party id, std::shared_ptr<io::NetIOMP> network, std::tuple<Ring, Ring, Ring> &triple, Ring x, Ring y);

std::vector<std::tuple<Ring, Ring, Ring>> preprocess_bin(Party id, RandomGenerators &rngs, std::vector<Ring> &vals_to_p1, size_t &idx, size_t n);

std::vector<std::tuple<Ring, Ring, Ring>> preprocess_bin(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n);

std::vector<Ring> evaluate_bin(Party id, std::shared_ptr<io::NetIOMP> network, size_t n, std::vector<std::tuple<Ring, Ring, Ring>> &triples,
                               std::vector<Ring> x, std::vector<Ring> y);

std::tuple<Ring, Ring, Ring> preprocess_one_bin(Party id, RandomGenerators &rngs, std::vector<Ring> &vals_to_p1, size_t &idx);

Ring evaluate_one_bin(Party id, std::shared_ptr<io::NetIOMP> network, std::tuple<Ring, Ring, Ring> &triple, Ring x, Ring y);

};  // namespace mul
