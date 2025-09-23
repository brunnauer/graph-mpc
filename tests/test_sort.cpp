#include <cassert>
#include <random>

#include "../setup/comm.h"
#include "../setup/utils.h"
#include "../src/graphmpc/sort.h"
#include "../src/utils/permutation.h"

void test_sort(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file, Graph &g) {
    std::cout << "------ test_sort ------" << std::endl << std::endl;
    json output_data;
    bool ssd = true;
    auto network = std::make_shared<io::NetIOMP>(net_conf, ssd);

    const size_t n_bits = std::ceil(std::log2(n));

    std::vector<Ring> input_vector(n);
    for (size_t i = 0; i < n; ++i) input_vector[i] = rand() % n;

    std::vector<std::vector<Ring>> bits(n_bits);
    std::vector<std::vector<Ring>> bit_shares(n_bits);

    for (size_t i = 0; i < n_bits; ++i) {
        bits[i].resize(n);
        bit_shares[i].resize(n);
        for (size_t j = 0; j < n; ++j) {
            bits[i][j] = (input_vector[j] & (1 << i)) >> i;
        }
    }

    for (size_t i = 0; i < n_bits; ++i) {
        bit_shares[i] = share::random_share_secret_vec_2P(id, rngs, bits[i]);
    }

    MPPreprocessing preproc;
    Party recv_shuffle = P0;
    Party recv_mul = P0;
    /* Preprocessing */
    StatsPoint start_pre(*network);
    if (id != D) network->recv_buffered(D);
    sort::get_sort_preprocess(id, rngs, network, n, bit_shares.size(), preproc, recv_shuffle, recv_mul, ssd);
    if (id == D) network->send_all();
    StatsPoint end_pre(*network);

    auto rbench_pre = end_pre - start_pre;
    output_data["benchmarks_pre"].push_back(rbench_pre);
    size_t bytes_sent_pre = 0;
    for (const auto &val : rbench_pre["communication"]) {
        bytes_sent_pre += val.get<int64_t>();
    }

    /* Preprocessing communication assertions */
    if (id == D) {
        // size_t total_comm = 4 * SORT_COMM_PRE(id, n, n_bits);
        // assert(bytes_sent_pre == total_comm);
    }
    /* Evaluation */
    StatsPoint start_online(*network);
    auto sort_share = sort::get_sort_evaluate(id, rngs, network, n, preproc, bit_shares, ssd);
    StatsPoint end_online(*network);

    auto rbench = end_online - start_online;
    output_data["benchmarks"].push_back(rbench);

    size_t bytes_sent = 0;
    for (const auto &val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    /* Evaluation communication assertions */
    if (id != D) {
        size_t total_comm = 4 * SORT_COMM_ONLINE(n, n_bits);
        assert(total_comm == bytes_sent);
    }

    Permutation sort = share::reveal_perm(id, network, sort_share);
    auto sorted_vector = sort(input_vector);

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
    }
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Run a simple test for sorting.");
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