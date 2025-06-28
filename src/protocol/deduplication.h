#pragma once

#include "../utils/types.h"
#include "clip.h"
#include "permute.h"
#include "sort.h"

struct DeduplicationPreprocessing {
    SortPreprocessing sort_preproc;
    ShufflePre apply_perm_share;
    std::vector<Ring> unshuffle_B;
    std::vector<std::tuple<Ring, Ring, Ring>> eqz_triples_1;
    std::vector<std::tuple<Ring, Ring, Ring>> eqz_triples_2;
    std::vector<std::tuple<Ring, Ring, Ring>> mul_triples;
    std::vector<std::tuple<Ring, Ring, Ring>> B2A_triples;
};

DeduplicationPreprocessing deduplication_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE);

void deduplication_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                            DeduplicationPreprocessing &preproc, std::vector<std::vector<Ring>> &src_bits, std::vector<std::vector<Ring>> &dst_bits,
                            std::vector<Ring> &src, std::vector<Ring> &dst);

void deduplication(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                   std::vector<std::vector<Ring>> &src_bits, std::vector<std::vector<Ring>> &dst_bits, std::vector<Ring> &src, std::vector<Ring> &dst);
