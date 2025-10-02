#pragma once

#include <emp-tool/emp-tool.h>

#include <cstdint>
#include <random>

#include "types.h"

class RandomGenerators {
    emp::PRG _rng_01;
    emp::PRG _rng_D0;
    emp::PRG _rng_D0_unshuffle;
    emp::PRG _rng_D0_comp;
    emp::PRG _rng_D1;
    emp::PRG _rng_D1_unshuffle;
    emp::PRG _rng_D1_comp;
    emp::PRG _rng_D;
    emp::PRG _rng_self;
    std::vector<uint64_t> seeds_hi;
    std::vector<uint64_t> seeds_lo;

   public:
    RandomGenerators(std::vector<uint64_t> seeds_hi, std::vector<uint64_t> seeds_lo) : seeds_hi(seeds_hi), seeds_lo(seeds_lo) { reseed(); }

    void reseed() {
        auto seed_block = emp::makeBlock(seeds_hi[0], seeds_lo[0]);
        _rng_self.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[1], seeds_lo[1]);
        _rng_D.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[2], seeds_lo[2]);
        _rng_D0.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[3], seeds_lo[3]);
        _rng_D1.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[4], seeds_lo[4]);
        _rng_01.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[5], seeds_lo[5]);
        _rng_D0_unshuffle.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[6], seeds_lo[6]);
        _rng_D1_unshuffle.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[7], seeds_lo[7]);
        _rng_D0_comp.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[8], seeds_lo[8]);
        _rng_D1_comp.reseed(&seed_block, 0);
    }

    emp::PRG &rng_01() { return _rng_01; }
    emp::PRG &rng_D0() { return _rng_D0; }
    emp::PRG &rng_D0_unshuffle() { return _rng_D0_unshuffle; }
    emp::PRG &rng_D0_comp() { return _rng_D0_comp; }
    emp::PRG &rng_D1() { return _rng_D1; }
    emp::PRG &rng_D1_unshuffle() { return _rng_D1_unshuffle; }
    emp::PRG &rng_D1_comp() { return _rng_D1_comp; }
    emp::PRG &rng_D() { return _rng_D; }
    emp::PRG &rng_self() { return _rng_self; }

    void print_state() {
        std::cout << "--- RNG counters --- " << std::endl;
        std::cout << "01: " << _rng_01.counter << std::endl;
        std::cout << "D0: " << _rng_D0.counter << std::endl;
        std::cout << "D0_u: " << _rng_D0_unshuffle.counter << std::endl;
        std::cout << "D0_c: " << _rng_D0_comp.counter << std::endl;
        std::cout << "D1: " << _rng_D1.counter << std::endl;
        std::cout << "D1_u: " << _rng_D1_unshuffle.counter << std::endl;
        std::cout << "D1_c: " << _rng_D1_comp.counter << std::endl << std::endl;
    }
};
