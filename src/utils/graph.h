#pragma once

#include <omp.h>

#include <vector>

#include "types.h"

struct Graph {
    std::vector<Ring> src;
    std::vector<Ring> dst;
    std::vector<Ring> isV;
    std::vector<Ring> payload;
    size_t size = 0;

    Graph() = default;
    Graph(size_t n_rows) {
        src = std::vector<Ring>(n_rows);
        dst = std::vector<Ring>(n_rows);
        isV = std::vector<Ring>(n_rows);
        payload = std::vector<Ring>(n_rows);
        size = n_rows;
    }

    std::vector<std::vector<Ring>> src_bits() {
        size_t n_bits = sizeof(Ring) * 8;
        std::vector<std::vector<Ring>> bits(n_bits);
        for (size_t i = 0; i < n_bits; ++i) {
            bits[i].resize(size);
            for (size_t j = 0; j < size; ++j) {
                bits[i][j] = (src[j] & (1 << i)) >> i;
            }
        }
        return bits;
    }

    std::vector<std::vector<Ring>> dst_bits() {
        size_t n_bits = sizeof(Ring) * 8;
        std::vector<std::vector<Ring>> bits(n_bits);
        for (size_t i = 0; i < n_bits; ++i) {
            bits[i].resize(size);
            for (size_t j = 0; j < size; ++j) {
                bits[i][j] = (dst[j] & (1 << i)) >> i;
            }
        }
        return bits;
    }

    std::vector<std::vector<Ring>> payload_bits() {
        size_t n_bits = sizeof(Ring) * 8;
        std::vector<std::vector<Ring>> bits(n_bits);
        for (size_t i = 0; i < n_bits; ++i) {
            bits[i].resize(size);
            for (size_t j = 0; j < size; ++j) {
                bits[i][j] = (payload[j] & (1 << i)) >> i;
            }
        }
        return bits;
    }

    void print() {
        for (size_t i = 0; i < size; ++i) {
            std::cout << src[i] << " " << dst[i] << " " << isV[i] << " " << payload[i] << std::endl;
        }
        std::cout << std::endl;
    }
};

struct SecretSharedGraph {
    std::vector<std::vector<Ring>> src_bits;
    std::vector<std::vector<Ring>> dst_bits;
    std::vector<Ring> isV_bits;
    std::vector<std::vector<Ring>> payload_bits;
    size_t size = 0;

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
        std::vector<std::vector<Ring>> bits(n_bits);
        for (size_t i = 0; i < n_bits; ++i) {
            bits[i].resize(input_vec.size());
            for (size_t j = 0; j < input_vec.size(); ++j) {
                bits[i][j] = (input_vec[j] & (1 << i)) >> i;
            }
        }
        return bits;
    }
};
