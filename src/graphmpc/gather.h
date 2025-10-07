#pragma once

#include "function.h"

class Gather_1 : public Function {
   public:
    Gather_1(ProtocolConfig *conf, std::vector<Ring> *input, std::vector<Ring> *output) : Function(conf, {}, {}, input, output) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        std::vector<Ring> data(input->size());
        Ring sum = 0;
        for (size_t i = 0; i < data.size(); ++i) {
            sum += input->at(i);
            data[i] = sum;
        }
        *output = data;
    }
};

class Gather_2 : public Function {
   public:
    Gather_2(ProtocolConfig *conf, std::vector<Ring> *input, std::vector<Ring> *output) : Function(conf, {}, {}, input, output), nodes(conf->nodes) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        std::vector<Ring> data(input->size());
        Ring sum = 0;

        for (size_t i = 0; i < nodes; ++i) {
            data[i] = input->at(i) - sum;
            sum += data[i];
        }
#pragma omp_parallel for if (data.size() - nodes > 1000)
        for (size_t i = nodes; i < data.size(); ++i) {
            data[i] = 0;
        }
        *output = data;
    }

   private:
    size_t nodes;
};