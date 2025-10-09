#pragma once

#include "function.h"

class Flip : public Function {
   public:
    Flip(ProtocolConfig *conf, std::vector<Ring> *input, std::vector<Ring> *output) : Function(conf, {}, {}, input, output) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        assert(output->size() >= input->size());

        // #pragma omp parallel for if (input->size() > 10000)
        std::vector<Ring> result(input->size());
        for (size_t i = 0; i < input->size(); ++i) {
            if (id == P0) {
                result[i] = 1 - input->at(i);
            } else if (id == P1) {
                result[i] = -input->at(i);
            }
        }
        *output = result;
    }
};