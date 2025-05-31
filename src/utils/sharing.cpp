#include "sharing.h"

Row share::random_share_3P(Party pid, RandomGenerators &rngs) {
    switch (pid) {
        case P0: {
            Row share;
            rngs.rng_D0().random_data(&share, sizeof(Row));
            return share;
        }
        case P1: {
            Row share;
            rngs.rng_D1().random_data(&share, sizeof(Row));
            return share;
        }
        case D: {
            Row share_1;
            Row share_2;
            rngs.rng_D0().random_data(&share_1, sizeof(Row));
            rngs.rng_D1().random_data(&share_2, sizeof(Row));
            return (share_1 + share_2);
        }
    }
}

Row share::random_share_secret_3P(Party pid, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, Row &secret) {
    switch (pid) {
        case P0: {
            Row share;
            rngs.rng_D0().random_data(&share, sizeof(Row));
            return share;
        }
        case P1: {
            Row share;
            network->recv(2, &share, sizeof(Row));
            return share;
        }
        case D: {
            Row share_0;
            rngs.rng_D0().random_data(&share_0, sizeof(Row));
            Row share_1 = secret - share_0;
            network->send(1, &share_1, sizeof(Row));
            return secret;
        }
    }
}

Row share::random_share_secret_2P(Party pid, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, Row &secret) {
    Row share;
    switch (pid) {
        case P0: {
            rngs.rng_01().random_data(&share, sizeof(Row));
            return secret - share;
        }
        case P1: {
            rngs.rng_01().random_data(&share, sizeof(Row));
            return share;
        }
        default:
            return share;
    }
}

std::vector<Row> share::random_share_secret_vec_2P(Party pid, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, std::vector<Row> &secret_vec) {
    std::vector<Row> share(secret_vec.size());
    switch (pid) {
        case P0: {
            for (size_t i = 0; i < share.size(); ++i) {
                rngs.rng_01().random_data(&share[i], sizeof(Row));
                share[i] = secret_vec[i] - share[i];
            }
            break;
        }
        case P1: {
            for (size_t i = 0; i < share.size(); ++i) {
                rngs.rng_01().random_data(&share[i], sizeof(Row));
            }
            break;
        }
        default:
            break;
    }
    return share;
}

Row share::reveal(Party partner, std::shared_ptr<io::NetIOMP> network, Row &share) {
    Row share_other;
    if (partner == P0) {
        network->send(partner, &share, sizeof(Row));
        network->recv(partner, &share_other, sizeof(Row));
    } else {
        network->recv(partner, &share_other, sizeof(Row));
        network->send(partner, &share, sizeof(Row));
    }
    return share + share_other;
}

std::vector<Row> share::reveal_vec(ProtocolConfig &c, std::vector<Row> &share) {
    auto network = c.network;
    auto pid = c.pid;
    auto BLOCK_SIZE = c.BLOCK_SIZE;

    std::vector<Row> result(share.size());

    switch (pid) {
        case P0: {
            std::vector<Row> share_other(share.size());

            send_vec(P1, network, share.size(), share, BLOCK_SIZE);
            recv_vec(P1, network, share_other, BLOCK_SIZE);

            for (size_t i = 0; i < result.size(); ++i) {
                result[i] = share[i] + share_other[i];
            }
            return result;
        }
        case P1: {
            std::vector<Row> share_other(share.size());

            recv_vec(P0, network, share_other, BLOCK_SIZE);
            send_vec(P0, network, share.size(), share, BLOCK_SIZE);

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
    std::vector<Row> revealed_perm_vec = reveal_vec(c, perm_vec);
    return Permutation(revealed_perm_vec);
}

SecretSharedGraph share::random_share_graph(ProtocolConfig &c, Graph &graph) {
    auto pid = c.pid;
    auto rngs = c.rngs;
    auto network = c.network;

    auto src = graph.src;
    auto dst = graph.dst;
    auto isV = graph.isV;
    auto payload = graph.payload;

    auto src_bits = graph.src_bits();
    auto dst_bits = graph.dst_bits();
    auto payload_bits = graph.payload_bits();

    std::vector<std::vector<Row>> src_bits_shared(sizeof(Row) * 8);
    std::vector<std::vector<Row>> dst_bits_shared(sizeof(Row) * 8);
    std::vector<Row> isV_shared(isV.size());
    std::vector<std::vector<Row>> payload_bits_shared(sizeof(Row) * 8);

    for (size_t i = 0; i < sizeof(Row) * 8; ++i) {
        src_bits_shared[i].resize(graph.size);
        dst_bits_shared[i].resize(graph.size);
        payload_bits_shared[i].resize(graph.size);
    }

    for (size_t i = 0; i < src_bits.size(); ++i) {
        src_bits_shared[i] = random_share_secret_vec_2P(pid, rngs, network, src_bits[i]);
        dst_bits_shared[i] = random_share_secret_vec_2P(pid, rngs, network, dst_bits[i]);
        payload_bits_shared[i] = random_share_secret_vec_2P(pid, rngs, network, payload_bits[i]);
    }
    isV_shared = random_share_secret_vec_2P(pid, rngs, network, isV);

    SecretSharedGraph shared_graph;
    shared_graph.src_bits = src_bits_shared;
    shared_graph.dst_bits = dst_bits_shared;
    shared_graph.isV_bits = isV_shared;
    shared_graph.payload_bits = payload_bits_shared;
    shared_graph.size = graph.size;

    c.rngs = rngs;
    return shared_graph;
}

Graph share::reveal_graph(ProtocolConfig &c, SecretSharedGraph &g) {
    auto pid = c.pid;
    auto network = c.network;

    Graph g_new(g.size);

    if (pid == D) return g_new;

    Party partner = pid == P0 ? P1 : P0;
    size_t n_bits = sizeof(Row) * 8;

    std::vector<std::vector<Row>> src_bits(n_bits);
    std::vector<std::vector<Row>> dst_bits(n_bits);
    std::vector<std::vector<Row>> payload_bits(n_bits);

    std::vector<Row> src(g.size);
    std::vector<Row> dst(g.size);
    std::vector<Row> isV(g.size);
    std::vector<Row> payload(g.size);

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