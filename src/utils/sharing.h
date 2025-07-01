#pragma once

#include <omp.h>

#include "../io/comm.h"
#include "../setup/utils.h"
#include "bits.h"
#include "graph.h"
#include "perm.h"
#include "random_generators.h"
#include "types.h"

namespace share {
Ring random_share_secret_2P(Party id, RandomGenerators &rngs, Ring &secret);

Ring random_share_secret_2P_bin(Party id, RandomGenerators &rngs, Ring &secret);

std::vector<Ring> random_share_secret_vec_2P(Party id, RandomGenerators &rngs, std::vector<Ring> &secret_vec);

std::vector<Ring> random_share_secret_vec_2P_bin(Party id, RandomGenerators &rngs, std::vector<Ring> &secret_vec);

Ring random_share_3P(Party id, RandomGenerators &rngs);

Ring random_share_3P_bin(Party id, RandomGenerators &rngs);

Ring random_share_secret_3P(Party id, RandomGenerators &rngs, std::vector<Ring> &shares_P1, size_t &idx, Ring &secret);

Ring random_share_secret_3P_bin(Party id, RandomGenerators &rngs, std::vector<Ring> &vals_to_p1, size_t &idx, Ring &secret);

SecretSharedGraph random_share_graph(Party id, RandomGenerators &rngs, size_t n_bits, Graph &g);

std::vector<Ring> reveal_vec(Party id, std::shared_ptr<NetworkInterface> network, std::vector<Ring> &share);

std::vector<Ring> reveal_vec_bin(Party id, std::shared_ptr<NetworkInterface> network, std::vector<Ring> &share);

Permutation reveal_perm(Party id, std::shared_ptr<NetworkInterface> network, Permutation &share);

Graph reveal_graph(Party id, std::shared_ptr<NetworkInterface> network, size_t n_bits, SecretSharedGraph &g);

};  // namespace share