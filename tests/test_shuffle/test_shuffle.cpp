#include <cassert>

#include "../../setup/setup.h"
#include "../../src/protocol/shuffle.h"
#include "../../src/utils/sharing.h"

void test_shuffle(const bpo::variables_map &opts) {
    std::cout << "------ test_shuffle ------" << std::endl << std::endl;
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
    std::vector<Row> input_vector(vec_size);
    for (size_t i = 0; i < vec_size; i++) input_vector[i] = i;

    Party party = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);
    ProtocolConfig conf(party, rngs, network, vec_size, 1000000);

    std::vector<Row> share = share::random_share_secret_vec_2P(conf, input_vector);

    /* Protocol run */
    PermShare perm_share_one = shuffle::get_shuffle(conf);
    PermShare perm_share_two = shuffle::get_shuffle(conf);
    PermShare perm_share_merged = shuffle::get_merged_shuffle(conf, perm_share_one, perm_share_two);
    std::vector<Row> shuffle_share = shuffle::shuffle(conf, share, perm_share_one, true);
    std::vector<Row> repeat_share = shuffle::shuffle(conf, share, perm_share_one, false);
    std::vector<Row> unshuffle_share = shuffle::unshuffle(conf, repeat_share, perm_share_one);
    std::vector<Row> new_shuffle_share = shuffle::shuffle(conf, share, perm_share_two, true);
    std::vector<Row> second_unshuffle_share = shuffle::unshuffle(conf, new_shuffle_share, perm_share_two);
    std::vector<Row> merged_share = shuffle::shuffle(conf, share, perm_share_merged, true);

    std::vector<Row> res = share::reveal_vec(conf, shuffle_share);

    if (pid != D) {
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

    auto repeat_res = share::reveal_vec(conf, repeat_share);

    if (pid != D) {
        std::cout << std::endl << "Result of repeat: ";
        for (int i = 0; i < repeat_res.size() - 1; ++i) {
            std::cout << repeat_res[i] << ", ";
        }
        std::cout << repeat_res[res.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        assert(res == repeat_res);
    }

    res = share::reveal_vec(conf, unshuffle_share);

    if (pid != D) {
        std::cout << std::endl << "Result of unshuffle: ";
        for (int i = 0; i < res.size() - 1; ++i) {
            std::cout << res[i] << ", ";
        }
        std::cout << res[res.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        assert(res == input_vector);
    }

    auto new_shuffle = share::reveal_vec(conf, new_shuffle_share);

    if (pid != D) {
        std::cout << std::endl << "Result of new shuffle: ";
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

    res = share::reveal_vec(conf, second_unshuffle_share);

    if (pid != D) {
        std::cout << std::endl << "Result of second unshuffle: ";
        for (int i = 0; i < res.size() - 1; ++i) {
            std::cout << res[i] << ", ";
        }
        std::cout << res[res.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        assert(res == input_vector);
    }

    res = share::reveal_vec(conf, merged_share);

    if (pid != D) {
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