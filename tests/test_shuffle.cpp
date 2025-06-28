#include <cassert>
#include <functional>

#include "../setup/setup.h"
#include "../src/protocol/shuffle.h"
#include "../src/utils/sharing.h"
#include "constants.h"

void test_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE) {
    std::cout << "------ test_shuffle ------" << std::endl << std::endl;
    json output_data;

    /* setting up the input vector */
    std::vector<Ring> input_vector(n);
    for (size_t i = 0; i < n; i++) input_vector[i] = i;

    const size_t n_shuffles = 3;
    const size_t n_repeats = 1;
    const size_t n_unshuffles = 2;
    const size_t n_merged_shuffles = 2;

    std::vector<Ring> share = share::random_share_secret_vec_2P(id, rngs, input_vector);

    /* Preprocessing */
    StatsPoint start_pre(*network);

    ShufflePre perm_share_one = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    ShufflePre perm_share_two = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    std::vector<Ring> unshuffle_B_one = shuffle::get_unshuffle(id, rngs, network, n, BLOCK_SIZE, perm_share_one);
    std::vector<Ring> unshuffle_B_two = shuffle::get_unshuffle(id, rngs, network, n, BLOCK_SIZE, perm_share_two);
    ShufflePre perm_share_merged_one = shuffle::get_merged_shuffle(id, rngs, network, n, BLOCK_SIZE, perm_share_one, perm_share_two);
    ShufflePre perm_share_three = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, false);
    ShufflePre perm_share_merged_two = shuffle::get_merged_shuffle(id, rngs, network, n, BLOCK_SIZE, perm_share_two, perm_share_three);

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
        size_t total_comm = 4 * (n_shuffles * shuffle_comm_pre(n) + n_unshuffles * unshuffle_comm_pre(n) + n_merged_shuffles * merged_shuffle_comm_pre(n));
        assert(bytes_sent_pre == total_comm);
    }

    StatsPoint start_online(*network);

    std::vector<Ring> shuffle_share_one = shuffle::shuffle(id, rngs, network, share, perm_share_one, n, BLOCK_SIZE);
    std::vector<Ring> repeat_share = shuffle::shuffle(id, rngs, network, share, perm_share_one, n, BLOCK_SIZE);
    std::vector<Ring> unshuffle_share_one = shuffle::unshuffle(id, rngs, network, perm_share_one, unshuffle_B_one, repeat_share, n, BLOCK_SIZE);
    std::vector<Ring> shuffle_share_two = shuffle::shuffle(id, rngs, network, share, perm_share_two, n, BLOCK_SIZE);
    std::vector<Ring> unshuffle_share_two = shuffle::unshuffle(id, rngs, network, perm_share_two, unshuffle_B_two, shuffle_share_two, n, BLOCK_SIZE);
    std::vector<Ring> merged_share_one = shuffle::shuffle(id, rngs, network, share, perm_share_merged_one, n, BLOCK_SIZE);
    std::vector<Ring> repeat_merged_share = shuffle::shuffle(id, rngs, network, share, perm_share_merged_one, n, BLOCK_SIZE);
    std::vector<Ring> shuffle_share_three = shuffle::shuffle(id, rngs, network, share, perm_share_three, n, BLOCK_SIZE);
    std::vector<Ring> merged_share_two = shuffle::shuffle(id, rngs, network, share, perm_share_merged_two, n, BLOCK_SIZE);

    StatsPoint end_online(*network);

    auto rbench = end_online - start_online;
    output_data["benchmarks"].push_back(rbench);

    size_t bytes_sent = 0;
    for (const auto &val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    /* Evaluation communication assertions */
    if (id != D) {
        size_t total_comm = 4 * (n_shuffles * shuffle_comm_online(n) + n_repeats * shuffle_comm_online(n) + n_unshuffles * unshuffle_comm_online(n) +
                                 n_merged_shuffles * merged_shuffle_comm_online(n) + shuffle_comm_online(n));
        assert(total_comm == bytes_sent);
    }

    std::vector<Ring> res = share::reveal_vec(id, network, BLOCK_SIZE, shuffle_share_one);

    if (id != D) {
        std::cout << std::endl << "Result of shuffle: ";
        for (int i = 0; i < res.size() - 1; ++i) {
            std::cout << res[i] << ", ";
        }
        std::cout << res[res.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        bool shuffled = false;
        for (size_t i = 0; i < res.size(); ++i) {
            if (res[i] != i) shuffled = shuffled || true;
        }
        assert(shuffled);
    }

    auto repeat_res = share::reveal_vec(id, network, BLOCK_SIZE, repeat_share);

    if (id != D) {
        std::cout << std::endl << "Result of repeat: ";
        for (int i = 0; i < repeat_res.size() - 1; ++i) {
            std::cout << repeat_res[i] << ", ";
        }
        std::cout << repeat_res[res.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        assert(res == repeat_res);
    }

    res = share::reveal_vec(id, network, BLOCK_SIZE, unshuffle_share_one);

    if (id != D) {
        std::cout << std::endl << "Result of unshuffle: ";
        for (int i = 0; i < res.size() - 1; ++i) {
            std::cout << res[i] << ", ";
        }
        std::cout << res[res.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        assert(res == input_vector);
    }

    auto new_shuffle = share::reveal_vec(id, network, BLOCK_SIZE, shuffle_share_two);

    if (id != D) {
        std::cout << std::endl << "Result of second shuffle: ";
        for (int i = 0; i < new_shuffle.size() - 1; ++i) {
            std::cout << new_shuffle[i] << ", ";
        }
        std::cout << new_shuffle[new_shuffle.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        bool shuffled = false;
        for (size_t i = 0; i < new_shuffle.size(); ++i) {
            if (new_shuffle[i] != i) shuffled = shuffled || true;
        }
        assert(shuffled);
        assert(res != new_shuffle);
    }

    res = share::reveal_vec(id, network, BLOCK_SIZE, unshuffle_share_two);

    if (id != D) {
        std::cout << std::endl << "Result of second unshuffle: ";
        for (int i = 0; i < res.size() - 1; ++i) {
            std::cout << res[i] << ", ";
        }
        std::cout << res[res.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        assert(res == input_vector);
    }

    res = share::reveal_vec(id, network, BLOCK_SIZE, merged_share_one);

    if (id != D) {
        std::cout << std::endl << "Result of merged shuffle: ";
        for (int i = 0; i < res.size() - 1; ++i) {
            std::cout << res[i] << ", ";
        }
        std::cout << res[res.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        bool shuffled = false;
        for (size_t i = 0; i < res.size(); ++i) {
            if (res[i] != i) shuffled = shuffled || true;
        }
        assert(shuffled);
    }

    res = share::reveal_vec(id, network, BLOCK_SIZE, repeat_merged_share);

    if (id != D) {
        std::cout << std::endl << "Result of repeating merged shuffle: ";
        for (int i = 0; i < res.size() - 1; ++i) {
            std::cout << res[i] << ", ";
        }
        std::cout << res[res.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        bool shuffled = false;
        for (size_t i = 0; i < res.size(); ++i) {
            if (res[i] != i) shuffled = shuffled || true;
        }
        assert(shuffled);
    }

    auto third_shuffle = share::reveal_vec(id, network, BLOCK_SIZE, shuffle_share_three);

    if (id != D) {
        std::cout << std::endl << "Result of third shuffle: ";
        for (int i = 0; i < third_shuffle.size() - 1; ++i) {
            std::cout << third_shuffle[i] << ", ";
        }
        std::cout << third_shuffle[third_shuffle.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        bool shuffled = false;
        for (size_t i = 0; i < third_shuffle.size(); ++i) {
            if (third_shuffle[i] != i) shuffled = shuffled || true;
        }
        assert(shuffled);
    }

    auto merged_shuffle_two = share::reveal_vec(id, network, BLOCK_SIZE, merged_share_two);

    if (id != D) {
        std::cout << std::endl << "Result of second merged shuffle: ";
        for (int i = 0; i < merged_shuffle_two.size() - 1; ++i) {
            std::cout << merged_shuffle_two[i] << ", ";
        }
        std::cout << merged_shuffle_two[merged_shuffle_two.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        bool shuffled = false;
        for (size_t i = 0; i < merged_shuffle_two.size(); ++i) {
            if (merged_shuffle_two[i] != i) shuffled = shuffled || true;
        }
        assert(shuffled);
    }
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