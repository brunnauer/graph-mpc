#include "compaction.h"

/* ----- Preprocessing ----- */
void compaction::preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv, bool ssd) {
    mul::preprocess(id, rngs, network, n, preproc, recv, false, ssd);
}

/* ----- Evaluation ----- */
Permutation compaction::evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                                 std::vector<Ring> &input_share, bool ssd) {
    if (id == D) return std::vector<Ring>(n);

    std::vector<Ring> output(n);
    std::vector<Ring> vals_send(2 * n);
    std::vector<std::tuple<Ring, Ring, Ring>> triples;

    if (ssd) {
        triples = network->mul_disk.read_triples(n);
    } else {
        triples = extract(preproc.triples, n);
    }

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
            s_1.push_back(s - s_0[i]);
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
            network->send_vec(P1, 2 * n, vals_send);
            network->recv_vec(P1, 2 * n, vals_receive);
        } else {
            network->recv_vec(P0, 2 * n, vals_receive);
            network->send_vec(P0, 2 * n, vals_send);
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
Permutation compaction::get_compaction(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                                       std::vector<Ring> &input_share, Party &recv) {
    preprocess(id, rngs, network, n, preproc, recv);
    network->sync();
    return evaluate(id, rngs, network, n, preproc, input_share);
}
