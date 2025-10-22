#include "circuit.h"

void Circuit::build() {
    set_inputs();
    pre_mp();
    compute_sorts();
    prepare_shuffles();
    size_t data_out;
    if (in.data_parallel.size() > 0) {
        for (size_t i = 0; i < in.data_parallel.size(); ++i) {
            in.data_parallel[i] = message_passing(in.data_parallel[i]);
        }
    } else {
        data_out = message_passing(in.data);
    }
    shuffle_idx += 3;
    auto data_post_mp = post_mp(data_out);
    output(data_post_mp);
}

void Circuit::set_inputs() {
    in.src_order_bits.resize(bits + 1);
    in.dst_order_bits.resize(bits + 1);
    for (size_t i = 0; i < bits + 1; ++i) {
        in.src_order_bits[i] = input();
    }
    for (size_t i = 0; i < bits + 1; ++i) {
        in.dst_order_bits[i] = input();
    }
    in.src = input();
    in.dst = input();
    in.isV_inv = input();
    in.data = input();

    for (size_t i = 0; i < in.data_parallel.size(); ++i) {
        in.data_parallel[i] = input();
    }
}

void Circuit::level_order() {
    std::vector<size_t> wire_level(f_queue.size(), 0);
    std::vector<size_t> function_level(f_queue.size(), 0);
    size_t depth = 0;

    for (auto &f : f_queue) {
        if (f->type == Output) continue;
        size_t max_depth = 0;

        auto wire_depth = wire_level[f->in1_idx];
        max_depth = std::max(max_depth, wire_depth);

        wire_depth = wire_level[f->in2_idx];
        max_depth = std::max(max_depth, wire_depth);

        if (f->interactive()) max_depth++;

        function_level[f->f_id] = max_depth;
        wire_level[f->out_idx] = max_depth;

        depth = std::max(depth, max_depth);
    }
    for (auto &f : f_queue) {
        if (f->type == Output) {
            function_level[f->f_id] = depth;
        }
    }

    circ.resize(depth + 1);

    for (size_t i = 0; i < f_queue.size(); ++i) {
        auto &f = f_queue[i];
        if (f == nullptr) {
            std::cout << "nullptr function at index " << i << std::endl;
            continue;
        }
        circ[function_level[f->f_id]].push_back(f);
    }
    f_queue.clear();
}

void Circuit::compute_sorts() {
    ctx.src_order = sort(in.src_order_bits, bits + 1);
    ctx.dst_order = sort(in.dst_order_bits, bits + 1);
    ctx.vtx_order = sort_iteration(ctx.src_order, in.isV_inv);
}

void Circuit::prepare_shuffles() {
    /* Prepare vtx order */
    ctx.vtx_shuffle_idx = shuffle_idx;
    ctx.src_shuffle_idx = ++shuffle_idx;
    ctx.dst_shuffle_idx = ++shuffle_idx;

    auto vtx_order_shuffled = shuffle(ctx.vtx_order, ctx.vtx_shuffle_idx);
    auto src_order_shuffled = shuffle(ctx.src_order, ctx.src_shuffle_idx);
    auto dst_order_shuffled = shuffle(ctx.dst_order, ctx.dst_shuffle_idx);

    ctx.clear_shuffled_vtx_order = reveal(vtx_order_shuffled);
    ctx.clear_shuffled_src_order = reveal(src_order_shuffled);
    ctx.clear_shuffled_dst_order = reveal(dst_order_shuffled);
}

size_t Circuit::message_passing(size_t &data) {
    auto data_shuffled = shuffle(data, ctx.vtx_shuffle_idx);
    auto data_vtx = permute(data_shuffled, ctx.clear_shuffled_vtx_order);

    size_t vtx_src_idx = shuffle_idx + 1;
    size_t src_dst_idx = shuffle_idx + 2;
    size_t dst_vtx_idx = shuffle_idx + 3;

    for (size_t i = 0; i < depth; ++i) {
        auto data_old = data_vtx;
        data_vtx = add_const(data_vtx, weights[weights.size() - 1 - i]);

        /* Propagate-1 */
        auto data_vtx_propagate = propagate_1(data_vtx);

        /* Switch Perm from vtx to src order */
        data_shuffled = reverse_permute(data_vtx_propagate, ctx.clear_shuffled_vtx_order);
        data_shuffled = merged_shuffle(data_shuffled, vtx_src_idx, ctx.vtx_shuffle_idx, ctx.src_shuffle_idx);
        auto data_src = permute(data_shuffled, ctx.clear_shuffled_src_order);

        auto data_corr_shuffled = reverse_permute(data_vtx, ctx.clear_shuffled_vtx_order);
        data_corr_shuffled = merged_shuffle(data_corr_shuffled, vtx_src_idx, ctx.vtx_shuffle_idx, ctx.src_shuffle_idx);
        auto data_corr = permute(data_corr_shuffled, ctx.clear_shuffled_src_order);

        /* Propagate-2 */
        data_src = propagate_2(data_src, data_corr);

        /* Switch Perm from src to dst order */
        data_shuffled = reverse_permute(data_src, ctx.clear_shuffled_src_order);
        data_shuffled = merged_shuffle(data_shuffled, src_dst_idx, ctx.src_shuffle_idx, ctx.dst_shuffle_idx);
        auto data_dst = permute(data_shuffled, ctx.clear_shuffled_dst_order);

        /* Gather-1 */
        data_dst = gather_1(data_dst);

        /* Switch Perm from dst to vtx order */
        data_shuffled = reverse_permute(data_dst, ctx.clear_shuffled_dst_order);
        data_shuffled = merged_shuffle(data_shuffled, dst_vtx_idx, ctx.dst_shuffle_idx, ctx.vtx_shuffle_idx);
        data_vtx = permute(data_shuffled, ctx.clear_shuffled_vtx_order);

        /* Gather-2 */
        data_vtx = gather_2(data_vtx);

        data_vtx = apply(data_old, data_vtx);
    }
    return data_vtx;
}

