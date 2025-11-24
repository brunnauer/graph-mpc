#pragma once

#include <emp-tool/emp-tool.h>

#include <cstdint>
#include <random>

#include "types.h"

class RandomGenerators {
    emp::PRG _rng_01;
    emp::PRG _rng_D0_prep;
    emp::PRG _rng_D0_send;
    emp::PRG _rng_D0_recv;
    emp::PRG _rng_D1_prep;
    emp::PRG _rng_D1_send;
    emp::PRG _rng_D1_recv;
    emp::PRG _rng_self;

   public:
    std::vector<uint64_t> seeds_hi;
    std::vector<uint64_t> seeds_lo;

    RandomGenerators() = default;
    RandomGenerators(std::vector<uint64_t> seeds_hi, std::vector<uint64_t> seeds_lo) : seeds_hi(seeds_hi), seeds_lo(seeds_lo) { reseed(); }

    void reseed() {
        auto seed_block = emp::makeBlock(seeds_hi[0], seeds_lo[0]);
        _rng_self.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[1], seeds_lo[1]);
        _rng_D0_recv.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[2], seeds_lo[2]);
        _rng_D1_recv.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[3], seeds_lo[3]);
        _rng_01.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[4], seeds_lo[4]);
        _rng_D0_send.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[5], seeds_lo[5]);
        _rng_D1_send.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[6], seeds_lo[6]);
        _rng_D0_prep.reseed(&seed_block, 0);

        seed_block = emp::makeBlock(seeds_hi[7], seeds_lo[7]);
        _rng_D1_prep.reseed(&seed_block, 0);
    }

    emp::PRG &rng_01() { return _rng_01; }
    emp::PRG &rng_D0_prep() { return _rng_D0_prep; }
    emp::PRG &rng_D0_recv() { return _rng_D0_recv; }
    emp::PRG &rng_D0_send() { return _rng_D0_send; }
    emp::PRG &rng_D1_prep() { return _rng_D1_prep; }
    emp::PRG &rng_D1_recv() { return _rng_D1_recv; }
    emp::PRG &rng_D1_send() { return _rng_D1_send; }
    emp::PRG &rng_self() { return _rng_self; }

    void print_state() {
        std::cout << "--- RNG counters --- " << std::endl;
        std::cout << "01: " << _rng_01.counter << std::endl;
        std::cout << "D0_prep: " << _rng_D0_prep.counter << std::endl;
        std::cout << "D0_send: " << _rng_D0_send.counter << std::endl;
        std::cout << "D0_recv: " << _rng_D0_recv.counter << std::endl;
        std::cout << "D1_prep: " << _rng_D1_prep.counter << std::endl;
        std::cout << "D1_send: " << _rng_D1_send.counter << std::endl;
        std::cout << "D1_recv: " << _rng_D1_recv.counter << std::endl << std::endl;
    }
};
