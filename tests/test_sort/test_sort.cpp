#include <cassert>

#include "../../setup/setup.h"
#include "../../src/sharing.h"
#include "../../src/sorting.h"

void test_shuffle(const bpo::variables_map &opts) {
    auto vec_size = opts["vec-size"].as<size_t>();

    size_t pid, nP, repeat, threads, shuffle_num;
    std::shared_ptr<io::NetIOMP> network = nullptr;
    uint64_t seeds_h[5];
    uint64_t seeds_l[5];
    json output_data;
    bool save_output;
    std::string save_file;

    setup::setupExecution(opts, pid, nP, repeat, threads, shuffle_num, network, seeds_h, seeds_l, save_output, save_file);
    output_data["details"] = {{"pid", pid},         {"num_parties", nP}, {"threads", threads},  {"seeds_h", seeds_h},
                              {"seeds_l", seeds_l}, {"repeat", repeat},  {"vec-size", vec_size}};

    std::cout << "--- Details ---\n";
    for (const auto &[key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    /* Setting up the input vector */
    int input_bits[7] = {1, 1, 0, 0, 0, 1, 0};
    std::vector<Row> input_vector(vec_size);

    for (size_t i = 0; i < vec_size; i++) input_vector[i] = input_bits[i % 7];

    Party party = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);
    Sort sort(party, input_vector.size(), rngs, network);
    std::vector<Row> share(vec_size);

    if (pid == 0) {
        Share::random_share_secret_vec_send(P1, rngs, *network, share, input_vector);
    } else if (pid == 1) {
        Share::random_share_secret_vec_recv(P0, *network, share);
    }

    /* Protocol run */
    sort.set_input(share);
    sort.sort_iteration();
    std::vector<Row> res = sort.reveal();

    std::cout << "Sorted bit_vec: ";

    for (size_t i = 0; i < res.size() - 1; ++i) {
        std::cout << res[i] << ", ";
    }
    std::cout << res[res.size() - 1] << std::endl;

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
        test_shuffle(opts);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}