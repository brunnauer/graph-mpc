#pragma once

#include <omp.h>

#include "../io/comm.h"
#include "../setup/utils.h"
#include "graph.h"
#include "perm.h"
#include "protocol_config.h"
#include "types.h"

namespace share {

Ring random_share_3P(ProtocolConfig &c);

Ring random_share_secret_3P(ProtocolConfig &c, std::vector<Ring> &vals_to_p1, size_t &idx, Ring &secret);

Ring random_share_secret_2P(ProtocolConfig &c, Ring &secret);

std::vector<Ring> random_share_secret_vec_2P(ProtocolConfig &c, std::vector<Ring> &secret_vec);

Ring reveal(ProtocolConfig &c, Ring &share);

std::vector<Ring> reveal_vec(ProtocolConfig &conf, std::vector<Ring> &share);

Permutation reveal_perm(ProtocolConfig &conf, Permutation &share);

SecretSharedGraph random_share_graph(ProtocolConfig &conf, Graph &graph);

Graph reveal_graph(ProtocolConfig &conf, SecretSharedGraph &shared_graph);
};  // namespace share