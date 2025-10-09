#pragma once

#include <unistd.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../utils/shuffle_preproc.h"

class FileWriter {
   public:
    FileWriter() = default;

    FileWriter(Party id, std::string filename) : id(id), filename(filename), read_idx(0), _size(0) {}

    ~FileWriter() { std::filesystem::remove(filename); }

    void reset_idx() { read_idx = 0; }

    size_t size() { return _size; }

    void write_shuffle(ShufflePre &shuffle) {
        std::vector<Ring> log(6);
        if (shuffle.pi_0.not_null()) log[0] = 1;
        if (shuffle.pi_1.not_null()) log[1] = 1;
        if (shuffle.pi_0_p.not_null()) log[2] = 1;
        if (shuffle.pi_1_p.not_null()) log[3] = 1;
        if (shuffle.B.size() > 0) log[4] = 1;
        if (shuffle.R.size() > 0) log[5] = 1;

        write_vec(log);

        if (shuffle.pi_0.not_null()) write_vec(shuffle.pi_0.perm_vec);
        if (shuffle.pi_1.not_null()) write_vec(shuffle.pi_1.perm_vec);
        if (shuffle.pi_0_p.not_null()) write_vec(shuffle.pi_0_p.perm_vec);
        if (shuffle.pi_1_p.not_null()) write_vec(shuffle.pi_1_p.perm_vec);
        if (shuffle.B.size() > 0) write_vec(shuffle.B);
        if (shuffle.R.size() > 0) write_vec(shuffle.R);
    }

    void write_vec(const std::vector<Ring> &data) {
        std::ofstream out(filename, std::ios::binary | std::ios::app);
        if (!out) throw std::runtime_error("Failed to open file for writing.");
        out.write(reinterpret_cast<const char *>(data.data()), data.size() * sizeof(Ring));
        _size += data.size();
    }

    ShufflePre read_shuffle(size_t n, bool repeat = false) {
        ShufflePre perm_share;

        std::vector<Ring> log = read(6);
        size_t n_read = 6;

        if (log[0] == 1) {
            auto pi_0_vec = read(n);
            perm_share.pi_0 = Permutation(pi_0_vec);
            n_read += n;
        }
        if (log[1] == 1) {
            auto pi_1_vec = read(n);
            perm_share.pi_1 = Permutation(pi_1_vec);
            n_read += n;
        }
        if (log[2] == 1) {
            auto pi_0_p_vec = read(n);
            perm_share.pi_0_p = Permutation(pi_0_p_vec);
            n_read += n;
        }
        if (log[3] == 1) {
            auto pi_1_p_vec = read(n);
            perm_share.pi_1_p = Permutation(pi_1_p_vec);
            n_read += n;
        }
        if (log[4] == 1) {
            perm_share.B = read(n);
            n_read += n;
        }
        if (log[5] == 1) {
            perm_share.R = read(n);
            n_read += n;
        }
        if (repeat) read_idx -= (n_read * sizeof(Ring));

        return perm_share;
    }

    void print_content() {
        auto vec = read(_size);
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i != vec.size() - 1) {
                std::cout << vec[i] << ", ";
            } else {
                std::cout << vec[i] << std::endl;
            }
        }
    }

    std::vector<Ring> read(size_t n_elems) {
        std::vector<Ring> output;

        std::ifstream in(filename, std::ios::binary);
        if (!in) throw std::runtime_error("Cannot open file");

        in.seekg(0, std::ios::end);
        size_t fileSize = in.tellg();

        in.seekg(read_idx, std::ios::beg);
        size_t max_elements = (fileSize - read_idx) / sizeof(Ring);
        size_t elements_to_read = std::min(n_elems, max_elements);

        output.resize(elements_to_read);
        in.read(reinterpret_cast<char *>(output.data()), elements_to_read * sizeof(Ring));
        size_t bytes_read = in.gcount();
        in.close();

        read_idx += bytes_read;

        return output;
    }

    std::vector<std::tuple<Ring, Ring, Ring>> read_triples(size_t n) {
        size_t n_elems = 3 * n;
        std::vector<Ring> elems = read(n_elems);
        std::vector<std::tuple<Ring, Ring, Ring>> triples;

        for (size_t i = 0; i < n; ++i) {
            std::tuple<Ring, Ring, Ring> triple = {elems[3 * i], elems[3 * i + 1], elems[3 * i + 2]};
            triples.push_back(triple);
        }
        return triples;
    }

   private:
    Party id;
    std::string filename;
    size_t read_idx;
    size_t _size;  // n_elems
};
