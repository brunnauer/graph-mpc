#pragma once

#include <omp.h>

#include "../io/netmp.h"
#include "bits.h"
#include "permutation.h"

namespace share {

std::vector<Ring> random_share_secret_vec_2P(Party id, RandomGenerators &rngs, std::vector<Ring> &secret_vec, Party sender = P0);

std::vector<Ring> random_share_secret_vec_2P_bin(Party id, RandomGenerators &rngs, std::vector<Ring> &secret_vec);

std::vector<Ring> random_share_vec_3P(Party id, RandomGenerators &rngs, size_t n, bool binary = false);

std::vector<Ring> random_share_secret_vec_3P(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, std::vector<Ring> &secret,
                                             Party &recv, bool binary = false);

std::vector<Ring> reveal_vec(Party id, std::shared_ptr<io::NetIOMP> network, std::vector<Ring> &share);

std::vector<Ring> reveal_vec_bin(Party id, std::shared_ptr<io::NetIOMP> network, std::vector<Ring> &share);

Permutation reveal_perm(Party id, std::shared_ptr<io::NetIOMP> network, Permutation &share);

};  // namespace share