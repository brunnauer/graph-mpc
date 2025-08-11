#include <cassert>
#include <functional>

#include "../setup/stats.h"
#include "../setup/utils.h"
#include "../src/io/netmp.h"

void test_network(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file) {
    std::cout << "------ test_network ------" << std::endl << std::endl;
    json output_data;

    /* setting up the input vector */
    std::vector<Ring> input_vector(n);
    for (size_t i = 0; i < n; i++) input_vector[i] = i;

    std::shared_ptr<io::NetIOMP> network = std::make_shared<io::NetIOMP>(net_conf, false);

    switch (id) {
        case D: {
            network->add_send(P0, input_vector);
            network->add_send(P1, input_vector);
            network->send_all();
            network->sync();
            break;
        }
        case P0: {
            size_t n = 10;
            network->send(P1, &n, sizeof(size_t));
            network->recv_buffered(D);
            auto res = network->read(D, n);
            network->sync();
            setup::print_vec(res);
            break;
        }
        case P1: {
            size_t n;
            network->recv(P0, &n, sizeof(size_t));
            std::cout << "Received " << n << " from P0." << std::endl;
            network->recv_buffered(D);
            auto res = network->read(D, n);
            network->sync();
            setup::print_vec(res);
            break;
        }
    }

    return;
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Run a simple test for sending and receiving messages.");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_test(opts, test_network);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}