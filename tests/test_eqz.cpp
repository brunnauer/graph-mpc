#include <algorithm>
#include <cassert>

#include "../src/graphmpc/equals_zero.h"
#include "../src/setup/cmdline.h"
#include "../src/utils/sharing.h"

void test_eqz(bpo::variables_map &opts) {
    std::cout << "--- Test EqualsZero ---" << std::endl;

    Party id = (Party)opts["pid"].as<size_t>();
    auto conf = setup::setupProtocol(opts);
    auto network = setup::setupNetwork(opts);
    auto rngs = setup::setupRNGs(opts);
    conf.ssd = true;

    Party recv = P0;
    size_t size = 10;
    std::unordered_map<Party, std::vector<Ring>> preproc;
    std::vector<Ring> online_vals;
    preproc[P0] = {};
    preproc[P1] = {};

    FileWriter triples_disk(id, "triples_" + std::to_string(id) + ".bin");
    FileWriter preproc_disk(id, "triples_" + std::to_string(id) + ".bin");
    std::vector<Ring> test_vector = {0, 1, 1, 1, 1, 1, 1, 1, 1, 0};
    auto test_vector_shared = share::random_share_secret_vec_2P(id, conf.rngs, test_vector);
    std::vector<Ring> result(10);

    EQZ eqz0(&conf, &preproc, &online_vals, &test_vector_shared, &result, recv, size, 0, &preproc_disk, &triples_disk);
    EQZ eqz1(&conf, &preproc, &online_vals, &result, &result, recv, size, 1, &preproc_disk, &triples_disk);
    EQZ eqz2(&conf, &preproc, &online_vals, &result, &result, recv, size, 2, &preproc_disk, &triples_disk);
    EQZ eqz3(&conf, &preproc, &online_vals, &result, &result, recv, size, 3, &preproc_disk, &triples_disk);
    EQZ eqz4(&conf, &preproc, &online_vals, &result, &result, recv, size, 4, &preproc_disk, &triples_disk);

    if (id != D) {
        size_t n_recv;
        network->recv(D, &n_recv, sizeof(size_t));
        std::cout << "Receiving " << n_recv << " preprocessing values." << std::endl;
        // network->recv_vec(D, n_recv, preproc.at(id));
        network->recv_vec(D, n_recv, preproc_disk);
    }
    eqz0.preprocess();
    eqz1.preprocess();
    eqz2.preprocess();
    eqz3.preprocess();
    eqz4.preprocess();
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

    /* EQZ Layer 1 */
    eqz0.evaluate_send();
    size_t n_send = 20;
    size_t n_recv = 20;
    std::vector<Ring> data_recv(n_recv);
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, data_recv);
    } else {
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
    }
    for (size_t i = 0; i < n_recv; ++i) {
        online_vals[i] ^= data_recv[i];
    }
    eqz0.evaluate_recv();

    /* EQZ Layer 1 */
    eqz1.evaluate_send();
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, data_recv);
    } else {
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
    }
    for (size_t i = 0; i < n_recv; ++i) {
        online_vals[i] ^= data_recv[i];
    }
    eqz1.evaluate_recv();

    /* EQZ Layer 2 */
    eqz2.evaluate_send();
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, data_recv);
    } else {
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
    }
    for (size_t i = 0; i < n_recv; ++i) {
        online_vals[i] ^= data_recv[i];
    }
    eqz2.evaluate_recv();

    /* EQZ Layer 3 */
    eqz3.evaluate_send();
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, data_recv);
    } else {
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
    }
    for (size_t i = 0; i < n_recv; ++i) {
        online_vals[i] ^= data_recv[i];
    }
    eqz3.evaluate_recv();

    /* EQZ Layer 4 */
    eqz4.evaluate_send();
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, data_recv);
    } else {
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
    }
    for (size_t i = 0; i < n_recv; ++i) {
        online_vals[i] ^= data_recv[i];
    }
    eqz4.evaluate_recv();

    /* Assertions */
    auto result1 = share::reveal_vec_bin(id, network, result);
    setup::print_vec(result1);

    std::vector<Ring> expected_result({1, 0, 0, 0, 0, 0, 0, 0, 0, 1});
    assert(result1 == expected_result);
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptionsTest());

    bpo::options_description cmdline("Test for the secure computation of the Reach Score.");
    cmdline.add(prog_opts);
    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        test_eqz(opts);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}