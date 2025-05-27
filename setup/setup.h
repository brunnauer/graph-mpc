#pragma once

#include <boost/program_options.hpp>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "../src/io/netmp.h"
#include "utils.h"

namespace bpo = boost::program_options;
using json = nlohmann::json;

namespace setup {
bpo::options_description programOptions();
bpo::variables_map parseOptions(bpo::options_description &cmdline, bpo::options_description &prog_opts, int argc, char *argv[]);
void setupExecution(const bpo::variables_map &opts, size_t &pid, size_t &nP, size_t &repeat, size_t &threads, size_t &shuffle_num, size_t &nodes,
                    std::shared_ptr<io::NetIOMP> &network, uint64_t *seeds_h, uint64_t *seeds_l, bool &save_output, std::string &save_file);
}  // namespace setup