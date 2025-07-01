#include <cassert>
#include <random>

#include "../setup/setup.h"
#include "../src/protocol/message_passing.h"
#include "../src/utils/perm.h"
#include "constants.h"

std::vector<Ring> apply(std::vector<Ring> &old_payload, std::vector<Ring> &new_payload) {
    std::vector<Ring> result(old_payload.size());

    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = old_payload[i] + new_payload[i];
    }
    return result;
}

void pre_mp_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, MPPreprocessing &preproc) { return; }

void post_mp_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, MPPreprocessing &preproc) { return; }

void pre_mp_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, MPPreprocessing &preproc, SecretSharedGraph &g) {
    return;
}

void post_mp_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, SecretSharedGraph &g, MPPreprocessing &preproc,
                      std::vector<Ring> &payload) {
    g.payload = payload;
    g.payload_bits = to_bits(payload, sizeof(Ring) * 8);
}

void test_mp(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t BLOCK_SIZE) {
    std::cout << "------ test_mp ------" << std::endl << std::endl;

    json output_data;
    network->init();

    /**
     *      0 == 1
     *       \  //
     *         2
     */

    const size_t n_bits = sizeof(Ring) * 8;
    const size_t n_iterations = 2;
    n = 8;
    Graph g;
    g.size = n;
    g.src = std::vector<Ring>({0, 1, 2, 0, 1, 2, 2, 2});
    g.dst = std::vector<Ring>({0, 1, 2, 1, 0, 1, 0, 1});
    g.isV = std::vector<Ring>({1, 1, 1, 0, 0, 0, 0, 0});
    g.payload = std::vector<Ring>({1, 2, 3, 0, 0, 0, 0, 0});

    std::vector<Ring> weights(n_iterations);

    SecretSharedGraph g_shared = share::random_share_graph(id, rngs, n_bits, g);

    StatsPoint start_pre(*network);
    auto preproc = mp::preprocess(id, rngs, network, g.size, n_bits, n_iterations, pre_mp_preprocess, post_mp_preprocess);
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
        size_t total_comm = 4 * mp_comm_pre(n, n_iterations);
        assert(bytes_sent_pre == total_comm);
    }

    StatsPoint start_online(*network);
    mp::evaluate(id, rngs, network, g.size, n_bits, n_iterations, 3, g_shared, weights, apply, pre_mp_evaluate, post_mp_evaluate, preproc);
    StatsPoint end_online(*network);

    auto rbench = end_online - start_online;
    output_data["benchmarks"].push_back(rbench);

    size_t bytes_sent = 0;
    for (const auto &val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    /* Evaluation communication assertions */
    if (id != D) {
        size_t total_comm = 4 * mp_comm_online(n, n_iterations);
        assert(total_comm == bytes_sent);
    }

    /* Correctness assertions */
    auto res_g = share::reveal_graph(id, network, n_bits, g_shared);

    if (id != D) {
        res_g.print();
        assert(res_g.payload[0] == 18);
        assert(res_g.payload[1] == 21);
        assert(res_g.payload[2] == 3);
        assert(res_g.payload[3] == 0);
        assert(res_g.payload[4] == 0);
        assert(res_g.payload[5] == 0);
        assert(res_g.payload[6] == 0);
        assert(res_g.payload[7] == 0);
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
        setup::run_test(opts, test_mp);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}