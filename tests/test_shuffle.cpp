#include <algorithm>
#include <cassert>

#include "../src/graphmpc/merged_shuffle.h"
#include "../src/graphmpc/shuffle.h"
#include "../src/graphmpc/shuffle_repeat.h"
#include "../src/graphmpc/unshuffle.h"
#include "../src/setup/cmdline.h"
#include "../src/utils/sharing.h"

void test_shuffle(bpo::variables_map &opts) {
    std::cout << "--- Test Shuffle ---" << std::endl;

    Party id = (Party)opts["pid"].as<size_t>();
    auto conf = setup::setupProtocol(opts);
    auto network = setup::setupNetwork(opts);
    auto rngs = setup::setupRNGs(opts);

    conf.ssd = true;

    Party recv = P0;
    std::unordered_map<Party, std::vector<Ring>> preproc;
    FileWriter shuffles_disk(id, "shuffles_" + std::to_string(id) + ".bin");
    std::vector<Ring> online_vals;
    preproc[P0] = {};
    preproc[P1] = {};

    std::vector<Ring> test_vector = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto test_vector_shared = share::random_share_secret_vec_2P(id, conf.rngs, test_vector);
    std::vector<Ring> pi_shuffled_vector(10);
    std::vector<Ring> omega_shuffled_vector(10);
    std::vector<Ring> repeat_vector(10);
    std::vector<Ring> unshuffle_vector(10);
    std::vector<Ring> merged_shuffled_vector(10);

    ShufflePre pi;
    ShufflePre omega;
    ShufflePre pi_omega;

    Shuffle shuffle(&conf, &preproc, &online_vals, &test_vector_shared, &pi_shuffled_vector, recv, &pi, &shuffles_disk);
    ShuffleRepeat repeat(&conf, &preproc, &online_vals, &test_vector_shared, &repeat_vector, &pi, recv, &shuffles_disk);
    Unshuffle unshuffle(&conf, &preproc, &online_vals, &pi_shuffled_vector, &unshuffle_vector, &pi, recv, &shuffles_disk);
    Shuffle shuffle2(&conf, &preproc, &online_vals, &test_vector_shared, &omega_shuffled_vector, recv, &omega, &shuffles_disk);
    MergedShuffle merged(&conf, &preproc, &online_vals, &test_vector_shared, &merged_shuffled_vector, recv, &pi_omega, &pi, &omega, &shuffles_disk);

    if (id != D) {
        size_t n_recv;
        network->recv(D, &n_recv, sizeof(size_t));
        std::cout << "Receiving " << n_recv << " preprocessing values." << std::endl;
        if (conf.ssd)
            network->recv_vec(D, n_recv, shuffles_disk);
        else
            network->recv_vec(D, n_recv, preproc.at(id));
    }
    shuffle.preprocess();
    shuffle2.preprocess();
    repeat.preprocess();
    unshuffle.preprocess();
    merged.preprocess();
    if (id == D) {
        for (auto &party : {P0, P1}) {
            auto &data_send = preproc.at(party);
            size_t n_send = data_send.size();
            std::cout << "Sending " << n_send << " preprocessing values." << std::endl;
            network->send(party, &n_send, sizeof(size_t));
            network->send_vec(party, n_send, data_send);
        }
        return;
    }

    shuffle.evaluate_send();
    shuffle2.evaluate_send();
    repeat.evaluate_send();
    size_t n_send = 30;
    size_t n_recv = 30;
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, online_vals);
    } else {
        std::vector<Ring> data_recv(n_recv);
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
        online_vals = data_recv;
    }
    shuffle.evaluate_recv();
    shuffle2.evaluate_recv();
    repeat.evaluate_recv();

    // n_send = 10;
    // n_recv = 10;
    // if (id == P0) {
    // network->send_vec(P1, n_send, online_vals);
    // network->recv_vec(P1, n_recv, online_vals);
    //} else {
    // std::vector<Ring> data_recv(n_recv);
    // network->recv_vec(P0, n_recv, data_recv);
    // network->send_vec(P0, n_send, online_vals);
    // online_vals = data_recv;
    //}

    unshuffle.evaluate_send();
    n_send = 10;
    n_recv = 10;
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, online_vals);
    } else {
        std::vector<Ring> data_recv(n_recv);
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
        online_vals = data_recv;
    }
    unshuffle.evaluate_recv();

    merged.evaluate_send();
    n_send = 10;
    n_recv = 10;
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, online_vals);
    } else {
        std::vector<Ring> data_recv(n_recv);
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
        online_vals = data_recv;
    }
    merged.evaluate_recv();

    auto result = share::reveal_vec(id, network, pi_shuffled_vector);
    auto result1 = share::reveal_vec(id, network, omega_shuffled_vector);
    auto result2 = share::reveal_vec(id, network, repeat_vector);
    auto result3 = share::reveal_vec(id, network, unshuffle_vector);
    auto result4 = share::reveal_vec(id, network, merged_shuffled_vector);

    setup::print_vec(result);
    setup::print_vec(result1);
    setup::print_vec(result2);
    setup::print_vec(result3);
    setup::print_vec(result4);
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptionsTest());

    bpo::options_description cmdline("Test for the secure computation of the Reach Score.");
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