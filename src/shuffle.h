#pragma once

#include <cstdint>
#include <iostream>
#include <numeric>
#include <tuple>
#include <vector>

#include "perm.h"
#include "random_generators.h"
#include "utils/types.h"

typedef struct {
    Permutation pi_0;
    Permutation pi_1;
    Permutation pi_0_p;
    Permutation pi_1_p;
    std::vector<Row> R_0;
    std::vector<Row> R_1;
    std::vector<Row> B_0;
    std::vector<Row> B_1;
} PreprocessingResult;

class Shuffle {
   public:
    Shuffle(size_t _n_rows, size_t _n_rounds, RandomGenerators &rngs);
    std::tuple<std::vector<Row>, std::vector<Row>> shuffle(std::vector<Row> T_0, std::vector<Row> T_1);
    PreprocessingResult preprocess();

   private:
    size_t n_rows, n_rounds;
    RandomGenerators rngs;
    std::vector<std::shared_ptr<Permutation>> pis_0, pis_1, pis_0_p, pis_1_p;
};
