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
    void print() {
        std::cout << "pi_0: ";
        pi_0.print();
        std::cout << std::endl;

        std::cout << "pi_1: ";
        pi_1.print();
        std::cout << std::endl;

        std::cout << "pi_0_p: ";
        pi_0_p.print();
        std::cout << std::endl;

        std::cout << "pi_1_p: ";
        pi_1_p.print();
        std::cout << std::endl;

        if (!R_0.empty()) {
            std::cout << "R_0: ";
            for (int i = 0; i < R_0.size(); ++i) {
                std::cout << R_0[i];
                if (i != R_0.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
        }
        if (!R_1.empty()) {
            std::cout << "R_1: ";
            for (int i = 0; i < R_1.size(); ++i) {
                std::cout << R_1[i];
                if (i != R_1.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
        }
        if (!B_0.empty()) {
            std::cout << "B_0: ";
            for (int i = 0; i < B_0.size(); ++i) {
                std::cout << B_0[i];
                if (i != B_0.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
        }
        if (!B_1.empty()) {
            std::cout << "B_1: ";
            for (int i = 0; i < B_1.size(); ++i) {
                std::cout << B_1[i];
                if (i != B_1.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
        }
    }
    ShufflePreprocessing() = default;
    ShufflePreprocessing(Permutation &pi_0, Permutation &pi_1, Permutation &pi_0_p, Permutation &pi_1_p, std::vector<R> R_0, std::vector<R> R_1,
                         std::vector<R> B_0, std::vector<R> B_1)
        : pi_0(pi_0), pi_1(pi_1), pi_0_p(pi_0_p), pi_1_p(pi_1_p), R_0(R_0), R_1(R_1), B_0(B_0), B_1(B_1) {}
    ~ShufflePreprocessing() = default;
};

class Shuffle {
   public:
    Shuffle(Party pid, size_t n_rows, size_t n_rounds, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network);
    ~Shuffle();
    void set_input(std::vector<Row> &input);
    std::vector<std::shared_ptr<ShufflePreprocessing<Row>>> get_preproc();
    void run();
    void run_offline();
    void run_online();
    std::vector<Row> result();

   private:
    Party pid;
    size_t n_rows, n_rounds;
    size_t shuffle_idx = 0;
    const size_t BLOCK_SIZE = 100000000;
    RandomGenerators rngs;
    std::vector<std::shared_ptr<ShufflePreprocessing<Row>>> preproc;
    std::shared_ptr<io::NetIOMP> network;
    std::vector<Row> wire;

    void evaluate();
    void evaluate_send_vals(std::vector<Row> &vals);
    void evaluate_recv_vals(std::vector<Row> &vals);
    void preprocess();
    void preprocess_compute(std::vector<Row> &shared_secret_D0, std::vector<Row> &shared_secret_D1);
};
