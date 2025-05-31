#include "sharing.h"

Ring share::random_share_3P(ProtocolConfig &c) {
    switch (c.pid) {
        case P0: {
            Ring share;
            c.rngs.rng_D0_comp().random_data(&share, sizeof(Ring));
            return share;
        }
        case P1: {
            Ring share;
            c.rngs.rng_D1_comp().random_data(&share, sizeof(Ring));
            return share;
        }
        case D: {
            Ring share_1;
            Ring share_2;
            c.rngs.rng_D0_comp().random_data(&share_1, sizeof(Ring));
            c.rngs.rng_D1_comp().random_data(&share_2, sizeof(Ring));
            return (share_1 + share_2);
        }
    }
}

Ring share::random_share_secret_3P(ProtocolConfig &c, std::vector<Ring> &vals_to_p1, size_t &idx, Ring &secret) {
    switch (c.pid) {
        case P0: {
            Ring share;
            c.rngs.rng_D0_comp().random_data(&share, sizeof(Ring));
            return share;
        }
        case P1: {
            Ring share = vals_to_p1[idx];
            ++idx;
            return share;
        }
        case D: {
            Ring share_0;
            c.rngs.rng_D0_comp().random_data(&share_0, sizeof(Ring));
            Ring share_1 = secret - share_0;
            vals_to_p1[idx] = share_1;
            idx++;
            return secret;
        }
    }
}

Ring share::random_share_secret_2P(ProtocolConfig &c, Ring &secret) {
    Ring share;
    switch (c.pid) {
        case P0: {
            c.rngs.rng_01().random_data(&share, sizeof(Ring));
            return secret - share;
        }
        case P1: {
            c.rngs.rng_01().random_data(&share, sizeof(Ring));
            return share;
        }
        default:
            return share;
    }
}

std::vector<Ring> share::random_share_secret_vec_2P(ProtocolConfig &c, std::vector<Ring> &secret_vec) {
    std::vector<Ring> share(secret_vec.size());
    switch (c.pid) {
        case P0: {
            for (size_t i = 0; i < share.size(); ++i) {
                c.rngs.rng_01().random_data(&share[i], sizeof(Ring));
                share[i] = secret_vec[i] - share[i];
            }
            break;
        }
        case P1: {
            for (size_t i = 0; i < share.size(); ++i) {
                c.rngs.rng_01().random_data(&share[i], sizeof(Ring));
            }
            break;
        }
        default:
            break;
    }
    return share;
}

Ring share::reveal(ProtocolConfig &c, Ring &share) {
    Ring share_other;
    if (c.pid == P0) {
        c.network->send(P1, &share, sizeof(Ring));
        c.network->recv(P1, &share_other, sizeof(Ring));
    } else if (c.pid == P1) {
        c.network->recv(P0, &share_other, sizeof(Ring));
        c.network->send(P0, &share, sizeof(Ring));
    }
    return share + share_other;
}

std::vector<Ring> share::reveal_vec(ProtocolConfig &c, std::vector<Ring> &share) {
    auto network = c.network;
    auto pid = c.pid;
    auto BLOCK_SIZE = c.BLOCK_SIZE;

    std::vector<Ring> result(share.size());

    switch (pid) {
        case P0: {
            std::vector<Ring> share_other(share.size());

            send_vec(P1, network, share.size(), share, BLOCK_SIZE);
            recv_vec(P1, network, share_other, BLOCK_SIZE);

#pragma omp parallel for if (result.size() > 10000)
            for (size_t i = 0; i < result.size(); ++i) {
                result[i] = share[i] + share_other[i];
            }
            return result;
        }
        case P1: {
            std::vector<Ring> share_other(share.size());

            recv_vec(P0, network, share_other, BLOCK_SIZE);
            send_vec(P0, network, share.size(), share, BLOCK_SIZE);

#pragma omp parallel for if (result.size() > 10000)
            for (size_t i = 0; i < result.size(); ++i) {
                result[i] = share[i] + share_other[i];
            }
            return result;
        }
        default:
            return result;
    }
}

