#include <chrono>
#include <iostream>
#include <thread>

#include "../cmdline.h"
#include "input_client.h"

void launch_client(int id, size_t start_idx, std::string ip_0, std::string ip_1, int port_0, int port_1, std::string input_file, emp::PRG rng, size_t n_bits,
                   std::string &password) {
    Graph g = Graph::parse(input_file);
    std::cout << "Graph loaded from file." << std::endl;

    auto [share_0, share_1] = g.secret_share(rng, n_bits);

    /* Send shares*/
    InputClient client(id, n_bits, password);
    client.connect(ip_0, port_0);
    client.send_graph(share_0, start_idx);

    client.connect(ip_1, port_1);
    client.send_graph(share_1, start_idx);
}

int main(int argc, char **argv) {
    auto prog_opts(setup::clientProgramOptions());

    bpo::options_description cmdline("Launching a client that sends part of the full graph.");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    int id, port_0, port_1;
    size_t start_idx, n_bits;
    std::string ip_0, ip_1, input_file, password;

    setup::setupClient(opts, id, start_idx, ip_0, ip_1, port_0, port_1, input_file, n_bits, password);

    emp::PRG rng;
    auto seed_block = emp::makeBlock(14132, 68436);
    rng.reseed(&seed_block, 0);

    launch_client(id, start_idx, ip_0, ip_1, port_0, port_1, input_file, rng, n_bits, password);
}