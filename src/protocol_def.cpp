#include "protocol_def.h"

void MPFunctions::pre_mp_preprocessing(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits,
                                       MPPreprocessing &preproc, Party &recv_shuffle, Party &recv_mul, bool save_to_disk) {
#if defined PI_K
    deduplication_preprocess(id, rngs, network, n, n_bits, preproc, recv_shuffle, recv_mul, save_to_disk);
#endif
}

void MPFunctions::apply_v_preprocessing(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits,
                                        MPPreprocessing &preproc, Party &recv_shuffle, Party &recv_mul, bool save_to_disk) {}

void MPFunctions::post_mp_preprocessing(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, size_t n_vertices,
                                        MPPreprocessing &preproc, Party &recv_shuffle, Party &recv_mul, bool save_to_disk) {
#if defined PI_R
    clip::equals_zero_preprocess(id, rngs, network, n_vertices, preproc, recv_mul, save_to_disk);
    clip::B2A_preprocess(id, rngs, network, n_vertices, preproc, recv_mul, save_to_disk);
#endif
}

void MPFunctions::pre_mp_eval(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, MPPreprocessing &preproc,
                              Graph &g, bool save_to_disk) {
#if defined PI_K
    deduplication_evaluate(id, rngs, network, n, preproc, g, save_to_disk);
#endif
}

void MPFunctions::apply_v_eval(std::vector<Ring> &old_data, std::vector<Ring> &new_data, bool save_to_disk) {
/* Implement ApplyV */
#if defined MP || defined PI_R
    for (size_t i = 0; i < old_data.size(); ++i) {
        old_data[i] += new_data[i];
    }
#elif defined PI_M || defined PI_K
    old_data = new_data;
#endif
}

void MPFunctions::post_mp_eval(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, MPPreprocessing &preproc,
                               Graph &g, bool save_to_disk) {
#if defined PI_R
    std::vector<Ring> nodes_data(g._n_vertices);
    std::copy(g._data.begin(), g._data.begin() + nodes_data.size(), nodes_data.begin());

    auto data_p = clip::equals_zero_evaluate(id, rngs, network, preproc, nodes_data, save_to_disk);
    data_p = clip::B2A_evaluate(id, rngs, network, g._n_vertices, preproc, data_p, save_to_disk);
    data_p = clip::flip(id, data_p);

    std::copy(data_p.begin(), data_p.end(), g._data.begin());
#endif
}
