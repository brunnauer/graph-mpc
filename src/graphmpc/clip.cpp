#include "clip.h"

/* ----- Preprocessing ----- */
void clip::equals_zero_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv,
                                  bool ssd) {
    const size_t n_layers = 5;
    const size_t n_triples = n_layers * n;

    mul::preprocess(id, rngs, network, n_triples, preproc, recv, true, ssd);
}

void clip::B2A_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv, bool ssd) {
    mul::preprocess(id, rngs, network, n, preproc, recv, false, ssd);
}

/* ----- Evaluation ----- */
std::vector<Ring> clip::equals_zero_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, MPPreprocessing &preproc,
                                             std::vector<Ring> &input_share, bool ssd) {
    const size_t n_layers = 5;
    const size_t n = input_share.size();  // Change
    std::vector<Ring> res(input_share);

    /* x_0 == -x_1 <=> ~x_0 ^ -x_1 */
    if (id == P0) {
#pragma omp parallel for if (n > 10000)
        for (auto &elem : res) elem = ~elem;
    }

    else if (id == P1) {
#pragma omp parallel for if (n > 10000)
        for (auto &elem : res) elem = -elem;
    }

    for (size_t layer = 0; layer < n_layers; ++layer) {
        if (id != D) {
            std::vector<Ring> shares_left(res);
            std::vector<Ring> shares_right(res);

            size_t width = 1 << (4 - layer);
#pragma omp parallel for if (n > 10000)
            for (auto &elem : shares_left) elem >>= width;

            res = mul::evaluate(id, network, n, preproc, shares_left, shares_right, true, ssd);

            if (layer == 4) {
#pragma omp parallel for if (n > 10000)
                for (auto &elem : res) {
                    elem <<= 31;
                    elem >>= 31;
                }
            }
        }
    }
    return res;
}

std::vector<Ring> clip::B2A_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
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

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < n; ++i) {
        auto [a, b, mul] = triples[i];
        auto xa = (id == P0 ? 1 : 0) * (input_share[i] & 1) + a;
        auto yb = (id == P0 ? 0 : 1) * (input_share[i] & 1) + b;
        vals_send[2 * i] = xa;
        vals_send[2 * i + 1] = yb;
    }

    std::vector<Ring> vals_receive(2 * n);
    if (id == P0) {
        network->send_vec(P1, 2 * n, vals_send);
        network->recv_vec(P1, 2 * n, vals_receive);
    } else {
        network->recv_vec(P0, 2 * n, vals_receive);
        network->send_vec(P0, 2 * n, vals_send);
    }

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < 2 * n; ++i) {
        vals_send[i] += vals_receive[i];
    }

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < n; ++i) {
        auto [a, b, mul] = triples[i];
        auto xa = vals_send[2 * i];
        auto yb = vals_send[2 * i + 1];

        auto mul_result = (xa * yb * (id)) - (xa * b) - (yb * a) + mul;
        output[i] = (input_share[i] & 1) - 2 * mul_result;
    }
    return output;
}

std::vector<Ring> clip::flip(Party id, std::vector<Ring> &input_share) {
    std::vector<Ring> result(input_share.size());

#pragma omp parallel for if (input_share.size() > 10000)
    for (size_t i = 0; i < result.size(); ++i) {
        if (id == P0) {
            result[i] = 1 - input_share[i];
        } else if (id == P1) {
            result[i] = -input_share[i];
        }
    }
    return result;
}
