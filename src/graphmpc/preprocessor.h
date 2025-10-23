#pragma once

#include "../utils/structs.h"
#include "circuit.h"
#include "storage.h"

class Preprocessor {
   public:
    Preprocessor(ProtocolConfig &conf, Storage *store, std::shared_ptr<io::NetIOMP> network)
        : data(store), id(conf.id), size(conf.size), ssd(conf.ssd), rngs(&conf.rngs), network(network), recv(P0) {}

    void run(Circuit *circ);

   private:
    Storage *data;

    Party id;
    size_t size;
    bool ssd;
    RandomGenerators *rngs;
    std::shared_ptr<io::NetIOMP> network;

    Party recv;

    void preprocess(Circuit *circ);

    std::vector<Ring> random_share_secret_vec_3P(std::vector<Ring> &secret, bool binary);
};