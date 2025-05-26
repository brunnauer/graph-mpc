#pragma once

#include <memory>
#include <vector>

#include "./utils/types.h"
#include "io/comm.h"
#include "perm.h"
#include "protocol_config.h"
#include "sharing.h"

namespace compaction {

void preprocess(ProtocolConfig &conf, std::vector<Row> &triple_a, std::vector<Row> &triple_b, std::vector<Row> &triple_c);

Permutation evaluate(ProtocolConfig &conf, std::vector<Row> &triple_a, std::vector<Row> &triple_b, std::vector<Row> &triple_c, std::vector<Row> &input_share);

Permutation get_compaction(ProtocolConfig &conf, std::vector<Row> &input_share);

};  // namespace compaction