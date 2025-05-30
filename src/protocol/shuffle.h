#pragma once

#include "../utils/perm.h"
#include "../utils/protocol_config.h"
#include "../utils/types.h"
#include "io/comm.h"

struct PermShare {
    Permutation pi_0;
    Permutation pi_1;
    Permutation pi_0_p;
    Permutation pi_1_p;
    std::vector<Row> B;
    std::vector<Row> R;
};

namespace shuffle {

PermShare get_shuffle_compute(ProtocolConfig &c, std::vector<Row> &shared_secret_D0, std::vector<Row> &shared_secret_D1);

PermShare get_shuffle(ProtocolConfig &c);

std::vector<Row> shuffle(ProtocolConfig &c, std::vector<Row> &input_share, PermShare &perm_share, bool save);

Permutation shuffle(ProtocolConfig &c, Permutation &perm, PermShare &perm_share, bool save);

std::vector<Row> unshuffle(ProtocolConfig &c, std::vector<Row> &input_share, PermShare &perm_share);

Permutation unshuffle(ProtocolConfig &c, Permutation &perm, PermShare &perm_share);

PermShare get_merged_shuffle(ProtocolConfig &c, PermShare &pi_share, PermShare &omega_share);

};  // namespace shuffle