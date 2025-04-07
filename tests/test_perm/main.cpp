#include <cassert>
#include <iostream>
#include <numeric>

#include "../src/perm.h"
#include "../src/prng/duthomhas/csprng.hpp"
#include "../src/random_generators.h"
#include "../src/utils/types.h"

RandomGenerators rngs((uint64_t[4]){123456789, 123456789, 123456789, 123456789}, (uint64_t[4]){123456789, 123456789, 123456789, 123456789});

bool contains_duplicates(Permutation p) {
    for (int i = 0; i < p.size(); ++i) {
        for (int j = 0; j < p.size(); ++j) {
            if (i != j) {
                if (p[i] == p[j]) {
                    return true;
                }
            }
        }
    }
    return false;
}

void test_plausibility() {
    Permutation swap_4_5({0, 1, 2, 3, 5, 4, 6, 7, 8, 9});
    Permutation swap_0_9({9, 1, 2, 3, 4, 5, 6, 7, 8, 0});
    std::vector<Row> test_vec = std::vector<Row>({0, 0, 0, 0, 0, 1, 1, 1, 1, 1});

    std::vector<Row> res1 = swap_4_5(test_vec);
    assert((res1 == std::vector<Row>({0, 0, 0, 0, 1, 0, 1, 1, 1, 1})));

    std::vector<Row> res2 = swap_0_9(res1);
    assert((res2 == std::vector<Row>({1, 0, 0, 0, 1, 0, 1, 1, 1, 0})));
}

void test_associativity() {
    const int n_elems = 100;
    Permutation pi_0 = Permutation::random(n_elems, rngs.rng_D0());
    Permutation pi_1 = Permutation::random(n_elems, rngs.rng_D1());
    Permutation pi_2 = Permutation::random(n_elems, rngs.rng_D1());

    std::vector<Row> test_table = std::vector<Row>(n_elems);
    std::iota(test_table.begin(), test_table.end(), 0);

    std::vector<Row> res1 = ((pi_0 * pi_1) * pi_2)(test_table);
    std::vector<Row> res2 = (pi_0 * (pi_1 * pi_2))(test_table);

    assert((res1 == res2));
}

void test_inverse() {
    const int n_elems = 100;
    Permutation perm = Permutation::random(n_elems, rngs.rng_D());
    Permutation inv = perm.inverse();

    std::vector<Row> test_vec = std::vector<Row>(n_elems);
    std::iota(test_vec.begin(), test_vec.end(), 0);

    assert(((perm * inv)(test_vec) == test_vec));
}

void test_pi_1_p() {
    const int n_elems = 100;
    Permutation pi_0 = Permutation::random(n_elems, rngs.rng_D0());
    Permutation pi_1 = Permutation::random(n_elems, rngs.rng_D1());
    Permutation pi_0_p = Permutation::random(n_elems, rngs.rng_D0());

    Permutation pi_1_p = pi_0_p.inverse() * (pi_0 * pi_1);
    assert((pi_0_p * pi_1_p == pi_0 * pi_1));
}

int main(int argc, char **argv) {
    test_plausibility();
    test_associativity();
    test_inverse();
    test_pi_1_p();

    return 0;
}