#pragma once

#include "../io/netmp.h"
#include "../setup/configs.h"
#include "../utils/graph.h"
#include "../utils/preprocessings.h"
#include "message_passing.h"

class MPProtocol {
   public:
    MPProtocol(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> &network)
        : id(conf.id),
          size(conf.size),
          nodes(conf.nodes),
          depth(conf.depth),
          bits(std::ceil(std::log2(nodes + 2))),
          rngs(conf.rngs),
          weights(conf.weights),
          network(network),
          ssd(conf.ssd) {}

    virtual void pre_mp_preprocessing(MPPreprocessing &preproc) = 0;

    virtual void apply_preprocessing(MPPreprocessing &preproc) = 0;

    virtual void post_mp_preprocessing(MPPreprocessing &preproc) = 0;

    virtual void pre_mp_evaluation(MPPreprocessing &preproc, Graph &g) = 0;

    virtual void apply_evaluation(MPPreprocessing &preproc, Graph &g, std::vector<Ring> &new_payload) = 0;

    virtual void post_mp_evaluation(MPPreprocessing &preproc, Graph &g) = 0;

    Party id;
    size_t size, nodes, depth, bits;
    RandomGenerators rngs;
    std::vector<Ring> weights;
    std::shared_ptr<io::NetIOMP> network;
    bool ssd;

    MPPreprocessing preproc;

    Party recv_shuffle = P0;
    Party recv_mul = P0;

    void print() {
        std::cout << "----- Protocol Configuration -----" << std::endl;
        std::cout << "Party: " << id << std::endl;
        std::cout << "Size: " << size << std::endl;
        std::cout << "Nodes: " << nodes << std::endl;
        std::cout << "Depth: " << depth << std::endl;
        std::cout << "Bits: " << bits << std::endl;
        std::cout << "SSD utilization: " << ssd << std::endl;
        std::cout << std::endl;
    }

    void reset() {
        preproc = {};
        rngs.reseed();
        recv_shuffle = P0;
        recv_mul = P0;
    }

    Graph benchmark_graph() {
        Graph g;
        if (id == P0) {
            for (size_t i = 0; i < nodes / 2; i++) g.add_list_entry(i + 1, i + 1, 1);
            for (size_t i = 0; i < (size - nodes) / 2; i++) g.add_list_entry(1, 2, 0);
        }
        if (id == P1) {
            for (size_t i = nodes / 2; i < nodes; i++) g.add_list_entry(i + 1, i + 1, 1);
            for (size_t i = (size - nodes) / 2; i < size - nodes; i++) g.add_list_entry(1, 2, 0);
        }
        return g.share_subgraphs(id, rngs, network, bits);
    }

    void mp_preprocess(bool parallel = false) {
        if (id != D) network->recv_buffered(D);

        pre_mp_preprocessing(preproc);

        /* Sort preprocessing */
        if (preproc.deduplication) {  // TODO: change branch condition
            sort::get_sort_preprocess(id, rngs, network, size, bits + 2, preproc, recv_shuffle, recv_mul,
                                      ssd);  // One bit more, as deduplication appends a bit to the end
            sort::sort_iteration_preprocess(id, rngs, network, size, preproc, recv_shuffle, recv_mul,
                                            ssd);  // Only one iteration, as dst_sort has almost been completed during deduplication
        } else {
            sort::get_sort_preprocess(id, rngs, network, size, bits + 1, preproc, recv_shuffle, recv_mul, ssd);
            sort::get_sort_preprocess(id, rngs, network, size, bits + 1, preproc, recv_shuffle, recv_mul, ssd);
        }
        sort::sort_iteration_preprocess(id, rngs, network, size, preproc, recv_shuffle, recv_mul, ssd);  // vtx_order sort iteration

        /* Apply / Switch Perm Preprocessing */
        preproc.vtx_order_shuffle = shuffle::get_shuffle(id, rngs, network, size, recv_shuffle);  // Shuffle for applying/switching to vertex order
        preproc.src_order_shuffle = shuffle::get_shuffle(id, rngs, network, size, recv_shuffle);  // Shuffle for switching to src order
        preproc.dst_order_shuffle = shuffle::get_shuffle(id, rngs, network, size, recv_shuffle);  // Shuffle for switching to dst order

        preproc.vtx_src_merge = shuffle::get_merged_shuffle(id, rngs, network, size, preproc.vtx_order_shuffle,
                                                            preproc.src_order_shuffle);  // Merged shuffle to switch between vtx and src order
        preproc.src_dst_merge = shuffle::get_merged_shuffle(id, rngs, network, size, preproc.src_order_shuffle,
                                                            preproc.dst_order_shuffle);  // Merged shuffle to swtich from src order to dst order
        preproc.dst_vtx_merge = shuffle::get_merged_shuffle(id, rngs, network, size, preproc.dst_order_shuffle,
                                                            preproc.vtx_order_shuffle);  // Merged shuffle to switch from dst order to vtx order

        size_t n_parallel_executions = parallel ? nodes : 1;
        for (size_t i = 0; i < n_parallel_executions; ++i) {
            /* Post-MP Preprocessing (clipping) */
            post_mp_preprocessing(preproc);
        }

        if (id != D) network->recv_disk.~FileWriter();
        if (id == D) network->send_all();
    }

