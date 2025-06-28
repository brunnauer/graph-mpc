#include <algorithm>

#include "../setup/setup.h"
#include "../src/protocol/message_passing.h"
#include "../src/utils/graph.h"

std::vector<Ring> apply(std::vector<Ring> &old_payload, std::vector<Ring> &new_payload) {
    std::vector<Ring> result(old_payload.size());

    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = old_payload[i] + new_payload[i];
    }
    return result;
}
void pre_mp_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, MPPreprocessing &preproc) {
    return;
}

void post_mp_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, MPPreprocessing &preproc) {
    preproc.eqz_triples = clip::equals_zero_preprocess(id, rngs, network, n, BLOCK_SIZE);
    preproc.B2A_triples = clip::B2A_preprocess(id, rngs, network, n, BLOCK_SIZE);
}

void pre_mp_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, MPPreprocessing &preproc,
                     SecretSharedGraph &g) {
    return;
}

void post_mp_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, SecretSharedGraph &g,
                      MPPreprocessing &preproc, std::vector<Ring> &payload) {
    std::vector<Ring> payload_v_eqz = clip::equals_zero_evaluate(id, rngs, network, BLOCK_SIZE, preproc.eqz_triples, payload);
    std::vector<Ring> payload_v_B2A = clip::B2A_evaluate(id, rngs, network, BLOCK_SIZE, preproc.B2A_triples, payload_v_eqz);
    auto payload_v_flip = clip::flip(id, payload_v_B2A);
    g.payload = payload_v_flip;
    g.payload_bits = to_bits(payload_v_flip, sizeof(Ring) * 8);
}

void benchmark(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, size_t repeat, size_t n_vertices,
               bool save_output, std::string save_file) {
    json output_data;

    Graph g(n);

    for (size_t i = 0; i < n_vertices; ++i) {
        g.add_list_entry(i, i, 1, 0);
    }

    for (size_t i = n_vertices; i < n; ++i) {
        Ring rand = std::rand() % n_vertices;
        g.add_list_entry(rand, (rand + 1) % n_vertices, 0, 0);
    }

    g.payload[0] = 1;

    const size_t n_bits = sizeof(Ring) * 1;
    const size_t n_iterations = 1;
    std::vector<Ring> weights(n_iterations);

    SecretSharedGraph g_shared = share::random_share_graph(id, rngs, n_bits, g);

    for (size_t r = 0; r < repeat; ++r) {
        std::cout << "--- Repetition " << r + 1 << " ---" << std::endl;

        StatsPoint start_pre(*network);
        auto preproc = mp::preprocess(id, rngs, network, g.size, BLOCK_SIZE, n_bits, 1, pre_mp_preprocess, post_mp_preprocess);
        StatsPoint end_pre(*network);
        network->sync();

        auto rbench = end_pre - start_pre;
        output_data["benchmarks"].push_back(rbench);

        size_t bytes_sent = 0;
        for (const auto &val : rbench["communication"]) {
            bytes_sent += val.get<int64_t>();
        }

        std::cout << "preprocessing time: " << rbench["time"] << " ms" << std::endl;
        std::cout << "preprocessing sent: " << bytes_sent << " bytes" << std::endl;

        StatsPoint start(*network);
        mp::evaluate(id, rngs, network, g.size, BLOCK_SIZE, n_bits, n_iterations, n_vertices, g_shared, weights, apply, pre_mp_evaluate, post_mp_evaluate,
                     preproc);
        StatsPoint end(*network);

        rbench = end - start;
        output_data["benchmarks"].push_back(rbench);

        bytes_sent = 0;
        for (const auto &val : rbench["communication"]) {
            bytes_sent += val.get<int64_t>();
        }

        std::cout << "online time: " << rbench["time"] << " ms" << std::endl;
        std::cout << "online sent: " << bytes_sent << " bytes" << std::endl;

        std::cout << std::endl;
    }

    output_data["stats"] = {{"peak_virtual_memory", peakVirtualMemory()}, {"peak_resident_set_size", peakResidentSetSize()}};

    std::cout << "--- Overall Statistics ---\n";
    for (const auto &[key, value] : output_data["stats"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    if (save_output) {
        saveJson(output_data, save_file);
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
        setup::run_benchmark(opts, benchmark);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}