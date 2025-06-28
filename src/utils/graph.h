#pragma once

#include <omp.h>

#include <cassert>
#include <vector>

#include "types.h"

struct Graph {
    std::vector<Ring> src;
    std::vector<Ring> dst;
    std::vector<Ring> isV;
    std::vector<Ring> payload;
    size_t size = 0;
    size_t n_vertices = 0;

    Graph() = default;
    Graph(size_t n_rows) {
        src = std::vector<Ring>(n_rows);
        dst = std::vector<Ring>(n_rows);
        isV = std::vector<Ring>(n_rows);
        payload = std::vector<Ring>(n_rows);
        size = n_rows;
    }

    void add_list_entry(Ring _src, Ring _dst, Ring _isV, Ring _payload) {
        src.push_back(_src);
        dst.push_back(_dst);
        isV.push_back(_isV);
        payload.push_back(_payload);
        size++;

        if (_isV) n_vertices++;
    }

    void add_list_entry(Ring _src, Ring _dst, Ring _isV) {
        src.push_back(_src);
        dst.push_back(_dst);
        isV.push_back(_isV);
        payload.push_back(0);
        size++;

        if (_isV) n_vertices++;
    }

    void print() {
        for (size_t i = 0; i < size; ++i) {
            std::cout << src[i] << " " << dst[i] << " " << isV[i] << " " << payload[i] << std::endl;
        }
        std::cout << std::endl;
    }
};

struct SecretSharedGraph {
    std::vector<Ring> src;
    std::vector<Ring> dst;
    std::vector<Ring> isV;
    std::vector<Ring> payload;

    std::vector<std::vector<Ring>> src_bits;
    std::vector<std::vector<Ring>> dst_bits;
    std::vector<Ring> isV_bits;
    std::vector<std::vector<Ring>> payload_bits;

    size_t size = 0;

    SecretSharedGraph() = default;
    SecretSharedGraph(std::vector<std::vector<Ring>> &src_bits, std::vector<std::vector<Ring>> &dst_bits, std::vector<Ring> &isV_bits,
                      std::vector<std::vector<Ring>> &payload_bits)
        : src_bits(src_bits), dst_bits(dst_bits), isV_bits(isV_bits), payload_bits(payload_bits) {
        size_t n_bits = src_bits.size();
        size_t n = isV_bits.size();

        assert(dst_bits.size() == n_bits);
        assert(payload_bits.size() == n_bits);

        for (size_t i = 0; i < n_bits; ++i) {
            assert(src_bits[i].size() == n);
            assert(dst_bits[i].size() == n);
            assert(payload_bits[i].size() == n);
        }

        size = n;
    }
};
