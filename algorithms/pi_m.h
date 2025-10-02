#pragma once

#include "../src/graphmpc/mp_protocol.h"

class PiMProtocol : public MPProtocol {
   public:
    PiMProtocol(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> &network) : MPProtocol(conf, network) {}

    virtual void pre_mp_preprocessing(MPPreprocessing &preproc) {}

    virtual void apply_preprocessing(MPPreprocessing &preproc) {}

    virtual void post_mp_preprocessing(MPPreprocessing &preproc) {}

    virtual void pre_mp_evaluation(MPPreprocessing &preproc, Graph &g) {}

    virtual void apply_evaluation(MPPreprocessing &preproc, Graph &g, std::vector<Ring> &new_data) { g._data = new_data; }

    virtual void post_mp_evaluation(MPPreprocessing &preproc, Graph &g) {}
};