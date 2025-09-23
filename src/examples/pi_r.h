#pragma once

#include "../message_passing.h"

class PiRProtocol : public ProtocolDef {
   public:
    PiRProtocol(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, size_t n_vertices, size_t n_iterations,
                bool ssd, bool save_output = false, std::string save_file = "")
        : ProtocolDef(id, rngs, network, n, n_bits, n_vertices, n_iterations, ssd, save_output, save_file) {}

    virtual void pre_mp_preprocessing(MPPreprocessing &preproc) {}

    virtual void apply_preprocessing(MPPreprocessing &preproc) {}

    virtual void post_mp_preprocessing(MPPreprocessing &preproc) {
        clip::equals_zero_preprocess(id, rngs, network, n_vertices, preproc, recv_mul, ssd);
        clip::B2A_preprocess(id, rngs, network, n_vertices, preproc, recv_mul, ssd);
    }

    virtual void pre_mp_evaluation(MPPreprocessing &preproc, Graph &g) {}

    virtual void apply_evaluation(MPPreprocessing &preproc, Graph &g, std::vector<Ring> &new_data) {
        for (size_t i = 0; i < g.size(); ++i) {
            g._data[i] += new_data[i];
        }
    }

    virtual void post_mp_evaluation(MPPreprocessing &preproc, Graph &g) {
        std::vector<Ring> nodes_data(n_vertices);
        std::copy(g._data.begin(), g._data.begin() + nodes_data.size(), nodes_data.begin());

        auto data_p = clip::equals_zero_evaluate(id, rngs, network, preproc, nodes_data, ssd);
        data_p = clip::B2A_evaluate(id, rngs, network, n_vertices, preproc, data_p, ssd);
        data_p = clip::flip(id, data_p);

        std::copy(data_p.begin(), data_p.end(), g._data.begin());
    }
};