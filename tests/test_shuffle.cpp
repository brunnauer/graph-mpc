#include <cassert>
#include <functional>
#include <random>

#include "../setup/utils.h"
#include "../src/graphmpc/shuffle.h"
#include "../src/utils/sharing.h"

void test_shuffle(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file) {
    std::cout << "------ test_shuffle ------" << std::endl << std::endl;
    json output_data;
    std::shared_ptr<io::NetIOMP> network = std::make_shared<io::NetIOMP>(net_conf, true);

    std::vector<Ring> input_vector(n);
    for (size_t i = 0; i < n; i++) input_vector[i] = i;

    auto input_share = share::random_share_secret_vec_2P(id, rngs, input_vector);

    Party recv_larger;
    int coin;
    rngs.rng_D().random_data(&coin, sizeof(int));
    coin %= 2;
    Party recv_larger_msg = (Party)coin;

    if (id != D) network->recv_buffered(D);
    auto perm_share = shuffle::get_shuffle(id, rngs, network, n, recv_larger_msg);
    if (id == D) network->send_all();

    auto shuffled_input = shuffle::shuffle(id, rngs, network, n, perm_share, input_share);
    auto result = share::reveal_vec(id, network, shuffled_input);

    setup::print_vec(result);

    return;
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Run a simple test for shuffling and unshuffling");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_test(opts, test_shuffle);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}