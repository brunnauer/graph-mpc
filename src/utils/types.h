#pragma once
#include <cstdint>

enum Party {
    P0,
    P1,
    P2
};

constexpr Party Dealer = P2;

typedef uint32_t Row;