#include "circuit.h"

void Circuit::build() {
    set_inputs();
    pre_mp();
    compute_sorts();
    prepare_shuffles();
    SIMD_wire_id data_out;
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
    level_order();
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
    std::vector<size_t> wire_level(gates.size(), 0);
    std::vector<size_t> function_level(gates.size(), 0);
    size_t depth = 0;

    for (auto &f : gates) {
        if (f->type == Output) continue;
        size_t max_depth = 0;

        auto wire_depth = wire_level[f->in1_idx];
        max_depth = std::max(max_depth, wire_depth);

        wire_depth = wire_level[f->in2_idx];
        max_depth = std::max(max_depth, wire_depth);

        if (f->interactive()) max_depth++;

        function_level[f->g_id] = max_depth;
        wire_level[f->out_idx] = max_depth;

        depth = std::max(depth, max_depth);
    }
    for (auto &f : gates) {
        if (f->type == Output) {
            function_level[f->g_id] = depth;
        }
    }

    circ.resize(depth + 1);

    for (size_t i = 0; i < gates.size(); ++i) {
        auto &f = gates[i];
        if (f == nullptr) {
            std::cout << "nullptr function at index " << i << std::endl;
            continue;
        }
        circ[function_level[f->g_id]].push_back(f);
    }
    gates.clear();
}

void Circuit::pre_mp() {}

SIMD_wire_id Circuit::apply(SIMD_wire_id &data_old, SIMD_wire_id &data_new) { return data_new; }

SIMD_wire_id Circuit::post_mp(SIMD_wire_id &data) { return data; }

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

SIMD_wire_id Circuit::message_passing(SIMD_wire_id &data) {
    auto data_shuffled = shuffle(data, ctx.vtx_shuffle_idx);
    auto data_vtx = permute(data_shuffled, ctx.clear_shuffled_vtx_order);

    size_t vtx_src_idx = shuffle_idx + 1;
    size_t src_dst_idx = shuffle_idx + 2;
    size_t dst_vtx_idx = shuffle_idx + 3;

    for (size_t i = 0; i < depth; ++i) {
        auto data_old = data_vtx;
        data_vtx = add_const_SIMD(data_vtx, weights[weights.size() - 1 - i]);

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

SIMD_wire_id Circuit::sort(std::vector<SIMD_wire_id> &bit_keys, SIMD_wire_id bits) {
    auto perm = compaction(bit_keys[0]);
    for (SIMD_wire_id bit = 1; bit < bits; ++bit) {
        perm = sort_iteration(perm, bit_keys[bit]);
    }
    return perm;
}

SIMD_wire_id Circuit::sort_iteration(SIMD_wire_id &perm, SIMD_wire_id &keys) {
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

SIMD_wire_id Circuit::input() {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Input, gates.size(), output));
    return output;
}

void Circuit::output(SIMD_wire_id &input) {
    SIMD_wire_id output;
    gates.push_back(std::make_shared<Gate>(Output, gates.size(), input, output));
}

SIMD_wire_id Circuit::propagate_1(SIMD_wire_id &input) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Propagate1, gates.size(), input, output));
    return output;
}

SIMD_wire_id Circuit::propagate_2(SIMD_wire_id &input1, SIMD_wire_id &input2) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Propagate2, gates.size(), input1, input2, output));
    return output;
}

SIMD_wire_id Circuit::gather_1(SIMD_wire_id &input) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Gather1, gates.size(), input, output));
    return output;
}

SIMD_wire_id Circuit::gather_2(SIMD_wire_id &input) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Gather2, gates.size(), input, output));
    return output;
}

SIMD_wire_id Circuit::shuffle(SIMD_wire_id &input, SIMD_wire_id shuffle_idx) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Shuffle, gates.size(), input, output, shuffle_idx));
    n_shuffles++;
    return output;
}

SIMD_wire_id Circuit::unshuffle(SIMD_wire_id &input, SIMD_wire_id shuffle_idx) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Unshuffle, gates.size(), input, output, shuffle_idx));
    n_shuffles++;
    return output;
}

SIMD_wire_id Circuit::merged_shuffle(SIMD_wire_id &input, SIMD_wire_id shuffle_idx, SIMD_wire_id pi_idx, SIMD_wire_id omega_idx) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(MergedShuffle, gates.size(), input, output, shuffle_idx, pi_idx, omega_idx));
    n_shuffles++;
    return output;
}

