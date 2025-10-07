#pragma once

#include "function.h"

class EQZ : public Function {
   public:
    EQZ(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
        std::vector<Ring> *output, Party &recv)
        : Function(conf, preproc_vals, online_vals, input, output), recv(recv) {}

    void preprocess() override {
        size_t n_layers = 5;
        size_t n_triples = n_layers * size;
        std::vector<Ring> a = share::random_share_vec_3P(id, *rngs, n_triples, true);
        std::vector<Ring> b = share::random_share_vec_3P(id, *rngs, n_triples, true);
        std::vector<Ring> mul(n_triples);

        for (size_t i = 0; i < n_triples; ++i) mul[i] = (a[i] & b[i]);

        std::vector<Ring> c = random_share_secret_vec_3P(mul);

        // TODO: write to disk
        if (id != D) {
            if (ssd) {
                std::vector<Ring> triples;
                for (size_t i = 0; i < size; ++i) {
                    triples.push_back(a[i]);
                    triples.push_back(b[i]);
                    triples.push_back(c[i]);
                }
                triples_disk->write_vec(triples);
            } else {
                for (size_t i = 0; i < size; ++i) triples.push_back({a[i], b[i], c[i]});
            }
        }
        /* Alternate receiver */
        recv = recv == P0 ? P1 : P0;
    }

    void evaluate_send() override {
        const size_t n_layers = 5;
        std::vector<Ring> res(*input);

        /* x_0 == -x_1 <=> ~x_0 ^ -x_1 */
        if (id == P0) {
#pragma omp parallel for if (size > 10000)
            for (auto &elem : res) elem = ~elem;
        }

        else if (id == P1) {
#pragma omp parallel for if (size > 10000)
            for (auto &elem : res) elem = -elem;
        }

        for (size_t layer = 0; layer < n_layers; ++layer) {
            if (id != D) {
                std::vector<Ring> shares_left(res);
                std::vector<Ring> shares_right(res);

                size_t width = 1 << (4 - layer);
#pragma omp parallel for if (size > 10000)
                for (auto &elem : shares_left) elem >>= width;

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    auto [a, b, _] = triples[i];
                    Ring xa, yb;
                    xa = shares_left[i] ^ a;
                    yb = shares_right[i] ^ b;
                    online_vals->push_back(xa);
                    online_vals->push_back(yb);
                }
            }
        }
    }

    void evaluate_recv() override {
        if (id == D) return;
        size_t n_layers = 5;
        // #pragma omp parallel for if (size > 10000)
        // for (size_t i = 0; i < 2 * n; ++i) {
        // vals_send[i] += vals_receive[i];
        //}
        for (size_t layer = 0; layer < n_layers; ++layer) {
#pragma omp parallel for if (size > 10000)
            for (size_t i = 0; i < size; ++i) {
                auto [a, b, mul] = triples[i];

                auto xa = online_vals->at(2 * i);
                auto yb = online_vals->at(2 * i + 1);

                output->at(i) = (xa & yb) * (id) ^ xa & b ^ yb & a ^ mul;

                if (layer == 4) {
#pragma omp parallel for if (size > 10000)
                    for (auto &elem : *output) {
                        elem <<= 31;
                        elem >>= 31;
                    }
                }
            }
        }
    }

   private:
    std::vector<std::tuple<Ring, Ring, Ring>> triples;
    FileWriter *triples_disk;
    Party &recv;

    std::vector<Ring> random_share_secret_vec_3P(std::vector<Ring> &secret) {
        if (id == D) {
            assert(secret.size() == size);
            std::vector<Ring> share_0(size);
            std::vector<Ring> share_1(size);

            if (recv == P1) {
                for (size_t i = 0; i < size; ++i) rngs->rng_D0_comp().random_data(&share_0[i], sizeof(Ring));
            }
            if (recv == P0) {
                for (size_t i = 0; i < size; ++i) rngs->rng_D1_comp().random_data(&share_0[i], sizeof(Ring));
            }

            for (size_t i = 0; i < size; ++i) share_1[i] = (secret[i] ^ share_0[i]);

            preproc_vals->at(recv).insert(preproc_vals->at(recv).end(), share_1.begin(), share_1.end());
            return secret;
        } else if (id == recv) {
            std::vector<Ring> share = read_preproc(size);
            return share;
        } else {
            std::vector<Ring> share(size);
            if (id == P0) {
                for (size_t i = 0; i < size; ++i) rngs->rng_D0_comp().random_data(&share[i], sizeof(Ring));
            }
            if (id == P1) {
                for (size_t i = 0; i < size; ++i) rngs->rng_D1_comp().random_data(&share[i], sizeof(Ring));
            }
            return share;
        }
    }
};