#pragma once

#include "io/comm.h"
#include "perm.h"
#include "protocol_config.h"
#include "utils/types.h"

struct PermShare {
    Permutation pi_0;
    Permutation pi_1;
    Permutation pi_0_p;
    Permutation pi_1_p;
    std::vector<Row> B;
    std::vector<Row> R;
};

namespace shuffle {

PermShare get_shuffle_compute(ProtocolConfig &conf, std::vector<Row> &shared_secret_D0, std::vector<Row> &shared_secret_D1);

PermShare get_shuffle(ProtocolConfig &conf);

std::vector<Row> shuffle(ProtocolConfig &conf, std::vector<Row> &input_share, PermShare &perm_share, bool save);

std::vector<Row> shuffle(ProtocolConfig &conf, Permutation perm, PermShare &perm_share, bool save);

std::vector<Row> unshuffle(ProtocolConfig &conf, std::vector<Row> &input_share, PermShare &perm_share);

PermShare get_merged_shuffle(ProtocolConfig &conf, PermShare &pi_share, PermShare &omega_share);

};