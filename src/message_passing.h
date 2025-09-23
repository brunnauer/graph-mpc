#pragma once

#include <functional>
#include <nlohmann/json.hpp>

#include "graphmpc/clip.h"
#include "graphmpc/deduplication.h"
#include "graphmpc/message_passing.h"
#include "io/netmp.h"
#include "utils/graph.h"
#include "utils/random_generators.h"
#include "utils/types.h"

using json = nlohmann::json;

class ProtocolDef {
   public:
    virtual void pre_mp_preprocessing(MPPreprocessing &preproc) = 0;

    virtual void apply_preprocessing(MPPreprocessing &preproc) = 0;

    virtual void post_mp_preprocessing(MPPreprocessing &preproc) = 0;

    virtual void pre_mp_evaluation(MPPreprocessing &preproc, Graph &g) = 0;

    virtual void apply_evaluation(MPPreprocessing &preproc, Graph &g, std::vector<Ring> &new_payload) = 0;

    virtual void post_mp_evaluation(MPPreprocessing &preproc, Graph &g) = 0;

    size_t comm_pre() { return bytes_sent_pre / sizeof(Ring); }

    size_t comm_eval() { return bytes_sent_eval / sizeof(Ring); }

    void run(Graph &g, std::vector<Ring> &weights, Execution exc, bool parallel = false) {
        /* Preprocessing */
        StatsPoint start_pre(*network);
        if (id != D) network->recv_buffered(D);
        mp_preprocess(parallel);
        if (id != D) network->recv_disk.~FileWriter();
        if (id == D) network->send_all();
        StatsPoint end_pre(*network);

        auto rbench_pre = end_pre - start_pre;
        output_data["benchmarks_pre"].push_back(rbench_pre);

        for (const auto &val : rbench_pre["communication"]) {
            bytes_sent_pre += val.get<int64_t>();
        }
        if (exc == BENCHMARK) {
            std::cout << "preprocessing time: " << rbench_pre["time"] << " ms" << std::endl;
            std::cout << "preprocessing sent: " << bytes_sent_pre << " bytes" << std::endl;
        }

        /* Network Sync */
        // network->sync();

        /* Evaluation */
        StatsPoint start_online(*network);
        mp_evaluate(g, weights, parallel);
        StatsPoint end_online(*network);

        auto rbench = end_online - start_online;
        output_data["benchmarks"].push_back(rbench);

        for (const auto &val : rbench["communication"]) {
            bytes_sent_eval += val.get<int64_t>();
        }

        if (exc == BENCHMARK) {
            std::cout << "online time: " << rbench["time"] << " ms" << std::endl;
            std::cout << "online sent: " << bytes_sent_eval << " bytes" << std::endl;
            std::cout << "Communication (elements): " << bytes_sent_eval / sizeof(Ring) / n << "n" << std::endl;

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
    }

    // protected:
    ProtocolDef(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, size_t n_vertices, size_t n_iterations,
                bool ssd, bool save_output, std::string save_file)
        : id(id),
          rngs(rngs),
          network(network),
          n(n),
          n_bits(n_bits),
          n_vertices(n_vertices),
          n_iterations(n_iterations),
          ssd(ssd),
          save_output(save_output),
          save_file(save_file) {}

    MPPreprocessing preproc;

    Party id;
    RandomGenerators rngs;
    std::shared_ptr<io::NetIOMP> network;
    size_t n;
    size_t n_bits;
    size_t n_vertices;
    size_t n_iterations;
    bool ssd;

    json output_data;
    bool save_output;
    std::string save_file;

    size_t bytes_sent_pre = 0;
    size_t bytes_sent_eval = 0;

    Party recv_shuffle = P0;
    Party recv_mul = P0;

    void mp_preprocess(bool parallel = false) {
        pre_mp_preprocessing(preproc);

        /* Sort preprocessing */
        if (preproc.deduplication) {  // TODO: change branch condition
            sort::get_sort_preprocess(id, rngs, network, n, n_bits + 2, preproc, recv_shuffle, recv_mul,
                                      ssd);  // One bit more, as deduplication appends a bit to the end
            sort::sort_iteration_preprocess(id, rngs, network, n, preproc, recv_shuffle, recv_mul,
                                            ssd);  // Only one iteration, as dst_sort has almost been completed during deduplication
        } else {
            sort::get_sort_preprocess(id, rngs, network, n, n_bits + 1, preproc, recv_shuffle, recv_mul, ssd);
            sort::get_sort_preprocess(id, rngs, network, n, n_bits + 1, preproc, recv_shuffle, recv_mul, ssd);
        }
        sort::sort_iteration_preprocess(id, rngs, network, n, preproc, recv_shuffle, recv_mul, ssd);  // vtx_order sort iteration

        /* Apply / Switch Perm Preprocessing */
        preproc.vtx_order_shuffle = shuffle::get_shuffle(id, rngs, network, n, recv_shuffle);  // Shuffle for applying/switching to vertex order
        preproc.src_order_shuffle = shuffle::get_shuffle(id, rngs, network, n, recv_shuffle);  // Shuffle for switching to src order
        preproc.dst_order_shuffle = shuffle::get_shuffle(id, rngs, network, n, recv_shuffle);  // Shuffle for switching to dst order

        preproc.vtx_src_merge = shuffle::get_merged_shuffle(id, rngs, network, n, preproc.vtx_order_shuffle,
                                                            preproc.src_order_shuffle);  // Merged shuffle to switch between vtx and src order
        preproc.src_dst_merge = shuffle::get_merged_shuffle(id, rngs, network, n, preproc.src_order_shuffle,
                                                            preproc.dst_order_shuffle);  // Merged shuffle to swtich from src order to dst order
        preproc.dst_vtx_merge = shuffle::get_merged_shuffle(id, rngs, network, n, preproc.dst_order_shuffle,
                                                            preproc.vtx_order_shuffle);  // Merged shuffle to switch from dst order to vtx order

        size_t n_parallel_executions = parallel ? n_vertices : 1;
        for (size_t i = 0; i < n_parallel_executions; ++i) {
            /* Post-MP Preprocessing (clipping) */
            post_mp_preprocessing(preproc);
        }
    }

    void run_preparation(Graph &g) {
        g.init_mp(id);

        /* Run PreMP */
        pre_mp_evaluation(preproc, g);

        /* Compute the three orders */
        preproc.src_order = sort::get_sort_evaluate(id, rngs, network, n, preproc, g.src_order_bits, ssd);

        if (preproc.deduplication) {
            /* One more sort iteration to get dst_order */
            preproc.dst_order =
                sort::sort_iteration_evaluate(id, rngs, network, n, preproc.dst_order, preproc, g.dst_order_bits[g.dst_order_bits.size() - 1], ssd);
        } else {
            preproc.dst_order = sort::get_sort_evaluate(id, rngs, network, n, preproc, g.dst_order_bits, ssd);
        }

        preproc.vtx_order = sort::sort_iteration_evaluate(id, rngs, network, n, preproc.src_order, preproc, g.isV_inv, ssd);

        /* Prepare Permutations for apply / switch perm */
        mp::prepare_permutations(id, rngs, network, n, preproc);

        /* Apply Perm */
        std::vector<Ring> shuffled_input_share = shuffle::shuffle(id, rngs, network, n, preproc.vtx_order_shuffle, g._data);
        g._data = preproc.clear_shuffled_vtx_order(shuffled_input_share);
    }

    void run_message_passing(Graph &g, std::vector<Ring> &data_v, std::vector<Ring> &weights) {
        for (size_t i = 0; i < n_iterations; ++i) {
            /* Add current weight only to vertices */
            if (id == P0 && weights.size() > 0) {
#pragma omp parallel for if (n_vertices > 10000)
                for (size_t j = 0; j < n_vertices; ++j) data_v[j] += weights[weights.size() - 1 - i];
            }
            /* Propagate-1 */
            auto data_p = mp::propagate_1(data_v, n_vertices);

            /* Switch Perm from vtx to src order */
            auto shuffled_data_p = preproc.clear_shuffled_vtx_order.inverse()(data_p);
            auto double_shuffled_data_p = shuffle::shuffle(id, rngs, network, n, preproc.vtx_src_merge, shuffled_data_p);
            auto data_src = preproc.clear_shuffled_src_order(double_shuffled_data_p);

            auto shuffled_data_v = preproc.clear_shuffled_vtx_order.inverse()(data_v);
            auto double_shuffled_data_v = shuffle::shuffle(id, rngs, network, n, preproc.vtx_src_merge, shuffled_data_v);
            auto data_corr = preproc.clear_shuffled_src_order(double_shuffled_data_v);

            /* Propagate-2 */
            data_p = mp::propagate_2(data_src, data_corr);

            /* Switch Perm from src to dst order*/
            shuffled_data_p = preproc.clear_shuffled_src_order.inverse()(data_p);
            double_shuffled_data_p = shuffle::shuffle(id, rngs, network, n, preproc.src_dst_merge, shuffled_data_p);
            auto data_dst = preproc.clear_shuffled_dst_order(double_shuffled_data_p);

            /* Gather-1*/
            data_p = mp::gather_1(data_dst);

            /* Switch Perm from dst to vtx order */
            shuffled_data_p = preproc.clear_shuffled_dst_order.inverse()(data_p);
            double_shuffled_data_p = shuffle::shuffle(id, rngs, network, n, preproc.dst_vtx_merge, shuffled_data_p);
            data_p = preproc.clear_shuffled_vtx_order(double_shuffled_data_p);

            /* Gather-2 */
            auto update = mp::gather_2(data_p, n_vertices);

            /* ApplyV */
            apply_evaluation(preproc, g, update);
        }
    }

    void mp_evaluate(Graph &g, std::vector<Ring> &weights, bool parallel) {
        if (id == D) return;

        run_preparation(g);

        /* Preparing payloads for pi_r if necessary */
        size_t n_parallel_executions = parallel ? g._n_vertices : 1;
        std::vector<std::vector<Ring>> payloads(n_parallel_executions);
        for (size_t i = 0; i < n_parallel_executions; ++i) {
            payloads[i] = g._data;
            if (parallel) {
                std::vector<Ring> one(g._size);
                one[i] = 1;
                auto shared_one = share::random_share_secret_vec_2P(id, rngs, one);
                payloads[i][i] = shared_one[i];
            }
        }

        /* Run message-passing */
        std::vector<Ring> result_payload(n);
        for (size_t i = 0; i < n_parallel_executions; ++i) {
            g._data = payloads[i];

            /* Run message passing */
            run_message_passing(g, g._data, weights);

            /* Run post-mp (clipping) */
            post_mp_evaluation(preproc, g);

            if (parallel) {
                auto sum = g._data[0];
                for (size_t k = 1; k < n; k++) sum += g._data[k];
                result_payload[i] = sum;
            }
        }

        if (parallel) {
            g._data = result_payload;
        }
    }
};