SIMD_wire_id Circuit::compaction(SIMD_wire_id &input) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Compaction, gates.size(), input, output, n_mults));
    n_mults++;
    return output;
}

SIMD_wire_id Circuit::reveal(SIMD_wire_id &input) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Reveal, gates.size(), input, output));
    return output;
}

SIMD_wire_id Circuit::permute(SIMD_wire_id &input, SIMD_wire_id &perm) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Permute, gates.size(), input, perm, output));
    return output;
}

SIMD_wire_id Circuit::reverse_permute(SIMD_wire_id &input, SIMD_wire_id &perm) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(ReversePermute, gates.size(), input, perm, output));
    return output;
}

SIMD_wire_id Circuit::_equals_zero(SIMD_wire_id &input, SIMD_wire_id size, SIMD_wire_id layer) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(EQZ, gates.size(), input, output, size, layer, n_mults));
    n_mults++;
    return output;
}

SIMD_wire_id Circuit::equals_zero(SIMD_wire_id &input, SIMD_wire_id size) {
    input = _equals_zero(input, size, 0);
    input = _equals_zero(input, size, 1);
    input = _equals_zero(input, size, 2);
    input = _equals_zero(input, size, 3);
    input = _equals_zero(input, size, 4);
    return input;
}

SIMD_wire_id Circuit::bit2A(SIMD_wire_id &input, SIMD_wire_id size) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Bit2A, gates.size(), input, output, size, n_mults));
    n_mults++;
    return output;
}

SIMD_wire_id Circuit::deduplication_sub(SIMD_wire_id &input1) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(DeduplicationSub, gates.size(), input1, output));
    return output;
}

SIMD_wire_id Circuit::deduplication_insert(SIMD_wire_id &input1) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Insert, gates.size(), input1, output));
    return output;
}

wire_id Circuit::mul(wire_id &x, wire_id &y, bool binary) {
    wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Mul, gates.size(), x, y, output, n_mults, binary));
    n_mults++;
    return output;
}

SIMD_wire_id Circuit::mul_SIMD(SIMD_wire_id &x, SIMD_wire_id &y, bool binary) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(MulSIMD, gates.size(), x, y, output, n_mults, binary));
    n_mults++;
    return output;
}

SIMD_wire_id Circuit::mul_SIMD(SIMD_wire_id &x, SIMD_wire_id &y, SIMD_wire_id size, bool binary) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(MulSIMD, gates.size(), x, y, output, size, n_mults, binary));
    n_mults++;
    return output;
}

SIMD_wire_id Circuit::flip(SIMD_wire_id &input) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Flip, gates.size(), input, output));
    return output;
}

wire_id Circuit::add(wire_id &input1, wire_id &input2) {
    wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Add, gates.size(), input1, input2, output));
    return output;
}

SIMD_wire_id Circuit::add_SIMD(SIMD_wire_id &input1, SIMD_wire_id &input2) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(AddSIMD, gates.size(), input1, input2, output));
    return output;
}

wire_id Circuit::sub(wire_id &input1, wire_id &input2) {
    wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(Sub, gates.size(), input1, input2, output));
    return output;
}

SIMD_wire_id Circuit::sub_SIMD(SIMD_wire_id &input1, SIMD_wire_id &input2) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(SubSIMD, gates.size(), input1, input2, output));
    return output;
}

wire_id Circuit::add_const(wire_id &data, Ring val) {
    wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(AddConst, gates.size(), data, val, output));
    return output;
}

SIMD_wire_id Circuit::add_const_SIMD(SIMD_wire_id &data, Ring val) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(AddConstSIMD, gates.size(), data, val, output));
    return output;
}

wire_id Circuit::mul_const(wire_id &data, Ring val) {
    wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(MulConst, gates.size(), data, val, output));
    return output;
}

SIMD_wire_id Circuit::mul_const_SIMD(SIMD_wire_id &data, Ring val) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(MulConstSIMD, gates.size(), data, val, output));
    return output;
}

SIMD_wire_id Circuit::construct_data(std::vector<SIMD_wire_id> &parallel_data) {
    SIMD_wire_id output = n_wires;
    n_wires++;
    gates.push_back(std::make_shared<Gate>(ConstructData, gates.size(), parallel_data[parallel_data.size() - 1], output, parallel_data));
    return output;
}

SIMD_wire_id Circuit::clip(SIMD_wire_id &data) {
    data = equals_zero(data, nodes);
    data = bit2A(data, nodes);
    return flip(data);
}
