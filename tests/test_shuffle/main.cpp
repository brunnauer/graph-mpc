#include "../src/perm.h"
#include "../src/prng/duthomhas/csprng.hpp"
#include "../src/random_generators.h"
#include "../src/shuffle.h"
#include "../src/utils/types.h"

void test_correctness() {
    const size_t n_rows = 100;
    const size_t n_rounds = 1;

    /* Dummy seed */
    uint64_t seeds[4] = {123456789, 123456789, 123456789, 123456789};
    RandomGenerators _rngs(seeds, seeds);

    Shuffle sh(n_rows, n_rounds, _rngs);
    PreprocessingResult res = sh.preprocess();

    assert((res.pi_0 * res.pi_1 == res.pi_0_p * res.pi_1_p));

    std::vector<Row> B(n_rows);
    for (int i = 0; i < n_rows; ++i) {
        B[i] = res.B_0[i] + res.B_1[i];
    }

    std::vector<Row> sum(n_rows);
    for (int i = 0; i < n_rows; ++i) {
        sum[i] = (res.pi_0 * res.pi_1)(res.R_1)[i] + res.pi_0_p(res.R_0)[i];
    }

    assert((B == sum));
}

int main(int argc, char **argv) {
    return 0;
}