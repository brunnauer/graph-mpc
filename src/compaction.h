#pragma once

#include <memory>
#include <vector>

#include "./utils/types.h"
#include "io/comm.h"
#include "io/netmp.h"
#include "sharing.h"

class Compaction {
   public:
    Compaction(Party id, size_t n_rows, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network) : id(id), n_rows(n_rows), rngs(rngs), network(network) {};

    void preprocess() {
        for (size_t i = 0; i < n_rows; ++i) {
            triple_a[i] = Share::get_random_share(id, rngs);
            triple_b[i] = Share::get_random_share(id, rngs);
            Row c = triple_a[i] * triple_b[i];
            triple_c[i] = Share::get_random_share_secret(id, rngs, shared_secret_vec, idx_shared_secret_vec, c);
        }
    }

    void evaluate() {
        std::vector<Row> vals(n_rows);

        evaluate_send(vals);

        Party partner = id == P0 ? P1 : P0;
        if (id == P0) {
            send_vec(partner, network, vals.size(), vals, BLOCK_SIZE);
            recv_vec(partner, network, vals, BLOCK_SIZE);
        } else {
            std::vector<Row> data_recv(n_rows);
            recv_vec(partner, network, data_recv, BLOCK_SIZE);
            send_vec(partner, network, vals.size(), vals, BLOCK_SIZE);
            std::copy(data_recv.begin(), data_recv.end(), vals.begin());
        }

        evaluate_recv(vals);
    }

    void evaluate_send(std::vector<Row> &vals) {
        if (id != D) {
            std::vector<Row> f_0;
            // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
            for (size_t i = 0; i < n_rows; ++i) {
                f_0.push_back(-wire[i]);
                if (id == P0) {
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
                s += wire[i];
                s_1.push_back(s - s_0[i]);  // s_0[i] see below
            }

            // We now have to compute s_0 + input * (s_1 - s_0) (element-wise multiplication).
            // Previously, we already subtracted s_0 from s_1, so we just compute s_0 + input * s_1
            // s_0 is added after the communication though, here, we just multiply.

            for (size_t i = 0; i < n_rows; i++) {
                auto xa = triple_a[i] + wire[i];
                auto yb = triple_b[i] + s_1[i];
                vals.push_back(xa);
                vals.push_back(yb);
            }
        }
    }

    void evaluate_recv(std::vector<Row> &vals) {
        if (id != D) {
            // We have to compute s_0 + input * (s_1 - s_0) (element-wise multiplication).
            // input * (s_1 - s_0) is already being computed.
            // Recompute s_0 to not have to save this somewhere from when it was computed before sending.
            std::vector<Row> f_0;
            // set f_0 to 1 - input
            for (size_t i = 0; i < n_rows; i++) {
                f_0.push_back(-wire[i]);
                if (id == P0) {
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

                // TODO: check if this is correct
                wire[i] = s_0[i] + (vals[2 * idx_mult] * vals[2 * idx_mult + 1] * (id)) - (vals[2 * idx_mult] * b) - (vals[2 * idx_mult + 1] * a) + c;
                idx_mult++;
            }
        }
    }

   private:
    Party id;
    size_t n_rows;
    size_t idx_shared_secret_vec = 0;
    size_t idx_mult = 0;
    const size_t BLOCK_SIZE = 100000;
    RandomGenerators rngs;
    std::shared_ptr<io::NetIOMP> network;
    std::vector<Row> wire;
    std::vector<Row> triple_a, triple_b, triple_c;
    std::vector<Row> shared_secret_vec;
};