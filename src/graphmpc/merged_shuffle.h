#pragma once

#include "shuffle.h"

class MergedShuffle : public Shuffle {
   public:
    MergedShuffle(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
                  std::vector<Ring> *output, Party &recv, ShufflePre *perm_share, ShufflePre *pi_share, ShufflePre *omega_share)
        : Shuffle(conf, preproc_vals, online_vals, input, output, recv, perm_share), pi_share(pi_share), omega_share(omega_share) {}

    void preprocess() override {
        if (perm_share->preprocessed) return;
        std::vector<Ring> sigma_0_p_vec(size);
        std::vector<Ring> sigma_1_vec(size);

        std::vector<Ring> B_0(size);
        std::vector<Ring> B_1(size);
        std::vector<Ring> R_0, R_1;

        switch (id) {
            case D: {
                Ring rand;

                /* Sampling 1: R_0 / R_1 */
                for (int i = 0; i < size; ++i) {
                    rngs->rng_D0().random_data(&rand, sizeof(Ring));
                    R_0.push_back(rand);
                }

                for (int i = 0; i < size; ++i) {
                    rngs->rng_D1().random_data(&rand, sizeof(Ring));
                    R_1.push_back(rand);
                }

                /* Computing / Sampling merged permutation */
                Permutation pi = pi_share->pi_0 * pi_share->pi_1;
                Permutation omega = omega_share->pi_0 * omega_share->pi_1;

                Permutation merged = pi(omega.perm_vec);

                Permutation sigma_0 = Permutation::random(size, rngs->rng_D0());
                Permutation sigma_1_p = Permutation::random(size, rngs->rng_D1());

                Permutation sigma_0_p = sigma_1_p.inverse() * merged;
                Permutation sigma_1 = sigma_0.inverse() * merged;

                sigma_0_p_vec = sigma_0_p.perm_vec;
                sigma_1_vec = sigma_1.perm_vec;

                perm_share->pi_0 = sigma_0;
                perm_share->pi_1 = sigma_1;

                Ring R;
                rngs->rng_self().random_data(&R, sizeof(Ring));

                /* Compute B_0 / B_1 */
                B_0 = (sigma_0 * sigma_1)(R_1);
                B_1 = (sigma_0 * sigma_1)(R_0);

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

                auto &P0_buf = preproc_vals->at(P0);
                P0_buf.insert(P0_buf.end(), sigma_0_p_vec.begin(), sigma_0_p_vec.end());
                P0_buf.insert(P0_buf.end(), B_0.begin(), B_0.end());
                auto &P1_buf = preproc_vals->at(P1);
                P1_buf.insert(P1_buf.end(), sigma_1_vec.begin(), sigma_1_vec.end());
                P1_buf.insert(P1_buf.end(), B_1.begin(), B_1.end());

                break;
            }
            case P0: {
                sigma_0_p_vec = read_preproc(size);
                B_0 = read_preproc(size);
                perm_share->pi_0_p = Permutation(sigma_0_p_vec);
                perm_share->B = B_0;
                break;
            }
            case P1: {
                sigma_1_vec = read_preproc(size);
                B_1 = read_preproc(size);
                perm_share->pi_1 = Permutation(sigma_1_vec);
                perm_share->B = B_1;

                break;
            }
        }
        perm_share->preprocessed = true;
    }

   private:
    ShufflePre *pi_share;
    ShufflePre *omega_share;
};