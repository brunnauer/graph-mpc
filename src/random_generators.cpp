#include "random_generators.h"

duthomhas::csprng &RandomGenerators::rng_01() {
    return prng_01;
}
duthomhas::csprng &RandomGenerators::rng_D0() {
    return prng_D0;
}
duthomhas::csprng &RandomGenerators::rng_D1() {
    return prng_D1;
}
duthomhas::csprng &RandomGenerators::rng_D() {
    return prng_D;
}
duthomhas::csprng &RandomGenerators::rng_self() {
    return prng_self;
}

RandomGenerators::RandomGenerators(uint64_t seeds_hi[5], uint64_t seeds_lo[5]) {
    std::seed_seq sseq0 = {seeds_hi[0], seeds_lo[0]};
    std::seed_seq sseq1 = {seeds_hi[1], seeds_lo[1]};
    std::seed_seq sseq2 = {seeds_hi[2], seeds_lo[2]};
    std::seed_seq sseq3 = {seeds_hi[3], seeds_lo[3]};
    std::seed_seq sseq4 = {seeds_hi[4], seeds_lo[4]};

    prng_01.seed(sseq0);
    prng_D0.seed(sseq1);
    prng_D1.seed(sseq2);
    prng_D.seed(sseq3);
    prng_self.seed(sseq4);
}
