#include <cassert>
#include <random>

#include "../setup/setup.h"
#include "../src/protocol/permute.h"
#include "../src/protocol/sort.h"
#include "../src/utils/perm.h"
#include "../src/utils/sharing.h"
#include "constants.h"

void test_sw_perm(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE) {
    std::cout << "------ test_sw_perm ------" << std::endl << std::endl;
    json output_data;

    /* Setting up the input vector */
    int input_bits[7] = {1, 1, 0, 0, 0, 1, 0};
    std::vector<Ring> bit_vector(n);
    std::vector<Ring> input_vector(n);
    std::vector<Ring> input_share(n);

    for (size_t i = 0; i < n; i++) {
        bit_vector[i] = input_bits[i % 7];
        input_vector[i] = rand() % n;
    }

    std::vector<std::vector<Ring>> bits(sizeof(Ring) * 8);
    std::vector<std::vector<Ring>> bit_shares(sizeof(Ring) * 8);

    for (size_t i = 0; i < sizeof(Ring) * 8; ++i) {
        bits[i].resize(n);
        bit_shares[i].resize(n);
        for (size_t j = 0; j < n; ++j) {
            bits[i][j] = (input_vector[j] & (1 << i)) >> i;
        }
    }

    input_share = share::random_share_secret_vec_2P(id, rngs, input_vector);
    for (size_t i = 0; i < bit_shares.size(); ++i) {
        bit_shares[i] = share::random_share_secret_vec_2P(id, rngs, bits[i]);
    }

    Permutation sort_share = sort::get_sort(id, rngs, network, n, BLOCK_SIZE, bit_shares);
    auto sorted_input_share = permute::apply_perm(id, rngs, network, n, BLOCK_SIZE, sort_share, input_share);

    /* Inverting the bits */
    for (size_t i = 0; i < sizeof(Ring) * 8; ++i) {
        bits[i].resize(n);
        bit_shares[i].resize(n);
        for (size_t j = 0; j < n; ++j) {
            bits[i][j] = 1 - bits[i][j];
        }
    }

    /* Sharing again */
    for (size_t i = 0; i < bit_shares.size(); ++i) {
        bit_shares[i] = share::random_share_secret_vec_2P(id, rngs, bits[i]);
    }

    /* Generating and applying reverse sort */
    Permutation reverse_sort_share = sort::get_sort(id, rngs, network, n, BLOCK_SIZE, bit_shares);

    /* Preprocessing */
    StatsPoint start_pre(*network);
    auto preproc = permute::switch_perm_preprocess(id, rngs, network, n, BLOCK_SIZE);
    StatsPoint end_pre(*network);

    auto preproc_1 = permute::switch_perm_preprocess(id, rngs, network, n, BLOCK_SIZE);

    auto rbench_pre = end_pre - start_pre;
    output_data["benchmarks_pre"].push_back(rbench_pre);
    size_t bytes_sent_pre = 0;
    for (const auto &val : rbench_pre["communication"]) {
        bytes_sent_pre += val.get<int64_t>();
    }

    /* Preprocessing communication assertions */
    if (id == D) {
        /* n_elems * 4 Bytes per element */
        size_t total_comm = 4 * switch_perm_comm_pre(n);
        assert(bytes_sent_pre == total_comm);
    }

    network->sync();

    /* Evaluation*/
    StatsPoint start_online(*network);
    auto reverse_sorted_input_share =
        permute::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, sort_share, reverse_sort_share, preproc, sorted_input_share);
    StatsPoint end_online(*network);

    auto inverse_share = permute::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, reverse_sort_share, sort_share, preproc_1, reverse_sorted_input_share);

    /* Evaluation communication assertions */
    auto rbench = end_online - start_online;
    output_data["benchmarks"].push_back(rbench);

    size_t bytes_sent = 0;
    for (const auto &val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    /* Evaluation communication assertions */
    if (id != D) {
        size_t total_comm = 4 * first_switch_perm_comm_online(n);
        assert(total_comm == bytes_sent);
    }

    /* Correctness assertions */
    std::cout << "Original input_vector: ";
    for (size_t i = 0; i < input_vector.size() - 1; ++i) {
        std::cout << input_vector[i] << ", ";
    }
    std::cout << input_vector[input_vector.size() - 1] << std::endl;

    auto sorted_vector = share::reveal_vec(id, network, BLOCK_SIZE, sorted_input_share);

    if (id != D) {
        std::cout << "Sorted input_vector: ";
        for (size_t i = 0; i < sorted_vector.size() - 1; ++i) {
            std::cout << sorted_vector[i] << ", ";
        }
        std::cout << sorted_vector[input_vector.size() - 1] << std::endl;

        for (size_t i = 0; i < sorted_vector.size() - 1; ++i) {
            assert(sorted_vector[i] <= sorted_vector[i + 1]);
        }

        auto reverse_sorted = share::reveal_vec(id, network, BLOCK_SIZE, reverse_sorted_input_share);

        std::cout << "Switch perm applied: ";
        for (size_t i = 0; i < reverse_sorted.size() - 1; ++i) {
            std::cout << reverse_sorted[i] << ", ";
        }
        std::cout << reverse_sorted[input_vector.size() - 1] << std::endl;

        for (size_t i = 0; i < reverse_sorted.size() - 1; ++i) {
            assert(reverse_sorted[i] >= reverse_sorted[i + 1]);
        }

        auto inverse = share::reveal_vec(id, network, BLOCK_SIZE, inverse_share);

        std::cout << "Inverse switch perm applied: ";
        for (size_t i = 0; i < inverse.size() - 1; ++i) {
            std::cout << inverse[i] << ", ";
        }
        std::cout << inverse[input_vector.size() - 1] << std::endl << std::endl;
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
        setup::run_test(opts, test_sw_perm);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}