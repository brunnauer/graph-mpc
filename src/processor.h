#pragma once

#include <memory>
#include <queue>

#include "io/comm.h"
#include "io/netmp.h"
#include "protocol/clip.h"
#include "protocol/compaction.h"
#include "protocol/message_passing.h"
#include "protocol/permute.h"
#include "protocol/shuffle.h"
#include "protocol/sort.h"
#include "utils/graph.h"
#include "utils/random_generators.h"
#include "utils/sharing.h"
#include "utils/types.h"

enum FunctionToken { SORT, SORT_ITERATION, SWITCH_PERM, APPLY_PERM, CLIP };

class Processor {
   public:
    Processor(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, const size_t BLOCK_SIZE)
        : id(id), rngs(rngs), network(network), n(n), BLOCK_SIZE(BLOCK_SIZE) {}

    void add(FunctionToken function);
    void set_graph(SecretSharedGraph &graph) { g = graph; }

    void run_mp_preprocessing(size_t n_iterations);
    void run_mp_evaluation(size_t n_iterations, size_t n_vertices);

    SecretSharedGraph get_graph() { return g; }

   private:
    Party id;
    RandomGenerators rngs;
    std::shared_ptr<io::NetIOMP> network;
    size_t n;
    const size_t BLOCK_SIZE;

    SecretSharedGraph g;

    std::vector<FunctionToken> queue;
    MPPreprocessing mp_preproc;

    ShufflePre last_shuffle;
    bool pre_completed = false;

    size_t preprocessing_data_size(Party id);
    void run_preprocessing_dealer();
    void run_preprocessing_parties();

    /*
     void run_evaluation_1(std::vector<Ring> &send);
     void run_evaluation_2(std::vector<Ring> &vals);
     size_t evaluation_data_size();
     std::vector<Ring> add_vals(std::vector<Ring> &sent, std::vector<Ring> &received);
     */
};