#include <cassert>

#include "../../setup/setup.h"
#include "../../src/sharing.h"
#include "../../src/shuffle.h"

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
    std::vector<Row> input_vector(vec_size);
    for (size_t i = 0; i < vec_size; i++) input_vector[i] = i;

    Party party = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);
    ProtocolConfig conf(party, rngs, network, vec_size, 1000000);
    Shuffle shuffle(conf, shuffle_num);

    std::vector<Row> share(vec_size);

    if (pid == 0) {
        share::random_share_secret_vec_send(P1, rngs, *network, share, input_vector);
    } else if (pid == 1) {
        share::random_share_secret_vec_recv(P0, *network, share);
    }

    for (size_t r = 0; r < repeat; ++r) {
        std::cout << "--- Repetition " << r + 1 << " ---" << std::endl;

        /* Protocol run */
        shuffle.set_input(share);
        shuffle.run();
        std::vector<Row> res = shuffle.reveal();

        /* Printing result */
        if (pid != D) {
            std::cout << std::endl << "Result of shuffle: ";
            for (int i = 0; i < res.size() - 1; ++i) {
                std::cout << res[i] << ", ";
            }
            std::cout << res[res.size() - 1] << std::endl;
            std::cout << std::endl << std::endl;
        }

        /* Assertions */
        if (pid != D) {
            bool shuffled = false;
            for (int i = 0; i < res.size(); ++i) {
                /* Check if vector contains all elements */
                assert(std::find(res.begin(), res.end(), i) != res.end());

                /* Check if vector is shuffled */
                shuffled = shuffled || (res[i] != i);
            }
            assert(shuffled);
        }

        shuffle.set_input(share);
        shuffle.repeat();
        res = shuffle.reveal();

        /* Printing result */
        if (pid != D) {
            std::cout << std::endl << "Result of repeating shuffle: ";
            for (int i = 0; i < res.size() - 1; ++i) {
                std::cout << res[i] << ", ";
            }
            std::cout << res[res.size() - 1] << std::endl;
            std::cout << std::endl << std::endl;
        }

        shuffle.set_input(share);
        shuffle.repeat();
        res = shuffle.reveal();

        /* Printing result */
        if (pid != D) {
            std::cout << std::endl << "Result of repeating shuffle: ";
            for (int i = 0; i < res.size() - 1; ++i) {
                std::cout << res[i] << ", ";
            }
            std::cout << res[res.size() - 1] << std::endl;
            std::cout << std::endl << std::endl;
        }

        shuffle.unshuffle();
        res = shuffle.reveal();

        if (pid != D) {
            std::cout << std::endl << "Result of reversing last shuffle: ";
            for (int i = 0; i < res.size() - 1; ++i) {
                std::cout << res[i] << ", ";
            }
            std::cout << res[res.size() - 1] << std::endl;
            std::cout << std::endl << std::endl;
        }

        shuffle.set_input(share);
        shuffle.run();
        res = shuffle.reveal();

        if (pid != D) {
            std::cout << std::endl << "Result of new shuffle: ";
            for (int i = 0; i < res.size() - 1; ++i) {
                std::cout << res[i] << ", ";
            }
            std::cout << res[res.size() - 1] << std::endl;
            std::cout << std::endl << std::endl;
        }

        shuffle.unshuffle();
        res = shuffle.reveal();

        if (pid != D) {
            std::cout << std::endl << "Result of reversing last shuffle: ";
            for (int i = 0; i < res.size() - 1; ++i) {
                std::cout << res[i] << ", ";
            }
            std::cout << res[res.size() - 1] << std::endl;
            std::cout << std::endl << std::endl;
        }
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