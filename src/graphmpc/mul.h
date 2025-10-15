#pragma once

#include "function.h"

class Mul : public Function {
   public:
    Mul(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input1,
        std::vector<Ring> *input2, std::vector<Ring> *output, Party &recv, bool binary)
        : Function(conf, preproc_vals, online_vals, input1, input2, output), recv(recv), binary(binary) {}

    Mul(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input1,
        std::vector<Ring> *input2, std::vector<Ring> *output, Party &recv, bool binary, size_t size)
        : Function(conf, preproc_vals, online_vals, input1, input2, output, size), recv(recv), binary(binary) {}

    void preprocess() override {
        triples_a.resize(size);
        triples_b.resize(size);
        triples_c.resize(size);

        triples_a = share::random_share_vec_3P(id, *rngs, size, binary);
        triples_b = share::random_share_vec_3P(id, *rngs, size, binary);
        std::vector<Ring> mul(size);

        if (binary) {
            for (size_t i = 0; i < size; ++i) mul[i] = (triples_a[i] & triples_b[i]);
        } else {
            for (size_t i = 0; i < size; ++i) mul[i] = (triples_a[i] * triples_b[i]);
        }

        triples_c = random_share_secret_vec_3P(mul);

        if (id != D) {
            if (ssd) {
                triples_disk->write_vec(triples_a);
                triples_disk->write_vec(triples_b);
                triples_disk->write_vec(triples_c);
            }
            triples_a.clear();
            triples_b.clear();
            triples_c.clear();
        }
        /* Alternate receiver */
        recv = recv == P0 ? P1 : P0;
    }

    void evaluate_send() override {
        if (ssd) {
            triples_a = triples_disk->read(size);
            triples_b = triples_disk->read(size);
            triples_c = triples_disk->read(size);
        }
        std::vector<Ring> data_send(2 * size);
#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            Ring a = triples_a[i];
            Ring b = triples_b[i];
            Ring xa, yb;
            if (binary) {
                xa = input->at(i) ^ a;
                yb = input2->at(i) ^ b;
            } else {
                xa = input->at(i) + a;
                yb = input2->at(i) + b;
            }
            data_send[2 * i] = xa;
            data_send[2 * i + 1] = yb;
        }
        online_vals->insert(online_vals->end(), data_send.begin(), data_send.end());
    }

    void evaluate_recv() override {
        auto data_recv = read_online(2 * size);
        auto outptr = output->data();
#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            Ring a = triples_a[i];
            Ring b = triples_b[i];
            Ring c = triples_c[i];

            auto xa = data_recv[2 * i];
            auto yb = data_recv[2 * i + 1];

            if (binary)
                outptr[i] = (xa & yb) * (id) ^ xa & b ^ yb & a ^ c;
            else
                outptr[i] = (xa * yb * (id)) - (xa * b) - (yb * a) + c;
        }
    }

   protected:
    Party &recv;

    std::vector<Ring> triples_a;
    std::vector<Ring> triples_b;
    std::vector<Ring> triples_c;
    FileWriter *triples_disk;
    bool binary;

    std::vector<Ring> random_share_secret_vec_3P(std::vector<Ring> &secret) {
        if (id == D) {
            assert(secret.size() == size);
            std::vector<Ring> share_0(size);
            std::vector<Ring> share_1(size);

            if (recv == P1) {
                for (size_t i = 0; i < size; ++i) rngs->rng_D0_prep().random_data(&share_0[i], sizeof(Ring));
            }
            if (recv == P0) {
                for (size_t i = 0; i < size; ++i) rngs->rng_D1_prep().random_data(&share_0[i], sizeof(Ring));
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
                for (size_t i = 0; i < size; ++i) rngs->rng_D0_prep().random_data(&share[i], sizeof(Ring));
            }
            if (id == P1) {
                for (size_t i = 0; i < size; ++i) rngs->rng_D1_prep().random_data(&share[i], sizeof(Ring));
            }
            return share;
        }
    }
};