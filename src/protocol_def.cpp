#include "protocol_def.h"

void MPFunctions::pre_mp_preprocessing(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits,
                                       MPPreprocessing &preproc, Party &recv) {
#if defined PI_K
    deduplication_preprocess(id, rngs, network, n, n_bits, preproc, recv);
#endif
}

void MPFunctions::apply_v_preprocessing(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits,
                                        MPPreprocessing &preproc, Party &recv) {}

void MPFunctions::post_mp_preprocessing(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits,
                                        MPPreprocessing &preproc, Party &recv) {}

void MPFunctions::pre_mp_eval(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, MPPreprocessing &preproc,
                              Graph &g) {
#if defined PI_K
    deduplication_evaluate(id, rngs, network, n, preproc, g);
#endif
}

void MPFunctions::apply_v_eval(std::vector<Ring> &old_data, std::vector<Ring> &new_data) {
/* Implement ApplyV */
#if defined MP
    for (size_t i = 0; i < old_data.size(); ++i) {
        old_data[i] += new_data[i];
    }
#elif defined PI_M || defined PI_K
    old_data = new_data;
#endif
}

void MPFunctions::post_mp_eval(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, Graph &g) {}
