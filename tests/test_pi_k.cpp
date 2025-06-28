#include <omp.h>

#include <algorithm>
#include <cassert>

#include "../setup/setup.h"
#include "../src/protocol/deduplication.h"
#include "../src/protocol/message_passing.h"
#include "../src/utils/bits.h"
#include "../src/utils/perm.h"
#include "constants.h"

std::vector<Ring> apply(std::vector<Ring> &old_payload, std::vector<Ring> &new_payload) { return new_payload; }

void pre_mp_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, MPPreprocessing &preproc) {
    preproc.deduplication_pre = deduplication_preprocess(id, rngs, network, n, BLOCK_SIZE);
}

void post_mp_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, MPPreprocessing &preproc) {
    return;
}

void pre_mp_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, MPPreprocessing &preproc,
                     SecretSharedGraph &g) {
    deduplication_evaluate(id, rngs, network, n, BLOCK_SIZE, preproc.deduplication_pre, g.src_bits, g.dst_bits, g.src, g.dst);
}

void post_mp_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, SecretSharedGraph &g,
                      MPPreprocessing &preproc, std::vector<Ring> &payload) {
    g.payload = payload;
    g.payload_bits = to_bits(payload, sizeof(Ring) * 8);
}

void test_pi_k(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE) {
    std::cout << "------ test_pi_k ------" << std::endl << std::endl;
    json output_data;

    /*
    Graph instance:
    v1 - v2
    || / ||
    v3   v4

    Which in list form is (kind of random order here for testing):
    (1,1,1)
    (2,2,1)
    (1,2,0)
    (2,1,0)
    (1,3,0)
    (1,3,0)
    (3,1,0)
    (4,4,1)
    (3,3,1)
    (3,1,0)
    (3,2,0)
    (2,3,0)
    (2,4,0)
    (4,2,0)
    (2,4,0)
    (4,2,0)
    */

    Graph g;
    g.add_list_entry(1, 1, 1);
    g.add_list_entry(2, 2, 1);
    g.add_list_entry(1, 2, 0);
    g.add_list_entry(2, 1, 0);
    g.add_list_entry(1, 3, 0);
    g.add_list_entry(1, 3, 0);
    g.add_list_entry(3, 1, 0);
    g.add_list_entry(4, 4, 1);
    g.add_list_entry(3, 3, 1);
    g.add_list_entry(3, 1, 0);
    g.add_list_entry(3, 2, 0);
    g.add_list_entry(2, 3, 0);
    g.add_list_entry(2, 4, 0);
    g.add_list_entry(4, 2, 0);
    g.add_list_entry(2, 4, 0);
    g.add_list_entry(4, 2, 0);

    n = g.size;
    std::vector<Ring> weights = {10000000, 100000, 1000, 1};
    const size_t n_iterations = weights.size();
    const size_t n_bits = sizeof(Ring) * 8;
    const size_t n_vertices = 4;

    if (id != D) g.print();

    SecretSharedGraph g_shared = share::random_share_graph(id, rngs, n_bits, g);

    /* Preprocessing */
    StatsPoint start_pre(*network);
    auto preproc = mp::preprocess(id, rngs, network, n, BLOCK_SIZE, n_bits + 1, n_iterations, pre_mp_preprocess, post_mp_preprocess);
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
        size_t total_comm = 4 * pi_k_comm_pre(n, n_iterations);
        assert(bytes_sent_pre == total_comm);
    }

    StatsPoint start_online(*network);
    mp::evaluate(id, rngs, network, n, BLOCK_SIZE, n_bits + 1, n_iterations, n_vertices, g_shared, weights, apply, pre_mp_evaluate, post_mp_evaluate, preproc);
    StatsPoint end_online(*network);

    auto rbench = end_online - start_online;
    output_data["benchmarks"].push_back(rbench);

    size_t bytes_sent = 0;
    for (const auto &val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    /* Evaluation communication assertions */
    if (id != D) {
        size_t total_comm = 4 * pi_k_comm_online(n, n_iterations);
        assert(total_comm == bytes_sent);
    }

    auto res_g = share::reveal_graph(id, network, BLOCK_SIZE, n_bits, g_shared);

    if (id != D) {
        res_g.print();

        assert(res_g.payload[0] == 20510023);  // 2 of length 1, 5 of length 2, 10 of length 3, 23 of length 4
        assert(res_g.payload[1] == 30513025);  // 3 of length 1, 5 of length 2, 13 of length 3, 25 of length 4
        assert(res_g.payload[2] == 20510023);  // 2 of length 1, 5 of length 2, 10 of length 3, 23 of length 4
        assert(res_g.payload[3] == 10305013);  // 1 of length 1, 3 of length 2,  5 of length 3, 13 of length 4
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
        setup::run_test(opts, test_pi_k);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}