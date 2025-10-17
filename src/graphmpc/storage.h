#pragma once

#include "circuit.h"

class Storage {
   public:
    Storage(ProtocolConfig &conf, Circuit *circ) : size(conf.size), ssd(conf.ssd), unshuffles_idx(0), triples_idx(0), triples_read_idx(0) {
        if (!ssd) {
            for (size_t i = 0; i < circ->n_shuffles; ++i) {
                shuffles.push_back(std::make_shared<ShufflePre>());
            }
            unshuffles.resize(circ->n_unshuffles);
            triples_a.resize(circ->n_triples);
            triples_b.resize(circ->n_triples);
            triples_c.resize(circ->n_triples);
        } else {
            triples_disk = FileWriter(conf.id, "triples_" + std::to_string(conf.id) + ".bin");
            for (size_t i = 0; i < circ->n_shuffles; ++i) {
                shuffles_disk[i] = FileWriter(conf.id, "shuffle_" + std::to_string(i) + "_" + std::to_string(conf.id) + ".bin");
            }
        }
    }

    void store_shuffle(size_t shuffle_idx) {
        if (ssd) {
            auto perm_share = *shuffles[shuffle_idx];
            shuffles_disk[shuffle_idx].write_shuffle(perm_share);
            shuffles[shuffle_idx] = nullptr;
        }
    }

    std::shared_ptr<ShufflePre> load_shuffle(size_t shuffle_idx) {
        std::shared_ptr<ShufflePre> shuffle;
        if (!ssd) {
            shuffle = shuffles[shuffle_idx];
        } else {
            auto perm_share = shuffles_disk[shuffle_idx].read_shuffle(size);
            shuffle = std::make_shared<ShufflePre>(perm_share);
        }
        return shuffle;
    }

    void store_unshuffle(std::vector<Ring> &unshuffle) {
        unshuffles[unshuffles_idx] = unshuffle;
        unshuffles_idx++;
        if (unshuffles_idx == unshuffles.size()) unshuffles_idx = 0;
    }

    std::vector<Ring> load_unshuffle() {
        auto unshuffle = unshuffles[unshuffles_idx];
        unshuffles_idx++;
        return unshuffle;
    }

    void store_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c) {
        if (!ssd) {
            for (size_t i = 0; i < a.size(); ++i) {
                triples_a[triples_idx * size + i] = a[i];
            }
            for (size_t i = 0; i < b.size(); ++i) {
                triples_b[triples_idx * size + i] = b[i];
            }
            for (size_t i = 0; i < c.size(); ++i) {
                triples_c[triples_idx * size + i] = c[i];
            }
            triples_idx++;
            if (triples_idx == triples_a.size()) triples_idx = 0;
        } else {
            triples_disk.write_vec(a);
            triples_disk.write_vec(b);
            triples_disk.write_vec(c);
        }
    }

    void load_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c) {
        if (!ssd) {
            a = std::vector<Ring>({triples_a.begin() + (size * triples_read_idx), triples_a.begin() + (size * triples_read_idx) + size});
            b = std::vector<Ring>({triples_b.begin() + (size * triples_read_idx), triples_b.begin() + (size * triples_read_idx) + size});
            c = std::vector<Ring>({triples_c.begin() + (size * triples_read_idx), triples_c.begin() + (size * triples_read_idx) + size});
            triples_idx++;
        } else {
            a = triples_disk.read(size);
            b = triples_disk.read(size);
            c = triples_disk.read(size);
        }
    }

   private:
    size_t size;
    bool ssd;

    std::vector<std::shared_ptr<ShufflePre>> shuffles;
    std::vector<std::vector<Ring>> unshuffles;
    std::vector<Ring> triples_a;
    std::vector<Ring> triples_b;
    std::vector<Ring> triples_c;
    //    size_t shuffles_idx;
    size_t unshuffles_idx;
    size_t triples_idx;
    size_t triples_read_idx;

    FileWriter triples_disk;
    std::vector<FileWriter> shuffles_disk;
};
