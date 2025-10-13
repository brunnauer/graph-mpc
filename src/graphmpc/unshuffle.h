#pragma once

#include "shuffle.h"

class Unshuffle : public Shuffle {
   public:
    Unshuffle(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
              std::vector<Ring> *output, ShufflePre *perm_share, Party &recv)
        : Shuffle(conf, preproc_vals, online_vals, input, output, recv, perm_share) {}

    void preprocess() override {
        std::vector<Ring> B_0(size);
        std::vector<Ring> B_1(size);

        if (id == D) {
            /* Sampling 1: R_0, R_1*/
            std::vector<Ring> R_0(size);
            std::vector<Ring> R_1(size);

            for (size_t i = 0; i < size; ++i) {
                rngs->rng_D0_send().random_data(&R_0[i], sizeof(Ring));
            }

            for (size_t i = 0; i < size; ++i) {
                rngs->rng_D1_send().random_data(&R_1[i], sizeof(Ring));
            }

            /* Compute B_0, B_1 */
            Ring R;
            rngs->rng_self().random_data(&R, sizeof(Ring));

            Permutation pi_inv = (perm_share->pi_0 * perm_share->pi_1).inverse();

            B_0 = pi_inv(R_1);
            B_1 = pi_inv(R_0);

#pragma omp parallel for if (size > 10000)
            for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

#pragma omp parallel for if (size > 10000)
            for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

            preproc_vals->at(P0).insert(preproc_vals->at(P0).end(), B_0.begin(), B_0.end());
            preproc_vals->at(P1).insert(preproc_vals->at(P1).end(), B_1.begin(), B_1.end());

            return;
        } else {
            if (ssd) {
                unshuffle = unshuffles_disk->read(size);
            } else {
                unshuffle = read_preproc(size);
            }
            return;
        }
    }

    void evaluate_send() override {
        if (id == D) return;

        std::vector<Ring> output_share(size);
        std::vector<Ring> vec_t(size);
        std::vector<Ring> R(size);
        Ring rand;

        /* Sampling 1: R_0 / R_1 */
        for (size_t i = 0; i < size; ++i) {
            if (id == P0)
                rngs->rng_D0_send().random_data(&R[i], sizeof(Ring));
            else
                rngs->rng_D1_send().random_data(&R[i], sizeof(Ring));
        }

#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) vec_t[i] = input->at(i) + R[i];

        Permutation perm = id == P0 ? perm_share->pi_0 : perm_share->pi_1_p;

        std::vector<Ring> vec_t_permuted(size);
        vec_t_permuted = perm.inverse()(vec_t);
        vec_t.swap(vec_t_permuted);

        online_vals->insert(online_vals->end(), vec_t.begin(), vec_t.end());
    }

    void evaluate_recv() override {
        Permutation perm;
        perm = id == P0 ? perm_share->pi_0_p : perm_share->pi_1;
        std::vector<Ring> vec_t = read_online(size);

        /* Apply inverse */
        std::vector<Ring> output_share = perm.inverse()(vec_t);

        /* Last step: subtract B_0 / B_1 */
#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) output_share[i] -= unshuffle[i];

        *output = output_share;
    }

   protected:
    std::vector<Ring> unshuffle;
    FileWriter *unshuffles_disk;
};
