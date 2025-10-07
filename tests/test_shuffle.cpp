#include <algorithm>
#include <cassert>

#include "../src/graphmpc/shuffler.h"
#include "../src/setup/cmdline.h"
#include "../src/utils/sharing.h"

void test_shuffle(bpo::variables_map &opts) {
    Party id = (Party)opts["pid"].as<size_t>();
    auto network = setup::setupNetwork(opts);
    auto rngs = setup::setupRNGs(opts);

    std::vector<Ring> test_vector = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto test_vector_shared = share::random_share_secret_vec_2P(id, rngs, test_vector);

    Shuffler shuffle(id, rngs, network, test_vector.size(), false);
    shuffle.push_shuffle();
    auto shuffled_vector = shuffle.shuffle(test_vector_shared);

    auto result = share::reveal_vec(id, network, shuffled_vector);
    setup::print_vec(result);
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptionsTest());

    bpo::options_description cmdline("Test for the secure computation of the Reach Score.");
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