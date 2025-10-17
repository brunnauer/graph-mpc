#include "circuit.h"

void Circuit::build() {
    set_inputs();
    pre_mp();
    compute_sorts();
    prepare_shuffles();
    auto data_out = message_passing(in.data);
    data_out = post_mp(data_out);
    output(data_out);
}

void Circuit::set_inputs() {
    in.src_order_bits.resize(bits + 2);
    in.dst_order_bits.resize(bits + 2);
    for (size_t i = 0; i < bits + 2; ++i) {
        in.src_order_bits[i] = input();
    }
    for (size_t i = 0; i < bits + 2; ++i) {
        in.dst_order_bits[i] = input();
    }
    in.isV_inv = input();
    in.data = input();
}

void Circuit::level_order() {
    std::vector<size_t> wire_level(n_wires, 0);
    std::vector<size_t> function_level(f_queue.size(), 0);
    size_t depth = 0;

    for (auto &f : f_queue) {
        size_t max_depth = 0;
        for (size_t i = 0; i < f->in1.size(); ++i) {
            auto wire_depth = wire_level[f->in1[i]];
            max_depth = std::max(max_depth, wire_depth);
        }
        for (size_t i = 0; i < f->in2.size(); ++i) {
            auto wire_depth = wire_level[f->in2[i]];
            max_depth = std::max(max_depth, wire_depth);
        }

        if (f->interactive()) max_depth++;

        function_level[f->f_id] = max_depth;
        for (size_t i = 0; i < f->output.size(); ++i) {
            wire_level[f->output[i]] = max_depth;
        }

        depth = std::max(depth, max_depth);
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

std::vector<Ring> Circuit::message_passing(std::vector<Ring> &data) {
    auto data_shuffled = shuffle(data, ctx.vtx_shuffle_idx);
    auto data_vtx = permute(data_shuffled, ctx.clear_shuffled_vtx_order);

    size_t vtx_src_idx = ++shuffle_idx;
    size_t src_dst_idx = ++shuffle_idx;
    size_t dst_vtx_idx = ++shuffle_idx;

    for (size_t i = 0; i < depth; ++i) {
        data_vtx = add_const(data_vtx, weights[weights.size() - 1 - i]);

        /* Propagate-1 */
        auto data_vtx_propagate = propagate_1(data_vtx);

        /* Switch Perm from vtx to src order */
        data_shuffled = permute(data_vtx_propagate, ctx.clear_shuffled_vtx_order, true);
        data_shuffled = merged_shuffle(data_shuffled, vtx_src_idx, ctx.vtx_shuffle_idx, ctx.src_shuffle_idx);
        auto data_src = permute(data_shuffled, ctx.clear_shuffled_src_order);

        auto data_corr_shuffled = permute(data_vtx, ctx.clear_shuffled_vtx_order, true);
        data_corr_shuffled = merged_shuffle(data_corr_shuffled, vtx_src_idx, ctx.vtx_shuffle_idx, ctx.src_shuffle_idx);
        auto data_corr = permute(data_corr_shuffled, ctx.clear_shuffled_src_order);

        /* Propagate-2 */
        data_src = propagate_2(data_src, data_corr);

        /* Switch Perm from src to dst order */
        data_shuffled = permute(data_src, ctx.clear_shuffled_src_order, true);
        data_shuffled = merged_shuffle(data_shuffled, src_dst_idx, ctx.src_shuffle_idx, ctx.dst_shuffle_idx);
        auto data_dst = permute(data_shuffled, ctx.clear_shuffled_dst_order);

        /* Gather-1 */
        data_dst = gather_1(data_dst);

        /* Switch Perm from dst to vtx order */
        data_shuffled = permute(data_dst, ctx.clear_shuffled_dst_order, true);
        data_shuffled = merged_shuffle(data_shuffled, dst_vtx_idx, ctx.dst_shuffle_idx, ctx.vtx_shuffle_idx);
        data_vtx = permute(data_shuffled, ctx.clear_shuffled_vtx_order);

        /* Gather-2 */
        data_vtx = gather_2(data_vtx);

        data_vtx = apply(data_vtx);
    }
    return data;
}

std::vector<Ring> Circuit::sort(std::vector<std::vector<Ring>> &bit_keys, size_t bits) {
    auto perm = compaction(bit_keys[0]);
    for (size_t bit = 1; bit < bits; ++bit) {
        perm = sort_iteration(perm, bit_keys[bit]);
    }
    return perm;
}

std::vector<Ring> Circuit::sort_iteration(std::vector<Ring> &perm, std::vector<Ring> &keys) {
    auto perm_shuffled = shuffle(perm, shuffle_idx);
    auto keys_shuffled = shuffle(keys, shuffle_idx);

    auto clear_perm_shuffled = reveal(perm_shuffled);
    auto keys_sorted = permute(keys_shuffled, clear_perm_shuffled);

    auto perm_next = compaction(keys_sorted);

    perm_next = permute(perm_next, clear_perm_shuffled, true);
    perm_next = unshuffle(perm_next, shuffle_idx);
    shuffle_idx++;
    return perm_next;
}

/* ----- Single functions ----- */

std::vector<Ring> Circuit::input() {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Input, f_queue.size(), output));
    return output;
}