Permutation share::reveal_perm(ProtocolConfig &c, Permutation &share) {
    auto perm_vec = share.get_perm_vec();
    std::vector<Ring> revealed_perm_vec = reveal_vec(c, perm_vec);
    return Permutation(revealed_perm_vec);
}

SecretSharedGraph share::random_share_graph(ProtocolConfig &c, Graph &g) {
    auto src_bits = g.src_bits();
    auto dst_bits = g.dst_bits();
    auto payload_bits = g.payload_bits();

    std::vector<std::vector<Ring>> src_bits_shared(sizeof(Ring) * 8);
    std::vector<std::vector<Ring>> dst_bits_shared(sizeof(Ring) * 8);
    std::vector<Ring> isV_shared(g.isV.size());
    std::vector<std::vector<Ring>> payload_bits_shared(sizeof(Ring) * 8);

    for (size_t i = 0; i < sizeof(Ring) * 8; ++i) {
        src_bits_shared[i].resize(g.size);
        dst_bits_shared[i].resize(g.size);
        payload_bits_shared[i].resize(g.size);
    }

    for (size_t i = 0; i < src_bits.size(); ++i) {
        src_bits_shared[i] = random_share_secret_vec_2P(c, src_bits[i]);
        dst_bits_shared[i] = random_share_secret_vec_2P(c, dst_bits[i]);
        payload_bits_shared[i] = random_share_secret_vec_2P(c, payload_bits[i]);
    }
    isV_shared = random_share_secret_vec_2P(c, g.isV);

    SecretSharedGraph shared_graph;
    shared_graph.src_bits = src_bits_shared;
    shared_graph.dst_bits = dst_bits_shared;
    shared_graph.isV_bits = isV_shared;
    shared_graph.payload_bits = payload_bits_shared;
    shared_graph.size = g.size;

    return shared_graph;
}

Graph share::reveal_graph(ProtocolConfig &c, SecretSharedGraph &g) {
    auto pid = c.pid;
    auto network = c.network;

    Graph g_new(g.size);

    if (pid == D) return g_new;

    Party partner = pid == P0 ? P1 : P0;
    size_t n_bits = sizeof(Ring) * 8;

    std::vector<std::vector<Ring>> src_bits(n_bits);
    std::vector<std::vector<Ring>> dst_bits(n_bits);
    std::vector<std::vector<Ring>> payload_bits(n_bits);

    std::vector<Ring> src(g.size);
    std::vector<Ring> dst(g.size);
    std::vector<Ring> isV(g.size);
    std::vector<Ring> payload(g.size);

    for (size_t i = 0; i < n_bits; ++i) {
        src_bits[i] = share::reveal_vec(c, g.src_bits[i]);
    }

    for (size_t i = 0; i < n_bits; ++i) {
        dst_bits[i] = share::reveal_vec(c, g.dst_bits[i]);
    }

    for (size_t i = 0; i < n_bits; ++i) {
        payload_bits[i] = share::reveal_vec(c, g.payload_bits[i]);
    }

    isV = share::reveal_vec(c, g.isV_bits);

    for (size_t i = 0; i < g.size; ++i) {
        src[i] = 0;
        for (size_t j = 0; j < n_bits; j++) {
            src[i] += src_bits[j][i] << j;
        }
    }

    for (size_t i = 0; i < g.size; ++i) {
        dst[i] = 0;
        for (size_t j = 0; j < n_bits; j++) {
            dst[i] += dst_bits[j][i] << j;
        }
    }

    for (size_t i = 0; i < g.size; ++i) {
        payload[i] = 0;
        for (size_t j = 0; j < n_bits; j++) {
            payload[i] += payload_bits[j][i] << j;
        }
    }

    g_new.src = src;
    g_new.dst = dst;
    g_new.isV = isV;
    g_new.payload = payload;

    return g_new;
}