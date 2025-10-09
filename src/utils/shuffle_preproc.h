#pragma once

#include "../utils/permutation.h"
#include "types.h"

class ShufflePre {
   public:
    ShufflePre() = default;

    ShufflePre(Permutation &pi_0, Permutation &pi_1, Permutation &pi_0_p, Permutation &pi_1_p, std::vector<Ring> &B, std::vector<Ring> &R)
        : pi_0(pi_0), pi_1(pi_1), pi_0_p(pi_0_p), pi_1_p(pi_1_p), B(B), R(R) {};

    Permutation pi_0;
    Permutation pi_1;
    Permutation pi_0_p;
    Permutation pi_1_p;
    std::vector<Ring> B;
    std::vector<Ring> R;
    bool preprocessed = false;
};