#include "sharing.h"

Ring share::random_share_secret_2P(Party id, RandomGenerators &rngs, Ring &secret) {
    Ring share;
    switch (id) {
        case P0: {
            rngs.rng_01().random_data(&share, sizeof(Ring));
            return secret - share;
        }
        case P1: {
            rngs.rng_01().random_data(&share, sizeof(Ring));
            return share;
        }
        default:
            return share;
    }
}

Ring share::random_share_secret_2P_bin(Party id, RandomGenerators &rngs, Ring &secret) {
    Ring share;
    switch (id) {
        case P0: {
            rngs.rng_01().random_data(&share, sizeof(Ring));
            return secret ^ share;
        }
        case P1: {
            rngs.rng_01().random_data(&share, sizeof(Ring));
            return share;
        }
        default:
            return share;
    }
}

std::vector<Ring> share::random_share_secret_vec_2P(Party id, RandomGenerators &rngs, std::vector<Ring> &secret_vec, Party sender) {
    std::vector<Ring> share(secret_vec.size());
    if (id == D)
        return share;
    else if (id == sender) {
        for (size_t i = 0; i < share.size(); ++i) {
            rngs.rng_01().random_data(&share[i], sizeof(Ring));
            share[i] = secret_vec[i] - share[i];
        }
    } else {
        for (size_t i = 0; i < share.size(); ++i) {
            rngs.rng_01().random_data(&share[i], sizeof(Ring));
        }
    }
    return share;
}

Permutation share::random_share_secret_perm_2P(Party id, RandomGenerators &rngs, Permutation &secret_perm) {
    std::vector<Ring> share(secret_perm.size());
    switch (id) {
        case P0: {
            for (size_t i = 0; i < share.size(); ++i) {
                rngs.rng_01().random_data(&share[i], sizeof(Ring));
                share[i] = secret_perm[i] - share[i];
            }
            break;
        }
        case P1: {
            for (size_t i = 0; i < share.size(); ++i) {
                rngs.rng_01().random_data(&share[i], sizeof(Ring));
            }
            break;
        }
        default:
            break;
    }
    return Permutation(share);
}

std::vector<Ring> share::random_share_secret_vec_2P_bin(Party id, RandomGenerators &rngs, std::vector<Ring> &secret_vec) {
    std::vector<Ring> share(secret_vec.size());
    switch (id) {
        case P0: {
            for (size_t i = 0; i < share.size(); ++i) {
                rngs.rng_01().random_data(&share[i], sizeof(Ring));
                share[i] = secret_vec[i] ^ share[i];
            }
            break;
        }
        case P1: {
            for (size_t i = 0; i < share.size(); ++i) {
                rngs.rng_01().random_data(&share[i], sizeof(Ring));
            }
            break;
        }
        default:
            break;
    }
    return share;
}

Ring share::random_share_3P(Party id, RandomGenerators &rngs) {
    switch (id) {
        case P0: {
            Ring share;
            rngs.rng_D0_comp().random_data(&share, sizeof(Ring));
            return share;
        }
        case P1: {
            Ring share;
            rngs.rng_D1_comp().random_data(&share, sizeof(Ring));
            return share;
        }
        case D: {
            Ring share_1;
            Ring share_2;
            rngs.rng_D0_comp().random_data(&share_1, sizeof(Ring));
            rngs.rng_D1_comp().random_data(&share_2, sizeof(Ring));
            return (share_1 + share_2);
        }
    }
}

Ring share::random_share_3P_bin(Party id, RandomGenerators &rngs) {
    switch (id) {
        case P0: {
            Ring share;
            rngs.rng_D0_comp().random_data(&share, sizeof(Ring));
            return share;
        }
        case P1: {
            Ring share;
            rngs.rng_D1_comp().random_data(&share, sizeof(Ring));
            return share;
        }
        case D: {
            Ring share_1;
            Ring share_2;
            rngs.rng_D0_comp().random_data(&share_1, sizeof(Ring));
            rngs.rng_D1_comp().random_data(&share_2, sizeof(Ring));
            return (share_1 ^ share_2);
        }
    }
}

