#include <cassert>

#include "../setup/comm.h"
#include "../setup/utils.h"
#include "../src/graphmpc/clip.h"
#include "../src/utils/random_generators.h"
#include "../src/utils/sharing.h"

void test_eqz(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file) {
    json output_data;
    auto network = std::make_shared<io::NetIOMP>(net_conf, true);

    std::cout << std::endl << "------ test_eqz ------" << std::endl;

    std::vector<Ring> test_vec(n);
    for (size_t i = 0; i < n; ++i) test_vec[i] = i;
    auto test_vec_share = share::random_share_secret_vec_2P(id, rngs, test_vec);

    MPPreprocessing preproc;
    Party recv = P1;

    if (id != D) network->recv_buffered(D);

    clip::equals_zero_preprocess(id, rngs, network, n, preproc, recv, true);
    clip::B2A_preprocess(id, rngs, network, n, preproc, recv, true);
    if (id == D) network->send_all();

    // network->sync();

    auto res = clip::equals_zero_evaluate(id, rngs, network, preproc, test_vec_share, true);
    res = clip::B2A_evaluate(id, rngs, network, n, preproc, res, true);

    /* Correctness assertions */
    auto result = share::reveal_vec(id, network, res);

    if (id != D) {
        std::cout << "Result of eqz: ";
        setup::print_vec(result);

        for (size_t i = 0; i < n; ++i) {
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
        setup::run_test(opts, test_eqz);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}