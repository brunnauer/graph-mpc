#pragma once

#include <omp.h>

#include <algorithm>

#include "../setup/utils.h"
#include "../utils/graph.h"
#include "../utils/protocol_config.h"
#include "sorting.h"

struct MPPreprocessing {
    SortPreprocessing src_order_pre;
    SortPreprocessing dst_order_pre;
    SortPreprocessing vtx_order_pre;
    PermShare apply_perm_pre;
    std::vector<std::tuple<PermShare, PermShare, PermShare>> sw_perm_pre;
};

namespace mp {
std::vector<Ring> propagate_1(std::vector<Ring> &input_vector, size_t num_v);

std::vector<Ring> propagate_2(std::vector<Ring> &input_vector, std::vector<Ring> &correction_vector);

std::vector<Ring> gather_1(std::vector<Ring> &input_vector);

std::vector<Ring> gather_2(std::vector<Ring> &input_vector, size_t num_v);

std::vector<Ring> apply(std::vector<Ring> &in1, std::vector<Ring> &in2);

std::tuple<std::vector<std::vector<Ring>>, std::vector<std::vector<Ring>>, std::vector<Ring>> init(ProtocolConfig &c, SecretSharedGraph &g);

MPPreprocessing run_preprocess(ProtocolConfig &c, size_t n_iterations);

void run_evaluate(ProtocolConfig &c, SecretSharedGraph &graph, size_t n_iterations, size_t num_v, MPPreprocessing &preproc);

void run(ProtocolConfig &c, SecretSharedGraph &graph, size_t n_iterations, size_t num_v);
}  // namespace mp