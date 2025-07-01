#pragma once

#include <tuple>
#include <vector>

#include "random_generators.h"
#include "sharing.h"
#include "types.h"

namespace mul {

std::vector<std::tuple<Ring, Ring, Ring>> preprocess(Party id, RandomGenerators &rngs, std::vector<Ring> &vals_to_p1, size_t &idx, size_t n);

std::vector<std::tuple<Ring, Ring, Ring>> preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n);

std::vector<Ring> evaluate(Party id, std::shared_ptr<NetworkInterface> network, size_t n, std::vector<std::tuple<Ring, Ring, Ring>> &triples,
                           std::vector<Ring> x, std::vector<Ring> y);

void evaluate_1(std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> &mult_vals, std::vector<Ring> x, std::vector<Ring> y, size_t n);

std::vector<Ring> evaluate_2(Party id, std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> &vals, std::vector<Ring> x, std::vector<Ring> y,
                             size_t n);

std::tuple<Ring, Ring, Ring> preprocess_one(Party id, RandomGenerators &rngs, std::vector<Ring> &vals_to_p1, size_t &idx);

Ring evaluate_one(Party id, std::shared_ptr<NetworkInterface> network, std::tuple<Ring, Ring, Ring> &triple, Ring x, Ring y);

std::vector<std::tuple<Ring, Ring, Ring>> preprocess_bin(Party id, RandomGenerators &rngs, std::vector<Ring> &vals_to_p1, size_t &idx, size_t n);

std::vector<std::tuple<Ring, Ring, Ring>> preprocess_bin(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n);

std::vector<Ring> evaluate_bin(Party id, std::shared_ptr<NetworkInterface> network, size_t n, std::vector<std::tuple<Ring, Ring, Ring>> &triples,
                               std::vector<Ring> x, std::vector<Ring> y);

std::tuple<Ring, Ring, Ring> preprocess_one_bin(Party id, RandomGenerators &rngs, std::vector<Ring> &vals_to_p1, size_t &idx);

Ring evaluate_one_bin(Party id, std::shared_ptr<NetworkInterface> network, std::tuple<Ring, Ring, Ring> &triple, Ring x, Ring y);

};  // namespace mul
