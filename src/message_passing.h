#pragma once

#include <algorithm>

#include "../setup/utils.h"
#include "graph.h"
#include "protocol_config.h"
#include "sorting.h"

namespace mp {
std::vector<Row> propagate_1(ProtocolConfig &conf, std::vector<Row> &input_vector, size_t num_v);

std::vector<Row> propagate_2(ProtocolConfig &conf, std::vector<Row> &input_vector, std::vector<Row> &correction_vector);

std::vector<Row> gather_1(ProtocolConfig &conf, std::vector<Row> &input_vector);

std::vector<Row> gather_2(ProtocolConfig &conf, std::vector<Row> &input_vector, size_t num_v);

std::vector<Row> apply(ProtocolConfig &conf, std::vector<Row> &in1, std::vector<Row> &in2);

void run(ProtocolConfig &conf, SecretSharedGraph &graph, size_t n_iterations, size_t num_vert);
}  // namespace mp