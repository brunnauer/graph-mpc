#include "storage.h"

Storage::Storage(ProtocolConfig &conf, Circuit *circ) : id(conf.id), size(conf.size), ssd(conf.ssd) {
    if (id == D || id == P0) pi_0.resize(circ->shuffle_idx + 1);
    if (id == D || id == P1) pi_1.resize(circ->shuffle_idx + 1);
    if (id == P0) pi_0_p.resize(circ->shuffle_idx + 1);
    if (id == P1) pi_1_p.resize(circ->shuffle_idx + 1);

    preprocessed.resize(circ->shuffle_idx + 1);

    if (ssd) {
        if (id == D) {
            preproc_disk.resize(2);
            preproc_disk[P0] = FileWriter("preproc_" + std::to_string(id) + "_P0.bin");
            preproc_disk[P1] = FileWriter("preproc_" + std::to_string(id) + "_P1.bin");
            B_disk.resize(2);
            B_disk[P0] = FileWriter("B_" + std::to_string(id) + "_P0.bin");
            B_disk[P1] = FileWriter("B_" + std::to_string(id) + "_P1.bin");
        }

        if (id == P0) {
            preproc_disk.resize(1);
            preproc_disk[P0] = FileWriter("preproc_" + std::to_string(id) + ".bin");
        }

        if (id == P1) {
            preproc_disk.resize(2);
            preproc_disk[P1] = FileWriter("preproc_" + std::to_string(id) + ".bin");
        }

        for (size_t i = 0; i < circ->n_mults; ++i) {
            triples_disk.emplace_back("triples_" + std::to_string(i) + "_" + std::to_string(conf.id) + ".bin");
        }
    } else {
        preproc.resize(2);

        if (id == D) {
            B.resize(2);
            B[P0].reserve(size * circ->n_shuffles);
            B[P1].reserve(size * circ->n_shuffles);
        }

        triples_a.resize(circ->n_mults);
        triples_b.resize(circ->n_mults);
        triples_c.resize(circ->n_mults);
    }
}

Storage::~Storage() {
    if (ssd) {
        for (auto &disk : triples_disk) {
            std::filesystem::remove(disk.name());
        }
        if (id == D) {
            for (auto &disk : preproc_disk) {
                std::filesystem::remove(disk.name());
            }
        }
        if (id == P0) {
            std::filesystem::remove(preproc_disk[P0].name());
        }
        if (id == P1) {
            std::filesystem::remove(preproc_disk[P1].name());
        }
    }
}

std::vector<Ring> Storage::read_preproc(size_t n_elems) {
    if (ssd) {
        std::vector<Ring> data(n_elems);
        preproc_disk[id].read(data, n_elems);
        return data;
    } else {
        if (n_elems > preproc[id].size()) {
            throw new std::invalid_argument("Cannot read more preproc values than available.");
        } else {
            std::vector<Ring> &buffer = preproc[id];
            std::vector<Ring> data(n_elems);
            data = {buffer.begin() + read_idx, buffer.begin() + read_idx + n_elems};

            read_idx += n_elems;
            if (read_idx >= buffer.size()) read_idx = 0;
            return data;
        }
    }
}

void Storage::store_B(Party dst, std::vector<Ring> &vec) {
    if (ssd) {
        B_disk[dst].write(vec.data(), vec.size());
    } else {
        B[dst].insert(B[dst].end(), vec.begin(), vec.end());
    }
}

void Storage::store_vec(Party dst, std::vector<Ring> &vec) {
    if (ssd) {
        preproc_disk[dst].write(vec.data(), vec.size());
    } else {
        preproc[dst].insert(preproc[dst].end(), vec.begin(), vec.end());
    }
}

void Storage::store_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c, size_t mul_idx) {
    if (ssd) {
        triples_disk[mul_idx].write(a.data(), a.size());
        triples_disk[mul_idx].write(b.data(), b.size());
        triples_disk[mul_idx].write(c.data(), c.size());
    } else {
        triples_a[mul_idx] = a;
        triples_b[mul_idx] = b;
        triples_c[mul_idx] = c;
    }
}

void Storage::load_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c, size_t mul_idx) {
    if (ssd) {
        triples_disk[mul_idx].read(a, size);
        triples_disk[mul_idx].read(b, size);
        triples_disk[mul_idx].read(c, size);
    } else {
        a = triples_a[mul_idx];
        b = triples_b[mul_idx];
        c = triples_c[mul_idx];
    }
}
void Storage::load_triples(std::vector<Ring> &a, std::vector<Ring> &b, std::vector<Ring> &c, size_t mul_idx, size_t triple_size) {
    if (ssd) {
        triples_disk[mul_idx].read(a, triple_size);
        triples_disk[mul_idx].read(b, triple_size);
        triples_disk[mul_idx].read(c, triple_size);
    } else {
        a = triples_a[mul_idx];
        b = triples_b[mul_idx];
        c = triples_c[mul_idx];
    }
}

void Storage::reset() {
    if (ssd) {  // Clear all files
        for (auto &disk : triples_disk) {
            disk.reset();
        }
        if (id == D || id == P0) preproc_disk[P0].reset();

        if (id == D || id == P1) preproc_disk[P1].reset();

        if (id == D) {
            B_disk[P0].reset();
            B_disk[P1].reset();
        }
    } else {
        if (id == D || id == P0) {
            preproc[P0].clear();
        }

        if (id == D || id == P1) {
            preproc[P1].clear();
        }
        if (id == D) {
            B[P0].clear();
            B[P1].clear();
        }
    }

    for (auto &perm : pi_0) {
        perm.perm_vec.clear();
    }
    for (auto &perm : pi_1) {
        perm.perm_vec.clear();
    }
    for (auto &perm : pi_0_p) {
        perm.perm_vec.clear();
    }
    for (auto &perm : pi_1_p) {
        perm.perm_vec.clear();
    }
    for (size_t i = 0; i < preprocessed.size(); ++i) {
        preprocessed[i] = false;
    }
}
