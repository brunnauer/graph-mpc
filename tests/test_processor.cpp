#include <cassert>

#include "../setup/setup.h"
#include "../src/processor.h"

void test_processor(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE) {
    std::cout << "------ test_processor ------" << std::endl << std::endl;
    json output_data;
    Processor proc(id, rngs, network, 8, BLOCK_SIZE);

    Graph g;
    g.size = 8;
    g.src = std::vector<Ring>({0, 1, 2, 0, 1, 2, 2, 2});
    g.dst = std::vector<Ring>({0, 1, 2, 1, 0, 1, 0, 1});
    g.isV = std::vector<Ring>({1, 1, 1, 0, 0, 0, 0, 0});
    g.payload = std::vector<Ring>({1, 2, 3, 0, 0, 0, 0, 0});

    SecretSharedGraph g_shared = share::random_share_graph(id, rngs, 32, g);

    proc.set_graph(g_shared);
    proc.run_mp_preprocessing(2);
    proc.run_mp_evaluation(2, 3);

    auto g_new = proc.get_graph();
    auto g_revealed = share::reveal_graph(id, network, BLOCK_SIZE, 32, g_new);

    if (id != D) {
        g_revealed.print();

        assert(g_revealed.payload[0] == 1);
        assert(g_revealed.payload[1] == 1);
        assert(g_revealed.payload[2] == 1);
        assert(g_revealed.payload[3] == 0);
        assert(g_revealed.payload[4] == 0);
        assert(g_revealed.payload[5] == 0);
        assert(g_revealed.payload[6] == 0);
        assert(g_revealed.payload[7] == 0);
    }
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark a simple test for shuffling and unshuffling");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_test(opts, test_processor);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}