#pragma once

#include "circuit.h"

class Storage {
   public:
    Storage(ProtocolConfig &conf, Circuit *circ);

    ~Storage();

    void store_shuffle(size_t shuffle_idx);

    std::shared_ptr<ShufflePre> load_shuffle(size_t shuffle_idx);

    void store_unshuffle(std::vector<Ring> &unshuffle);

    std::vector<Ring> load_unshuffle();

    void store_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c, size_t mul_idx);

    void load_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c, size_t mul_idx);

    void load_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c, size_t mul_idx, size_t triple_size);

    void reset();

   private:
    Party id;
    size_t size;
    bool ssd;

    std::vector<std::shared_ptr<ShufflePre>> shuffles;
    std::vector<std::vector<Ring>> unshuffles;
    std::vector<std::vector<Ring>> triples_a;
    std::vector<std::vector<Ring>> triples_b;
    std::vector<std::vector<Ring>> triples_c;
    size_t unshuffles_idx;

    std::vector<FileWriter> shuffles_disk;
    FileWriter unshuffles_disk;
    std::vector<FileWriter> triples_disk;
};
