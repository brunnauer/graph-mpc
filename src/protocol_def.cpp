#include "protocol_def.h"

void MPFunctions::pre_mp_preprocessing() {}
void MPFunctions::apply_v_preprocessing() {}
void MPFunctions::post_mp_preprocessing() {}

void MPFunctions::pre_mp_eval(Graph &g) {}

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

void MPFunctions::post_mp_eval(Graph &g) {}
