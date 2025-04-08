#pragma once

#include <NTL/ZZ_p.h>

#include <cstdint>

enum Party {
    P0,
    P1,
    P2
};

constexpr Party Dealer = P2;
constexpr uint64_t FIELDSIZE = 4;  // bytes
using Field = NTL::ZZ_p;

typedef uint32_t Row;