Ring share::random_share_secret_3P(Party id, RandomGenerators &rngs, std::vector<Ring> &shares, size_t &idx, Ring &secret, Party recv) {
    if (id == D) {
        Ring share_0;
        if (recv == P1) rngs.rng_D0_comp().random_data(&share_0, sizeof(Ring));
        if (recv == P0) rngs.rng_D1_comp().random_data(&share_0, sizeof(Ring));
        Ring share_1 = secret - share_0;
        shares.push_back(share_1);
        return secret;
    } else if (id == recv) {
        Ring share = shares[idx];
        idx++;
        return share;
    } else {
        Ring share;
        if (id == P0) rngs.rng_D0_comp().random_data(&share, sizeof(Ring));
        if (id == P1) rngs.rng_D1_comp().random_data(&share, sizeof(Ring));
        return share;
    }
}

Ring share::random_share_secret_3P(Party id, RandomGenerators &rngs, std::vector<Ring> &shares_P1, size_t &idx, Ring &secret) {
    switch (id) {
        case P0: {
            Ring share;
            rngs.rng_D0_comp().random_data(&share, sizeof(Ring));
            return share;
        }
        case P1: {
            Ring share = shares_P1[idx];
            idx++;
            return share;
        }
        case D: {
            Ring share_0;
            rngs.rng_D0_comp().random_data(&share_0, sizeof(Ring));
            Ring share_1 = secret - share_0;
            shares_P1.push_back(share_1);
            return secret;
        }
    }
}

Ring share::random_share_secret_3P_bin(Party id, RandomGenerators &rngs, std::vector<Ring> &vals_to_p1, size_t &idx, Ring &secret) {
    switch (id) {
        case P0: {
            Ring share;
            rngs.rng_D0_comp().random_data(&share, sizeof(Ring));
            return share;
        }
        case P1: {
            Ring share = vals_to_p1[idx];
            idx++;
            return share;
        }
        case D: {
            Ring share_0;
            rngs.rng_D0_comp().random_data(&share_0, sizeof(Ring));
            Ring share_1 = secret ^ share_0;
            vals_to_p1.push_back(share_1);
            idx++;
            return secret;
        }
    }
}

std::vector<Ring> share::reveal_vec(Party id, std::shared_ptr<io::NetIOMP> network, std::vector<Ring> &share) {
    std::vector<Ring> result(share.size());

    switch (id) {
        case P0: {
            std::vector<Ring> share_other(share.size());

            network->send_vec(P1, share.size(), share);
            network->recv_vec(P1, share.size(), share_other);

            for (size_t i = 0; i < result.size(); ++i) {
                result[i] = share[i] + share_other[i];
            }
            return result;
        }
        case P1: {
            std::vector<Ring> share_other(share.size());

            network->recv_vec(P0, share.size(), share_other);
            network->send_vec(P0, share.size(), share);

            for (size_t i = 0; i < result.size(); ++i) {
                result[i] = share[i] + share_other[i];
            }
            return result;
        }
        default:
            return result;
    }
}

std::vector<Ring> share::reveal_vec_bin(Party id, std::shared_ptr<io::NetIOMP> network, std::vector<Ring> &share) {
    std::vector<Ring> result(share.size());
    std::vector<Ring> share_other(share.size());

    switch (id) {
        case P0: {
            network->send_vec(P1, share.size(), share);
            network->recv_vec(P1, share.size(), share_other);
            break;
        }
        case P1: {
            network->recv_vec(P0, share.size(), share_other);
            network->send_vec(P0, share.size(), share);
            break;
        }
        default:
            return result;
    }

    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = share[i] ^ share_other[i];
    }
    return result;
}

Permutation share::reveal_perm(Party id, std::shared_ptr<io::NetIOMP> network, Permutation &share) {
    auto perm_vec = share.get_perm_vec();
    std::vector<Ring> revealed_perm_vec = reveal_vec(id, network, perm_vec);
    return Permutation(revealed_perm_vec);
}
