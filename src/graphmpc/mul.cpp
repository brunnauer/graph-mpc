#include "mul.h"

void mul::preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Party &recv) {
    std::vector<Ring> vals_send;
    size_t idx = 0;

    if (id == recv) vals_send = network->read(D, n);
    for (size_t i = 0; i < n; ++i) {
        Ring a = share::random_share_3P(id, rngs);
        Ring b = share::random_share_3P(id, rngs);
        Ring mul = a * b;
        Ring c = share::random_share_secret_3P(id, rngs, vals_send, idx, mul, recv);
        preproc.triples.push({a, b, c});
    }
    if (id == D) network->add_send(recv, vals_send);

    /* Alternate receiver */
    recv = recv == P0 ? P1 : P0;
}

std::vector<std::tuple<Ring, Ring, Ring>> mul::preprocess(Party id, RandomGenerators &rngs, std::vector<Ring> &shares, size_t &idx, size_t n) {
    std::vector<std::tuple<Ring, Ring, Ring>> triples;
    for (size_t i = 0; i < n; ++i) {
        Ring a = share::random_share_3P(id, rngs);
        Ring b = share::random_share_3P(id, rngs);
        Ring mul = a * b;
        Ring c = share::random_share_secret_3P(id, rngs, shares, idx, mul);
        triples.push_back({a, b, c});
    }
    return triples;
}

std::vector<std::tuple<Ring, Ring, Ring>> mul::preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n) {
    std::vector<Ring> vals_to_P1;
    size_t idx = 0;

    if (id == P1) vals_to_P1 = network->read(D, n);
    auto triples = mul::preprocess(id, rngs, vals_to_P1, idx, n);
    if (id == D) {
        // for (const auto &elem : vals_to_P1) network->add_send_streamed(P1, elem);
        network->add_send(P1, vals_to_P1);
    }

    return triples;
}

std::vector<Ring> mul::evaluate(Party id, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, std::vector<Ring> x, std::vector<Ring> y) {
    auto triples = extract(preproc.triples, n);
    return evaluate(id, network, n, triples, x, y);
}

std::vector<Ring> mul::evaluate(Party id, std::shared_ptr<io::NetIOMP> network, size_t n, std::vector<std::tuple<Ring, Ring, Ring>> &triples,
                                std::vector<Ring> x, std::vector<Ring> y) {
    std::vector<Ring> vals_send;
    std::vector<Ring> vals_receive(n * 2);
    std::vector<Ring> output(n);
    size_t idx_mult = 0;

    if (id == D) return output;

    for (size_t i = 0; i < n; ++i) {
        auto [a, b, _] = triples[i];
        auto xa = x[i] + a;
        auto yb = y[i] + b;
        vals_send.push_back(xa);
        vals_send.push_back(yb);
    }

    if (id == P0) {
        network->send_vec(P1, 2 * n, vals_send);
        network->recv_vec(P1, 2 * n, vals_receive);
    } else if (id == P1) {
        network->recv_vec(P0, 2 * n, vals_receive);
        network->send_vec(P0, 2 * n, vals_send);
    }

    for (size_t i = 0; i < vals_send.size(); ++i) vals_send[i] += vals_receive[i];

    for (size_t i = 0; i < n; ++i) {
        auto [a, b, mul] = triples[i];

        auto xa = vals_send[2 * idx_mult];
        auto yb = vals_send[2 * idx_mult + 1];

        output[i] = (xa * yb * (id)) - (xa * b) - (yb * a) + mul;
        idx_mult++;
    }
    return output;
}

std::tuple<Ring, Ring, Ring> mul::preprocess_one(Party id, RandomGenerators &rngs, std::vector<Ring> &vals_to_p1, size_t &idx) {
    Ring a = share::random_share_3P(id, rngs);
    Ring b = share::random_share_3P(id, rngs);
    Ring mul = a * b;
    Ring d = share::random_share_secret_3P(id, rngs, vals_to_p1, idx, mul);
    return {a, b, d};
}

