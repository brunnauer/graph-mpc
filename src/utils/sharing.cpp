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

std::vector<Ring> share::random_share_secret_vec_2P(Party id, RandomGenerators &rngs, std::vector<Ring> &secret_vec) {
    std::vector<Ring> share(secret_vec.size());
    switch (id) {
        case P0: {
            for (size_t i = 0; i < share.size(); ++i) {
                rngs.rng_01().random_data(&share[i], sizeof(Ring));
                share[i] = secret_vec[i] - share[i];
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

SecretSharedGraph share::random_share_graph(Party id, RandomGenerators &rngs, size_t n_bits, Graph &g) {
    auto src_bits = to_bits(g.src, n_bits);
    auto dst_bits = to_bits(g.dst, n_bits);
    auto payload_bits = to_bits(g.payload, n_bits);

    std::vector<std::vector<Ring>> src_bits_shared(n_bits);
    std::vector<std::vector<Ring>> dst_bits_shared(n_bits);
    std::vector<Ring> isV_shared(g.isV.size());
    std::vector<std::vector<Ring>> payload_bits_shared(n_bits);

    for (size_t i = 0; i < n_bits; ++i) {
        src_bits_shared[i].resize(g.size);
        dst_bits_shared[i].resize(g.size);
        payload_bits_shared[i].resize(g.size);
    }

    for (size_t i = 0; i < n_bits; ++i) {
        src_bits_shared[i] = random_share_secret_vec_2P(id, rngs, src_bits[i]);
        dst_bits_shared[i] = random_share_secret_vec_2P(id, rngs, dst_bits[i]);
        payload_bits_shared[i] = random_share_secret_vec_2P(id, rngs, payload_bits[i]);
    }
    isV_shared = random_share_secret_vec_2P(id, rngs, g.isV);

    SecretSharedGraph shared_graph(src_bits_shared, dst_bits_shared, isV_shared, payload_bits_shared);
    shared_graph.src = share::random_share_secret_vec_2P(id, rngs, g.src);
    shared_graph.dst = share::random_share_secret_vec_2P(id, rngs, g.dst);
    shared_graph.isV = share::random_share_secret_vec_2P(id, rngs, g.isV);
    shared_graph.payload = share::random_share_secret_vec_2P(id, rngs, g.payload);
    return shared_graph;
}

std::vector<Ring> share::reveal_vec(Party id, std::shared_ptr<NetworkInterface> network, std::vector<Ring> &share) {
    std::vector<Ring> result(share.size());

    switch (id) {
        case P0: {
            std::vector<Ring> share_other(share.size());

            network->send_now(P1, share);
            share_other = network->recv(P1, share.size());

            for (size_t i = 0; i < result.size(); ++i) {
                result[i] = share[i] + share_other[i];
            }
            return result;
        }
        case P1: {
            std::vector<Ring> share_other(share.size());

            share_other = network->recv(P0, share.size());
            network->send_now(P0, share);

            for (size_t i = 0; i < result.size(); ++i) {
                result[i] = share[i] + share_other[i];
            }
            return result;
        }
        default:
            return result;
    }
}

std::vector<Ring> share::reveal_vec_bin(Party id, std::shared_ptr<NetworkInterface> network, std::vector<Ring> &share) {
    std::vector<Ring> result(share.size());
    std::vector<Ring> share_other(share.size());

    switch (id) {
        case P0: {
            network->send_now(P1, share);
            share_other = network->recv(P1, share.size());
            break;
        }
        case P1: {
            share_other = network->recv(P0, share.size());
            network->send_now(P0, share);
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

Permutation share::reveal_perm(Party id, std::shared_ptr<NetworkInterface> network, Permutation &share) {
    auto perm_vec = share.get_perm_vec();
    std::vector<Ring> revealed_perm_vec = reveal_vec(id, network, perm_vec);
    return Permutation(revealed_perm_vec);
}

Graph share::reveal_graph(Party id, std::shared_ptr<NetworkInterface> network, size_t n_bits, SecretSharedGraph &g) {
    Graph g_new(g.size);

    if (id == D) return g_new;

    std::vector<std::vector<Ring>> src_bits(n_bits);
    std::vector<std::vector<Ring>> dst_bits(n_bits);
    std::vector<std::vector<Ring>> payload_bits(n_bits);

    std::vector<Ring> src(g.size);
    std::vector<Ring> dst(g.size);
    std::vector<Ring> isV(g.size);
    std::vector<Ring> payload(g.size);

    for (size_t i = 0; i < n_bits; ++i) {
        src_bits[i] = share::reveal_vec(id, network, g.src_bits[i]);
    }

    for (size_t i = 0; i < n_bits; ++i) {
        dst_bits[i] = share::reveal_vec(id, network, g.dst_bits[i]);
    }

    for (size_t i = 0; i < n_bits; ++i) {
        payload_bits[i] = share::reveal_vec(id, network, g.payload_bits[i]);
    }

    isV = share::reveal_vec(id, network, g.isV_bits);

    g_new.src = from_bits(src_bits, g.size);
    g_new.dst = from_bits(dst_bits, g.size);
    g_new.isV = isV;
    g_new.payload = from_bits(payload_bits, g.size);

    return g_new;
}