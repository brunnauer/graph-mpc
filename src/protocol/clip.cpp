#include "clip.h"

/* ----- Preprocessing ----- */
std::vector<std::tuple<Ring, Ring, Ring>> clip::equals_zero_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n) {
    const size_t n_layers = 5;
    const size_t n_triples = n_layers * n;

    return mul::preprocess_bin(id, rngs, network, n_triples);
}

std::vector<Ring> clip::equals_zero_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n) {
    const size_t n_layers = 5;
    const size_t n_triples = n_layers * n;

    std::vector<Ring> vals_to_P1;
    size_t idx = 0;

    auto triples = mul::preprocess_bin(id, rngs, vals_to_P1, idx, n_triples);
    return vals_to_P1;
}

std::vector<std::tuple<Ring, Ring, Ring>> clip::equals_zero_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals,
                                                                               size_t &idx) {
    const size_t n_layers = 5;
    const size_t n_triples = n_layers * n;

    auto triples = mul::preprocess_bin(id, rngs, vals, idx, n_triples);
    return triples;
}

std::vector<std::tuple<Ring, Ring, Ring>> clip::B2A_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n) {
    return mul::preprocess(id, rngs, network, n);
}

std::vector<Ring> clip::B2A_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n) {
    std::vector<Ring> vals_to_P1;
    size_t idx = 0;

    auto triples = mul::preprocess(id, rngs, vals_to_P1, idx, n);

    return vals_to_P1;
}

std::vector<std::tuple<Ring, Ring, Ring>> clip::B2A_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals, size_t &idx) {
    return mul::preprocess(id, rngs, vals, idx, n);
}

/* ----- Evaluation ----- */
std::vector<Ring> clip::equals_zero_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network,
                                             std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> &input_share) {
    const size_t n_layers = 5;
    std::vector<Ring> result(input_share.size());

    for (size_t i = 0; i < input_share.size(); ++i) {
        auto share = input_share[i];

        /* x_0 == -x_1 <=> ~x_0 ^ -x_1 */
        if (id == P0)
            share = ~share;
        else if (id == P1)
            share = -share;

        for (size_t layer = 0; layer < 5; ++layer) {
            if (id != D) {
                auto left = share;
                auto right = share;

                size_t width = 1 << (4 - layer);
                left >>= width;

                auto res = mul::evaluate_one_bin(id, network, triples[(i * n_layers) + layer], left, right);

                if (layer == 4) {
                    res <<= 31;
                    res >>= 31;
                }

                share = res;
            }
        }
        result[i] = share;
    }
    return result;
}

