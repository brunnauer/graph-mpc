#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <numeric>
#include <tuple>
#include <vector>

#include "io/netmp.h"
#include "perm.h"
#include "random_generators.h"
#include "utils/types.h"

template <class R>
struct ShufflePreprocessing {
    Permutation pi_0, pi_1, pi_0_p, pi_1_p;
    std::vector<R> R_0, R_1, B_0, B_1;
    ShufflePreprocessing() = default;
    ShufflePreprocessing(Permutation &pi_0, Permutation &pi_1, Permutation &pi_0_p, Permutation &pi_1_p, std::vector<R> R_0, std::vector<R> R_1, std::vector<R> B_0, std::vector<R> B_1)
        : pi_0(pi_0), pi_1(pi_1), pi_0_p(pi_0_p), pi_1_p(pi_1_p), R_0(R_0), R_1(R_1), B_0(B_0), B_1(B_1) {}
};

class Shuffle {
   public:
    Shuffle(Party pid, size_t n_rows, size_t n_rounds, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network);
    void run();
    void evaluate(size_t shuffle_idx);
    void evaluate_send(size_t shuffle_idx, std::vector<Row> &vals);
    void evaluate_recv(size_t shuffle_idx, std::vector<Row> &vals);
    void preprocess();
    void preprocess_compute(std::vector<Row> &shared_secret_D0, std::vector<Row> &shared_secret_D1);
    void set_input(std::vector<Row> &input);

   private:
    Party pid;
    size_t n_rows, n_rounds;
    size_t current_round = 0;
    const size_t BLOCK_SIZE = 100000000;
    RandomGenerators rngs;
    std::vector<std::shared_ptr<Permutation>> pis_0, pis_1, pis_0_p, pis_1_p;
    std::vector<std::shared_ptr<ShufflePreprocessing<Row>>> preproc;
    std::shared_ptr<io::NetIOMP> network;
    std::vector<Row> wire;
};
