#pragma once

#include "function.h"

class Update : public Function {
   public:
    Update(ProtocolConfig *conf, std::vector<Ring> *input, std::vector<Ring> *output) : Function(conf, {}, {}, input, output) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override { output = input; }
};