#pragma once

#include <functional>

#include "graphmpc/message_passing.h"
#include "utils/graph.h"
#include "utils/types.h"

namespace MPFunctions {
void pre_mp_preprocessing();
void apply_v_preprocessing();
void post_mp_preprocessing();

void pre_mp_eval(Graph &g);
void apply_v_eval(std::vector<Ring> &old_data, std::vector<Ring> &new_data);
void post_mp_eval(Graph &);

};  // namespace MPFunctions

// #define MP
#define PI_M
//  #define PI_K