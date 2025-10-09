#pragma once

#include <cmath>   // std::log2, std::ceil
#include <memory>  // std::unique_ptr, std::make_unique
#include <unordered_map>
#include <vector>

#include "../io/netmp.h"
#include "../setup/configs.h"
#include "../utils/graph.h"
#include "add_weights.h"
#include "bit2a.h"
#include "compaction.h"
#include "deduplication.h"
#include "equals_zero.h"
#include "function.h"
#include "gather.h"
#include "merged_shuffle.h"
#include "permute.h"
#include "propagate.h"
#include "reveal.h"
#include "shuffle.h"
#include "shuffle_repeat.h"
#include "unshuffle.h"
#include "update.h"

struct MPContext {
    std::unordered_map<Party, std::vector<Ring>> preproc;
    std::vector<Ring> data_recv;
    std::vector<Ring> mult_vals;
    std::vector<Ring> and_vals;
    std::vector<Ring> shuffle_vals;
    std::vector<Ring> reveal_vals;

    std::vector<Ring> vtx_order;
    std::vector<Ring> src_order;
    std::vector<Ring> dst_order;
    std::vector<Ring> clear_shuffled_vtx_order;
    std::vector<Ring> clear_shuffled_src_order;
    std::vector<Ring> clear_shuffled_dst_order;

    /* Shuffles for Switching Permutations */
    ShufflePre vtx_order_shuffle;
    ShufflePre src_order_shuffle;
    ShufflePre dst_order_shuffle;
    ShufflePre vtx_src_merge;
    ShufflePre src_dst_merge;
    ShufflePre dst_vtx_merge;
};

struct Wires {
    std::vector<Ring> mp_data_vtx;
    std::vector<Ring> mp_data;
    std::vector<Ring> mp_data_corr;
    std::vector<Ring> sort_perm;
    std::vector<Ring> sort_bits;
    std::vector<Ring> sort_next_perm;
    std::vector<Ring> deduplication_perm;
    std::vector<Ring> deduplication_src;
    std::vector<Ring> deduplication_dst;
    std::vector<Ring> deduplication_src_dupl;
    std::vector<Ring> deduplication_dst_dupl;
    std::vector<Ring> deduplication_duplicates;
};

class MPProtocol {
   public:
    MPProtocol(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> &network)
        : conf(conf),
          id(conf.id),
          size(conf.size),
          nodes(conf.nodes),
          depth(conf.depth),
          bits(static_cast<size_t>(std::ceil(std::log2(static_cast<double>(nodes) + 2.0)))),
          rngs(conf.rngs),
          weights(conf.weights),
          network(network),
          ssd(conf.ssd) {
        ctx.preproc[P0] = {};
        ctx.preproc[P1] = {};
        ctx.vtx_order.resize(size);
        ctx.src_order.resize(size);
        ctx.dst_order.resize(size);
        ctx.clear_shuffled_vtx_order.resize(size);
        ctx.clear_shuffled_src_order.resize(size);
        ctx.clear_shuffled_dst_order.resize(size);
        w.mp_data_vtx.resize(size);
        w.mp_data.resize(size);
        w.mp_data_corr.resize(size);
        w.sort_next_perm.resize(size);
        w.sort_perm.resize(size);
        w.sort_bits.resize(size);
        f_queue.resize(1);

        initialize_shuffle(ctx.vtx_order_shuffle);
        initialize_shuffle(ctx.src_order_shuffle);
        initialize_shuffle(ctx.dst_order_shuffle);
        initialize_shuffle(ctx.vtx_src_merge);
        initialize_shuffle(ctx.src_dst_merge);
        initialize_shuffle(ctx.dst_vtx_merge);
    }

    virtual void build() {
        pre_mp();
        build_initialization();
        build_message_passing();
        post_mp();
    }

    virtual void pre_mp() = 0;
    virtual void apply() = 0;
    virtual void post_mp() = 0;

    virtual void add_compute_sorts();  // Can be overwritten

    void preprocess();
    void evaluate();

    void set_input(Graph &graph) { g = graph; }

