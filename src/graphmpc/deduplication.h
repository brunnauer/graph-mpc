#pragma once

#include "../utils/graph.h"
#include "../utils/permutation.h"
#include "../utils/preprocessings.h"
#include "../utils/types.h"
#include "clip.h"
#include "shuffle.h"
#include "sort.h"

void deduplication_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, MPPreprocessing &preproc,
                              Party &recv);

void deduplication_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Graph &g);

// void deduplication(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, std::vector<std::vector<Ring>> &src_bits,
//                  std::vector<std::vector<Ring>> &dst_bits, std::vector<Ring> &src, std::vector<Ring> &dst);
