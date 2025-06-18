#include <cassert>

#include "../setup/setup.h"
#include "../src/protocol/clip.h"
#include "../src/utils/random_generators.h"
#include "../src/utils/sharing.h"
#include "constants.h"

void test_eqz(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE) {
    json output_data;

    std::cout << std::endl << "------ test_eqz ------" << std::endl;
    std::vector<Ring> test_vec(n);
    for (size_t i = 0; i < n; ++i) test_vec[i] = i;
    auto test_vec_share = share::random_share_secret_vec_2P(id, rngs, test_vec);

    StatsPoint start_pre(*network);
    auto eqz_triples = clip::equals_zero_preprocess(id, rngs, network, n, BLOCK_SIZE);
    StatsPoint end_pre(*network);

    auto rbench_pre = end_pre - start_pre;
    output_data["benchmarks_pre"].push_back(rbench_pre);
    size_t bytes_sent_pre = 0;
    for (const auto &val : rbench_pre["communication"]) {
        bytes_sent_pre += val.get<int64_t>();
    }

    /* Preprocessing communication assertions */
    if (id == D) {
        /* n_elems * 4 Bytes per element */
        size_t total_comm = 4 * eqz_comm_pre(n);
        assert(bytes_sent_pre == total_comm);
    }

    StatsPoint start_online(*network);
    auto eqz_vec = clip::equals_zero_evaluate(id, rngs, network, BLOCK_SIZE, eqz_triples, test_vec_share);
    StatsPoint end_online(*network);

    auto rbench = end_online - start_online;
    output_data["benchmarks"].push_back(rbench);

    size_t bytes_sent = 0;
    for (const auto &val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    /* Evaluation communication assertions */
    if (id != D) {
        size_t total_comm = 4 * eqz_comm_online(n);
        assert(total_comm == bytes_sent);
    }

    /* Correctness assertions */
    std::vector<Ring> B2A_vec = clip::B2A(id, rngs, network, BLOCK_SIZE, eqz_vec);
    auto result = share::reveal_vec(id, network, BLOCK_SIZE, B2A_vec);

    if (id != D) {
        std::cout << "Result of eqz: ";
        for (auto &elem : result) {
            std::cout << elem << ", ";
        }
        std::cout << std::endl;

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