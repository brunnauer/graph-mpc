#include <cassert>
#include <random>

#include "../setup/setup.h"
#include "../src/protocol/sort.h"
#include "../src/utils/perm.h"
#include "constants.h"

void test_sort(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE) {
    std::cout << "------ test_sort ------" << std::endl << std::endl;
    json output_data;

    const size_t n_bits = sizeof(Ring) * 8;
    const size_t n_compactions = n_bits;
    const size_t n_shuffles = n_bits - 1;
    const size_t n_unshuffles = n_bits - 1;
    const size_t n_repeats = n_bits - 1;
    const size_t n_reveals = n_bits - 1;

    std::vector<Ring> input_vector(n);
    for (size_t i = 0; i < n; ++i) input_vector[i] = rand() % n;

    std::vector<std::vector<Ring>> bits(sizeof(Ring) * 8);
    std::vector<std::vector<Ring>> bit_shares(sizeof(Ring) * 8);

    for (size_t i = 0; i < sizeof(Ring) * 8; ++i) {
        bits[i].resize(n);
        bit_shares[i].resize(n);
        for (size_t j = 0; j < n; ++j) {
            bits[i][j] = (input_vector[j] & (1 << i)) >> i;
        }
    }

    for (size_t i = 0; i < bit_shares.size(); ++i) {
        bit_shares[i] = share::random_share_secret_vec_2P(id, rngs, bits[i]);
    }

    /* Preprocessing */
    StatsPoint start_pre(*network);
    auto sort_preproc = sort::get_sort_preprocess(id, rngs, network, n, BLOCK_SIZE, bit_shares.size());
    StatsPoint end_pre(*network);

    auto rbench_pre = end_pre - start_pre;
    output_data["benchmarks_pre"].push_back(rbench_pre);
    size_t bytes_sent_pre = 0;
    for (const auto &val : rbench_pre["communication"]) {
        bytes_sent_pre += val.get<int64_t>();
    }

    /* Preprocessing communication assertions */
    if (id == D) {
        size_t total_comm = 4 * sort_comm_pre(n, n_bits);
        assert(bytes_sent_pre == total_comm);
    }

    /* Evaluation */
    StatsPoint start_online(*network);
    auto sort_share = sort::get_sort_evaluate(id, rngs, network, n, BLOCK_SIZE, bit_shares, sort_preproc);
    StatsPoint end_online(*network);

    auto rbench = end_online - start_online;
    output_data["benchmarks"].push_back(rbench);

    size_t bytes_sent = 0;
    for (const auto &val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    /* Evaluation communication assertions */
    if (id != D) {
        size_t total_comm = 4 * sort_comm_online(n, n_bits);
        assert(total_comm == bytes_sent);
    }

    for (size_t i = 0; i < sizeof(Ring) * 8; ++i) {
        for (size_t j = 0; j < n; ++j) {
            bits[i][j] = 1 - bits[i][j];
        }
    }

    for (size_t i = 0; i < bit_shares.size(); ++i) {
        bit_shares[i] = share::random_share_secret_vec_2P(id, rngs, bits[i]);
    }
    // auto reverse_sort_preproc = sort::get_sort_preprocess(party, rngs, network, vec_size, BLOCK_SIZE, bit_shares.size());
    // auto reverse_sort_share = sort::get_sort_evaluate(party, rngs, network, vec_size, BLOCK_SIZE, bit_shares, reverse_sort_preproc);

    auto reverse_sort_share = sort::get_sort(id, rngs, network, n, BLOCK_SIZE, bit_shares);

    Permutation reverse_sort = share::reveal_perm(id, network, BLOCK_SIZE, reverse_sort_share);
    Permutation sort = share::reveal_perm(id, network, BLOCK_SIZE, sort_share);

    auto sorted_vector = sort(input_vector);
    auto reverse_sorted_vector = reverse_sort(input_vector);

    if (id != D) {
        std::cout << "Original input_vector: ";

        for (size_t i = 0; i < input_vector.size() - 1; ++i) {
            std::cout << input_vector[i] << ", ";
        }
        std::cout << input_vector[input_vector.size() - 1] << std::endl;

        std::cout << "Sorted input_vector: ";

        for (size_t i = 0; i < sorted_vector.size() - 1; ++i) {
            std::cout << sorted_vector[i] << ", ";
        }
        std::cout << sorted_vector[input_vector.size() - 1] << std::endl;

        for (size_t i = 0; i < sorted_vector.size() - 1; ++i) {
            assert(sorted_vector[i] <= sorted_vector[i + 1]);
        }

        std::cout << "Reverse sorted input_vector: ";

        for (size_t i = 0; i < reverse_sorted_vector.size() - 1; ++i) {
            std::cout << reverse_sorted_vector[i] << ", ";
        }
        std::cout << reverse_sorted_vector[input_vector.size() - 1] << std::endl << std::endl;
    }
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark a simple test for sorting.");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_test(opts, test_sort);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}