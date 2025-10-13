#include <cassert>
#include <iostream>
#include <numeric>

#include "../src/utils/permutation.h"
#include "../src/utils/types.h"

RandomGenerators rngs(std::vector<uint64_t>({123456789, 123456789, 123456789, 123456789, 123456789, 123456789, 123456789, 123456789, 123456789}),
                      std::vector<uint64_t>({123456789, 123456789, 123456789, 123456789, 123456789, 123456789, 123456789, 123456789, 123456789}));

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
    Permutation swap_4_5(std::vector<Ring>({0, 1, 2, 3, 5, 4, 6, 7, 8, 9}));
    Permutation swap_0_9(std::vector<Ring>({9, 1, 2, 3, 4, 5, 6, 7, 8, 0}));
    std::vector<Ring> test_vec = std::vector<Ring>({0, 0, 0, 0, 0, 1, 1, 1, 1, 1});

    std::vector<Ring> res1 = swap_4_5(test_vec);
    assert((res1 == std::vector<Ring>({0, 0, 0, 0, 1, 0, 1, 1, 1, 1})));

    std::vector<Ring> res2 = swap_0_9(res1);
    assert((res2 == std::vector<Ring>({1, 0, 0, 0, 1, 0, 1, 1, 1, 0})));
}

void test_associativity() {
    const int n_elems = 100;
    Permutation pi_0 = Permutation::random(n_elems, rngs.rng_D0_recv());
    Permutation pi_1 = Permutation::random(n_elems, rngs.rng_D1_recv());
    Permutation pi_2 = Permutation::random(n_elems, rngs.rng_D1_recv());

    std::vector<Ring> test_table = std::vector<Ring>(n_elems);
    std::iota(test_table.begin(), test_table.end(), 0);

    std::vector<Ring> res1 = ((pi_0 * pi_1) * pi_2)(test_table);
    std::vector<Ring> res2 = (pi_0 * (pi_1 * pi_2))(test_table);

    assert((res1 == res2));
}

void test_inverse() {
    const int n_elems = 100;
    Permutation perm = Permutation::random(n_elems, rngs.rng_self());
    Permutation inv = perm.inverse();

    std::vector<Ring> test_vec = std::vector<Ring>(n_elems);
    std::iota(test_vec.begin(), test_vec.end(), 0);

    assert(((perm * inv)(test_vec) == test_vec));
}

void test_pi_1_p() {
    const int n_elems = 100;
    Permutation pi_0 = Permutation::random(n_elems, rngs.rng_D0_recv());
    Permutation pi_1 = Permutation::random(n_elems, rngs.rng_D1_recv());
    Permutation pi_0_p = Permutation::random(n_elems, rngs.rng_D0_recv());

    Permutation pi_1_p = pi_0_p.inverse() * (pi_0 * pi_1);
    assert((pi_0_p * pi_1_p == pi_0 * pi_1));
}

void test_fact_2_3() {
    const int n_elems = 100;
    Permutation pi = Permutation::random(n_elems, rngs.rng_self());
    Permutation sigma = Permutation::random(n_elems, rngs.rng_self());

    std::vector<Ring> a = std::vector<Ring>(n_elems);
    std::iota(a.begin(), a.end(), 0); /* 0, 1, 2, 3, ... */

    auto left = (pi * sigma).inverse()(a);
    auto right = (sigma.inverse() * pi.inverse())(a);

    assert(left == right);

    auto left_two = pi(pi.inverse()(a));
    auto right_two = pi.inverse()(pi(a));

    assert(left_two == right_two);
}

void test_observation_2_4() {
    const int n_elems = 100;
    Permutation pi = Permutation::random(n_elems, rngs.rng_self());
    Permutation sigma = Permutation::random(n_elems, rngs.rng_self());

    std::vector<Ring> a = std::vector<Ring>(n_elems);
    std::iota(a.begin(), a.end(), 0); /* 0, 1, 2, 3, ... */

    auto left = Permutation(pi(sigma.perm_vec));
    auto right = (sigma * pi.inverse());

    assert(left == right);
}

int main(int argc, char **argv) {
    test_plausibility();
    std::cout << "Passed plausibility." << std::endl;

    test_associativity();
    std::cout << "Passed associativity." << std::endl;

    test_inverse();
    std::cout << "Passed inverse." << std::endl;

    test_pi_1_p();
    std::cout << "Passed pi_1_p." << std::endl;

    test_fact_2_3();
    std::cout << "Passed fact_2_3." << std::endl;

    test_observation_2_4();
    std::cout << "Passed observation_2_4." << std::endl;
    return 0;
}