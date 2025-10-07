#pragma once

#include "function.h"

class Mul : public Function {
   public:
    Mul(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input1,
        std::vector<Ring> *input2, std::vector<Ring> *output, Party &recv, bool binary)
        : Function(conf, preproc_vals, online_vals, input1, input2, output), recv(recv), binary(binary) {}

    Mul(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input1,
        std::vector<Ring> *input2, std::vector<Ring> *output, Party &recv, bool binary, size_t &size)
        : Function(conf, preproc_vals, online_vals, input1, input2, output, size), recv(recv), binary(binary) {}

    void preprocess() override {
        std::vector<Ring> a = share::random_share_vec_3P(id, *rngs, size, binary);
        std::vector<Ring> b = share::random_share_vec_3P(id, *rngs, size, binary);
        std::vector<Ring> mul(size);

        if (binary) {
            for (size_t i = 0; i < size; ++i) mul[i] = (a[i] & b[i]);
        } else {
            for (size_t i = 0; i < size; ++i) mul[i] = (a[i] * b[i]);
        }

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
        if (id == D) return;

#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            auto [a, b, _] = triples[i];
            Ring xa, yb;
            if (binary) {
                xa = input->at(i) ^ a;
                yb = input2->at(i) ^ b;
            } else {
                xa = input->at(i) + a;
                yb = input2->at(i) + b;
            }
            online_vals->push_back(xa);
            online_vals->push_back(yb);
        }
    }

    void evaluate_recv() override {
#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            auto [a, b, mul] = triples[i];

            auto xa = online_vals->at(2 * i);
            auto yb = online_vals->at(2 * i + 1);

            if (binary)
                output->at(i) = (xa & yb) * (id) ^ xa & b ^ yb & a ^ mul;
            else
                output->at(i) = (xa * yb * (id)) - (xa * b) - (yb * a) + mul;
        }
    }

   protected:
    Party &recv;

    std::vector<std::tuple<Ring, Ring, Ring>> triples;
    FileWriter *triples_disk;
    bool binary;

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

            if (binary) {
                for (size_t i = 0; i < size; ++i) share_1[i] = (secret[i] ^ share_0[i]);
            } else {
                for (size_t i = 0; i < size; ++i) share_1[i] = (secret[i] - share_0[i]);
            }

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