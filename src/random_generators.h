#pragma once

#include <cstdint>
#include <random>

#include "prng/duthomhas/csprng.hpp"
#include "utils/types.h"

class RandomGenerators {
    duthomhas::csprng prng_01;
    duthomhas::csprng prng_D0;
    duthomhas::csprng prng_D1;
    duthomhas::csprng prng_D;
    duthomhas::csprng prng_self;

   public:
    RandomGenerators(uint64_t seeds_hi[5], uint64_t seeds_lo[5]);
    duthomhas::csprng &rng_01();
    duthomhas::csprng &rng_D0();
    duthomhas::csprng &rng_D1();
    duthomhas::csprng &rng_D();
    duthomhas::csprng &rng_self();
};