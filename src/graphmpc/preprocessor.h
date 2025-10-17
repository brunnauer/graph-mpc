#pragma once

#include "../utils/structs.h"
#include "circuit.h"
#include "storage.h"

class Preprocessor {
   public:
    Preprocessor(ProtocolConfig &conf, Storage *store, std::shared_ptr<io::NetIOMP> network)
        : preproc_disk(conf.id, "preproc_" + std::to_string(conf.id) + ".bin"),
          store(store),
          id(conf.id),
          size(conf.size),
          nodes(conf.nodes),
          depth(conf.depth),
          bits(std::ceil(std::log2(nodes + 2))),
          ssd(conf.ssd),
          rngs(&conf.rngs),
          network(network),
          recv(P0) {}

    void run(Circuit *circ);

   private:
    std::unordered_map<Party, std::vector<Ring>> preproc;
    FileWriter preproc_disk;
    Storage *store;

    Party id;
    size_t size, nodes, depth, bits;
    bool ssd;
    RandomGenerators *rngs;
    std::shared_ptr<io::NetIOMP> network;

    Party recv;

    void preprocess(Circuit *circ);

    std::vector<Ring> read_preproc(size_t n_elems);

    std::vector<Ring> random_share_secret_vec_3P(std::vector<Ring> &secret, bool binary);
};