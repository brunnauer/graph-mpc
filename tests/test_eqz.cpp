#include <cassert>

#include "../setup/setup.h"
#include "../src/protocol/clip.h"
#include "../src/utils/random_generators.h"
#include "../src/utils/sharing.h"

void test_eqz(const bpo::variables_map &opts) {
    std::cout << "------ test_mul ------" << std::endl << std::endl;
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

    Party party = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);
    const size_t BLOCK_SIZE = 100000;

    std::vector<Ring> x_vec(vec_size);
    std::vector<Ring> y_vec(vec_size);

    std::vector<Ring> share_x_vec(vec_size);
    std::vector<Ring> share_y_vec(vec_size);

    for (size_t i = 0; i < vec_size; ++i) {
        x_vec[i] = std::rand() % 2;
    }
    for (size_t i = 0; i < vec_size / 2; ++i) {
        y_vec[i] = 1;
    }
    for (size_t i = vec_size / 2; i < vec_size; ++i) {
        y_vec[i] = 0;
    }

    share_x_vec = share::random_share_secret_vec_2P(party, rngs, x_vec);
    share_y_vec = share::random_share_secret_vec_2P(party, rngs, y_vec);

    std::vector<Ring> vals_to_p1;
    size_t idx = 0;

    if (party == P1) recv_vec(D, network, vec_size, vals_to_p1, BLOCK_SIZE);

    auto triples = mul::preprocess(party, rngs, vals_to_p1, idx, vec_size);

    if (party == D) send_vec(P1, network, vals_to_p1.size(), vals_to_p1, BLOCK_SIZE);

    auto x_y_share = mul::evaluate(party, network, vec_size, BLOCK_SIZE, triples, share_x_vec, share_y_vec);
    auto x_y_revealed = share::reveal_vec(party, network, BLOCK_SIZE, x_y_share);

    if (pid != D) {
        std::cout << "x: ";
        for (auto &elem : x_vec) {
            std::cout << elem << ", ";
        }
        std::cout << std::endl;

        std::cout << "y: ";
        for (auto &elem : y_vec) {
            std::cout << elem << ", ";
        }
        std::cout << std::endl;

        std::cout << "Result of multiplication: ";
        for (auto &elem : x_y_revealed) {
            std::cout << elem << ", ";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl << "------ test_eqz ------" << std::endl;
    std::vector<Ring> test_vec(vec_size);
    for (size_t i = 0; i < vec_size; ++i) test_vec[i] = i;
    auto test_vec_share = share::random_share_secret_vec_2P(party, rngs, test_vec);

    auto eqz_vec = clip::equals_zero(party, rngs, network, BLOCK_SIZE, test_vec_share);
    std::vector<Ring> B2A_vec = clip::B2A(party, rngs, network, BLOCK_SIZE, eqz_vec);

    auto result = share::reveal_vec(party, network, BLOCK_SIZE, B2A_vec);

    if (pid != D) {
        std::cout << "Result of eqz: ";
        for (auto &elem : result) {
            std::cout << elem << ", ";
        }
        std::cout << std::endl;

        for (size_t i = 0; i < vec_size; ++i) {
            if (i == 0)
                assert(result[i] == 1);
            else
                assert(result[i] == 0);
        }
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
        test_eqz(opts);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}