#include "compaction.h"

/* ----- Preprocessing ----- */
std::vector<Ring> compaction::preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n) {
    std::vector<Ring> shares_P1;
    size_t idx = 0;

    auto triples = mul::preprocess(id, rngs, shares_P1, idx, n);
    return shares_P1;
}

std::vector<std::tuple<Ring, Ring, Ring>> compaction::preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &shares_P1,
                                                                         size_t &idx) {
    return mul::preprocess(id, rngs, shares_P1, idx, n);
}

std::vector<std::tuple<Ring, Ring, Ring>> compaction::preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n,
                                                                 size_t BLOCK_SIZE) {
    return mul::preprocess(id, rngs, network, n, BLOCK_SIZE);
}

/* ----- Evaluation ----- */
std::vector<Ring> compaction::evaluate_1(Party id, size_t n, std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> &input_share) {
    std::vector<Ring> output(n);
    std::vector<Ring> vals;

    size_t idx_mult = 0;

    if (id != D) {
        std::vector<Ring> f_0;
        // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
        for (size_t i = 0; i < n; ++i) {
            f_0.push_back(-input_share[i]);
            if (id == P0) {
                f_0[i] += 1;  // 1 as constant only to one share
            }
        }

        std::vector<Ring> s_0, s_1;
        Ring s = 0;
        // Set s_0 to prefix sum of f_0 and s_1 to prefix sum of f_1/input continuing from the prior final value
        for (size_t i = 0; i < n; ++i) {
            s += f_0[i];
            s_0.push_back(s);
        }
        for (size_t i = 0; i < n; ++i) {
            s += input_share[i];
            s_1.push_back(s - s_0[i]);  // s_0[i] see below
        }

        mul::evaluate_1(triples, vals, input_share, s_1, n);
    }
    return vals;
}

Permutation compaction::evaluate_2(Party id, size_t n, std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> &vals,
                                   std::vector<Ring> &input_share) {
    std::vector<Ring> output(n);
    if (id != D) {
        std::vector<Ring> f_0;
        // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
        for (size_t i = 0; i < n; ++i) {
            f_0.push_back(-input_share[i]);
            if (id == P0) {
                f_0[i] += 1;  // 1 as constant only to one share
            }
        }

        std::vector<Ring> s_0, s_1;
        Ring s = 0;
        // Set s_0 to prefix sum of f_0 and s_1 to prefix sum of f_1/input continuing from the prior final value
        for (size_t i = 0; i < n; ++i) {
            s += f_0[i];
            s_0.push_back(s);
        }
        for (size_t i = 0; i < n; ++i) {
            s += input_share[i];
            s_1.push_back(s - s_0[i]);  // s_0[i] see below
        }

        output = mul::evaluate_2(id, triples, vals, input_share, s_1, n);

#pragma omp parallel for if (n > 10000)
        for (size_t i = 0; i < output.size(); ++i) {
            output[i] += s_0[i];
            if (id == P0) output[i]--;
        }
    }

    return Permutation(output);
}

Permutation compaction::evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                 std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> &input_share) {
    std::vector<Ring> output(n);
    std::vector<Ring> vals_send(2 * n);

    if (id == D) return Permutation(output);

    if (n != D) {
        std::vector<Ring> f_0;
        // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
        for (size_t i = 0; i < n; ++i) {
            f_0.push_back(-input_share[i]);
            if (id == P0) {
                f_0[i] += 1;  // 1 as constant only to one share
            }
        }

        std::vector<Ring> s_0, s_1;
        Ring s = 0;
        // Set s_0 to prefix sum of f_0 and s_1 to prefix sum of f_1/input continuing from the prior final value
        for (size_t i = 0; i < n; ++i) {
            s += f_0[i];
            s_0.push_back(s);
        }

        for (size_t i = 0; i < n; ++i) {
            s += input_share[i];
            s_1.push_back(s - s_0[i]);  // s_0[i] see below
        }

#pragma omp parallel for if (n > 10000)
        for (size_t i = 0; i < n; ++i) {
            auto [a, b, _] = triples[i];
            auto xa = input_share[i] + a;
            auto yb = s_1[i] + b;
            vals_send[2 * i] = xa;
            vals_send[2 * i + 1] = yb;
        }

        std::vector<Ring> vals_receive(n * 2);
        if (id == P0) {
            send_vec(P1, network, vals_send.size(), vals_send, BLOCK_SIZE);
            recv_vec(P1, network, vals_receive, BLOCK_SIZE);
        } else {
            recv_vec(P0, network, vals_receive, BLOCK_SIZE);
            send_vec(P0, network, vals_send.size(), vals_send, BLOCK_SIZE);
        }

#pragma omp parallel for if (n > 10000)
        for (size_t i = 0; i < n * 2; ++i) {
            vals_send[i] += vals_receive[i];
        }

#pragma omp parallel for if (n > 10000)
        for (size_t i = 0; i < n; ++i) {
            auto [a, b, mul] = triples[i];

            auto xa = vals_send[2 * i];
            auto yb = vals_send[2 * i + 1];

            output[i] = (xa * yb * (id)) - (xa * b) - (yb * a) + mul;
        }

#pragma omp parallel for if (n > 10000)
        for (size_t i = 0; i < output.size(); ++i) {
            output[i] += s_0[i];
            if (id == P0) output[i]--;
        }
    }
    return Permutation(output);
}

/* ----- Ad-Hoc Preprocessing ----- */
Permutation compaction::get_compaction(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                       std::vector<Ring> &input_share) {
    auto triples = preprocess(id, rngs, network, n, BLOCK_SIZE);
    network->sync();
    return evaluate(id, rngs, network, n, BLOCK_SIZE, triples, input_share);
}
