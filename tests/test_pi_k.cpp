#include <algorithm>
#include <cassert>

#include "../algorithms/pi_k.h"
#include "../src/utils/comm.h"
#include "test.h"

class TestPiK : public Test {
   public:
    TestPiK(bpo::variables_map &opts) : Test(opts) {}

    virtual MPProtocol *create_protocol() {
        bool ssd = false;
        const size_t nodes = 4;
        const size_t depth = 4;
        std::vector<Ring> weights = {10000000, 100000, 1000, 1};
        bits = std::ceil(std::log2(nodes + 2));
        size_t size = 16;

        ProtocolConfig conf = {id, size, nodes, depth, rngs, ssd, weights};
        PiKProtocol *prot = new PiKProtocol(conf, network);

        return prot;
    }

    virtual Graph create_graph() {
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
        if (id == P0) {
            g.add_list_entry(1, 1, 1);
            g.add_list_entry(2, 2, 1);
            g.add_list_entry(1, 2, 0);
            g.add_list_entry(2, 1, 0);
            g.add_list_entry(1, 3, 0);
            g.add_list_entry(1, 3, 0);
            g.add_list_entry(3, 1, 0);
            g.add_list_entry(4, 4, 1);
        }
        if (id == P1) {
            g.add_list_entry(3, 3, 1);
            g.add_list_entry(3, 1, 0);
            g.add_list_entry(3, 2, 0);
            g.add_list_entry(2, 3, 0);
            g.add_list_entry(2, 4, 0);
            g.add_list_entry(4, 2, 0);
            g.add_list_entry(2, 4, 0);
            g.add_list_entry(4, 2, 0);
        }
        return g.share_subgraphs(id, rngs, network, bits);
    }

    virtual void run_assertions(Graph &result) {
        if (id != D) {
            result.print();

            assert(result._data[0] == 20510023);  // 2 of length 1, 5 of length 2, 10 of length 3, 23 of length 4
            assert(result._data[1] == 30513025);  // 3 of length 1, 5 of length 2, 13 of length 3, 25 of length 4
            assert(result._data[2] == 20510023);  // 2 of length 1, 5 of length 2, 10 of length 3, 23 of length 4
            assert(result._data[3] == 10305013);  // 1 of length 1, 3 of length 2,  5 of length 3, 13 of length 4
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
        auto test = TestPiK(opts);
        test.run();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}