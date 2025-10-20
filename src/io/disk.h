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

    FileWriter(std::string filename) : filename(filename), read_idx(0), _size(0) {
        std::filesystem::remove(filename);
        std::ofstream out(filename, std::ios::binary);
    }

    ~FileWriter() = default;

    void reset_idx() { read_idx = 0; }

    void skip(size_t n_elems) { read_idx += (n_elems * sizeof(Ring)); }

    size_t size() { return _size; }

    const std::string name() { return filename; }

    void write(const std::vector<Ring> &data) {
        std::ofstream out(filename, std::ios::binary | std::ios::app);
        if (!out) throw std::runtime_error("Failed to open file for writing.");
        out.write(reinterpret_cast<const char *>(data.data()), data.size() * sizeof(Ring));
        _size += data.size();
    }

    void overwrite(const std::vector<Ring> &data) {
        std::ofstream out(filename, std::ios::binary);  // Deletes all content!
        if (!out) throw std::runtime_error("Failed to open file for writing.");
        out.write(reinterpret_cast<const char *>(data.data()), data.size() * sizeof(Ring));
        _size = data.size();
    }

    void write_shuffle(ShufflePre &shuffle) {
        std::vector<Ring> log(7);
        if (shuffle.pi_0.not_null()) log[0] = 1;
        if (shuffle.pi_1.not_null()) log[1] = 1;
        if (shuffle.pi_0_p.not_null()) log[2] = 1;
        if (shuffle.pi_1_p.not_null()) log[3] = 1;
        if (shuffle.B.size() > 0) log[4] = 1;
        if (shuffle.R.size() > 0) log[5] = 1;
        log[6] = shuffle.preprocessed;

        if (log[0]) log.insert(log.end(), shuffle.pi_0.perm_vec.begin(), shuffle.pi_0.perm_vec.end());
        if (log[1]) log.insert(log.end(), shuffle.pi_1.perm_vec.begin(), shuffle.pi_1.perm_vec.end());
        if (log[2]) log.insert(log.end(), shuffle.pi_0_p.perm_vec.begin(), shuffle.pi_0_p.perm_vec.end());
        if (log[3]) log.insert(log.end(), shuffle.pi_1_p.perm_vec.begin(), shuffle.pi_1_p.perm_vec.end());
        if (log[4]) log.insert(log.end(), shuffle.B.begin(), shuffle.B.end());
        if (log[5]) log.insert(log.end(), shuffle.R.begin(), shuffle.R.end());

        overwrite(log);
    }

    ShufflePre read_shuffle(size_t size) {
        ShufflePre perm_share;

        std::vector<Ring> log = read(7);
        if (log.size() != 7) return perm_share;  // Empty perm_share

        perm_share.has_pi_0 = (log[0] == 1);
        perm_share.has_pi_1 = (log[1] == 1);
        perm_share.has_pi_0_p = (log[2] == 1);
        perm_share.has_pi_1_p = (log[3] == 1);
        perm_share.has_B = (log[4] == 1);
        perm_share.has_R = (log[5] == 1);
        perm_share.preprocessed = (log[6] == 1);

        if (log[0] == 1) {
            auto pi_0_vec = read(size);
            perm_share.pi_0 = Permutation(pi_0_vec);
        }
        if (log[1] == 1) {
            auto pi_1_vec = read(size);
            perm_share.pi_1 = Permutation(pi_1_vec);
        }
        if (log[2] == 1) {
            auto pi_0_p_vec = read(size);
            perm_share.pi_0_p = Permutation(pi_0_p_vec);
        }
        if (log[3] == 1) {
            auto pi_1_p_vec = read(size);
            perm_share.pi_1_p = Permutation(pi_1_p_vec);
        }
        if (log[4] == 1) {
            perm_share.B = read(size);
        }
        if (log[5] == 1) {
            perm_share.R = read(size);
        }

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
        if (fileSize == 0) return output;  // Empty file, no shuffle to read

        in.seekg(read_idx * sizeof(Ring), std::ios::beg);
        size_t max_elements = (fileSize / sizeof(Ring)) - read_idx;
        size_t elements_to_read = std::min(n_elems, max_elements);

        output.resize(elements_to_read);
        in.read(reinterpret_cast<char *>(output.data()), elements_to_read * sizeof(Ring));
        size_t bytes_read = in.gcount();
        in.close();

        read_idx += (bytes_read / sizeof(Ring));

        if (read_idx >= _size) reset_idx();

        return output;
    }

   private:
    std::string filename;
    size_t read_idx;
    size_t _size;  // n_elems
};
