#include <algorithm>
#include <cassert>

#include "../algorithms/pi_r.h"
#include "../src/utils/comm.h"
#include "test.h"

class TestPiR : public Test {
   public:
    TestPiR(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> network) : Test(conf, network) {}

    Circuit *create_circuit() override {
        auto circ = new PiRCircuit(conf);
        return circ;
    }

    Graph create_graph() override {
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
        std::vector<std::vector<Ring>> data_parallel(conf.nodes);

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
            g.add_list_entry(4, 4, 1);
            g.add_list_entry(3, 3, 1);
            g.add_list_entry(3, 1, 0);
            g.add_list_entry(2, 4, 0);
            g.add_list_entry(4, 2, 0);
            g.add_list_entry(2, 4, 0);
            g.add_list_entry(5, 4, 0);
            g.add_list_entry(4, 2, 0);

            size_t idx = 0;
            for (size_t i = 0; i < g.size; ++i) {
                if (g.isV[i]) {
                    std::vector<Ring> data(g.size);
                    data[i] = 1;
                    data_parallel[idx] = data;
                    idx++;
                }
            }
        }

        for (size_t i = 0; i < conf.nodes; ++i) {
            if (id == P1) data_parallel[i].resize(conf.size);
            data_parallel[i] = share::random_share_secret_vec_2P(id, conf.rngs, data_parallel[i]);
        }

        Graph g_shared = g.secret_share_parties(conf.id, conf.rngs, network, conf.bits, P0);
        g_shared.init_mp(conf.id);
        g_shared.data_parallel = data_parallel;

        return g_shared;
    }

    void run_assertions(std::vector<Ring> &result) override {
        if (id != D) {
            result = share::reveal_vec(id, network, result);

            print_vec(result);

            assert(result[0] == 4);
            assert(result[1] == 5);
            assert(result[2] == 3);
            assert(result[3] == 4);
            assert(result[4] == 3);

            std::cout << "test_pi_r passed." << std::endl;
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
        const size_t size = 17;
        const size_t nodes = 5;
        const size_t depth = 2;
        const size_t bits = std::ceil(std::log2(nodes + 2));
        auto rngs = setup::setupRNGs(opts);
        bool ssd = true;
        std::vector<Ring> weights(depth);

        ProtocolConfig conf = {id, size, nodes, depth, bits, rngs, ssd, weights};
        auto network = setup::setupNetwork(opts);

        auto test = TestPiR(conf, network);
        test.run();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}