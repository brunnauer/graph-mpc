#include <algorithm>
#include <cassert>

#include "../algorithms/pi_k.h"
#include "../src/utils/comm.h"
#include "test.h"

class TestPiK : public Test {
   public:
    TestPiK(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> network) : Test(conf, network) {}

    Circuit *create_circuit() override {
        auto circ = new PiKCircuit(conf);
        return circ;
    }

    Graph create_graph() override {
        /*
            Graph instance:
            v1 - v2
            || / ||
            v3   v4

            Which in list form is (kind of random order here for testing):
            (1,1,1)
            (2,2,1)
            (1,2,0)
            (2,1,0)
            (1,3,0)
            (1,3,0)
            (3,1,0)
            (4,4,1)
            (3,3,1)
            (3,1,0)
            (3,2,0)
            (2,3,0)
            (2,4,0)
            (4,2,0)
            (2,4,0)
            (4,2,0)
        */

        Graph g;
        g.add_list_entry(1, 1, 1);
        g.add_list_entry(2, 2, 1);
        g.add_list_entry(1, 2, 0);
        g.add_list_entry(2, 1, 0);
        g.add_list_entry(1, 3, 0);
        g.add_list_entry(1, 3, 0);
        g.add_list_entry(3, 1, 0);
        g.add_list_entry(4, 4, 1);
        g.add_list_entry(3, 3, 1);
        g.add_list_entry(3, 1, 0);
        g.add_list_entry(3, 2, 0);
        g.add_list_entry(2, 3, 0);
        g.add_list_entry(2, 4, 0);
        g.add_list_entry(4, 2, 0);
        g.add_list_entry(2, 4, 0);
        g.add_list_entry(4, 2, 0);

        Graph g_shared = g.secret_share_parties(conf.id, conf.rngs, network, conf.bits, P0);
        g_shared.init_mp(conf.id);
        return g_shared;
    }

    void run_assertions(std::vector<Ring> &result, size_t &bytes_sent_pre, size_t &bytes_sent_eval) override {
        if (conf.id == D) {
            size_t expected_pre = 4 * (24 * conf.size * conf.bits + 64 * conf.size - 12 + 8 * conf.size * conf.depth) +
                                  2 * sizeof(size_t);  // one element per party always sent to synchronize vector sizes
            size_t expected_eval = 0;

            assert(bytes_sent_pre == expected_pre);
            assert(bytes_sent_eval == expected_eval);
        } else {
            size_t expected_pre = 0;
            size_t expected_eval = 4 * (18 * conf.size * conf.bits + 58 * conf.size - 24 + 4 * conf.size * conf.depth);
            assert(bytes_sent_pre == expected_pre);
            assert(bytes_sent_eval == expected_eval);
            result = share::reveal_vec(id, network, result);

            print_vec(result);

            assert(result[0] == 20510023);  // 2 of length 1, 5 of length 2, 10 of length 3, 23 of length 4
            assert(result[1] == 30513025);  // 3 of length 1, 5 of length 2, 13 of length 3, 25 of length 4
            assert(result[2] == 20510023);  // 2 of length 1, 5 of length 2, 10 of length 3, 23 of length 4
            assert(result[3] == 10305013);  // 1 of length 1, 3 of length 2,  5 of length 3, 13 of length 4

            std::cout << "test_pi_k passed." << std::endl;
        }
    }
};

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptionsTest());

    bpo::options_description cmdline("Test for the secure computation of the Truncated Katz Score.");
    cmdline.add(prog_opts);
    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        Party id = (Party)opts["pid"].as<size_t>();
        const size_t size = 16;
        const size_t nodes = 4;
        const size_t depth = 4;
        const size_t bits = std::ceil(std::log2(nodes + 2));
        auto rngs = setup::setupRNGs(opts);
        bool ssd = true;
        std::vector<Ring> weights = {10000000, 100000, 1000, 1};

        ProtocolConfig conf = {id, size, nodes, depth, bits, rngs, ssd, weights};
        auto network = setup::setupNetwork(opts);

        auto test = TestPiK(conf, network);
        test.run();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}