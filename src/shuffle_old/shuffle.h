#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <numeric>
#include <tuple>
#include <vector>

#include "../io/comm.h"
#include "../io/netmp.h"
#include "../perm.h"
#include "../protocol_config.h"
#include "../random_generators.h"
#include "../utils/types.h"

class Shuffle {
   public:
    Shuffle(ProtocolConfig &conf, size_t n_rounds);
    ~Shuffle();
    void set_input(std::vector<Row> &input);
    void set_input(Permutation &input);
    void run();
    void get_shuffle();
    void shuffle();
    void unshuffle();
    void repeat();
    std::vector<Row> get_output();
    std::vector<Row> reveal();

   private:
    Party pid;
    size_t n_rows, n_rounds;
    size_t shuffle_idx = 0;
    bool _reverse = false;
    bool _new_perm = true;
    const size_t BLOCK_SIZE_PRE = 100000000;  // size of one message sent during preprocessing
    const size_t BLOCK_SIZE_EVAL = 100000;    // size of one message sent during evaluation // 100000000;
                                              // Was 100000 during LAN benchmarks as per Graphiti,
                                              // optimized to less rounds for WAN here!
    RandomGenerators rngs;
    std::shared_ptr<io::NetIOMP> network;
    std::vector<Row> wire;

    std::vector<Permutation> pi_0_vec;    // vector for storing preprocessed pi_0's (used only by P0)
    std::vector<Permutation> pi_0_p_vec;  // vector for storing preprocessed pi_0_p's (used only by P0)
    std::vector<Permutation> pi_1_vec;    // vector for storing preprocessed pi_1's (used only by P1)
    std::vector<Permutation> pi_1_p_vec;  // vector for storing preprocessed pi_1_p's (used only by P1)
    std::vector<std::vector<Row>> B_vec;  // vector for storing preprocessed B_0/B_1's
    std::vector<std::vector<Row>> R_vec;  // vector for storing R_0/R_1's

    void evaluate();
    void evaluate_compute_A(std::vector<Row> &vals);
    void evaluate_compute_output(std::vector<Row> &vals);
    void preprocess();
    void preprocess_compute(std::vector<Row> &shared_secret_D0, std::vector<Row> &shared_secret_D1);
    void preprocess_compute_reverse(std::vector<Row> &shared_secret_D0, std::vector<Row> &shared_secret_D1);
};
