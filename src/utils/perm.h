#pragma once

#include <emp-tool/emp-tool.h>
#include <omp.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <vector>

#include "./utils/types.h"

/***
 * Definition of the Permutation p:
 * --> Let p = {1, 2, 3, 4, 0}
 * --> Let t = {0, 1, 2, 3, 4}
 * --> Then p(t) = {4, 0, 1, 2, 3} s.t. p(t)[i] = t[p[i]]
 */

class Permutation {
   public:
    Permutation() : perm_vec(std::vector<Ring>()) {};

    Permutation(size_t n) {
        perm_vec = std::vector<Ring>(n);
        std::iota(perm_vec.begin(), perm_vec.end(), 0);
    };

    Permutation(std::vector<Ring> _perm_vec) : perm_vec(_perm_vec) {};

    [[nodiscard]] static Permutation random(int n_rows, emp::PRG &rng) {
        Permutation p = Permutation(n_rows);
        for (int i = 0; i < n_rows; ++i) {
            size_t k;
            rng.random_data(&k, sizeof(size_t));
            k %= (n_rows - i);
            std::swap(p.perm_vec[i], p.perm_vec[i + k]);
        }
        return p;
    }

    [[nodiscard]] Permutation inverse() {
        std::vector<Ring> inverse_vec(perm_vec.size());
#pragma omp parallel for if (perm_vec.size() > 10000)
        for (size_t i = 0; i < perm_vec.size(); ++i) {
            inverse_vec[perm_vec[i]] = i;
        }
        return Permutation(inverse_vec);
    }

    [[nodiscard]] size_t size() const { return perm_vec.size(); }

    [[nodiscard]] Ring operator[](size_t idx) { return perm_vec[idx]; }

    [[nodiscard]] std::vector<Ring> get_perm_vec() { return perm_vec; }

    template <typename T>
    std::vector<T> operator()(const std::vector<T> &input_vec) const {
        assert(input_vec.size() == perm_vec.size());
        std::vector<T> result(input_vec.size());
#pragma omp parallel for if (perm_vec.size() > 10000)
        for (size_t i = 0; i < perm_vec.size(); ++i) {
            if (perm_vec[i] > input_vec.size()) throw std::logic_error("Cannot apply secret shared permutation.");
            result[perm_vec[i]] = input_vec[i];
        }
        return result;
    }

    template <typename T>
    Permutation operator()(const Permutation &perm) const {
        assert(perm.perm_vec.size() == perm_vec.size());
        std::vector<T> result(perm.perm_vec.size());
        for (size_t i = 0; i < perm_vec.size(); ++i) {
            if (perm_vec[i] > perm.perm_vec.size()) throw std::logic_error("Cannot apply secret shared permutation.");
            result[perm_vec[i]] = perm.perm_vec[i];
        }
        return result;
    }

    [[nodiscard]] Permutation operator*(const Permutation &other) const {
        size_t n = perm_vec.size();
        std::vector<Ring> result(n);

#pragma omp parallel for if (perm_vec.size() > 10000)
        for (size_t i = 0; i < perm_vec.size(); ++i) {
            result[i] = perm_vec[other.perm_vec[i]];
        }
        return Permutation(result);
    }

    bool operator==(Permutation other) const { return other.perm_vec == perm_vec; }

    bool is_null() {
        bool null = true;
        for (const auto elem : perm_vec) {
            if (elem != 0) null = false;
        }
        return null;
    }

    bool not_null() {
        bool null = true;
        for (const auto elem : perm_vec) {
            if (elem != 0) null = false;
        }
        return !null;
    }

    void print() {
        std::cout << "{";
        for (size_t i = 0; i < perm_vec.size() - 1; ++i) {
            std::cout << perm_vec[i] << ", ";
        }
        std::cout << perm_vec[perm_vec.size() - 1] << "}" << std::endl;
    }

   private:
    std::vector<Ring> perm_vec;
};