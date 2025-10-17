#include <algorithm>
#include <cassert>

#include "../algorithms/pi_m.h"
#include "../src/utils/comm.h"
#include "test.h"

class TestPiM : public Test {
   public:
    TestPiM(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> network) : Test(conf, network) {}

    Circuit *create_circuit() override {
        auto circ = new PiMCircuit(conf);
        circ->build();
        circ->level_order();
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
        auto test = g_shared.reveal(conf.id, network);
        g_shared.init_mp(conf.id);
        return g_shared;
    }

    void run_assertions(Graph &result) override {
        if (conf.id != D) {
            result.print();

            assert(result.data[0] == 31030096);  // 3 of length 1, 10 of length 2, 30 of length 3,  96 of length 4
            assert(result.data[1] == 41036100);  // 4 of length 1, 10 of length 2, 36 of length 3, 100 of length 4
            assert(result.data[2] == 31030096);  // 3 of length 1, 10 of length 2, 30 of length 3,  96 of length 4
            assert(result.data[3] == 20820072);  // 2 of length 1,  8 of length 2, 20 of length 3,  72 of length 4
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
        Party id = (Party)opts["pid"].as<size_t>();
        const size_t size = 16;
        const size_t nodes = 4;
        const size_t depth = 4;
        const size_t bits = std::ceil(std::log2(nodes + 2));
        auto rngs = setup::setupRNGs(opts);
        bool ssd = false;
        std::vector<Ring> weights = {10000000, 100000, 1000, 1};

        ProtocolConfig conf = {id, size, nodes, depth, bits, rngs, ssd, weights};
        auto network = setup::setupNetwork(opts);

        auto test = TestPiM(conf, network);
        test.run();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}