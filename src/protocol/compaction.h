#pragma once

#include <memory>
#include <vector>

#include "../io/comm.h"
#include "../utils/perm.h"
#include "../utils/protocol_config.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

namespace compaction {

void preprocess(ProtocolConfig &conf, std::vector<Row> &triple_a, std::vector<Row> &triple_b, std::vector<Row> &triple_c);

Permutation evaluate(ProtocolConfig &conf, std::vector<Row> &triple_a, std::vector<Row> &triple_b, std::vector<Row> &triple_c, std::vector<Row> &input_share);

Permutation get_compaction(ProtocolConfig &conf, std::vector<Row> &input_share);

};  // namespace compaction