    void run_preparation(Graph &g) {
        g.init_mp(id);

        /* Run PreMP */
        pre_mp_evaluation(preproc, g);

        /* Compute the three orders */
        preproc.src_order = sort::get_sort_evaluate(id, rngs, network, size, preproc, g.src_order_bits, ssd);

        if (preproc.deduplication) {
            /* One more sort iteration to get dst_order */
            preproc.dst_order =
                sort::sort_iteration_evaluate(id, rngs, network, size, preproc.dst_order, preproc, g.dst_order_bits[g.dst_order_bits.size() - 1], ssd);
        } else {
            preproc.dst_order = sort::get_sort_evaluate(id, rngs, network, size, preproc, g.dst_order_bits, ssd);
        }

        preproc.vtx_order = sort::sort_iteration_evaluate(id, rngs, network, size, preproc.src_order, preproc, g.isV_inv, ssd);

        /* Prepare Permutations for apply / switch perm */
        mp::prepare_permutations(id, rngs, network, size, preproc);

        /* Apply Perm */
        std::vector<Ring> shuffled_input_share = shuffle::shuffle(id, rngs, network, size, preproc.vtx_order_shuffle, g._data);
        g._data = preproc.clear_shuffled_vtx_order(shuffled_input_share);
    }

    void run_message_passing(Graph &g, std::vector<Ring> &data_v, std::vector<Ring> &weights) {
        for (size_t i = 0; i < depth; ++i) {
            /* Add current weight only to vertices */
            if (id == P0 && weights.size() > 0) {
#pragma omp parallel for if (nodes > 10000)
                for (size_t j = 0; j < nodes; ++j) data_v[j] += weights[weights.size() - 1 - i];
            }
            /* Propagate-1 */
            auto data_p = mp::propagate_1(data_v, nodes);
            auto data_p1 = share::reveal_vec(id, network, data_p);

            /* Switch Perm from vtx to src order */
            auto shuffled_data_p = preproc.clear_shuffled_vtx_order.inverse()(data_p);
            auto double_shuffled_data_p = shuffle::shuffle(id, rngs, network, size, preproc.vtx_src_merge, shuffled_data_p);
            auto data_src = preproc.clear_shuffled_src_order(double_shuffled_data_p);

            auto shuffled_data_v = preproc.clear_shuffled_vtx_order.inverse()(data_v);
            auto double_shuffled_data_v = shuffle::shuffle(id, rngs, network, size, preproc.vtx_src_merge, shuffled_data_v);
            auto data_corr = preproc.clear_shuffled_src_order(double_shuffled_data_v);

            /* Propagate-2 */
            data_p = mp::propagate_2(data_src, data_corr);
            auto data_p2 = share::reveal_vec(id, network, data_p);

            /* Switch Perm from src to dst order*/
            shuffled_data_p = preproc.clear_shuffled_src_order.inverse()(data_p);
            double_shuffled_data_p = shuffle::shuffle(id, rngs, network, size, preproc.src_dst_merge, shuffled_data_p);
            auto data_dst = preproc.clear_shuffled_dst_order(double_shuffled_data_p);

            /* Gather-1*/
            data_p = mp::gather_1(data_dst);
            auto data_g1 = share::reveal_vec(id, network, data_p);

            /* Switch Perm from dst to vtx order */
            shuffled_data_p = preproc.clear_shuffled_dst_order.inverse()(data_p);
            double_shuffled_data_p = shuffle::shuffle(id, rngs, network, size, preproc.dst_vtx_merge, shuffled_data_p);
            data_p = preproc.clear_shuffled_vtx_order(double_shuffled_data_p);

            /* Gather-2 */
            auto update = mp::gather_2(data_p, nodes);
            auto data_g2 = share::reveal_vec(id, network, update);

            /* ApplyV */
            apply_evaluation(preproc, g, update);
        }
    }

    void mp_evaluate(Graph &g, bool parallel) {
        if (id == D) return;

        run_preparation(g);

        /* Preparing payloads for pi_r if necessary */
        size_t n_parallel_executions = parallel ? nodes : 1;
        std::vector<std::vector<Ring>> payloads(n_parallel_executions);
        for (size_t i = 0; i < n_parallel_executions; ++i) {
            payloads[i] = g._data;
            if (parallel) {
                std::vector<Ring> one(size);
                one[i] = 1;
                auto shared_one = share::random_share_secret_vec_2P(id, rngs, one);
                payloads[i][i] = shared_one[i];
            }
        }

        /* Run message-passing */
        std::vector<Ring> result_payload(size);
        for (size_t i = 0; i < n_parallel_executions; ++i) {
            g._data = payloads[i];

            /* Run message passing */
            run_message_passing(g, g._data, weights);
            auto data = share::reveal_vec(id, network, g._data);

            /* Run post-mp (clipping) */
            post_mp_evaluation(preproc, g);

            if (parallel) {
                auto sum = g._data[0];
                for (size_t k = 1; k < size; k++) sum += g._data[k];
                result_payload[i] = sum;
            }
        }

        if (parallel) {
            g._data = result_payload;
        }
    }
};