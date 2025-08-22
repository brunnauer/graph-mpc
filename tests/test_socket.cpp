#include "../setup/utils.h"
#include "../src/input-sharing/input_client.h"
#include "../src/input-sharing/input_server.h"
#include "../src/utils/graph.h"

void test_socket(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file) {
    if (id == P0) {
        InputServer server(id, std::to_string(4242), 2);  // Server expecting two clients
        server.connect_clients();
        Graph g = server.recv_graph();
        g.print();
    }

    if (id == P1) {  // Client 1
        Graph g;
        g.add_list_entry(0, 0, 1, 10);
        g.add_list_entry(1, 1, 1, 20);
        g.add_list_entry(2, 2, 1, 30);
        g.add_list_entry(0, 1, 0, 0);
        g.add_list_entry(0, 2, 0, 0);
        g.add_list_entry(1, 2, 0, 0);
        g.add_list_entry(2, 0, 0, 0);
        g.print();

        InputClient::Packet pkt;
        pkt.start = 0;
        pkt.end = g.size() * 4;
        pkt.entries = g.serialize();

        InputClient client(id);
        client.connect("localhost", 4242);
        client.send_graph(g);
    }

    if (id == D) {  // Client 2
        Graph g;
        g.add_list_entry(0, 0, 1, 10);
        g.add_list_entry(1, 1, 1, 20);
        g.add_list_entry(2, 2, 1, 30);
        g.add_list_entry(0, 1, 0, 0);
        g.add_list_entry(0, 2, 0, 0);
        g.add_list_entry(1, 2, 0, 0);
        g.add_list_entry(2, 0, 0, 0);
        g.print();

        InputClient::Packet pkt;
        pkt.start = 4 * 7;
        pkt.end = pkt.start + g.size() * 4;
        pkt.entries = g.serialize();

        InputClient client(id);
        client.connect("localhost", 4242);
        client.send_graph(g);
    }

    return;
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Run a simple test for sorting.");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_test(opts, test_socket);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}