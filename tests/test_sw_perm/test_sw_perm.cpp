#include <cassert>
#include <random>

#include "../../setup/setup.h"
#include "../../src/protocol/sorting.h"
#include "../../src/utils/perm.h"
#include "../../src/utils/sharing.h"

void test_sw_perm(const bpo::variables_map &opts) {
    std::cout << "------ test_sw_perm ------" << std::endl << std::endl;
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

    /* Setting up the input vector */
    int input_bits[7] = {1, 1, 0, 0, 0, 1, 0};
    std::vector<Row> bit_vector(vec_size);
    std::vector<Row> input_vector(vec_size);
    std::vector<Row> input_share(vec_size);

    for (size_t i = 0; i < vec_size; i++) {
        bit_vector[i] = input_bits[i % 7];
        input_vector[i] = rand() % vec_size;
    }

    Party party = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);
    ProtocolConfig conf(party, rngs, network, vec_size, 1000000);

    std::vector<std::vector<Row>> bits(sizeof(Row) * 8);
    std::vector<std::vector<Row>> bit_shares(sizeof(Row) * 8);

    for (size_t i = 0; i < sizeof(Row) * 8; ++i) {
        bits[i].resize(vec_size);
        bit_shares[i].resize(vec_size);
        for (size_t j = 0; j < vec_size; ++j) {
            bits[i][j] = (input_vector[j] & (1 << i)) >> i;
        }
    }

    input_share = share::random_share_secret_vec_2P(conf, input_vector);
    for (size_t i = 0; i < bit_shares.size(); ++i) {
        bit_shares[i] = share::random_share_secret_vec_2P(conf, bits[i]);
    }

    /* Sorting a vector with entries larger than one bit */
    Permutation sort_share = sort::get_sort(conf, bit_shares);
    auto sorted_input_share = sort::apply_perm(conf, sort_share, input_share);

    /* Inverting the bits */
    for (size_t i = 0; i < sizeof(Row) * 8; ++i) {
        bits[i].resize(vec_size);
        bit_shares[i].resize(vec_size);
        for (size_t j = 0; j < vec_size; ++j) {
            bits[i][j] = 1 - bits[i][j];
        }
    }

    /* Sharing again */
    for (size_t i = 0; i < bit_shares.size(); ++i) {
        bit_shares[i] = share::random_share_secret_vec_2P(conf, bits[i]);
    }

    /* Generating and applying reverse sort */
    Permutation reverse_sort_share = sort::get_sort(conf, bit_shares);
    auto [pi, omega, merged] = sort::switch_perm_preprocess(conf);
    auto [pi_1, omega_1, merged_1] = sort::switch_perm_preprocess(conf);

    auto reverse_sorted_input_share = sort::switch_perm_evaluate(conf, sort_share, reverse_sort_share, pi, omega, merged, sorted_input_share);
    auto inverse_share = sort::switch_perm_evaluate(conf, reverse_sort_share, sort_share, pi_1, omega_1, merged_1, reverse_sorted_input_share);

    std::cout << "Original input_vector: ";
    for (size_t i = 0; i < input_vector.size() - 1; ++i) {
        std::cout << input_vector[i] << ", ";
    }
    std::cout << input_vector[input_vector.size() - 1] << std::endl;

    auto sorted_vector = share::reveal_vec(conf, sorted_input_share);

    if (pid != D) {
        std::cout << "Sorted input_vector: ";
        for (size_t i = 0; i < sorted_vector.size() - 1; ++i) {
            std::cout << sorted_vector[i] << ", ";
        }
        std::cout << sorted_vector[input_vector.size() - 1] << std::endl;

        for (size_t i = 0; i < sorted_vector.size() - 1; ++i) {
            assert(sorted_vector[i] <= sorted_vector[i + 1]);
        }

        auto reverse_sorted = share::reveal_vec(conf, reverse_sorted_input_share);

        std::cout << "Switch perm applied: ";
        for (size_t i = 0; i < reverse_sorted.size() - 1; ++i) {
            std::cout << reverse_sorted[i] << ", ";
        }
        std::cout << reverse_sorted[input_vector.size() - 1] << std::endl;

        for (size_t i = 0; i < reverse_sorted.size() - 1; ++i) {
            assert(reverse_sorted[i] >= reverse_sorted[i + 1]);
        }

        auto inverse = share::reveal_vec(conf, inverse_share);

        std::cout << "Inverse switch perm applied: ";
        for (size_t i = 0; i < inverse.size() - 1; ++i) {
            std::cout << inverse[i] << ", ";
        }
        std::cout << inverse[input_vector.size() - 1] << std::endl;
    }

    exit(0);
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark a simple test for shuffling and unshuffling");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        test_sw_perm(opts);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}