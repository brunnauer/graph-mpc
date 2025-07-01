#include <omp.h>

#include <algorithm>
#include <cassert>

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

void post_mp_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, MPPreprocessing &preproc) {
    preproc.eqz_triples = clip::equals_zero_preprocess(id, rngs, network, n);
    preproc.B2A_triples = clip::B2A_preprocess(id, rngs, network, n);
}

void pre_mp_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, MPPreprocessing &preproc, SecretSharedGraph &g) {
    return;
}

void post_mp_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, SecretSharedGraph &g, MPPreprocessing &preproc,
                      std::vector<Ring> &payload) {
    std::vector<Ring> payload_v_eqz = clip::equals_zero_evaluate(id, rngs, network, preproc.eqz_triples, payload);
    std::vector<Ring> payload_v_B2A = clip::B2A_evaluate(id, rngs, network, preproc.B2A_triples, payload_v_eqz);
    auto payload_v_flip = clip::flip(id, payload_v_B2A);
    g.payload = payload_v_flip;
    g.payload_bits = to_bits(payload_v_flip, sizeof(Ring) * 8);
}

void test_bfs(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t BLOCK_SIZE) {
    std::cout << "------ test_bfs ------" << std::endl << std::endl;
    json output_data;
    network->init();
    /**
     *                      10        8
     *                      /\     /\   /\
     *                      |      |     |
     *     0 <=> 6 -> 5 ->  4  <=> 2 <=> 1 <- 7
     *                     / /\
     *                    \/  \
     *                    9    3
     */

    Graph g;
    g.size = 25;
    g.src = std::vector<Ring>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 1, 1, 2, 2, 2, 3, 4, 4, 4, 5, 6, 6, 7});
    g.dst = std::vector<Ring>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 6, 2, 8, 1, 4, 8, 4, 2, 9, 10, 4, 0, 5, 1});
    g.isV = std::vector<Ring>({1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    g.payload = std::vector<Ring>({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    n = 25;

    const size_t n_bits = sizeof(Ring) * 8;
    const size_t n_iterations = 4;
    std::vector<Ring> weights(n_iterations);

    if (id != D) g.print();

    SecretSharedGraph g_shared = share::random_share_graph(id, rngs, n_bits, g);

    /* Preprocessing */
    StatsPoint start_pre(*network);
    auto preproc = mp::preprocess(id, rngs, network, g.size, n_bits, 4, pre_mp_preprocess, post_mp_preprocess);
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
        size_t total_comm = 4 * bfs_comm_pre(n, n_iterations);
        assert(bytes_sent_pre == total_comm);
    }

    StatsPoint start_online(*network);
    mp::evaluate(id, rngs, network, g.size, n_bits, n_iterations, 11, g_shared, weights, apply, pre_mp_evaluate, post_mp_evaluate, preproc);
    StatsPoint end_online(*network);

    auto rbench = end_online - start_online;
    output_data["benchmarks"].push_back(rbench);

    size_t bytes_sent = 0;
    for (const auto &val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    /* Evaluation communication assertions */
    if (id != D) {
        size_t total_comm = 4 * bfs_comm_online(n, n_iterations);
        assert(total_comm == bytes_sent);
    }

    auto res_g = share::reveal_graph(id, network, n_bits, g_shared);

    if (id != D) res_g.print();

    if (id != D) {
        /* Source node should still be discovered */
        assert(res_g.payload[0] == 1);
        /* Discovered nodes */
        assert(res_g.payload[2] == 1);
        assert(res_g.payload[4] == 1);
        assert(res_g.payload[5] == 1);
        assert(res_g.payload[6] == 1);
        assert(res_g.payload[9] == 1);
        assert(res_g.payload[10] == 1);
        /* Undiscovered nodes */
        assert(res_g.payload[1] == 0);
        assert(res_g.payload[3] == 0);
        assert(res_g.payload[7] == 0);
        assert(res_g.payload[8] == 0);
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
        setup::run_test(opts, test_bfs);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}