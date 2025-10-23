#include "storage.h"

Storage::Storage(ProtocolConfig &conf, Circuit *circ)
    : preproc_disk("preproc_" + std::to_string(conf.id) + ".bin"), id(conf.id), size(conf.size), ssd(conf.ssd) {
    if (id == D || id == P0) pi_0.resize(circ->shuffle_idx + 1);
    if (id == D || id == P1) pi_1.resize(circ->shuffle_idx + 1);
    if (id == P0) pi_0_p.resize(circ->shuffle_idx + 1);
    if (id == P1) pi_1_p.resize(circ->shuffle_idx + 1);

    if (id == D) {
        B[P0].reserve(size * circ->n_shuffles);
        B[P1].reserve(size * circ->n_shuffles);
    }

    preprocessed.resize(circ->shuffle_idx + 1);

    if (ssd) {
        for (size_t i = 0; i < circ->n_mults; ++i) {
            triples_disk.emplace_back("triples_" + std::to_string(i) + "_" + std::to_string(conf.id) + ".bin");
        }
    } else {
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
        std::filesystem::remove(preproc_disk.name());
    }
}

void Storage::read_preproc(std::vector<Ring> &buffer, size_t n_elems) {
    if (ssd) {
        preproc_disk.read(buffer, n_elems);
    } else {
        if (n_elems > preproc[id].size()) {
            throw new std::invalid_argument("Cannot read more preproc values than available.");
        } else {
            buffer.resize(n_elems);
            buffer = {preproc[id].begin(), preproc[id].begin() + n_elems};

            /* Delete read values from buffer */
            preproc[id].erase(preproc[id].begin(), preproc[id].begin() + n_elems);
        }
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
            std::filesystem::remove(disk.name());
        }
        std::filesystem::remove(preproc_disk.name());
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

    preproc.clear();
    B.clear();
}
