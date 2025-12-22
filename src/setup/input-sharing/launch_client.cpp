#include <chrono>
#include <iostream>
#include <thread>

#include "../cmdline.h"
#include "input_client.h"

void launch_client(std::shared_ptr<io::NetIOMP> network, emp::PRG rng, size_t start_idx, size_t bits, std::string input_file = "") {
    Graph g;
    if (input_file != "") {
        g = Graph::parse(input_file);
    } else {
        /* Optionally define graph here */
    }

    /* Send shares*/
    InputClient client(network, bits);
    client.send_graph(rng, g, start_idx);
    network->sync();
    network->sync();
    network->sync();
    auto result = client.recv_result(g.size);
    setup::print_vec(result);
}

int main(int argc, char **argv) {
    auto prog_opts(setup::clientProgramOptions());

    bpo::options_description cmdline("Launching a client that sends part of the full graph.");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    size_t start_idx, bits;
    std::string input_file;

    auto network = setup::setupNetwork(opts);
    setup::setupClient(opts, start_idx, bits, input_file);

    /* Dummy PRG */
    emp::PRG rng;
    auto seed_block = emp::makeBlock(14132, 68436);
    rng.reseed(&seed_block, 0);

    launch_client(network, rng, start_idx, bits, input_file);
}