#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <vector>

#include "prng/duthomhas/csprng.hpp"

/***
 * Definition of the Permutation p:
 * --> Let p = {1, 2, 3, 4, 0}
 * --> Let t = {0, 1, 2, 3, 4}
 * --> Then p(t) = {4, 0, 1, 2, 3} s.t. p(t)[i] = t[p[i]]
 */

class Permutation {
   public:
    Permutation() : perm_vec(std::vector<size_t>()) {};

    Permutation(size_t n) {
        perm_vec = std::vector<size_t>(n);
        std::iota(perm_vec.begin(), perm_vec.end(), 0);
    };

    Permutation(std::vector<size_t> _perm_vec) : perm_vec(_perm_vec) {};

    [[nodiscard]] static Permutation random(int n_rows, duthomhas::csprng rng) {
        Permutation p = Permutation(n_rows);
        for (int i = 0; i < n_rows; ++i) {
            size_t k = rng(size_t());
            k %= (n_rows - i);
            std::swap(p.perm_vec[i], p.perm_vec[i + k]);
        }
        return p;
    }

    [[nodiscard]] Permutation inverse() {
        std::vector<size_t> inverse_vec(perm_vec.size());
        for (int i = 0; i < perm_vec.size(); ++i) {
            inverse_vec[perm_vec[i]] = i;
        }
        return Permutation(inverse_vec);
    }

    [[nodiscard]] size_t size() const {
        return perm_vec.size();
    }

    [[nodiscard]] size_t operator[](size_t idx) {
        return perm_vec[idx];
    }

    template <typename T>
    std::vector<T> operator()(const std::vector<T> &input_vec) const {
        assert(input_vec.size() == perm_vec.size());
        std::vector<T> result(input_vec.size());
        for (size_t i = 0; i < perm_vec.size(); ++i) {
            result[i] = input_vec[perm_vec[i]];
        }
        return result;
    }

    template <typename T>
    std::vector<T> operator()(Permutation &perm) const {
        assert(perm.perm_vec.size() == perm_vec.size());
        std::vector<T> result(perm.perm_vec.size());
        for (size_t i = 0; i < perm_vec.size(); ++i) {
            result[i] = perm.perm_vec[perm_vec[i]];
        }
        return result;
    }

    [[nodiscard]] Permutation operator*(Permutation other) const {
        size_t n = perm_vec.size();
        std::vector<size_t> base(n);
        std::iota(base.begin(), base.end(), 0);

        return Permutation(operator()(other(base)));
    }

    bool operator==(Permutation other) const {
        return other.perm_vec == perm_vec;
    }

    void print() {
        std::cout << "{";
        for (int i = 0; i < perm_vec.size() - 1; ++i) {
            std::cout << perm_vec[i] << ", ";
        }
        std::cout << perm_vec[perm_vec.size() - 1] << "}" << std::endl;
    }

   private:
    std::vector<size_t> perm_vec;
};