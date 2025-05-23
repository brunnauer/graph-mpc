#include <cassert>
#include <random>

#include "../../setup/setup.h"
#include "../../src/perm.h"
#include "../../src/sharing.h"
#include "../../src/sorting.h"

void test_shuffle(const bpo::variables_map &opts) {
    auto vec_size = opts["vec-size"].as<size_t>();

    size_t pid, nP, repeat, threads, shuffle_num;
    std::shared_ptr<io::NetIOMP> network = nullptr;
    uint64_t seeds_h[5];
    uint64_t seeds_l[5];
    json output_data;
    bool save_output;
    std::string save_file;

    setup::setupExecution(opts, pid, nP, repeat, threads, shuffle_num, network, seeds_h, seeds_l, save_output, save_file);
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

    for (size_t i = 0; i < vec_size; i++) {
        bit_vector[i] = input_bits[i % 7];
        input_vector[i] = rand() % vec_size;
    }

    Party party = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);
    ProtocolConfig conf(party, rngs, network, vec_size, 1000000);

    std::vector<Row> bit_share(vec_size);
    std::vector<std::vector<Row>> bits(sizeof(Row) * 8);
    std::vector<std::vector<Row>> bit_shares(sizeof(Row) * 8);

    for (size_t i = 0; i < sizeof(Row) * 8; ++i) {
        bits[i].resize(vec_size);
        bit_shares[i].resize(vec_size);
        for (size_t j = 0; j < vec_size; ++j) {
            bits[i][j] = (input_vector[j] & (1 << i)) >> i;
        }
    }

    if (pid == 0) {
        share::random_share_secret_vec_send(P1, rngs, *network, bit_share, bit_vector);
        for (size_t i = 0; i < bit_shares.size(); ++i) {
            share::random_share_secret_vec_send(P1, rngs, *network, bit_shares[i], bits[i]);
        }
    } else if (pid == 1) {
        share::random_share_secret_vec_recv(P0, *network, bit_share);
        for (size_t i = 0; i < bit_shares.size(); ++i) {
            share::random_share_secret_vec_recv(P0, *network, bit_shares[i]);
        }
    }

    /* Test if sharing worked correctly */
    auto reveal_one = share::reveal(conf, bit_shares[3]);

    /* Sorting a single bit_vec */
    std::vector<Row> res_share = sort::sort_bit_vec(conf, bit_share);
    std::vector<Row> res = share::reveal(conf, res_share);

    /* Sorting a vector with entries larger than one bit */
    Permutation sort_share = sort::get_sort(conf, bit_shares);
    Permutation sort = share::reveal(conf, sort_share);
    auto sorted_vector = sort(input_vector);

    std::cout << "Sorted bit_vec: ";

    for (size_t i = 0; i < res.size() - 1; ++i) {
        std::cout << res[i] << ", ";
    }
    std::cout << res[res.size() - 1] << std::endl;

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
        test_shuffle(opts);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}