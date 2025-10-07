#pragma once

#include "function.h"

class DeduplicationSub : public Function {
   public:
    DeduplicationSub(ProtocolConfig *conf, std::vector<Ring> *vec_p, std::vector<Ring> *vec_dupl) : Function(conf, {}, {}, vec_p, vec_dupl) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        for (size_t i = 1; i < size; ++i) {
            output->at(i - 1) = input->at(i) - input->at(i - 1);
        }
    }
};