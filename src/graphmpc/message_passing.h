#pragma once

#include <omp.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <tuple>

#include "../io/netmp.h"
#include "../utils/preprocessings.h"
#include "../utils/sharing.h"
#include "shuffle.h"

namespace mp {

std::vector<Ring> propagate_1(std::vector<Ring> &input_vector, size_t n_vertices);

std::vector<Ring> propagate_2(std::vector<Ring> &input_vector, std::vector<Ring> &correction_vector);

std::vector<Ring> gather_1(std::vector<Ring> &input_vector);

std::vector<Ring> gather_2(std::vector<Ring> &input_vector, size_t n_vertices);

void prepare_permutations(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc);

}  // namespace mp