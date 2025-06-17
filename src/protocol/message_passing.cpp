#include "message_passing.h"

/* Input vector needs to be in vertex order */
std::vector<Ring> mp::propagate_1(std::vector<Ring> &input_vector, size_t n_vertices) {
    std::vector<Ring> data(input_vector.size());
    for (size_t i = n_vertices - 1; i > 0; --i) {
        data[i] = input_vector[i] - input_vector[i - 1];
    }
    data[0] = input_vector[0];
    for (size_t i = n_vertices; i < data.size(); ++i) {
        data[i] = input_vector[i];
    }
    return data;
}

/* Input vector needs to be in source order */
std::vector<Ring> mp::propagate_2(std::vector<Ring> &input_vector, std::vector<Ring> &correction_vector) {
    std::vector<Ring> data(input_vector.size());
    Ring sum = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        sum += input_vector[i];
        data[i] = sum - correction_vector[i];
    }
    return data;
}

/* Input vector needs to be in destination order */
std::vector<Ring> mp::gather_1(std::vector<Ring> &input_vector) {
    std::vector<Ring> data(input_vector.size());
    Ring sum = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        sum += input_vector[i];
        data[i] = sum;
    }
    return data;
}

/* Input vector needs to be in vertex order */
std::vector<Ring> mp::gather_2(std::vector<Ring> &input_vector, size_t n_vertices) {
    std::vector<Ring> data(input_vector.size());
    Ring sum = 0;

    for (size_t i = 0; i < n_vertices; ++i) {
        data[i] = input_vector[i] - sum;
        sum += data[i];
    }
#pragma omp_parallel for if (data.size() - num_v > 1000)
    for (size_t i = n_vertices; i < data.size(); ++i) {
        data[i] = 0;
    }
    return data;
}

std::tuple<std::vector<std::vector<Ring>>, std::vector<std::vector<Ring>>, std::vector<Ring>> mp::init(Party id, SecretSharedGraph &g) {
    /* Generate vector containing { 1-isV, src[0], src[1], ..., src[n_bits - 1] } */
    std::vector<std::vector<Ring>> src_order_bits(g.src_bits.size() + 1);

    std::vector<Ring> inverted_isV(g.isV_bits.size());
    for (size_t i = 0; i < inverted_isV.size(); ++i) {
        inverted_isV[i] = -g.isV_bits[i];
        if (id == P0) inverted_isV[i] += 1;
    }

    std::copy(g.src_bits.begin(), g.src_bits.end(), src_order_bits.begin() + 1);
    src_order_bits[0] = inverted_isV;

    /* Generate vector containing { isV, dst[0], dst[1], ..., dst[n_bits - 1] } */
    std::vector<std::vector<Ring>> dst_order_bits(g.dst_bits.size() + 1);

    std::copy(g.dst_bits.begin(), g.dst_bits.end(), dst_order_bits.begin() + 1);
    dst_order_bits[0] = g.isV_bits;

    return {src_order_bits, dst_order_bits, inverted_isV};
}

MPPreprocessing_Dealer mp::preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n, size_t n_iterations) {
    MPPreprocessing_Dealer preproc;
    size_t n_bits = sizeof(Ring) * 8;

    preproc.src_order_pre = sort::get_sort_preprocess_Dealer(id, rngs, n, n_bits + 1);
    preproc.dst_order_pre = sort::get_sort_preprocess_Dealer(id, rngs, n, n_bits + 1);
    preproc.vtx_order_pre = sort::sort_iteration_preprocess_Dealer(id, rngs, n);

    auto [apply_perm_share, apply_B_0, apply_B_1] = shuffle::get_shuffle_1(id, rngs, n);
    preproc.apply_perm_pre = {apply_B_0, apply_B_1};

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Four sw_perm preprocessing */
        for (size_t j = 0; j < 4; ++j) preproc.sw_perm_pre.push_back(sort::switch_perm_preprocess_Dealer(id, rngs, n));
    }
    return preproc;
}