    Graph get_output() { return g; }

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
        Graph g_shared = g.share_subgraphs(id, rngs, network, bits);
        g_shared.init_mp(id);
        return g_shared;
    }

    void reset() {
        rngs.reseed();
        recv_shuffle = P0;
        recv_mul = P0;
    }

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

   protected:
    ProtocolConfig conf;
    Party id;
    size_t size, nodes, depth, bits;
    RandomGenerators rngs;
    std::vector<Ring> weights;
    std::shared_ptr<io::NetIOMP> network;
    bool ssd;

    Graph g;

    std::vector<std::vector<std::unique_ptr<Function>>> f_queue;
    ShufflePre *current_shuffle;
    size_t current_layer = 0;

    MPContext ctx;
    Wires w;

    Party recv_shuffle = P0;
    Party recv_mul = P0;

    void online_communication();

    void add_sort(std::vector<std::vector<Ring>> &bit_keys, std::vector<Ring> &output, size_t bits);

    void add_sort_iteration(std::vector<Ring> &perm, std::vector<Ring> &keys, std::vector<Ring> &output);

    void build_initialization();

    void build_message_passing();

    void add_function(std::unique_ptr<Function> func) {
        auto &layer = f_queue[current_layer];
        if (is_independent_of_layer(func, layer)) {
            layer.push_back(std::move(func));
            return;
        }

        std::vector<std::unique_ptr<Function>> new_layer;
        new_layer.push_back(std::move(func));
        f_queue.push_back(std::move(new_layer));
        current_layer++;
    }

    bool is_independent_of_layer(const std::unique_ptr<Function> &func, const std::vector<std::unique_ptr<Function>> &layer) {
        for (const auto &f : layer) {
            if ((f->output == func->input || f->output == func->input2)) {
                return false;
            }
        }
        return true;
    }

    void add_shuffle(std::vector<Ring> &input, std::vector<Ring> &output) {
        auto shuffle_ptr = std::make_unique<Shuffle>(&conf, &ctx.preproc, &ctx.shuffle_vals, &input, &output, recv_shuffle);
        current_shuffle = shuffle_ptr->perm_share;
        add_function(std::move(shuffle_ptr));
    }

    void add_shuffle(std::vector<Ring> &input, std::vector<Ring> &output, ShufflePre *perm_share) {
        add_function(std::make_unique<Shuffle>(&conf, &ctx.preproc, &ctx.shuffle_vals, &input, &output, recv_shuffle, perm_share));
    }

    void repeat_shuffle(std::vector<Ring> &input, std::vector<Ring> &output) {
        add_function(std::make_unique<ShuffleRepeat>(&conf, &ctx.preproc, &ctx.shuffle_vals, &input, &output, current_shuffle, recv_shuffle));
    }

    void repeat_shuffle(std::vector<Ring> &input, std::vector<Ring> &output, ShufflePre &perm_share) {
        add_function(std::make_unique<ShuffleRepeat>(&conf, &ctx.preproc, &ctx.shuffle_vals, &input, &output, &perm_share, recv_shuffle));
    }

    void add_unshuffle(std::vector<Ring> &input, std::vector<Ring> &output) {
        add_function(std::make_unique<Unshuffle>(&conf, &ctx.preproc, &ctx.shuffle_vals, &input, &output, current_shuffle, recv_shuffle));
    }

    void add_merged_shuffle(std::vector<Ring> &input, std::vector<Ring> &output, ShufflePre &perm_share, ShufflePre &pi_share, ShufflePre &omega_share) {
        add_function(
            std::make_unique<MergedShuffle>(&conf, &ctx.preproc, &ctx.shuffle_vals, &input, &output, recv_shuffle, &perm_share, &pi_share, &omega_share));
    }

    void add_compaction(std::vector<Ring> &input, std::vector<Ring> &output) {
        add_function(std::make_unique<Compaction>(&conf, &ctx.preproc, &ctx.mult_vals, &input, &output, recv_mul));
    }

    void add_reveal(std::vector<Ring> &input, std::vector<Ring> &output) { add_function(std::make_unique<Reveal>(&conf, &ctx.reveal_vals, &input, &output)); }

    void add_permute(std::vector<Ring> &input, std::vector<Ring> &output, std::vector<Ring> &perm, bool inverse = false) {
        f_queue[current_layer].emplace_back(std::make_unique<Permute>(&conf, &input, &output, &perm, inverse));
    }

    void add_update(std::vector<Ring> &input, std::vector<Ring> &output) {
        f_queue[current_layer].emplace_back(std::make_unique<Update>(&conf, &input, &output));
    }

    void add_equals_zero(std::vector<Ring> &input, std::vector<Ring> &output, size_t size, size_t layer) {
        add_function(std::make_unique<EQZ>(&conf, &ctx.preproc, &ctx.and_vals, &input, &output, recv_mul, size, layer));
    }

    void add_Bit2A(std::vector<Ring> &input, std::vector<Ring> &output, size_t size) {
        add_function(std::make_unique<Bit2A>(&conf, &ctx.preproc, &ctx.mult_vals, &input, &output, recv_mul, size));
    }

    void add_insert(std::vector<Ring> &input) { f_queue[current_layer].emplace_back(std::make_unique<Insert>(&conf, &input)); }

    void add_deduplication_sub(std::vector<Ring> &vec_p, std::vector<Ring> &vec_dupl) {
        f_queue[current_layer].emplace_back(std::make_unique<DeduplicationSub>(&conf, &vec_p, &vec_dupl));
    }

    void add_mul(std::vector<Ring> &x, std::vector<Ring> &y, std::vector<Ring> &output, bool binary = false) {
        if (binary) {
            add_function(std::make_unique<Mul>(&conf, &ctx.preproc, &ctx.and_vals, &x, &y, &output, recv_mul, binary));
        } else {
            add_function(std::make_unique<Mul>(&conf, &ctx.preproc, &ctx.mult_vals, &x, &y, &output, recv_mul, binary));
        }
    }

    void add_mul(std::vector<Ring> &x, std::vector<Ring> &y, std::vector<Ring> &output, size_t size, bool binary = false) {
        if (binary) {
            add_function(std::make_unique<Mul>(&conf, &ctx.preproc, &ctx.and_vals, &x, &y, &output, recv_mul, binary, size));
        } else {
            add_function(std::make_unique<Mul>(&conf, &ctx.preproc, &ctx.mult_vals, &x, &y, &output, recv_mul, binary, size));
        }
    }

    void add_push_back(std::vector<std::vector<Ring>> &keys, std::vector<Ring> &vec) {
        f_queue[current_layer].emplace_back(std::make_unique<PushBack>(&conf, &keys, &vec));
    }

    void add_deduplication() {
        w.deduplication_perm.resize(size);
        w.deduplication_src.resize(size);
        w.deduplication_dst.resize(size);

        add_sort(g.dst_order_bits, ctx.dst_order, bits + 1);
        add_update(ctx.dst_order, w.deduplication_perm);

        for (size_t i = 0; i < bits; ++i) {  // Was g.src_bits().size()
            add_sort_iteration(w.deduplication_perm, g.src_bits[i], w.deduplication_perm);
        }

        add_shuffle(w.deduplication_perm, w.deduplication_perm);
        add_reveal(w.deduplication_perm, w.deduplication_perm);
        repeat_shuffle(g.src, w.deduplication_src);
        repeat_shuffle(g.dst, w.deduplication_dst);
        add_permute(w.deduplication_src, w.deduplication_src, w.deduplication_perm);
        add_permute(w.deduplication_dst, w.deduplication_dst, w.deduplication_perm);

        w.deduplication_src_dupl.resize(size - 1);
        w.deduplication_dst_dupl.resize(size - 1);
        w.deduplication_duplicates.resize(size - 1);

        add_deduplication_sub(w.deduplication_src, w.deduplication_src_dupl);
        add_deduplication_sub(w.deduplication_dst, w.deduplication_dst_dupl);

        add_equals_zero(w.deduplication_src_dupl, w.deduplication_src_dupl, size - 1, 0);
        add_equals_zero(w.deduplication_dst_dupl, w.deduplication_dst_dupl, size - 1, 0);
        add_equals_zero(w.deduplication_src_dupl, w.deduplication_src_dupl, size - 1, 1);
        add_equals_zero(w.deduplication_dst_dupl, w.deduplication_dst_dupl, size - 1, 1);
        add_equals_zero(w.deduplication_src_dupl, w.deduplication_src_dupl, size - 1, 2);
        add_equals_zero(w.deduplication_dst_dupl, w.deduplication_dst_dupl, size - 1, 2);
        add_equals_zero(w.deduplication_src_dupl, w.deduplication_src_dupl, size - 1, 3);
        add_equals_zero(w.deduplication_dst_dupl, w.deduplication_dst_dupl, size - 1, 3);
        add_equals_zero(w.deduplication_src_dupl, w.deduplication_src_dupl, size - 1, 4);
        add_equals_zero(w.deduplication_dst_dupl, w.deduplication_dst_dupl, size - 1, 4);

        add_mul(w.deduplication_src_dupl, w.deduplication_dst_dupl, w.deduplication_duplicates, size - 1, true);
        add_Bit2A(w.deduplication_duplicates, w.deduplication_duplicates, size - 1);
        add_insert(w.deduplication_duplicates);

        add_permute(w.deduplication_duplicates, w.deduplication_duplicates, w.deduplication_perm, true);
        add_unshuffle(w.deduplication_duplicates, w.deduplication_duplicates);
        add_push_back(g.src_order_bits, w.deduplication_duplicates);
        add_push_back(g.dst_order_bits, w.deduplication_duplicates);
        /* TODO: Handle push_back */
    }

    void initialize_shuffle(ShufflePre &perm_share) {
        if (id == P0) {
            perm_share.pi_0.perm_vec.resize(size);
            perm_share.pi_0_p.perm_vec.resize(size);
            perm_share.B.resize(size);
            perm_share.R.resize(size);
        }
        if (id == P1) {
            perm_share.pi_1.perm_vec.resize(size);
            perm_share.pi_1_p.perm_vec.resize(size);
            perm_share.B.resize(size);
            perm_share.R.resize(size);
        }
        if (id == D) {
            perm_share.pi_0.perm_vec.resize(size);
            perm_share.pi_1.perm_vec.resize(size);
        }
    }
};