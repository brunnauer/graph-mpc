#pragma once

#include <omp.h>

#include "../utils/perm.h"
#include "../utils/protocol_config.h"
#include "../utils/types.h"
#include "io/comm.h"

struct PermShare {
    Permutation pi_0;
    Permutation pi_1;
    Permutation pi_0_p;
    Permutation pi_1_p;
    std::vector<Ring> B;
    std::vector<Ring> R;
};

namespace shuffle {

PermShare get_shuffle_compute(ProtocolConfig &c, std::vector<Ring> &shared_secret_D0, std::vector<Ring> &shared_secret_D1);

PermShare get_shuffle(ProtocolConfig &c);

std::vector<Ring> shuffle(ProtocolConfig &c, std::vector<Ring> &input_share, PermShare &perm_share, bool save);

Permutation shuffle(ProtocolConfig &c, Permutation &perm, PermShare &perm_share, bool save);

std::vector<Ring> get_unshuffle(ProtocolConfig &c, PermShare &perm_share);

std::vector<Ring> unshuffle(ProtocolConfig &c, PermShare &shuffle_share, std::vector<Ring> &B, std::vector<Ring> &input_share);

Permutation unshuffle(ProtocolConfig &c, PermShare &shuffle_share, std::vector<Ring> &B, Permutation &perm);

PermShare get_merged_shuffle(ProtocolConfig &c, PermShare &pi_share, PermShare &omega_share);

};  // namespace shuffle