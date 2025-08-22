#pragma once

#include <functional>

#include "graphmpc/deduplication.h"
#include "graphmpc/message_passing.h"
#include "utils/graph.h"
#include "utils/types.h"

namespace MPFunctions {
void pre_mp_preprocessing(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, MPPreprocessing &preproc,
                          Party &recv_shuffle, Party &recv_mul, bool save_to_disk);

void apply_v_preprocessing(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, MPPreprocessing &preproc,
                           Party &recv_shuffle, Party &recv_mul, bool save_to_disk);
void post_mp_preprocessing(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, MPPreprocessing &preproc,
                           Party &recv_shuffle, Party &recv_mul, bool save_to_disk);

void pre_mp_eval(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, MPPreprocessing &preproc, Graph &g,
                 bool save_to_disk);
void apply_v_eval(std::vector<Ring> &old_data, std::vector<Ring> &new_data, bool save_to_disk);
void post_mp_eval(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, Graph &g, bool save_to_disk);

};  // namespace MPFunctions

// #define MP
// #define PI_M
#define PI_K