#include "sharing.h"

Row share::get_random_share(Party pid, RandomGenerators &rngs) {
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

Row share::get_random_share_secret(Party pid, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, Row &secret) {
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

void share::random_share_secret_send(Party dst_pid, RandomGenerators &rngs, io::NetIOMP &network, Row &share, Row &secret) {
    Row val_1;
    Row val_2;

    rngs.rng_self().random_data(&val_1, sizeof(Row));
    share = val_1;
    val_2 = secret - val_1;
    network.send(dst_pid, &val_2, sizeof(Row));
}

void share::random_share_secret_recv(Party src_pid, io::NetIOMP &network, Row &share) { network.recv(src_pid, &share, sizeof(Row)); }

void share::random_share_secret_vec_send(Party dst_pid, RandomGenerators &rngs, io::NetIOMP &network, std::vector<Row> &share_vec,
                                         std::vector<Row> &secret_vec) {
    for (size_t i = 0; i < secret_vec.size(); ++i) {
        random_share_secret_send(dst_pid, rngs, network, share_vec[i], secret_vec[i]);
    }
}

void share::random_share_secret_vec_recv(Party src_pid, io::NetIOMP &network, std::vector<Row> &share_vec) {
    for (size_t i = 0; i < share_vec.size(); ++i) {
        random_share_secret_recv(src_pid, network, share_vec[i]);
    }
}

SecretSharedGraph share::random_share_graph(ProtocolConfig &conf, Graph &graph) {
    auto pid = conf.pid;
    auto rngs = conf.rngs;
    auto network = conf.network;
    auto src = graph.src;
    auto dst = graph.dst;
    auto isV = graph.isV;
    auto payload = graph.payload;

    std::vector<std::vector<Row>> src_bits(sizeof(Row) * 8);
    std::vector<std::vector<Row>> dst_bits(sizeof(Row) * 8);
    std::vector<std::vector<Row>> payload_bits(sizeof(Row) * 8);

    std::vector<std::vector<Row>> src_bits_shared(sizeof(Row) * 8);
    std::vector<std::vector<Row>> dst_bits_shared(sizeof(Row) * 8);
    std::vector<Row> isV_shared(isV.size());
    std::vector<std::vector<Row>> payload_bits_shared(sizeof(Row) * 8);

    for (size_t i = 0; i < sizeof(Row) * 8; ++i) {
        src_bits[i].resize(graph.size);
        src_bits_shared[i].resize(graph.size);
        for (size_t j = 0; j < graph.size; ++j) {
            src_bits[i][j] = (src[j] & (1 << i)) >> i;
        }
    }

    for (size_t i = 0; i < sizeof(Row) * 8; ++i) {
        dst_bits[i].resize(graph.size);
        dst_bits_shared[i].resize(graph.size);
        for (size_t j = 0; j < graph.size; ++j) {
            dst_bits[i][j] = (dst[j] & (1 << i)) >> i;
        }
    }

    for (size_t i = 0; i < sizeof(Row) * 8; ++i) {
        payload_bits[i].resize(graph.size);
        payload_bits_shared[i].resize(graph.size);
        for (size_t j = 0; j < graph.size; ++j) {
            payload_bits[i][j] = (payload[j] & (1 << i)) >> i;
        }
    }

    if (pid == 0) {
        for (size_t i = 0; i < src_bits.size(); ++i) {
            random_share_secret_vec_send(P1, rngs, *network, src_bits_shared[i], src_bits[i]);
            random_share_secret_vec_send(P1, rngs, *network, dst_bits_shared[i], dst_bits[i]);
            random_share_secret_vec_send(P1, rngs, *network, payload_bits_shared[i], payload_bits[i]);
        }
        random_share_secret_vec_send(P1, rngs, *network, isV_shared, isV);
    } else if (pid == 1) {
        for (size_t i = 0; i < src_bits.size(); ++i) {
            random_share_secret_vec_recv(P0, *network, src_bits_shared[i]);
            random_share_secret_vec_recv(P0, *network, dst_bits_shared[i]);
            random_share_secret_vec_recv(P0, *network, payload_bits_shared[i]);
        }
        random_share_secret_vec_recv(P0, *network, isV_shared);
    }

    SecretSharedGraph shared_graph;
    shared_graph.src_bits = src_bits_shared;
    shared_graph.dst_bits = dst_bits_shared;
    shared_graph.isV_bits = isV_shared;
    shared_graph.payload_bits = payload_bits_shared;
    shared_graph.size = graph.size;

    conf.rngs = rngs;
    return shared_graph;
}

Graph share::reconstruct_shared_graph(ProtocolConfig &conf, SecretSharedGraph &g) {
    auto pid = conf.pid;
    auto network = conf.network;

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
        src_bits[i] = share::reconstruct_vec(partner, network, g.src_bits[i]);
    }
    for (size_t i = 0; i < n_bits; ++i) {
        dst_bits[i] = share::reconstruct_vec(partner, network, g.dst_bits[i]);
    }
    for (size_t i = 0; i < n_bits; ++i) {
        payload_bits[i] = share::reconstruct_vec(partner, network, g.payload_bits[i]);
    }
    isV = share::reconstruct_vec(partner, network, g.isV_bits);

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

    Graph g_new(g.size);
    g_new.src = src;
    g_new.dst = dst;
    g_new.isV = isV;
    g_new.payload = payload;

    return g_new;
}

Row share::reconstruct(Party partner, std::shared_ptr<io::NetIOMP> network, Row &share) {
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

std::vector<Row> share::reconstruct_vec(Party partner, std::shared_ptr<io::NetIOMP> network, std::vector<Row> &share_vec) {
    std::vector<Row> res(share_vec.size());
    for (size_t i = 0; i < res.size(); ++i) {
        res[i] = reconstruct(partner, network, share_vec[i]);
    }
    return res;
}

std::vector<Row> share::reveal(ProtocolConfig &conf, std::vector<Row> &share) {
    auto n_rows = conf.n_rows;
    auto network = conf.network;
    auto pid = conf.pid;

    std::vector<Row> outvals(n_rows);

    /* Don't throw error, as Dealer also has to execute this */
    if (share.size() != n_rows) {
        return outvals;
    }

    if (pid != D) {
        std::vector<Row> output_share_self(share.size());
        std::vector<Row> output_share_other(share.size());

        std::copy(share.begin(), share.end(), output_share_self.begin());

        size_t partner = (pid == P0) ? 1 : 0;
        network->send(partner, output_share_self.data(), output_share_self.size() * sizeof(Row));
        network->recv(partner, output_share_other.data(), output_share_other.size() * sizeof(Row));

        for (size_t i = 0; i < n_rows; ++i) {
            Row outmask = output_share_other[i];
            outvals[i] = output_share_self[i] + outmask;
        }
        return outvals;
    } else {
        return outvals;
    }
}

Permutation share::reveal(ProtocolConfig &conf, Permutation &share) {
    auto n_rows = conf.n_rows;
    auto network = conf.network;
    auto pid = conf.pid;

    std::vector<Row> outvals(n_rows);

    /* Don't throw error, as Dealer also has to execute this */
    if (share.size() != n_rows) {
        return outvals;
    }

    if (pid != D) {
        std::vector<Row> output_share_self(share.size());
        std::vector<Row> output_share_other(share.size());

        auto perm_vec = share.get_perm_vec();
        std::copy(perm_vec.begin(), perm_vec.end(), output_share_self.begin());

        if (pid == P0) {
            network->send(P1, output_share_self.data(), output_share_self.size() * sizeof(Row));
            network->recv(P1, output_share_other.data(), output_share_other.size() * sizeof(Row));
        } else {
            network->recv(P0, output_share_other.data(), output_share_other.size() * sizeof(Row));
            network->send(P0, output_share_self.data(), output_share_self.size() * sizeof(Row));
        }

        for (size_t i = 0; i < n_rows; ++i) {
            Row outmask = output_share_other[i];
            outvals[i] = output_share_self[i] + outmask;
        }
        return Permutation(outvals);
    } else {
        return Permutation(outvals);
    }
}