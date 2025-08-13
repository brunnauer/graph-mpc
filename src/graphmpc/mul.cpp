#include "mul.h"

void mul::preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv, bool binary,
                     bool save_to_disk) {
    std::vector<Ring> a = share::random_share_vec_3P(id, rngs, n, binary);
    std::vector<Ring> b = share::random_share_vec_3P(id, rngs, n, binary);
    std::vector<Ring> mul(n);

    if (binary) {
        for (size_t i = 0; i < n; ++i) mul[i] = (a[i] & b[i]);
    } else {
        for (size_t i = 0; i < n; ++i) mul[i] = (a[i] * b[i]);
    }

    std::vector<Ring> c = share::random_share_secret_vec_3P(id, rngs, network, n, mul, recv, binary);

    // TODO: write to disk
    if (id != D)
        for (size_t i = 0; i < n; ++i) {
            if (save_to_disk) {
                auto triple = std::tuple<Ring, Ring, Ring>({a[i], b[i], c[i]});
                network->preproc_disk.write_triple(triple);
            } else {
                preproc.triples.push({a[i], b[i], c[i]});
            }
        }

    /* Alternate receiver */
    recv = recv == P0 ? P1 : P0;
}

std::vector<Ring> mul::evaluate(Party id, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, std::vector<Ring> x, std::vector<Ring> y,
                                bool binary, bool save_to_disk) {
    if (id == D) return std::vector<Ring>(n);

    std::vector<std::tuple<Ring, Ring, Ring>> triples;
    if (save_to_disk) {
        triples = network->preproc_disk.read_triples(n);
    } else {
        triples = extract(preproc.triples, n);
    }

    std::vector<Ring> vals_send(2 * n);
    std::vector<Ring> vals_receive(2 * n);
    std::vector<Ring> output(n);

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < n; ++i) {
        auto [a, b, _] = triples[i];
        Ring xa, yb;
        if (binary) {
            xa = x[i] ^ a;
            yb = y[i] ^ b;
        } else {
            xa = x[i] + a;
            yb = y[i] + b;
        }
        vals_send[2 * i] = xa;
        vals_send[2 * i + 1] = yb;
    }

    if (id == P0) {
        network->send_vec(P1, 2 * n, vals_send);
        network->recv_vec(P1, 2 * n, vals_receive);
    } else if (id == P1) {
        network->recv_vec(P0, 2 * n, vals_receive);
        network->send_vec(P0, 2 * n, vals_send);
    }

    if (binary) {
#pragma omp parallel for if (n > 10000)
        for (size_t i = 0; i < vals_send.size(); ++i) vals_send[i] ^= vals_receive[i];
    } else {
#pragma omp parallel for if (n > 10000)
        for (size_t i = 0; i < vals_send.size(); ++i) vals_send[i] += vals_receive[i];
    }
#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < n; ++i) {
        auto [a, b, mul] = triples[i];

        auto xa = vals_send[2 * i];
        auto yb = vals_send[2 * i + 1];

        if (binary)
            output[i] = (xa & yb) * (id) ^ xa & b ^ yb & a ^ mul;
        else
            output[i] = (xa * yb * (id)) - (xa * b) - (yb * a) + mul;
    }
    return output;
}
