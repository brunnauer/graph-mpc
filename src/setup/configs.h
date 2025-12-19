#pragma once

#include <cassert>

#include "../utils/random_generators.h"
#include "../utils/types.h"

struct ProtocolConfig {
    Party id;
    size_t size, nodes, depth, bits;
    RandomGenerators rngs;
    bool ssd;
    std::vector<Ring> weights;
};

struct NetworkConfig {
    int id;
    size_t n_parties, n_clients, BLOCK_SIZE;
    int port;
    std::vector<std::string> IP;
    std::string certificate_path, private_key_path, trusted_cert_path;
    bool localhost;
};

struct BenchmarkConfig {
    std::string input_file, save_file;
    size_t repeat;
    bool save_output;
};
