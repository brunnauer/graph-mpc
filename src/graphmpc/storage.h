#pragma once

#include "circuit.h"

class Storage {
   public:
    Storage(ProtocolConfig &conf, Circuit *circ);

    ~Storage();

    void store_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c, size_t mul_idx);

    void load_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c, size_t mul_idx);

    void load_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c, size_t mul_idx, size_t triple_size);

    void reset();

    void read_preproc(std::vector<Ring> &buffer, size_t n_elems);

    std::vector<Permutation> pi_0;
    std::vector<Permutation> pi_1;
    std::vector<Permutation> pi_0_p;
    std::vector<Permutation> pi_1_p;
    std::vector<bool> preprocessed;

    std::unordered_map<Party, std::vector<Ring>> preproc;
    std::unordered_map<Party, std::vector<Ring>> B; /* Dealer saves B vectors extra as they need to be in the back */
    FileWriter preproc_disk;

   private:
    Party id;
    size_t size;
    bool ssd;

    std::vector<std::vector<Ring>> triples_a;
    std::vector<std::vector<Ring>> triples_b;
    std::vector<std::vector<Ring>> triples_c;

    std::vector<FileWriter> triples_disk;
};
