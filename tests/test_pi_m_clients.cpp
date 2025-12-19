#include <algorithm>
#include <cassert>

#include "../algorithms/pi_m.h"
#include "../src/utils/comm.h"
#include "test.h"

class TestPiMClients : public Test {
   public:
    TestPiMClients(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> network, Graph &graph) : Test(conf, network) { g = graph; }

    Circuit *create_circuit() override {
        auto circ = new PiMCircuit(conf);
        return circ;
    }

    Graph create_graph() override { return g; }

    void run_assertions(std::vector<Ring> &result, size_t &bytes_sent_pre, size_t &bytes_sent_eval) override {
        if (conf.id == D) {
            size_t expected_pre = 4 * (16 * conf.size * conf.bits + 27 * conf.size + 8 * conf.size * conf.depth) +
                                  2 * sizeof(size_t);  // one element per party always sent to synchronize vector sizes
            size_t expected_eval = 0;

            assert(bytes_sent_pre == expected_pre);
            assert(bytes_sent_eval == expected_eval);
        } else {
            size_t expected_pre = 0;
            size_t expected_eval = 4 * (12 * conf.size * conf.bits + 17 * conf.size + 4 * conf.size * conf.depth);
            assert(bytes_sent_pre == expected_pre);
            assert(bytes_sent_eval == expected_eval);

            result = share::reveal_vec(id, network, result);
            print_vec(result);

            assert(result[0] == 31030096);  // 3 of length 1, 10 of length 2, 30 of length 3,  96 of length 4
            assert(result[1] == 41036100);  // 4 of length 1, 10 of length 2, 36 of length 3, 100 of length 4
            assert(result[2] == 31030096);  // 3 of length 1, 10 of length 2, 30 of length 3,  96 of length 4
            assert(result[3] == 20820072);  // 2 of length 1,  8 of length 2, 20 of length 3,  72 of length 4

            std::cout << "test_pi_m passed." << std::endl;
        }
    }
};

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptionsTest());

    bpo::options_description cmdline("Test for the secure computation of the Reach Score.");
    cmdline.add(prog_opts);
    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        Graph graph;
        auto network = setup::setupNetwork(opts);
        setup::setupServer(opts, graph, network);

        Party id = (Party)opts["pid"].as<int>();
        const size_t size = graph.size;
        const size_t nodes = graph.nodes;
        const size_t depth = 4;
        const size_t bits = std::ceil(std::log2(nodes + 2));
        auto rngs = setup::setupRNGs(opts);
        bool ssd = true;
        std::vector<Ring> weights = {10000000, 100000, 1000, 1};

        ProtocolConfig conf = {id, size, nodes, depth, bits, rngs, ssd, weights};

        auto g_rev = graph.reveal(id, network);
        g_rev.print();

        std::cout << "----- Test Configuration -----" << std::endl;
        std::cout << "Party: " << id << std::endl;
        std::cout << "Size: " << size << std::endl;
        std::cout << "Nodes: " << nodes << std::endl;
        std::cout << "Bits: " << bits << std::endl;
        std::cout << "SSD: " << ssd << std::endl;

        auto test = TestPiMClients(conf, network, graph);
        test.run();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}