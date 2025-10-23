#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

enum Party { P0 = 0, P1 = 1, D = 2 };
enum Execution { BENCHMARK, TEST };
enum GType {
    Input,
    Output,
    Shuffle,
    Unshuffle,
    MergedShuffle,
    Compaction,
    Mul,
    Reveal,
    Permute,
    ReversePermute,
    AddConst,
    Add,
    Sub,
    Bit2A,
    EQZ,
    Gather1,
    Gather2,
    Propagate1,
    Propagate2,
    Flip,
    Insert,
    ConstructData
};

typedef uint32_t Ring;
