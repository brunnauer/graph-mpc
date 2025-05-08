#pragma once

#include <NTL/ZZ_p.h>

#include <cstdint>

enum Party { P0 = 0, P1 = 1, D = 2 };

constexpr uint64_t FIELDSIZE = 4;  // bytes
using Field = NTL::ZZ_p;

typedef uint32_t Row;