void Circuit::output(std::vector<Ring> &input) {
    std::vector<Ring> output;
    f_queue.push_back(std::make_shared<Function>(Output, f_queue.size(), input, output));
}

std::vector<Ring> Circuit::propagate_1(std::vector<Ring> &input) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Propagate1, f_queue.size(), input, output));
    return output;
}

std::vector<Ring> Circuit::propagate_2(std::vector<Ring> &input1, std::vector<Ring> &input2) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Propagate2, f_queue.size(), input1, input2, output));
    return output;
}

std::vector<Ring> Circuit::gather_1(std::vector<Ring> &input) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Gather1, f_queue.size(), input, output));
    return output;
}

std::vector<Ring> Circuit::gather_2(std::vector<Ring> &input) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Gather2, f_queue.size(), input, output));
    return output;
}

std::vector<Ring> Circuit::shuffle(std::vector<Ring> &input, size_t shuffle_idx) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Shuffle, f_queue.size(), input, output, shuffle_idx));
    n_shuffles++;

    return output;
}

std::vector<Ring> Circuit::unshuffle(std::vector<Ring> &input, size_t shuffle_idx) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Unshuffle, f_queue.size(), input, output, shuffle_idx));
    n_unshuffles++;
    return output;
}

std::vector<Ring> Circuit::merged_shuffle(std::vector<Ring> &input, size_t shuffle_idx, size_t pi_idx, size_t omega_idx) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(MergedShuffle, f_queue.size(), input, output, shuffle_idx, omega_idx, pi_idx));
    n_shuffles++;

    return output;
}

std::vector<Ring> Circuit::compaction(std::vector<Ring> &input) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Compaction, f_queue.size(), input, output));
    n_triples += size;
    return output;
}

std::vector<Ring> Circuit::reveal(std::vector<Ring> &input) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Reveal, f_queue.size(), input, output));
    return output;
}

std::vector<Ring> Circuit::permute(std::vector<Ring> &input, std::vector<Ring> &perm, bool inverse) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Permute, f_queue.size(), input, perm, output));
    return output;
}

std::vector<Ring> Circuit::equals_zero(std::vector<Ring> &input, size_t size, size_t layer) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(EQZ, f_queue.size(), input, output, size, layer));
    n_triples += size;
    return output;
}

std::vector<Ring> Circuit::bit2A(std::vector<Ring> &input, size_t size) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Bit2A, f_queue.size(), input, output));
    n_triples += size;
    return output;
}

Ring Circuit::sub(Ring &input1, Ring &input2) {
    Ring output = n_wires;
    n_wires++;

    f_queue.push_back(std::make_shared<Function>(Sub, f_queue.size(), input1, input2, output));
    return output;
}

std::vector<Ring> Circuit::mul(std::vector<Ring> &x, std::vector<Ring> &y, bool binary) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Mul, f_queue.size(), x, y, output, binary));
    n_triples += size;
    return output;
}

std::vector<Ring> Circuit::mul(std::vector<Ring> &x, std::vector<Ring> &y, size_t size, bool binary) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Mul, f_queue.size(), x, y, output, size, binary));
    n_triples += size;
    return output;
}

std::vector<Ring> Circuit::flip(std::vector<Ring> &input) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Flip, f_queue.size(), input, output));
    return output;
}

std::vector<Ring> Circuit::add(std::vector<Ring> &input1, std::vector<Ring> &input2) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(Add, f_queue.size(), input1, input2, output));
    return output;
}

std::vector<Ring> Circuit::add_const(std::vector<Ring> &data, Ring val) {
    std::vector<Ring> output(size);
    std::iota(output.begin(), output.end(), n_wires);
    n_wires += size;

    f_queue.push_back(std::make_shared<Function>(AddConst, f_queue.size(), data, val, output));
    return output;
}