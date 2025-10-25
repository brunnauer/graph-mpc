#pragma once

#include "circuit.h"

class Storage {
   public:
    Storage(ProtocolConfig &conf, Circuit *circ);

    ~Storage();

    void store_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c, size_t mul_idx);

    void load_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c, size_t mul_idx);

    void load_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c, size_t mul_idx, size_t triple_size);

    std::vector<Ring> read_preproc(size_t n_elems);

    void store_B(Party dst, std::vector<Ring> &B);

    void store_vec(Party dst, std::vector<Ring> &vec);

    void reset();

    std::vector<Permutation> pi_0;
    std::vector<Permutation> pi_1;
    std::vector<Permutation> pi_0_p;
    std::vector<Permutation> pi_1_p;
    std::vector<bool> preprocessed;

    std::vector<std::vector<Ring>> preproc;
    std::vector<std::vector<Ring>> B;
    std::vector<FileWriter> preproc_disk;
    std::vector<FileWriter> B_disk;

    size_t read_idx = 0;

   private:
    Party id;
    size_t size;
    bool ssd;

    std::vector<std::vector<Ring>> triples_a;
    std::vector<std::vector<Ring>> triples_b;
    std::vector<std::vector<Ring>> triples_c;

    std::vector<FileWriter> triples_disk;
};
