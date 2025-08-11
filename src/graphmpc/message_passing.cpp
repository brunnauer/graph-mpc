#include "message_passing.h"

void mp::prepare_permutations(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc) {
    /* Prepare vtx order */
    auto shuffled_vtx_order = shuffle::shuffle(id, rngs, network, n, preproc.vtx_order_shuffle, preproc.vtx_order);
    auto clear_vtx_order = share::reveal_perm(id, network, shuffled_vtx_order);
    preproc.clear_shuffled_vtx_order = clear_vtx_order;

    /* Prepare src order */
    auto shuffled_src_order = shuffle::shuffle(id, rngs, network, n, preproc.src_order_shuffle, preproc.src_order);
    preproc.clear_shuffled_src_order = share::reveal_perm(id, network, shuffled_src_order);

    /* Prepare dst order */
    auto shuffled_dst_order = shuffle::shuffle(id, rngs, network, n, preproc.dst_order_shuffle, preproc.dst_order);
    preproc.clear_shuffled_dst_order = share::reveal_perm(id, network, shuffled_dst_order);
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