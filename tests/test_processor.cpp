#include <cassert>

#include "../setup/setup.h"
#include "../src/processor.h"

void test_processor(const bpo::variables_map &opts) {
    std::cout << "------ test_processor ------" << std::endl << std::endl;
    auto vec_size = opts["vec-size"].as<size_t>();

    size_t pid, nP, repeat, threads, shuffle_num, nodes;
    std::shared_ptr<io::NetIOMP> network = nullptr;
    uint64_t seeds_h[9];
    uint64_t seeds_l[9];
    json output_data;
    bool save_output;
    std::string save_file;

    setup::setupExecution(opts, pid, nP, repeat, threads, shuffle_num, nodes, network, seeds_h, seeds_l, save_output, save_file);
    output_data["details"] = {{"pid", pid},         {"num_parties", nP}, {"threads", threads},  {"seeds_h", seeds_h},
                              {"seeds_l", seeds_l}, {"repeat", repeat},  {"vec-size", vec_size}};

    std::cout << "--- Details ---\n";
    for (const auto &[key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    /* Setting up the input vector */
    std::vector<Ring> input_vector(vec_size);
    for (size_t i = 0; i < vec_size; i++) input_vector[i] = i;

    Party party = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);
    const size_t BLOCK_SIZE = 1000000;
    Processor proc(party, rngs, network, 8, BLOCK_SIZE);

    Graph g;
    g.size = 8;
    g.src = std::vector<Ring>({0, 1, 2, 0, 1, 2, 2, 2});
    g.dst = std::vector<Ring>({0, 1, 2, 1, 0, 1, 0, 1});
    g.isV = std::vector<Ring>({1, 1, 1, 0, 0, 0, 0, 0});
    g.payload = std::vector<Ring>({1, 2, 3, 0, 0, 0, 0, 0});

    SecretSharedGraph g_shared = share::random_share_graph(party, rngs, g);

    proc.set_graph(g_shared);
    proc.run_mp_preprocessing(2);
    proc.run_mp_evaluation(2, 3);

    auto g_new = proc.get_graph();
    auto g_revealed = share::reveal_graph(party, network, BLOCK_SIZE, g_new);

    if (pid != D) {
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

    exit(0);
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark a simple test for shuffling and unshuffling");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        test_processor(opts);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}