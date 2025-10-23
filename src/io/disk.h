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

    void write(Ring *data, size_t n_elems) {
        std::ofstream out(filename, std::ios::binary | std::ios::app);
        if (!out) throw std::runtime_error("Failed to open file for writing.");
        out.write(reinterpret_cast<const char *>(data), n_elems * sizeof(Ring));
        _size += n_elems;
    }

    void overwrite(const std::vector<Ring> &data) {
        std::ofstream out(filename, std::ios::binary);  // Deletes all content!
        if (!out) throw std::runtime_error("Failed to open file for writing.");
        out.write(reinterpret_cast<const char *>(data.data()), data.size() * sizeof(Ring));
        _size = data.size();
    }

    void print_content() {
        std::vector<Ring> vec(_size);
        read(vec, _size);
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i != vec.size() - 1) {
                std::cout << vec[i] << ", ";
            } else {
                std::cout << vec[i] << std::endl;
            }
        }
    }

    void read(std::vector<Ring> &buffer, size_t n_elems) {
        std::ifstream in(filename, std::ios::binary);
        if (!in) throw std::runtime_error("Cannot open file");

        in.seekg(0, std::ios::end);
        size_t fileSize = in.tellg();
        if (fileSize == 0) return;  // Empty file, no shuffle to read

        in.seekg(read_idx * sizeof(Ring), std::ios::beg);
        size_t max_elements = (fileSize / sizeof(Ring)) - read_idx;
        size_t elements_to_read = std::min(n_elems, max_elements);

        buffer.resize(elements_to_read);
        in.read(reinterpret_cast<char *>(buffer.data()), elements_to_read * sizeof(Ring));
        size_t bytes_read = in.gcount();
        in.close();

        read_idx += (bytes_read / sizeof(Ring));

        if (read_idx >= _size) reset_idx();
    }

   private:
    std::string filename;
    size_t read_idx;
    size_t _size;  // n_elems
};