MPPreprocessing mp::preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, size_t n_iterations, std::vector<Ring> &vals, size_t &idx) {
    MPPreprocessing preproc;
    size_t n_bits = sizeof(Ring) * 8;

    preproc.src_order_pre = sort::get_sort_preprocess_Parties(id, rngs, n, n_bits + 1, vals, idx);
    preproc.dst_order_pre = sort::get_sort_preprocess_Parties(id, rngs, n, n_bits + 1, vals, idx);
    preproc.vtx_order_pre = sort::sort_iteration_preprocess_Parties(id, rngs, n, vals, idx);

    ShufflePre apply_perm_share = shuffle::get_shuffle_2(id, rngs, n, vals, idx, true);
    preproc.apply_perm_pre = apply_perm_share;

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Four sw_perm preprocessing */
        for (size_t j = 0; j < 4; ++j) preproc.sw_perm_pre.push_back(sort::switch_perm_preprocess_Parties(id, rngs, n, vals, idx));
    }
    return preproc;
}

MPPreprocessing mp::preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, size_t n_iterations) {
    MPPreprocessing preproc;
    size_t n_bits = sizeof(Ring) * 8;

    preproc.src_order_pre = sort::get_sort_preprocess(id, rngs, network, n, BLOCK_SIZE, n_bits + 1);
    preproc.dst_order_pre = sort::get_sort_preprocess(id, rngs, network, n, BLOCK_SIZE, n_bits + 1);
    preproc.vtx_order_pre = sort::sort_iteration_preprocess(id, rngs, network, n, BLOCK_SIZE);

    ShufflePre apply_perm_share = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    preproc.apply_perm_pre = apply_perm_share;

    for (size_t i = 0; i < n_iterations; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            preproc.sw_perm_pre.push_back(sort::switch_perm_preprocess(id, rngs, network, n, BLOCK_SIZE));
        }
    }

    preproc.eqz_triples = clip::equals_zero_preprocess(id, rngs, network, n, BLOCK_SIZE);
    preproc.B2A_triples = clip::B2A_preprocess(id, rngs, network, n, BLOCK_SIZE);

    return preproc;
}

void mp::evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, SecretSharedGraph &g,
                  size_t n_iterations, size_t n_vertices, MPPreprocessing &preproc, F_apply f_apply) {
    auto [src_order_bits, dst_order_bits, inverted_isV] = init(id, g);
    /* Compute the three permutations */
    Permutation src_order = sort::get_sort_evaluate(id, rngs, network, n, BLOCK_SIZE, src_order_bits, preproc.src_order_pre);
    Permutation dst_order = sort::get_sort_evaluate(id, rngs, network, n, BLOCK_SIZE, dst_order_bits, preproc.dst_order_pre);
    Permutation vtx_order = sort::sort_iteration_evaluate(id, rngs, network, n, BLOCK_SIZE, src_order, inverted_isV, preproc.vtx_order_pre);

    /* Bring payload into vertex order */
    auto payload_v = SecretSharedGraph::from_bits(g.payload_bits, g.size);
    payload_v = sort::apply_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, vtx_order, preproc.apply_perm_pre, payload_v);

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Propagate-1 */
        auto payload_p = propagate_1(payload_v, n_vertices);

        /* Switch Perm to src order */
        auto [pi, omega, merged] = preproc.sw_perm_pre[0];
        auto payload_src = sort::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, vtx_order, src_order, pi, omega, merged, payload_p);
        auto [pi_1, omega_1, merged_1] = preproc.sw_perm_pre[1];
        auto payload_corr = sort::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, vtx_order, src_order, pi_1, omega_1, merged_1, payload_v);

        /* Propagate-2 */
        payload_p = propagate_2(payload_src, payload_corr);

        /* Switch Perm to dst order*/
        auto [pi_2, omega_2, merged_2] = preproc.sw_perm_pre[2];
        auto payload_dst = sort::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, src_order, dst_order, pi_2, omega_2, merged_2, payload_p);

        /* Gather-1*/
        payload_p = gather_1(payload_dst);

        /* Switch Perm to vtx order */
        auto [pi_3, omega_3, merged_3] = preproc.sw_perm_pre[3];
        payload_p = sort::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, dst_order, vtx_order, pi_3, omega_3, merged_3, payload_p);

        preproc.sw_perm_pre.erase(preproc.sw_perm_pre.begin(), preproc.sw_perm_pre.begin() + 4);

        /* Gather-2 */
        auto update = gather_2(payload_p, n_vertices);

        /* Apply */
        payload_v = f_apply(payload_v, update);
    }

    std::vector<Ring> payload_v_eqz = clip::equals_zero_evaluate(id, rngs, network, BLOCK_SIZE, preproc.eqz_triples, payload_v);
    std::vector<Ring> payload_v_B2A = clip::B2A_evaluate(id, rngs, network, BLOCK_SIZE, preproc.B2A_triples, payload_v_eqz);

    auto payload_v_flip = clip::flip(id, payload_v_B2A);

    auto payload_bits_new = SecretSharedGraph::to_bits(payload_v_flip, sizeof(Ring) * 8);
    g.payload_bits = payload_bits_new;
}

