#pragma once

#include "mul.h"

class Bit2A : public Mul {
   public:
    Bit2A(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
          std::vector<Ring> *output, Party &recv)
        : Mul(conf, preproc_vals, online_vals, input, input, output, recv, false) {}

    void evaluate_send() override {
        if (id == D) return;

#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            auto [a, b, mul] = triples[i];
            auto xa = (id == P0 ? 1 : 0) * (input->at(i) & 1) + a;
            auto yb = (id == P0 ? 0 : 1) * (input->at(i) & 1) + b;
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

            auto mul_result = (xa * yb * (id)) - (xa * b) - (yb * a) + mul;
            output->at(i) = (input->at(i) & 1) - 2 * mul_result;
        }
    }
};