Ring mul::evaluate_one(Party id, std::shared_ptr<io::NetIOMP> network, std::tuple<Ring, Ring, Ring> &triple, Ring x, Ring y) {
    auto [a, b, mul] = triple;
    Ring xa = x + a;
    Ring yb = y + b;

    std::vector<Ring> vals_rcv(2);
    std::vector<Ring> vals(2);
    vals[0] = xa;
    vals[1] = yb;

    if (id == P0) {
        network->send_vec(P1, 2, vals);
        network->recv_vec(P1, 2, vals_rcv);
    } else if (id == P1) {
        network->recv_vec(P0, 2, vals_rcv);
        network->send_vec(P0, 2, vals);
    }

    xa += vals_rcv[0];
    yb += vals_rcv[1];

    return (xa * yb) * (id) - (xa * b) - (yb * a) + mul;
}

std::vector<std::tuple<Ring, Ring, Ring>> mul::preprocess_bin(Party id, RandomGenerators &rngs, std::vector<Ring> &vals_to_p1, size_t &idx, size_t n) {
    std::vector<std::tuple<Ring, Ring, Ring>> triples;
    for (size_t i = 0; i < n; ++i) {
        Ring a = share::random_share_3P_bin(id, rngs);
        Ring b = share::random_share_3P_bin(id, rngs);
        Ring mul = a & b;
        Ring d = share::random_share_secret_3P_bin(id, rngs, vals_to_p1, idx, mul);
        triples.push_back({a, b, d});
    }
    return triples;
}

std::vector<std::tuple<Ring, Ring, Ring>> mul::preprocess_bin(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n) {
    std::vector<Ring> vals_to_P1;
    size_t idx = 0;

    if (id == P1) vals_to_P1 = network->read(D, n);
    auto triples = mul::preprocess_bin(id, rngs, vals_to_P1, idx, n);
    if (id == D) network->add_send(P1, vals_to_P1);

    return triples;
}

std::vector<Ring> mul::evaluate_bin(Party id, std::shared_ptr<io::NetIOMP> network, size_t n, std::vector<std::tuple<Ring, Ring, Ring>> &triples,
                                    std::vector<Ring> x, std::vector<Ring> y) {
    std::vector<Ring> vals_send(2 * n);
    std::vector<Ring> vals_receive(2 * n);
    std::vector<Ring> output(n);

    if (id == D) return output;

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < n; ++i) {
        auto [a, b, _] = triples[i];
        auto xa = x[i] ^ a;
        auto yb = y[i] ^ b;
        vals_send[2 * i] = xa;
        vals_send[2 * i + 1] = yb;
    }

    std::vector<Ring> vals_recv(2 * n);
    if (id == P0) {
        network->send_vec(P1, 2 * n, vals_send);
        network->recv_vec(P1, 2 * n, vals_recv);
    } else {
        network->recv_vec(P0, 2 * n, vals_recv);
        network->send_vec(P0, 2 * n, vals_send);
    }

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < vals_send.size(); ++i) vals_send[i] ^= vals_receive[i];

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < n; ++i) {
        auto [a, b, mul] = triples[i];

        auto xa = vals_send[2 * i];
        auto yb = vals_send[2 * i + 1];

        output[i] = (xa & yb) * (id) ^ xa & b ^ yb & a ^ mul;
    }

    return output;
}

std::tuple<Ring, Ring, Ring> mul::preprocess_one_bin(Party id, RandomGenerators &rngs, std::vector<Ring> &vals_to_p1, size_t &idx) {
    Ring a = share::random_share_3P_bin(id, rngs);
    Ring b = share::random_share_3P_bin(id, rngs);
    Ring mul = a & b;
    Ring d = share::random_share_secret_3P_bin(id, rngs, vals_to_p1, idx, mul);
    return {a, b, d};
}

Ring mul::evaluate_one_bin(Party id, std::shared_ptr<io::NetIOMP> network, std::tuple<Ring, Ring, Ring> &triple, Ring x, Ring y) {
    auto [a, b, mul] = triple;
    Ring xa = x ^ a;
    Ring yb = y ^ b;

    std::vector<Ring> vals_rcv(2);
    std::vector<Ring> vals(2);
    vals[0] = xa;
    vals[1] = yb;

    if (id == P0) {
        network->send_vec(P1, 2, vals);
        network->recv_vec(P1, 2, vals_rcv);
    } else if (id == P1) {
        network->recv_vec(P0, 2, vals_rcv);
        network->send_vec(P0, 2, vals);
    }

    xa ^= vals_rcv[0];
    yb ^= vals_rcv[1];

    return (xa & yb) * (id) ^ xa & b ^ yb & a ^ mul;
}
