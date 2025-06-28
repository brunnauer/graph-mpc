#include "mul.h"

std::vector<std::tuple<Ring, Ring, Ring>> mul::preprocess(Party id, RandomGenerators &rngs, std::vector<Ring> &shares_P1, size_t &idx, size_t n) {
    std::vector<std::tuple<Ring, Ring, Ring>> triples;
    for (size_t i = 0; i < n; ++i) {
        Ring a = share::random_share_3P(id, rngs);
        Ring b = share::random_share_3P(id, rngs);
        Ring mul = a * b;
        Ring c = share::random_share_secret_3P(id, rngs, shares_P1, idx, mul);
        triples.push_back({a, b, c});
    }
    return triples;
}

std::vector<std::tuple<Ring, Ring, Ring>> mul::preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE) {
    std::vector<Ring> vals_to_P1;
    size_t idx = 0;

    if (id == P1) recv_vec(D, network, n, vals_to_P1, BLOCK_SIZE);
    auto triples = mul::preprocess(id, rngs, vals_to_P1, idx, n);
    if (id == D) send_vec(P1, network, n, vals_to_P1, BLOCK_SIZE);

    return triples;
}

std::vector<Ring> mul::evaluate(Party id, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, std::vector<std::tuple<Ring, Ring, Ring>> &triples,
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
        send_vec(P1, network, vals_send.size(), vals_send, BLOCK_SIZE);
        recv_vec(P1, network, vals_receive, BLOCK_SIZE);
    } else if (id == P1) {
        recv_vec(P0, network, vals_receive, BLOCK_SIZE);
        send_vec(P0, network, vals_send.size(), vals_send, BLOCK_SIZE);
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

void mul::evaluate_1(std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> &mult_vals, std::vector<Ring> x, std::vector<Ring> y, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        auto [a, b, _] = triples[i];
        auto xa = x[i] + a;
        auto yb = y[i] + b;
        mult_vals.push_back(xa);
        mult_vals.push_back(yb);
    }
}

std::vector<Ring> mul::evaluate_2(Party id, std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> &vals, std::vector<Ring> x,
                                  std::vector<Ring> y, size_t n) {
    size_t idx_mult = 0;
    std::vector<Ring> output(n);

    for (size_t i = 0; i < n; ++i) {
        auto [a, b, mul] = triples[i];

        auto xa = vals[2 * idx_mult];
        auto yb = vals[2 * idx_mult + 1];

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

Ring mul::evaluate_one(Party id, std::shared_ptr<io::NetIOMP> network, size_t BLOCK_SIZE, std::tuple<Ring, Ring, Ring> &triple, Ring x, Ring y) {
    auto [a, b, mul] = triple;
    Ring xa = x + a;
    Ring yb = y + b;

    std::vector<Ring> vals_rcv(2);
    std::vector<Ring> vals(2);
    vals[0] = xa;
    vals[1] = yb;

    if (id == P0) {
        send_vec(P1, network, 2, vals, BLOCK_SIZE);
        recv_vec(P1, network, vals_rcv, BLOCK_SIZE);
    } else if (id == P1) {
        recv_vec(P0, network, vals_rcv, BLOCK_SIZE);
        send_vec(P0, network, 2, vals, BLOCK_SIZE);
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

std::vector<std::tuple<Ring, Ring, Ring>> mul::preprocess_bin(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n,
                                                              size_t BLOCK_SIZE) {
    std::vector<Ring> vals_to_P1;
    size_t idx = 0;

    if (id == P1) recv_vec(D, network, n, vals_to_P1, BLOCK_SIZE);
    auto triples = mul::preprocess_bin(id, rngs, vals_to_P1, idx, n);
    if (id == D) send_vec(P1, network, n, vals_to_P1, BLOCK_SIZE);

    return triples;
}

std::vector<Ring> mul::evaluate_bin(Party id, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                    std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> x, std::vector<Ring> y) {
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

    if (id == P0) {
        send_vec(P1, network, vals_send.size(), vals_send, BLOCK_SIZE);
        recv_vec(P1, network, vals_receive, BLOCK_SIZE);
    } else {
        recv_vec(P0, network, vals_receive, BLOCK_SIZE);
        send_vec(P0, network, vals_send.size(), vals_send, BLOCK_SIZE);
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

Ring mul::evaluate_one_bin(Party id, std::shared_ptr<io::NetIOMP> network, size_t BLOCK_SIZE, std::tuple<Ring, Ring, Ring> &triple, Ring x, Ring y) {
    auto [a, b, mul] = triple;
    Ring xa = x ^ a;
    Ring yb = y ^ b;

    std::vector<Ring> vals_rcv(2);
    std::vector<Ring> vals(2);
    vals[0] = xa;
    vals[1] = yb;

    if (id == P0) {
        send_vec(P1, network, 2, vals, BLOCK_SIZE);
        recv_vec(P1, network, vals_rcv, BLOCK_SIZE);
    } else if (id == P1) {
        recv_vec(P0, network, vals_rcv, BLOCK_SIZE);
        send_vec(P0, network, 2, vals, BLOCK_SIZE);
    }

    xa ^= vals_rcv[0];
    yb ^= vals_rcv[1];

    return (xa & yb) * (id) ^ xa & b ^ yb & a ^ mul;
}
