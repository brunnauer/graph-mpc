#include <cassert>

#include "../../setup/setup.h"
#include "../../src/random_generators.h"
#include "../../src/sharing.h"

void test_sharing(const bpo::variables_map &opts) {
    auto vec_size = opts["vec-size"].as<size_t>();

    size_t pid, nP, repeat, threads;
    std::shared_ptr<io::NetIOMP> network = nullptr;
    uint64_t seeds_h[5];
    uint64_t seeds_l[5];
    json output_data;
    bool save_output;
    std::string save_file;

    setup::setupExecution(opts, pid, nP, repeat, threads, network, seeds_h, seeds_l, save_output, save_file);
    output_data["details"] = {{"pid", pid},         {"num_parties", nP}, {"threads", threads},  {"seeds_h", seeds_h},
                              {"seeds_l", seeds_l}, {"repeat", repeat},  {"vec-size", vec_size}};

    std::cout << "--- Details ---\n";
    for (const auto &[key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    RandomGenerators rngs(seeds_h, seeds_l);

    std::vector<Row> share(vec_size);
    std::vector<Row> input_table(vec_size);

    for (size_t i = 0; i < vec_size; ++i) {
        input_table[i] = i;
    }

    Party partner = (pid == P0 ? P1 : P0);
    if (pid == P0) {
        Share::random_share_secret_vec_send(partner, rngs, *network, share, input_table);
    } else if (pid == P1) {
        Share::random_share_secret_vec_recv(partner, *network, share);
    }

    std::cout << "Final share: ";
    for (const auto &elem : share) {
        std::cout << elem << ", ";
    }
    std::cout << std::endl;

    std::vector<Row> reconstructed = Share::reconstruct_vec(partner, *network, share);
    assert(input_table == reconstructed);

    std::cout << "Reconstructed: ";
    for (const auto &elem : reconstructed) {
        std::cout << elem << ", ";
    }
    std::cout << std::endl;
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark a simple test for shuffling and unshuffling");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        test_sharing(opts);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}