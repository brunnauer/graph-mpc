#include "message_passing.h"

/* Input vector needs to be in vertex order */
std::vector<Row> mp::propagate_1(ProtocolConfig &conf, std::vector<Row> &input_vector, size_t num_v) {
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
std::vector<Row> mp::propagate_2(ProtocolConfig &conf, std::vector<Row> &input_vector, std::vector<Row> &correction_vector) {
    std::vector<Row> data(input_vector.size());
    Row sum = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        sum += input_vector[i];
        data[i] = sum - correction_vector[i];
    }
    return data;
}

std::vector<Row> mp::gather_1(ProtocolConfig &conf, std::vector<Row> &input_vector) {
    std::vector<Row> data(input_vector.size());
    Row sum = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        sum += input_vector[i];
        data[i] = sum;
    }
    return data;
}

std::vector<Row> mp::gather_2(ProtocolConfig &conf, std::vector<Row> &input_vector, size_t num_v) {
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

std::vector<Row> mp::apply(ProtocolConfig &conf, std::vector<Row> &old_payload, std::vector<Row> &new_payload) {
    std::vector<Row> result(old_payload.size());
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = old_payload[i] + new_payload[i];
    }
    return result;
}

void mp::run(ProtocolConfig &conf, SecretSharedGraph &graph, size_t n_iterations, size_t num_vert) {
    auto pid = conf.pid;
    auto network = conf.network;

    /* Generate vector containing { 1-isV, src[0], src[1], ..., src[n_bits - 1] } */
    std::vector<std::vector<Row>> src_order_bits(graph.src_bits.size() + 1);

    std::vector<Row> inverted_isV(graph.isV_bits.size());
    for (size_t i = 0; i < inverted_isV.size(); ++i) {
        inverted_isV[i] = -graph.isV_bits[i];
        if (pid == P0) inverted_isV[i] += 1;
    }

    std::copy(graph.src_bits.begin(), graph.src_bits.end(), src_order_bits.begin() + 1);
    src_order_bits[0] = inverted_isV;

    /* Generate vector containing { isV, dst[0], dst[1], ..., dst[n_bits - 1] } */
    std::vector<std::vector<Row>> dst_order_bits(graph.dst_bits.size() + 1);

    std::copy(graph.dst_bits.begin(), graph.dst_bits.end(), dst_order_bits.begin() + 1);
    dst_order_bits[0] = graph.isV_bits;

    /* Compute the three permutations */
    std::cout << "Computing the three permutations ...";
    StatsPoint perm_start(*network);
    Permutation src_order = sort::get_sort(conf, src_order_bits);
    Permutation dst_order = sort::get_sort(conf, dst_order_bits);
    Permutation vtx_order = sort::sort_iteration(conf, src_order, inverted_isV);
    StatsPoint perm_end(*network);
    auto time = (perm_end - perm_start)["time"];
    std::cout << "Done. " << "time: " << time << std::endl << std::endl;

    std::cout << "Revealing the three permutations ...";
    StatsPoint rev_start(*network);
    auto src_order_rev = share::reveal(conf, src_order);
    auto dst_order_rev = share::reveal(conf, dst_order);
    auto vtx_order_rev = share::reveal(conf, vtx_order);
    StatsPoint rev_end(*network);
    time = (rev_end - rev_start)["time"];
    std::cout << "Done. " << "time: " << time << std::endl << std::endl;
    std::cout << "Done." << std::endl;

    /* Bring payload into vertex order */
    auto payload_v = SecretSharedGraph::from_bits(graph.payload_bits, graph.size);
    std::cout << "Applying vertex order ..." << std::endl;
    payload_v = sort::apply_perm(conf, vtx_order, payload_v);
    std::cout << "Done." << std::endl;

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Propagate-1 */
        std::cout << "Propagate-1 ..." << std::endl;
        auto payload_p = propagate_1(conf, payload_v, num_vert);
        std::cout << "Done." << std::endl;

        /* Switch Perm to src order */
        std::cout << "Sw-perm 1 ..." << std::endl;
        auto payload_src = sort::switch_perm(conf, vtx_order, src_order, payload_p);
        std::cout << "Done." << std::endl;
        std::cout << "Sw-perm 2 ..." << std::endl;
        auto payload_corr = sort::switch_perm(conf, vtx_order, src_order, payload_v);
        std::cout << "Done." << std::endl;

        /* Propagate-2 */
        std::cout << "Propagate-2 ..." << std::endl;
        payload_p = propagate_2(conf, payload_src, payload_corr);
        std::cout << "Done." << std::endl;

        /* Switch Perm to dst order*/
        std::cout << "Sw-perm 3 ..." << std::endl;
        auto payload_dst = sort::switch_perm(conf, src_order, dst_order, payload_p);
        std::cout << "Done." << std::endl;

        /* Gather-1*/
        std::cout << "Gather-1 ..." << std::endl;
        payload_p = gather_1(conf, payload_dst);
        std::cout << "Done." << std::endl;

        /* Switch Perm to vtx order */
        std::cout << "Sw-perm 4 ..." << std::endl;
        payload_p = sort::switch_perm(conf, dst_order, vtx_order, payload_p);
        std::cout << "Done." << std::endl;

        /* Gather-2 */
        std::cout << "Gather-2 ..." << std::endl;
        auto update = gather_2(conf, payload_p, num_vert);
        std::cout << "Done." << std::endl;

        /* Apply */
        std::cout << "Apply ..." << std::endl;
        payload_v = apply(conf, payload_v, update);
        std::cout << "Done." << std::endl;
    }

    std::cout << "to_bits ..." << std::endl;
    auto payload_bits_new = SecretSharedGraph::to_bits(payload_v, sizeof(Row) * 8);
    graph.payload_bits = payload_bits_new;
}
