#pragma once

#include "function.h"

class ConstructPayload : public Function {
   public:
    ConstructPayload(ProtocolConfig *conf, std::vector<std::vector<Ring>> *payloads, std::vector<Ring> *output)
        : Function(conf, {}, {}, {}, {}, output), payloads(payloads) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        std::vector<Ring> result(size);
        for (size_t i = 0; i < payloads->size(); ++i) {
            auto sum = payloads->at(i)[0];
            for (size_t j = 0; j < size; ++j) {
                sum += payloads->at(i)[j];
            }
            result[i] = sum;
        }
        *output = result;
    }

   private:
    std::vector<std::vector<Ring>> *payloads;
};