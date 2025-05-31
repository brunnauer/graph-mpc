#include "message_passing.h"

/* Input vector needs to be in vertex order */
std::vector<Row> mp::propagate_1(std::vector<Row> &input_vector, size_t num_v) {
    std::vector<Row> data(input_vector.size());
    for (size_t i = num_v - 1; i > 0; --i) {
        data[i] = input_vector[i] - input_vector[i - 1];
    }
    data[0] = input_vector[0];
    for (size_t i = num_v; i < data.size(); ++i) {
        data[i] = input_vector[i];
    }
    return data;
}

/* Input vector needs to be in source order */
std::vector<Row> mp::propagate_2(std::vector<Row> &input_vector, std::vector<Row> &correction_vector) {
    std::vector<Row> data(input_vector.size());
    Row sum = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        sum += input_vector[i];
        data[i] = sum - correction_vector[i];
    }
    return data;
}

std::vector<Row> mp::gather_1(std::vector<Row> &input_vector) {
    std::vector<Row> data(input_vector.size());
    Row sum = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        sum += input_vector[i];
        data[i] = sum;
    }
    return data;
}

std::vector<Row> mp::gather_2(std::vector<Row> &input_vector, size_t num_v) {
    std::vector<Row> data(input_vector.size());
    Row sum = 0;
    for (size_t i = 0; i < num_v; ++i) {
        data[i] = input_vector[i] - sum;
        sum += data[i];
    }
    for (size_t i = num_v; i < data.size(); ++i) {
        data[i] = 0;
    }
    return data;
}

std::vector<Row> mp::apply(std::vector<Row> &old_payload, std::vector<Row> &new_payload) {
    std::vector<Row> result(old_payload.size());
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = old_payload[i] + new_payload[i];
    }
    return result;
}

std::tuple<std::vector<std::vector<Row>>, std::vector<std::vector<Row>>, std::vector<Row>> mp::init(ProtocolConfig &c, SecretSharedGraph &g) {
    /* Generate vector containing { 1-isV, src[0], src[1], ..., src[n_bits - 1] } */
    std::vector<std::vector<Row>> src_order_bits(g.src_bits.size() + 1);

    std::vector<Row> inverted_isV(g.isV_bits.size());
    for (size_t i = 0; i < inverted_isV.size(); ++i) {
        inverted_isV[i] = -g.isV_bits[i];
        if (c.pid == P0) inverted_isV[i] += 1;
    }

    std::copy(g.src_bits.begin(), g.src_bits.end(), src_order_bits.begin() + 1);
    src_order_bits[0] = inverted_isV;

    /* Generate vector containing { isV, dst[0], dst[1], ..., dst[n_bits - 1] } */
    std::vector<std::vector<Row>> dst_order_bits(g.dst_bits.size() + 1);

    std::copy(g.dst_bits.begin(), g.dst_bits.end(), dst_order_bits.begin() + 1);
    dst_order_bits[0] = g.isV_bits;

    return {src_order_bits, dst_order_bits, inverted_isV};
}

MPPreprocessing mp::run_preprocess(ProtocolConfig &c, size_t n_iterations) {
    MPPreprocessing preproc;
    size_t n_bits = sizeof(Row) * 8;

    preproc.src_order_pre = sort::get_sort_preprocess(c, n_bits + 1);
    preproc.dst_order_pre = sort::get_sort_preprocess(c, n_bits + 1);
    preproc.vtx_order_pre = sort::get_sort_preprocess(c, 1);
    PermShare vtx_order_shuffle = shuffle::get_shuffle(c);
    std::vector<Row> vtx_order_B = shuffle::get_unshuffle(c, vtx_order_shuffle);
    preproc.vtx_order_pre.perm_share_vec.push_back(vtx_order_shuffle);
    preproc.vtx_order_pre.unshuffle_B_vec.push_back(vtx_order_B);

    preproc.apply_perm_pre = shuffle::get_shuffle(c);

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Four sw_perm preprocessing */
        for (size_t j = 0; j < 4; ++j) preproc.sw_perm_pre.push_back(sort::switch_perm_preprocess(c));
    }
    return preproc;
}