void mp::run(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, SecretSharedGraph &g, size_t n_iterations,
             size_t n_vertices, F_apply f_apply) {
    auto [src_order_bits, dst_order_bits, inverted_isV] = init(id, g);

    /* Compute the three permutations */
    Permutation src_order = sort::get_sort(id, rngs, network, n, BLOCK_SIZE, src_order_bits);
    Permutation dst_order = sort::get_sort(id, rngs, network, n, BLOCK_SIZE, dst_order_bits);
    Permutation vtx_order = sort::sort_iteration(id, rngs, network, n, BLOCK_SIZE, src_order, inverted_isV);

    /* Bring payload into vertex order */
    auto payload_v = SecretSharedGraph::from_bits(g.payload_bits, g.size);
    payload_v = sort::apply_perm(id, rngs, network, n, BLOCK_SIZE, vtx_order, payload_v);

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Propagate-1 */
        auto payload_p = propagate_1(payload_v, n_vertices);

        /* Switch Perm to src order */
        auto payload_src = sort::switch_perm(id, rngs, network, n, BLOCK_SIZE, vtx_order, src_order, payload_p);
        auto payload_corr = sort::switch_perm(id, rngs, network, n, BLOCK_SIZE, vtx_order, src_order, payload_v);

        /* Propagate-2 */
        payload_p = propagate_2(payload_src, payload_corr);

        /* Switch Perm to dst order*/
        auto payload_dst = sort::switch_perm(id, rngs, network, n, BLOCK_SIZE, src_order, dst_order, payload_p);

        /* Gather-1*/
        payload_p = gather_1(payload_dst);

        /* Switch Perm to vtx order */
        payload_p = sort::switch_perm(id, rngs, network, n, BLOCK_SIZE, dst_order, vtx_order, payload_p);

        /* Gather-2 */
        auto update = gather_2(payload_p, n_vertices);

        /* Apply */
        payload_v = f_apply(payload_v, update);
    }

    std::vector<Ring> payload_v_eqz = clip::equals_zero(id, rngs, network, BLOCK_SIZE, payload_v);
    std::vector<Ring> payload_v_B2A = clip::B2A(id, rngs, network, BLOCK_SIZE, payload_v_eqz);

    auto payload_v_flip = clip::flip(id, payload_v_B2A);

    auto payload_bits_new = SecretSharedGraph::to_bits(payload_v_flip, sizeof(Ring) * 8);
    g.payload_bits = payload_bits_new;
}
