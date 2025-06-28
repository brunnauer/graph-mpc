#pragma once

#include <omp.h>

#include "types.h"

static std::vector<Ring> from_bits(std::vector<std::vector<Ring>> &input_bits, size_t n) {
    std::vector<Ring> result(n);

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < n; ++i) {
        result[i] = 0;
        for (size_t j = 0; j < input_bits.size(); j++) {
            result[i] += input_bits[j][i] << j;
        }
    }
    return result;
}

static std::vector<std::vector<Ring>> to_bits(std::vector<Ring> &input_vec, size_t n_bits) {
    size_t n = input_vec.size();
    std::vector<std::vector<Ring>> bits(n_bits);

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < n_bits; ++i) {
        bits[i].resize(n);
        for (size_t j = 0; j < n; ++j) {
            bits[i][j] = (input_vec[j] & (1 << i)) >> i;
        }
    }
    return bits;
}
