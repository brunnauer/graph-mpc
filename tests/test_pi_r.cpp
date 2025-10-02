#include <algorithm>
#include <cassert>

#include "../algorithms/pi_r.h"
#include "../src/utils/comm.h"
#include "test.h"

class TestPiR : public Test {
   public:
    TestPiR(bpo::variables_map &opts) : Test(opts) {}

    virtual MPProtocol *create_protocol() {
        bool ssd = false;
        const size_t nodes = 5;
        const size_t depth = 2;
        std::vector<Ring> weights(depth);
        bits = std::ceil(std::log2(nodes + 2));
        size_t size = 17;

        ProtocolConfig conf = {id, size, nodes, depth, rngs, ssd};
        PiRProtocol *prot = new PiRProtocol(conf, network);

        return prot;
    }

    virtual Graph create_graph() {
        /*
         Graph instance:
         v1 - v2
         ||   ||
         v3   v4 - v5

         Which in list form is (kind of random order here for testing):
         (1,1,1)
         (2,2,1)
         (1,2,0)
         (4,5,0)
         (2,1,0)
         (1,3,0)
         (1,3,0)
         (3,1,0)
         (5,5,1)
         (4,4,1)
         (3,3,1)
         (3,1,0)
         (2,4,0)
         (4,2,0)
         (2,4,0)
         (5,4,0)
         (4,2,0)
         */

        Graph g;
        if (id == P0) {
            g.add_list_entry(1, 1, 1);
            g.add_list_entry(2, 2, 1);
            g.add_list_entry(1, 2, 0);
            g.add_list_entry(4, 5, 0);
            g.add_list_entry(2, 1, 0);
            g.add_list_entry(1, 3, 0);
            g.add_list_entry(1, 3, 0);
            g.add_list_entry(3, 1, 0);
            g.add_list_entry(5, 5, 1);
        }
        if (id == P1) {
            g.add_list_entry(4, 4, 1);
            g.add_list_entry(3, 3, 1);
            g.add_list_entry(3, 1, 0);
            g.add_list_entry(2, 4, 0);
            g.add_list_entry(4, 2, 0);
            g.add_list_entry(2, 4, 0);
            g.add_list_entry(5, 4, 0);
            g.add_list_entry(4, 2, 0);
        }
        Graph g_shared = g.share_subgraphs(id, rngs, network, bits);
        g_shared._n_vertices = 5;  // For helper

        return g_shared;
    }

    virtual void run_assertions(Graph &result) {
        if (id != D) {
            result.print();

            assert(result._data[0] == 4);
            assert(result._data[1] == 5);
            assert(result._data[2] == 3);
            assert(result._data[3] == 4);
            assert(result._data[4] == 3);
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
        auto test = TestPiR(opts);
        test.run(true);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}