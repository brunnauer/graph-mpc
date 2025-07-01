#pragma once

#include <boost/program_options.hpp>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <nlohmann/json.hpp>

#include "../src/io/network_interface.h"
#include "../src/utils/random_generators.h"
#include "utils.h"

namespace bpo = boost::program_options;
using json = nlohmann::json;

namespace setup {
bpo::options_description programOptions();

bpo::variables_map parseOptions(bpo::options_description &cmdline, bpo::options_description &prog_opts, int argc, char *argv[]);

void setupExecution(const bpo::variables_map &opts, size_t &pid, size_t &nP, size_t &repeat, size_t &threads, size_t &shuffle_num, size_t &nodes,
                    std::shared_ptr<NetworkInterface> &network, uint64_t *seeds_h, uint64_t *seeds_l, bool &save_output, std::string &save_file);

void run_test(const bpo::variables_map &opts,
              std::function<void(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t BLOCK_SIZE)> func);

void run_benchmark(const bpo::variables_map &opts,
                   std::function<void(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t BLOCK_SIZE, size_t repeat,
                                      size_t n_vertices, bool save_output, std::string save_file)>
                       func);
}  // namespace setup