#pragma once

#include <memory>
#include <vector>

#include "./utils/types.h"
#include "io/comm.h"
#include "perm.h"
#include "protocol_config.h"
#include "sharing.h"

namespace compaction {

void preprocess(ProtocolConfig &conf, std::vector<Row> &triple_a, std::vector<Row> &triple_b, std::vector<Row> &triple_c) {
    auto pid = conf.pid;
    auto rngs = conf.rngs;
    auto n_rows = conf.n_rows;
    auto network = conf.network;

    for (size_t i = 0; i < n_rows; ++i) {
        triple_a[i] = share::get_random_share(pid, rngs);
        triple_b[i] = share::get_random_share(pid, rngs);
        Row c = triple_a[i] * triple_b[i];
        triple_c[i] = share::get_random_share_secret(pid, rngs, network, c);
    }
    conf.network->sync();
}

Permutation evaluate(ProtocolConfig &conf, std::vector<Row> &triple_a, std::vector<Row> &triple_b, std::vector<Row> &triple_c, std::vector<Row> &input_share) {
    auto pid = conf.pid;
    auto n_rows = conf.n_rows;
    auto BLOCK_SIZE = conf.BLOCK_SIZE;
    auto network = conf.network;

    std::vector<Row> output(n_rows);
    std::vector<Row> vals;

    size_t idx_mult = 0;

    if (pid != D) {
        std::vector<Row> f_0;
        // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
        for (size_t i = 0; i < n_rows; ++i) {
            f_0.push_back(-input_share[i]);
            if (pid == P0) {
                f_0[i] += 1;  // 1 as constant only to one share
            }
        }

        std::vector<Row> s_0, s_1;
        Row s = 0;
        // Set s_0 to prefix sum of f_0 and s_1 to prefix sum of f_1/input continuing from the prior final value
        for (size_t i = 0; i < n_rows; ++i) {
            s += f_0[i];
            s_0.push_back(s);
        }
        for (size_t i = 0; i < n_rows; ++i) {
            s += input_share[i];
            s_1.push_back(s - s_0[i]);  // s_0[i] see below
        }

        // We now have to compute s_0 + input * (s_1 - s_0) (element-wise multiplication).
        // Previously, we already subtracted s_0 from s_1, so we just compute s_0 + input * s_1
        // s_0 is added after the communication though, here, we just multiply.

        for (size_t i = 0; i < n_rows; i++) {
            auto xa = triple_a[i] + input_share[i];
            auto yb = triple_b[i] + s_1[i];
            vals.push_back(xa);
            vals.push_back(yb);
        }
    }

    std::vector<Row> data_recv(n_rows);
    if (pid == P0) {
        send_vec(P1, network, vals.size(), vals, BLOCK_SIZE);
        recv_vec(P1, network, data_recv, BLOCK_SIZE);
    } else if (pid == P1) {
        recv_vec(P0, network, data_recv, BLOCK_SIZE);
        send_vec(P0, network, vals.size(), vals, BLOCK_SIZE);
    }

    for (size_t i = 0; i < vals.size(); ++i) vals[i] += data_recv[i];

    if (pid != D) {
        // We have to compute s_0 + input * (s_1 - s_0) (element-wise multiplication).
        // input * (s_1 - s_0) is already being computed.
        // Recompute s_0 to not have to save this somewhere from when it was computed before sending.
        std::vector<Row> f_0;
        // set f_0 to 1 - input
        for (size_t i = 0; i < n_rows; i++) {
            f_0.push_back(-input_share[i]);
            if (pid == P0) {
                f_0[i] += 1;  // 1 as constant only to one share
            }
        }

        std::vector<Row> s_0;
        Row s = 0;
        // Set s_0 to prefix sum of f_0
        for (size_t i = 0; i < n_rows; ++i) {
            s += f_0[i];
            s_0.push_back(s);
        }

        // Now, finalize the multiplications and add vector s_0.
        for (size_t i = 0; i < n_rows; ++i) {
            auto a = triple_a[i];
            auto b = triple_b[i];
            auto c = triple_c[i];

            auto xa = vals[2 * idx_mult];
            auto yb = vals[2 * idx_mult + 1];

            output[i] = s_0[i] + (xa * yb * (pid)) - (xa * b) - (yb * a) + c;
            if (pid == P0) output[i]--;
            idx_mult++;
        }
    }
    return Permutation(output);
}

Permutation get_compaction(ProtocolConfig &conf, std::vector<Row> &input_share) {
    std::vector<Row> triple_a, triple_b, triple_c;
    triple_a.resize(conf.n_rows);
    triple_b.resize(conf.n_rows);
    triple_c.resize(conf.n_rows);

    preprocess(conf, triple_a, triple_b, triple_c);
    return evaluate(conf, triple_a, triple_b, triple_c, input_share);
}

};  // namespace compaction