#include "message_passing.h"

/* ----- Helper Functions ----- */
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

/* ----- Preprocessing ----- */
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
        for (size_t j = 0; j < 4; ++j) preproc.sw_perm_pre.push_back(permute::switch_perm_preprocess_Dealer(id, rngs, n));
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
    // preproc.apply_perm_pre = apply_perm_share;

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Four sw_perm preprocessing */
        for (size_t j = 0; j < 4; ++j) preproc.sw_perm_pre.push_back(permute::switch_perm_preprocess_Parties(id, rngs, n, vals, idx));
    }
    return preproc;
}

MPPreprocessing mp::preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, size_t n_bits,
                               size_t n_iterations, F_pre_mp_preprocess f_preprocess, F_post_mp_preprocess f_postprocess) {
    MPPreprocessing preproc;

    f_preprocess(id, rngs, network, n, BLOCK_SIZE, preproc);
    preproc.src_order_pre = sort::get_sort_preprocess(id, rngs, network, n, BLOCK_SIZE, n_bits + 1);
    preproc.dst_order_pre = sort::get_sort_preprocess(id, rngs, network, n, BLOCK_SIZE, n_bits + 1);
    preproc.vtx_order_pre = sort::sort_iteration_preprocess(id, rngs, network, n, BLOCK_SIZE);

    preproc.apply_perm_share = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);

    for (size_t j = 0; j < 3; ++j) {
        preproc.sw_perm_pre.push_back(permute::switch_perm_preprocess(id, rngs, network, n, BLOCK_SIZE));
    }

    f_postprocess(id, rngs, network, n, BLOCK_SIZE, preproc);

    return preproc;
}

/* ----- Evaluation ----- */
void mp::evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, size_t n_bits, size_t n_iterations,
                  size_t n_vertices, SecretSharedGraph &g, std::vector<Ring> &weights, F_apply f_apply, F_pre_mp_evaluate f_preprocess,
                  F_post_mp_evaluate f_postprocess, MPPreprocessing &preproc) {
    /* Preprocess graph before message passing */
    f_preprocess(id, rngs, network, n, BLOCK_SIZE, preproc, g);

    /* Get the bit vectors for performing get_sort */
    auto [src_order_bits, dst_order_bits, inverted_isV] = init(id, g);

    /* Compute the three permutations */
    Permutation src_order = sort::get_sort_evaluate(id, rngs, network, n, BLOCK_SIZE, src_order_bits, preproc.src_order_pre);
    Permutation dst_order = sort::get_sort_evaluate(id, rngs, network, n, BLOCK_SIZE, dst_order_bits, preproc.dst_order_pre);
    Permutation vtx_order = sort::sort_iteration_evaluate(id, rngs, network, n, BLOCK_SIZE, src_order, inverted_isV, preproc.vtx_order_pre);

    /* Bring payload into vertex order */
    // auto payload_v = from_bits(g.payload_bits, g.size);
    auto payload_v = g.payload;
    payload_v = permute::apply_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, vtx_order, preproc.apply_perm_share, payload_v);

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Add current weight only to vertices */
        if (id == P0) {
            for (size_t j = 0; j < n_vertices; ++j) payload_v[j] += weights[weights.size() - 1 - i];
        }

        /* Propagate-1 */
        auto payload_p = propagate_1(payload_v, n_vertices);

        /* Switch Perm to src order */
        auto payload_src = permute::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, vtx_order, src_order, preproc.sw_perm_pre[0], payload_p);
        auto payload_corr = permute::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, vtx_order, src_order, preproc.sw_perm_pre[0], payload_v);

        /* Propagate-2 */
        payload_p = propagate_2(payload_src, payload_corr);

        /* Switch Perm to dst order*/
        auto payload_dst = permute::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, src_order, dst_order, preproc.sw_perm_pre[1], payload_p);

        /* Gather-1*/
        payload_p = gather_1(payload_dst);

        /* Switch Perm to vtx order */
        payload_p = permute::switch_perm_evaluate(id, rngs, network, n, BLOCK_SIZE, dst_order, vtx_order, preproc.sw_perm_pre[2], payload_p);

        /* Gather-2 */
        auto update = gather_2(payload_p, n_vertices);

        /* Apply */
        payload_v = f_apply(payload_v, update);
    }

    f_postprocess(id, rngs, network, n, BLOCK_SIZE, g, preproc, payload_v);
}

/* ----- Ad-Hoc Preprocessing ----- */

void mp::run(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, size_t n_iterations, size_t n_vertices,
             SecretSharedGraph &g, std::vector<Ring> &weights, F_apply f_apply, F_pre_mp f_preprocess, F_post_mp f_postprocess) {
    /* Preprocess graph before message passing */
    f_preprocess(id, rngs, network, n, BLOCK_SIZE, g);

    auto [src_order_bits, dst_order_bits, inverted_isV] = init(id, g);

    /* Compute the three permutations */
    Permutation src_order = sort::get_sort(id, rngs, network, n, BLOCK_SIZE, src_order_bits);
    Permutation dst_order = sort::get_sort(id, rngs, network, n, BLOCK_SIZE, dst_order_bits);
    Permutation vtx_order = sort::sort_iteration(id, rngs, network, n, BLOCK_SIZE, src_order, inverted_isV);

    /* Bring payload into vertex order */
    auto payload_v = from_bits(g.payload_bits, g.size);
    payload_v = permute::apply_perm(id, rngs, network, n, BLOCK_SIZE, vtx_order, payload_v);

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Add current weight */
        if (id == P0) {
            for (size_t j = 0; j < n_vertices; ++j) payload_v[j] += weights[weights.size() - 1 - i];
        }

        /* Propagate-1 */
        auto payload_p = propagate_1(payload_v, n_vertices);

        /* Switch Perm to src order */
        auto payload_src = permute::switch_perm(id, rngs, network, n, BLOCK_SIZE, vtx_order, src_order, payload_p);
        auto payload_corr = permute::switch_perm(id, rngs, network, n, BLOCK_SIZE, vtx_order, src_order, payload_v);

        /* Propagate-2 */
        payload_p = propagate_2(payload_src, payload_corr);

        /* Switch Perm to dst order*/
        auto payload_dst = permute::switch_perm(id, rngs, network, n, BLOCK_SIZE, src_order, dst_order, payload_p);

        /* Gather-1*/
        payload_p = gather_1(payload_dst);

        /* Switch Perm to vtx order */
        payload_p = permute::switch_perm(id, rngs, network, n, BLOCK_SIZE, dst_order, vtx_order, payload_p);

        /* Gather-2 */
        auto update = gather_2(payload_p, n_vertices);

        /* Apply */
        payload_v = f_apply(payload_v, update);
    }

    f_postprocess(id, rngs, network, n, BLOCK_SIZE, g, payload_v);
}
