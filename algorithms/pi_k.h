#pragma once

#include "../src/graphmpc/deduplication.h"
#include "../src/graphmpc/mp_protocol.h"

class PiKProtocol : public MPProtocol {
   public:
    PiKProtocol(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> &network) : MPProtocol(conf, network) {}

    virtual void pre_mp_preprocessing(MPPreprocessing &preproc) {
        deduplication_preprocess(id, rngs, network, size, bits, preproc, recv_shuffle, recv_mul, ssd);
    }

    virtual void apply_preprocessing(MPPreprocessing &preproc) {}

    virtual void post_mp_preprocessing(MPPreprocessing &preproc) {
        clip::equals_zero_preprocess(id, rngs, network, nodes, preproc, recv_mul, ssd);
        clip::B2A_preprocess(id, rngs, network, nodes, preproc, recv_mul, ssd);
    }

    virtual void pre_mp_evaluation(MPPreprocessing &preproc, Graph &g) { deduplication_evaluate(id, rngs, network, size, preproc, g, ssd); }

    virtual void apply_evaluation(MPPreprocessing &preproc, Graph &g, std::vector<Ring> &new_data) { g._data = new_data; }

    virtual void post_mp_evaluation(MPPreprocessing &preproc, Graph &g) {}
};