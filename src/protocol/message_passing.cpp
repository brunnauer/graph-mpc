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

void mp::run(ProtocolConfig &c, SecretSharedGraph &graph, size_t n_iterations, size_t num_v) {
    /* Generate vector containing { 1-isV, src[0], src[1], ..., src[n_bits - 1] } */
    std::vector<std::vector<Row>> src_order_bits(graph.src_bits.size() + 1);

    std::vector<Row> inverted_isV(graph.isV_bits.size());
    for (size_t i = 0; i < inverted_isV.size(); ++i) {
        inverted_isV[i] = -graph.isV_bits[i];
        if (c.pid == P0) inverted_isV[i] += 1;
    }

    std::copy(graph.src_bits.begin(), graph.src_bits.end(), src_order_bits.begin() + 1);
    src_order_bits[0] = inverted_isV;

    /* Generate vector containing { isV, dst[0], dst[1], ..., dst[n_bits - 1] } */
    std::vector<std::vector<Row>> dst_order_bits(graph.dst_bits.size() + 1);

    std::copy(graph.dst_bits.begin(), graph.dst_bits.end(), dst_order_bits.begin() + 1);
    dst_order_bits[0] = graph.isV_bits;

    /* Compute the three permutations */
    Permutation src_order = sort::get_sort(c, src_order_bits);
    Permutation dst_order = sort::get_sort(c, dst_order_bits);
    Permutation vtx_order = sort::sort_iteration(c, src_order, inverted_isV);

    auto src_order_rev = share::reveal_perm(c, src_order);
    auto dst_order_rev = share::reveal_perm(c, dst_order);
    auto vtx_order_rev = share::reveal_perm(c, vtx_order);

    /* Bring payload into vertex order */
    auto payload_v = SecretSharedGraph::from_bits(graph.payload_bits, graph.size);
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
    graph.payload_bits = payload_bits_new;
}
