#pragma once

#include <memory>
#include <vector>

#include "./utils/types.h"
#include "io/comm.h"
#include "io/netmp.h"
#include "sharing.h"

class Compaction {
   public:
    Compaction(Party pid, size_t n_rows, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network)
        : pid(pid), n_rows(n_rows), rngs(rngs), network(network) {
        triple_a.resize(n_rows);
        triple_b.resize(n_rows);
        triple_c.resize(n_rows);
    };

    void set_input(std::vector<Row> &input) { wire = input; }

    std::vector<Row> get_compaction() {
        preprocess();
        evaluate();
        return wire;
    }

    void preprocess() {
        for (size_t i = 0; i < n_rows; ++i) {
            triple_a[i] = Share::get_random_share(pid, rngs);
            triple_b[i] = Share::get_random_share(pid, rngs);
            Row c = triple_a[i] * triple_b[i];
            triple_c[i] = Share::get_random_share_secret(pid, rngs, network, c);
        }
        network->sync();
    }

    void evaluate() {
        std::vector<Row> vals;

        if (pid != D) {
            std::vector<Row> f_0;
            // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
            for (size_t i = 0; i < n_rows; ++i) {
                f_0.push_back(-wire[i]);
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
                f_0.push_back(-wire[i]);
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

                wire[i] = s_0[i] + (xa * yb * (pid)) - (xa * b) - (yb * a) + c;
                if (pid == P0) wire[i]--;
                idx_mult++;
            }
        }
    }

   private:
    Party pid;
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