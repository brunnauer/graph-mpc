#pragma once

#include "function.h"

class Add : public Function {
   public:
    Add(ProtocolConfig *conf, std::vector<Ring> *input1, std::vector<Ring> *input2, std::vector<Ring> *output)
        : Function(conf, {}, {}, input1, input2, output) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        std::vector<Ring> result(size);
        for (size_t i = 0; i < size; ++i) {
            result[i] = input->at(i) + input2->at(i);
        }
        *output = result;
    }
};