std::vector<Ring> clip::B2A_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network,
                                     std::vector<std::tuple<Ring, Ring, Ring>> &triples, std::vector<Ring> &input_share) {
    size_t n = input_share.size();
    std::vector<Ring> output(n);
    std::vector<Ring> vals_send(2 * n);

    if (id == D) return output;

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
        network->send_now(P1, vals_send);
        vals_receive = network->recv(P1, 2 * n);
    } else {
        vals_receive = network->recv(P0, 2 * n);
        network->send_now(P0, vals_send);
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

/* ----- Ad-Hoc Preprocessing ----- */
std::vector<Ring> clip::equals_zero(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, std::vector<Ring> &input_share) {
    const size_t n_layers = 5;
    const size_t n_triples = n_layers * input_share.size();

    std::vector<Ring> result(input_share.size());

    auto triples = mul::preprocess_bin(id, rngs, network, n_triples);

    for (size_t i = 0; i < input_share.size(); ++i) {
        auto share = input_share[i];

        /* x_0 == -x_1 <=> ~x_0 ^ -x_1 */
        if (id == P0)
            share = ~share;
        else if (id == P1)
            share = -share;

        for (size_t layer = 0; layer < 5; ++layer) {
            if (id != D) {
                auto left = share;
                auto right = share;

                size_t width = 1 << (4 - layer);
                left >>= width;

                auto res = mul::evaluate_one_bin(id, network, triples[(i * n_layers) + layer], left, right);

                if (layer == 4) {
                    res <<= 31;
                    res >>= 31;
                }

                share = res;
            }
        }
        result[i] = share;
    }
    return result;
}

std::vector<Ring> clip::equals_zero_two(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, std::vector<Ring> &input_share) {
    size_t n_triples = 5 * n;

    auto triples = mul::preprocess_bin(id, rngs, network, n_triples);

    std::vector<Ring> new_share(n);
    for (size_t i = 0; i < n; ++i) {
        /* Compute ~(x_0 ^ -x_1) == ~x_0 ^ -x_1 */
        if (id == P0) /* x_0 = ~x_0 */
            new_share[i] = ~input_share[i];
        else if (id == P1) /* x_1 = -x_1*/
            new_share[i] = -input_share[i];
    }

    /* Contains [[share_n-1^31 || share_n-2^31 || share_n-2^31 || ... || share_0^31]], [[share_n-1^30 || share_30^30 || share_29^30 || ... || share_0^30]],  */
    size_t n_rounds = input_share.size() / 32;
    size_t n_bits = sizeof(Ring) * 8;

    std::vector<std::vector<Ring>> bits(n_rounds + 1);
    for (size_t round = 0; round < n_rounds; ++round) {
        for (size_t i = 0; i < bits.size(); ++i) {
            bits[round].resize(n_bits);
            for (size_t j = 0; j < n_bits; ++j) {
                bits[round][i] |= ((new_share[round * n_bits + j] & (1 << i)) >> i) << j;
            }
        }
    }

    for (size_t i = 0; i < input_share.size() % 32; ++i) {
        for (size_t i = 0; i < bits.size(); ++i) {
            bits[n_rounds].resize(n_bits);
            for (size_t j = 0; j < n_bits; ++j) {
                bits[n_rounds][i] |= ((new_share[n_rounds * n_bits + j] & (1 << i)) >> i) << j;
            }
        }
    }

    std::vector<Ring> result(n);
    std::vector<Ring> layer_result(n_bits);

    for (size_t i = 0; i < n_rounds + 1; ++i) {
        layer_result = bits[i];
        Ring res;

        for (size_t layer = 0; layer < 5; ++layer) {
            size_t n_ands = n_bits / pow(2, layer + 1);

            for (size_t j = 0; j < n_ands; ++j) {
                auto triple = triples[triples.size()];
                triples.pop_back();
                layer_result[j] = mul::evaluate_one_bin(id, network, triple, layer_result[2 * j], layer_result[2 * j + 1]);
            }
        }
    }
    return new_share;
}

std::vector<Ring> clip::B2A(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, std::vector<Ring> &input_share) {
    const size_t n = input_share.size();

    auto triples = mul::preprocess(id, rngs, network, n);

    std::vector<Ring> result(n);
    if (id == D) return result;

    std::vector<Ring> vals_send(2 * n);

#pragma omp parallel for if (n > 1000)
    for (size_t i = 0; i < n; ++i) {
        auto [a, b, mul] = triples[i];
        auto xa = (id == P0 ? 1 : 0) * (input_share[i] & 1) + a;
        auto yb = (id == P0 ? 0 : 1) * (input_share[i] & 1) + b;

        vals_send[2 * i] = xa;
        vals_send[2 * i + 1] = yb;
    }

    std::vector<Ring> vals_receive;
    if (id == P0) {
        network->send_now(P1, vals_send);
        vals_receive = network->recv(P1, 2 * n);
    } else {
        vals_receive = network->recv(P0, 2 * n);
        network->send_now(P0, vals_send);
    }

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < 2 * n; ++i) {
        vals_send[i] += vals_receive[i];
    }

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < n; ++i) {
        auto [a, b, c] = triples[i];
        auto xa = vals_send[2 * i];
        auto yb = vals_send[2 * i + 1];

        auto mul_result = (xa * yb * (id)) - (xa * b) - (yb * a) + c;
        result[i] = (input_share[i] & 1) - 2 * mul_result;
    }

    return result;
}
