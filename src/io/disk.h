#pragma once

#include <unistd.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../utils/preprocessings.h"
#include "../utils/types.h"

class Disk {
   public:
    Disk() = default;

    Disk(Party id, std::string filename) : id(id), filename(filename), file_idx(0) {}

    ~Disk() { std::filesystem::remove(filename); }

    void write_shuffle(ShufflePre &shuffle) {
        if (shuffle.pi_0.not_null()) {
        }
    }

    void write_triple(std::tuple<Ring, Ring, Ring> &triple) {
        auto [a, b, c] = triple;
        std::vector<Ring> vec = std::vector<Ring>({a, b, c});
        write(vec);
    }

    void write(const std::vector<Ring> &data) {
        std::ofstream out(filename, std::ios::binary | std::ios::app);
        if (!out) throw std::runtime_error("Failed to open file for writing.");

        out.write(reinterpret_cast<const char *>(data.data()), data.size() * sizeof(Ring));
    }

    std::vector<Ring> read(size_t n_elems, const size_t CHUNK_SIZE = -1) {
        std::vector<Ring> output;

        std::ifstream in(filename, std::ios::binary);
        if (!in) throw std::runtime_error("Cannot open file");

        in.seekg(0, std::ios::end);
        size_t fileSize = in.tellg();

        // No more values to read
        if (file_idx >= fileSize) {
            in.close();
            std::ofstream out(filename, std::ios::binary | std::ios::trunc);
            out.close();
            file_idx = 0;
            return {};
        }

        in.seekg(file_idx, std::ios::beg);
        size_t max_elements = (fileSize - file_idx) / sizeof(Ring);
        size_t elements_to_read = std::min(n_elems, max_elements);

        output.resize(elements_to_read);
        in.read(reinterpret_cast<char *>(output.data()), elements_to_read * sizeof(Ring));
        file_idx += in.gcount();  // in.gcount() is in bytes
        in.close();

        // Optional: truncate file when all data is read
        if (file_idx >= fileSize) {
            std::ofstream out(filename, std::ios::binary | std::ios::trunc);
            out.close();
            file_idx = 0;
        }

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
    size_t file_idx;

    std::unordered_map<std::string, size_t> files;
};
