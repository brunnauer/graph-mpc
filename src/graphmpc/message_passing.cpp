#include "mp_protocol.h"

/* Case deduplication */
void MPProtocol::add_compute_sorts() {
    add_sort(g.src_order_bits, ctx.src_order, bits + 1);
    add_sort(g.dst_order_bits, ctx.dst_order, bits + 1);
    add_sort_iteration(ctx.src_order, g.isV_inv, ctx.vtx_order);
}

void MPProtocol::build_initialization() {
    add_compute_sorts();
    /* Prepare vtx order */
    add_shuffle(ctx.vtx_order, ctx.vtx_order, &ctx.vtx_order_shuffle);
    add_shuffle(ctx.src_order, ctx.src_order, &ctx.src_order_shuffle);
    add_shuffle(ctx.dst_order, ctx.dst_order, &ctx.dst_order_shuffle);

    add_reveal(ctx.vtx_order, ctx.clear_shuffled_vtx_order);
    add_reveal(ctx.src_order, ctx.clear_shuffled_src_order);
    add_reveal(ctx.dst_order, ctx.clear_shuffled_dst_order);

    add_shuffle(g.data, g.data, &ctx.vtx_order_shuffle);
    add_permute(g.data, w.mp_data_vtx, ctx.clear_shuffled_vtx_order);
}

void MPProtocol::build_message_passing() {
    for (size_t i = 0; i < depth; ++i) {
        f_queue[f_queue.size() - 1].emplace_back(std::make_unique<AddWeights>(&conf, &w.mp_data, &weights, i));

        add_update(w.mp_data, w.mp_data_corr);

        /* Propagate-1 */
        f_queue[f_queue.size() - 1].emplace_back(std::make_unique<Propagate_1>(&conf, &w.mp_data, &w.mp_data));

        /* Switch Perm from vtx to src order */
        add_permute(w.mp_data, w.mp_data, ctx.clear_shuffled_vtx_order, true);
        add_merged_shuffle(w.mp_data, w.mp_data, ctx.vtx_src_merge, ctx.vtx_order_shuffle, ctx.src_order_shuffle);
        add_permute(w.mp_data, w.mp_data, ctx.clear_shuffled_src_order);

        add_permute(w.mp_data_corr, w.mp_data_corr, ctx.clear_shuffled_vtx_order, true);
        add_merged_shuffle(w.mp_data_corr, w.mp_data_corr, ctx.vtx_src_merge, ctx.vtx_order_shuffle, ctx.src_order_shuffle);
        add_permute(w.mp_data_corr, w.mp_data_corr, ctx.clear_shuffled_src_order);

        /* Propagate-2 */
        f_queue[f_queue.size() - 1].emplace_back(std::make_unique<Propagate_2>(&conf, &w.mp_data, &w.mp_data_corr, &w.mp_data));

        /* Switch Perm from src to dst order */
        add_permute(w.mp_data, w.mp_data, ctx.clear_shuffled_src_order, true);
        add_merged_shuffle(w.mp_data, w.mp_data, ctx.src_dst_merge, ctx.src_order_shuffle, ctx.dst_order_shuffle);
        add_permute(w.mp_data, w.mp_data, ctx.clear_shuffled_dst_order);

        /* Gather-1 */
        f_queue[f_queue.size() - 1].emplace_back(std::make_unique<Gather_1>(&conf, &w.mp_data, &w.mp_data));

        /* Switch Perm from dst to vtx order */
        add_permute(w.mp_data, w.mp_data, ctx.clear_shuffled_dst_order, true);
        add_merged_shuffle(w.mp_data, w.mp_data, ctx.dst_vtx_merge, ctx.dst_order_shuffle, ctx.vtx_order_shuffle);
        add_permute(w.mp_data, w.mp_data, ctx.clear_shuffled_vtx_order);

        /* Gather-2 */
        f_queue[f_queue.size() - 1].emplace_back(std::make_unique<Gather_2>(&conf, &w.mp_data, &w.mp_data));

        apply();
    }
}