size_t Circuit::sort(std::vector<size_t> &bit_keys, size_t bits) {
    auto perm = compaction(bit_keys[0]);
    for (size_t bit = 1; bit < bits; ++bit) {
        perm = sort_iteration(perm, bit_keys[bit]);
    }
    return perm;
}

size_t Circuit::sort_iteration(size_t &perm, size_t &keys) {
    auto perm_shuffled = shuffle(perm, shuffle_idx);
    auto keys_shuffled = shuffle(keys, shuffle_idx);

    auto clear_perm_shuffled = reveal(perm_shuffled);
    auto keys_sorted = permute(keys_shuffled, clear_perm_shuffled);

    auto perm_next = compaction(keys_sorted);

    perm_next = reverse_permute(perm_next, clear_perm_shuffled);
    perm_next = unshuffle(perm_next, shuffle_idx);
    shuffle_idx++;
    return perm_next;
}

/* ----- Single functions ----- */

size_t Circuit::input() {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Input, f_queue.size(), output));
    return output;
}

void Circuit::output(size_t &input) {
    size_t output;
    f_queue.push_back(std::make_shared<Function>(Output, f_queue.size(), input, output));
}

size_t Circuit::propagate_1(size_t &input) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Propagate1, f_queue.size(), input, output));
    return output;
}

size_t Circuit::propagate_2(size_t &input1, size_t &input2) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Propagate2, f_queue.size(), input1, input2, output));
    return output;
}

size_t Circuit::gather_1(size_t &input) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Gather1, f_queue.size(), input, output));
    return output;
}

size_t Circuit::gather_2(size_t &input) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Gather2, f_queue.size(), input, output));
    return output;
}

size_t Circuit::shuffle(size_t &input, size_t shuffle_idx) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Shuffle, f_queue.size(), input, output, shuffle_idx));
    return output;
}

size_t Circuit::unshuffle(size_t &input, size_t shuffle_idx) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Unshuffle, f_queue.size(), input, output, shuffle_idx));
    n_unshuffles++;
    return output;
}

size_t Circuit::merged_shuffle(size_t &input, size_t shuffle_idx, size_t pi_idx, size_t omega_idx) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(MergedShuffle, f_queue.size(), input, output, shuffle_idx, pi_idx, omega_idx));
    return output;
}

size_t Circuit::compaction(size_t &input) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Compaction, f_queue.size(), input, output, n_mults));
    n_mults++;
    return output;
}

size_t Circuit::reveal(size_t &input) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Reveal, f_queue.size(), input, output));
    return output;
}

size_t Circuit::permute(size_t &input, size_t &perm) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Permute, f_queue.size(), input, perm, output));
    return output;
}

size_t Circuit::reverse_permute(size_t &input, size_t &perm) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(ReversePermute, f_queue.size(), input, perm, output));
    return output;
}

size_t Circuit::equals_zero(size_t &input, size_t size, size_t layer) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(EQZ, f_queue.size(), input, output, size, layer, n_mults));
    n_mults++;
    return output;
}

size_t Circuit::bit2A(size_t &input, size_t size) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Bit2A, f_queue.size(), input, output, size, n_mults));
    n_mults++;
    return output;
}

size_t Circuit::deduplication_sub(size_t &input1) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Sub, f_queue.size(), input1, output));
    return output;
}

size_t Circuit::deduplication_insert(size_t &input1) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Insert, f_queue.size(), input1, output));
    return output;
}

size_t Circuit::mul(size_t &x, size_t &y, bool binary) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Mul, f_queue.size(), x, y, output, n_mults, binary));
    n_mults++;
    return output;
}

size_t Circuit::mul(size_t &x, size_t &y, size_t size, bool binary) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Mul, f_queue.size(), x, y, output, size, n_mults, binary));
    n_mults++;
    return output;
}

size_t Circuit::flip(size_t &input) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Flip, f_queue.size(), input, output));
    return output;
}

size_t Circuit::add(size_t &input1, size_t &input2) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(Add, f_queue.size(), input1, input2, output));
    return output;
}

size_t Circuit::add_const(size_t &data, Ring val) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(AddConst, f_queue.size(), data, val, output));
    return output;
}

size_t Circuit::construct_data(std::vector<size_t> &parallel_data) {
    size_t output = n_wires;
    n_wires++;
    f_queue.push_back(std::make_shared<Function>(ConstructData, f_queue.size(), parallel_data[parallel_data.size() - 1], output, parallel_data));
    return output;
}
