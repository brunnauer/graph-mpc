#pragma once

#include <cassert>

#include "function.h"

class AddWeights : public Function {
   public:
    AddWeights(ProtocolConfig *conf, std::vector<Ring> *data_v, std::vector<Ring> *weights, size_t &iteration)
        : Function(conf, {}, {}, {}, {}), data_v(data_v), weights(weights), nodes(conf->nodes), iteration(iteration) {
        assert(weights->size() >= iteration);
    }

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        if (id == P0 && weights->size() > 0) {
#pragma omp parallel for if (nodes > 10000)
            for (size_t j = 0; j < nodes; ++j) data_v->at(j) += weights->at(weights->size() - 1 - iteration);
        }
    }

   private:
    std::vector<Ring> *weights;
    std::vector<Ring> *data_v;

    size_t nodes;
    size_t iteration;
};