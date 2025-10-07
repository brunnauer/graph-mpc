#pragma once

#include "function.h"

class Reveal : public Function {
   public:
    Reveal(ProtocolConfig *conf, std::vector<Ring> *online_vals, std::vector<Ring> *input, std::vector<Ring> *output)
        : Function(conf, {}, online_vals, input, output) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        auto share_other = read_online(size);
        std::vector<Ring> result(size);
        for (size_t i = 0; i < size; ++i) {
            result[i] = share_other[i] + input->at(i);
        }
        *output = result;
    }
};
