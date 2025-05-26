#pragma once

#include "compaction.h"
#include "perm.h"
#include "protocol_config.h"
#include "sharing.h"
#include "shuffle.h"

namespace sort {

Permutation sort_iteration(ProtocolConfig &conf, Permutation &perm, std::vector<Row> &bit_vec_share);

Permutation get_sort(ProtocolConfig &conf, std::vector<std::vector<Row>> &bit_shares);

std::vector<Row> apply_perm(ProtocolConfig &conf, Permutation &perm, std::vector<Row> &input_share);

std::vector<Row> switch_perm(ProtocolConfig &conf, Permutation &p1, Permutation &p2, std::vector<Row> &input_share);

};  // namespace sort