#pragma once

#include "circuit.h"

class Storage {
   public:
    Storage(ProtocolConfig &conf, Circuit *circ) : size(conf.size), ssd(conf.ssd), unshuffles_idx(0) {
        if (ssd) {
            shuffles.resize(circ->n_shuffles);  // Shuffles need to be stored in RAM either way

            for (size_t i = 0; i < circ->n_shuffles; ++i) {
                const std::string filename = "shuffle_" + std::to_string(i) + "_" + std::to_string(conf.id) + ".bin";
                shuffles_disk.emplace_back(filename);
            }

            unshuffles_disk = FileWriter("unshuffle_" + std::to_string(conf.id) + ".bin");

            for (size_t i = 0; i < circ->n_mults; ++i) {
                triples_disk.emplace_back("triples_" + std::to_string(i) + "_" + std::to_string(conf.id) + ".bin");
            }
        } else {
            for (size_t i = 0; i < circ->n_shuffles; ++i) {
                shuffles.push_back(std::make_shared<ShufflePre>());
            }
            unshuffles.resize(circ->n_unshuffles);
            triples_a.resize(circ->n_mults);
            triples_b.resize(circ->n_mults);
            triples_c.resize(circ->n_mults);
        }
    }

    ~Storage() {
        if (ssd) {
            for (auto &disk : shuffles_disk) {
                std::filesystem::remove(disk.name());
            }

            std::filesystem::remove(unshuffles_disk.name());

            for (auto &disk : triples_disk) {
                std::filesystem::remove(disk.name());
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
        if (ssd) {
            auto perm_share = shuffles_disk[shuffle_idx].read_shuffle(size);
            shuffles[shuffle_idx] = std::make_shared<ShufflePre>(perm_share);
        }
        return shuffles[shuffle_idx];
    }

    void store_unshuffle(std::vector<Ring> &unshuffle) {
        if (ssd) {
            unshuffles_disk.write(unshuffle);
            unshuffle.clear();
        } else {
            unshuffles[unshuffles_idx] = unshuffle;
            unshuffles_idx++;
            if (unshuffles_idx == unshuffles.size()) unshuffles_idx = 0;
        }
    }

    std::vector<Ring> load_unshuffle() {
        if (ssd) {
            return unshuffles_disk.read(size);  // Unshuffle preprocessing is always of size n
        } else {
            auto unshuffle = unshuffles[unshuffles_idx];
            unshuffles_idx++;
            if (unshuffles_idx == unshuffles.size()) unshuffles_idx = 0;
            return unshuffle;
        }
    }

    void store_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c, size_t mul_idx) {
        if (ssd) {
            triples_disk[mul_idx].write(a);
            triples_disk[mul_idx].write(b);
            triples_disk[mul_idx].write(c);
        } else {
            triples_a[mul_idx] = a;
            triples_b[mul_idx] = b;
            triples_c[mul_idx] = c;
        }
    }

    /* TODO: triples are sometimes of size (n-1), maybe write triples in lines and read lines irrespective of parameter size */
    void load_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c, size_t mul_idx) {
        if (ssd) {
            a = triples_disk[mul_idx].read(size);
            b = triples_disk[mul_idx].read(size);
            c = triples_disk[mul_idx].read(size);
        } else {
            a = triples_a[mul_idx];
            b = triples_b[mul_idx];
            c = triples_c[mul_idx];
        }
    }

    void reset() {
        if (ssd) {  // Clear all files
        } else {
            for (auto &shuffle : shuffles) {
                shuffle->pi_0.perm_vec.clear();
                shuffle->pi_1.perm_vec.clear();
                shuffle->pi_0_p.perm_vec.clear();
                shuffle->pi_1_p.perm_vec.clear();
                shuffle->B.clear();
                shuffle->R.clear();
                shuffle->has_pi_0 = false;
                shuffle->has_pi_1 = false;
                shuffle->has_pi_0_p = false;
                shuffle->has_pi_1_p = false;
                shuffle->has_R = false;
                shuffle->preprocessed = false;
            }
            for (auto &unshuffle : unshuffles) {
                unshuffle.clear();
            }
            for (auto &triple : triples_a) {
                triple.clear();
            }
            for (auto &triple : triples_b) {
                triple.clear();
            }
            for (auto &triple : triples_c) {
                triple.clear();
            }
        }
    }

   private:
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