void mp::run_evaluate(ProtocolConfig &c, SecretSharedGraph &g, size_t n_iterations, size_t num_v, MPPreprocessing &preproc) {
    auto [src_order_bits, dst_order_bits, inverted_isV] = init(c, g);
    /* Compute the three permutations */
    Permutation src_order = sort::get_sort_evaluate(c, src_order_bits, preproc.src_order_pre);
    Permutation dst_order = sort::get_sort_evaluate(c, dst_order_bits, preproc.dst_order_pre);
    Permutation vtx_order = sort::sort_iteration_evaluate(c, src_order, inverted_isV, preproc.vtx_order_pre);

    /* Bring payload into vertex order */
    auto payload_v = SecretSharedGraph::from_bits(g.payload_bits, g.size);
    payload_v = sort::apply_perm_evaluate(c, vtx_order, preproc.apply_perm_pre, payload_v);

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Propagate-1 */
        auto payload_p = propagate_1(payload_v, num_v);

        /* Switch Perm to src order */
        auto [pi, omega, merged] = preproc.sw_perm_pre[0];
        auto payload_src = sort::switch_perm_evaluate(c, vtx_order, src_order, pi, omega, merged, payload_p);
        auto [pi_1, omega_1, merged_1] = preproc.sw_perm_pre[1];
        auto payload_corr = sort::switch_perm_evaluate(c, vtx_order, src_order, pi_1, omega_1, merged_1, payload_v);

        /* Propagate-2 */
        payload_p = propagate_2(payload_src, payload_corr);

        /* Switch Perm to dst order*/
        auto [pi_2, omega_2, merged_2] = preproc.sw_perm_pre[2];
        auto payload_dst = sort::switch_perm_evaluate(c, src_order, dst_order, pi_2, omega_2, merged_2, payload_p);

        /* Gather-1*/
        payload_p = gather_1(payload_dst);

        /* Switch Perm to vtx order */
        auto [pi_3, omega_3, merged_3] = preproc.sw_perm_pre[3];
        payload_p = sort::switch_perm_evaluate(c, dst_order, vtx_order, pi_3, omega_3, merged_3, payload_p);

        preproc.sw_perm_pre.erase(preproc.sw_perm_pre.begin(), preproc.sw_perm_pre.begin() + 4);

        /* Gather-2 */
        auto update = gather_2(payload_p, num_v);

        /* Apply */
        payload_v = apply(payload_v, update);
    }

    auto payload_bits_new = SecretSharedGraph::to_bits(payload_v, sizeof(Row) * 8);
    g.payload_bits = payload_bits_new;
}

void mp::run(ProtocolConfig &c, SecretSharedGraph &g, size_t n_iterations, size_t num_v) {
    auto [src_order_bits, dst_order_bits, inverted_isV] = init(c, g);

    /* Compute the three permutations */
    Permutation src_order = sort::get_sort(c, src_order_bits);
    Permutation dst_order = sort::get_sort(c, dst_order_bits);
    Permutation vtx_order = sort::sort_iteration(c, src_order, inverted_isV);

    /* Bring payload into vertex order */
    auto payload_v = SecretSharedGraph::from_bits(g.payload_bits, g.size);
    payload_v = sort::apply_perm(c, vtx_order, payload_v);

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Propagate-1 */
        auto payload_p = propagate_1(payload_v, num_v);

        /* Switch Perm to src order */
        auto payload_src = sort::switch_perm(c, vtx_order, src_order, payload_p);
        auto payload_corr = sort::switch_perm(c, vtx_order, src_order, payload_v);

        /* Propagate-2 */
        payload_p = propagate_2(payload_src, payload_corr);

        /* Switch Perm to dst order*/
        auto payload_dst = sort::switch_perm(c, src_order, dst_order, payload_p);

        /* Gather-1*/
        payload_p = gather_1(payload_dst);

        /* Switch Perm to vtx order */
        payload_p = sort::switch_perm(c, dst_order, vtx_order, payload_p);

        /* Gather-2 */
        auto update = gather_2(payload_p, num_v);

        /* Apply */
        payload_v = apply(payload_v, update);
    }

    auto payload_bits_new = SecretSharedGraph::to_bits(payload_v, sizeof(Row) * 8);
    g.payload_bits = payload_bits_new;
}
