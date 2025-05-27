#pragma once

#include <fstream>
#include <iostream>

#include "./utils/types.h"

struct Graph {
    std::vector<Row> src;
    std::vector<Row> dst;
    std::vector<Row> isV;
    std::vector<Row> payload;
    size_t size = 0;

    Graph() = default;
    Graph(size_t n_rows) {
        src = std::vector<Row>(n_rows);
        dst = std::vector<Row>(n_rows);
        isV = std::vector<Row>(n_rows);
        payload = std::vector<Row>(n_rows);
        size = n_rows;
    }

    std::vector<std::vector<Row>> src_bits() {
        size_t n_bits = sizeof(Row) * 8;
        std::vector<std::vector<Row>> bits(n_bits);
        for (size_t i = 0; i < n_bits; ++i) {
            bits[i].resize(size);
            for (size_t j = 0; j < size; ++j) {
                bits[i][j] = (src[j] & (1 << i)) >> i;
            }
        }
        return bits;
    }

    std::vector<std::vector<Row>> dst_bits() {
        size_t n_bits = sizeof(Row) * 8;
        std::vector<std::vector<Row>> bits(n_bits);
        for (size_t i = 0; i < n_bits; ++i) {
            bits[i].resize(size);
            for (size_t j = 0; j < size; ++j) {
                bits[i][j] = (dst[j] & (1 << i)) >> i;
            }
        }
        return bits;
    }

    std::vector<std::vector<Row>> payload_bits() {
        size_t n_bits = sizeof(Row) * 8;
        std::vector<std::vector<Row>> bits(n_bits);
        for (size_t i = 0; i < n_bits; ++i) {
            bits[i].resize(size);
            for (size_t j = 0; j < size; ++j) {
                bits[i][j] = (payload[j] & (1 << i)) >> i;
            }
        }
        return bits;
    }

    void print() {
        for (size_t i = 0; i < size; ++i) {
            std::cout << src[i] << " " << dst[i] << " " << isV[i] << " " << payload[i] << std::endl;
        }
        std::cout << std::endl;
    }
};

struct SecretSharedGraph {
    std::vector<std::vector<Row>> src_bits;
    std::vector<std::vector<Row>> dst_bits;
    std::vector<Row> isV_bits;
    std::vector<std::vector<Row>> payload_bits;
    size_t size = 0;

    static std::vector<Row> from_bits(std::vector<std::vector<Row>> &input_bits, size_t n) {
        std::vector<Row> result(n);
        for (size_t i = 0; i < n; ++i) {
            result[i] = 0;
            for (size_t j = 0; j < input_bits.size(); j++) {
                result[i] += input_bits[j][i] << j;
            }
        }
        return result;
    }

    static std::vector<std::vector<Row>> to_bits(std::vector<Row> &input_vec, size_t n_bits) {
        std::vector<std::vector<Row>> bits(n_bits);
        for (size_t i = 0; i < n_bits; ++i) {
            bits[i].resize(input_vec.size());
            for (size_t j = 0; j < input_vec.size(); ++j) {
                bits[i][j] = (input_vec[j] & (1 << i)) >> i;
            }
        }
        return bits;
    }
};

/*
Graph from_file() {
    Graph g;
    std::ifstream g_file("graph.txt");
    std::vector<Row> src_vec;
    std::vector<Row> dst_vec;
    std::vector<Row> payload_vec;
    std::vector<Row> isV_vec;

    int src, dst, payload;
    bool isV;

    while (g_file >> src >> dst >> isV >> payload) {
        src_vec.push_back(src);
        dst_vec.push_back(dst);
        isV_vec.push_back(isV);
        payload_vec.push_back(payload);
    }

    g.src = src_vec;
    g.dst = dst_vec;
    g.isV = isV_vec;
    g.payload = payload_vec;
    return g;
}

void print(Graph &graph) {
}
*/