#pragma once

#include "../message_passing.h"

class PiMProtocol : public ProtocolDef {
   public:
    PiMProtocol(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, size_t n_vertices, size_t n_iterations,
                bool ssd, bool save_output = false, std::string save_file = "")
        : ProtocolDef(id, rngs, network, n, n_bits, n_vertices, n_iterations, ssd, save_output, save_file) {}

    virtual void pre_mp_preprocessing(MPPreprocessing &preproc) {}

    virtual void apply_preprocessing(MPPreprocessing &preproc) {}

    virtual void post_mp_preprocessing(MPPreprocessing &preproc) {}

    virtual void pre_mp_evaluation(MPPreprocessing &preproc, Graph &g) {}

    virtual void apply_evaluation(MPPreprocessing &preproc, Graph &g, std::vector<Ring> &new_data) { g._data = new_data; }

    virtual void post_mp_evaluation(MPPreprocessing &preproc, Graph &g) {}
};