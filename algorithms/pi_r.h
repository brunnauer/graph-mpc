#pragma once

#include "../src/graphmpc/clip.h"
#include "../src/graphmpc/mp_protocol.h"

class PiRProtocol : public MPProtocol {
   public:
    PiRProtocol(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> &network) : MPProtocol(conf, network) {}

    virtual void pre_mp_preprocessing(MPPreprocessing &preproc) {}

    virtual void apply_preprocessing(MPPreprocessing &preproc) {}

    virtual void post_mp_preprocessing(MPPreprocessing &preproc) {
        clip::equals_zero_preprocess(id, rngs, network, nodes, preproc, recv_mul, ssd);
        clip::B2A_preprocess(id, rngs, network, nodes, preproc, recv_mul, ssd);
    }

    virtual void pre_mp_evaluation(MPPreprocessing &preproc, Graph &g) {}

    virtual void apply_evaluation(MPPreprocessing &preproc, Graph &g, std::vector<Ring> &new_data) {
        for (size_t i = 0; i < g.size(); ++i) {
            g._data[i] += new_data[i];
        }
    }

    virtual void post_mp_evaluation(MPPreprocessing &preproc, Graph &g) {
        std::vector<Ring> nodes_data(nodes);
        std::copy(g._data.begin(), g._data.begin() + nodes_data.size(), nodes_data.begin());

        auto data_p = clip::equals_zero_evaluate(id, rngs, network, preproc, nodes_data, ssd);
        data_p = clip::B2A_evaluate(id, rngs, network, nodes, preproc, data_p, ssd);
        data_p = clip::flip(id, data_p);

        std::copy(data_p.begin(), data_p.end(), g._data.begin());
    }
};