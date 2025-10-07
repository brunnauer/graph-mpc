#pragma once

#include "function.h"

class Propagate_1 : public Function {
   public:
    Propagate_1(ProtocolConfig *conf, std::vector<Ring> *input, std::vector<Ring> *output) : Function(conf, {}, {}, input, output) {}

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

class Propagate_2 : public Function {
   public:
    Propagate_2(ProtocolConfig *conf, std::vector<Ring> *input1, std::vector<Ring> *input2, std::vector<Ring> *output)
        : Function(conf, {}, {}, input1, input2, output) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        std::vector<Ring> data(input->size());
        Ring sum = 0;
        for (size_t i = 0; i < data.size(); ++i) {
            sum += input->at(i);
            data[i] = sum - input2->at(i);
        }
        *output = data;
    }
};