#pragma once

#include "function.h"

class Shuffle : public Function {
   public:
    Shuffle(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
            std::vector<Ring> *output, Party &recv)
        : Function(conf, preproc_vals, online_vals, input, output), recv(recv), ssd(conf->ssd), perm_share(new ShufflePre()) {}

    Shuffle(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
            std::vector<Ring> *output, Party &recv, ShufflePre *perm_share)
        : Function(conf, preproc_vals, online_vals, input, output), recv(recv), ssd(conf->ssd), perm_share(perm_share) {}

    ShufflePre *perm_share;

    void preprocess() override {
        if (perm_share->preprocessed) return;

        size_t P0_recv_size, P1_recv_size;
        /* Load balancing */
        if (recv == P1) {
            P0_recv_size = size;
            P1_recv_size = 2 * size;
        } else {
            P0_recv_size = 2 * size;
            P1_recv_size = size;
        }

        std::vector<Ring> D0(P0_recv_size);
        std::vector<Ring> D1(P1_recv_size);

        size_t D0_idx = 0;
        size_t D1_idx = 0;

        switch (id) {
            case D: {
                /* Sampling 1: R_0, R_1*/
                std::vector<Ring> R_0(size);
                std::vector<Ring> R_1(size);

                for (size_t i = 0; i < size; ++i) {
                    rngs->rng_D0_send().random_data(&R_0[i], sizeof(Ring));
                }

                for (size_t i = 0; i < size; ++i) {
                    rngs->rng_D1_send().random_data(&R_1[i], sizeof(Ring));
                }

                Permutation pi_0;
                Permutation pi_1;
                Permutation pi_0_p;
                Permutation pi_1_p;

                /* Sampling 2: pi_0_p, pi_0, pi_1*/
                if (recv == P1) {
                    pi_0_p = Permutation::random(size, rngs->rng_D0_send());
                    pi_0 = Permutation::random(size, rngs->rng_D0_recv());
                    pi_1 = Permutation::random(size, rngs->rng_D1_send());

                    Permutation pi_0_p_inv = pi_0_p.inverse();
                    pi_1_p = (pi_0 * pi_1 * pi_0_p_inv);
                } else {
                    pi_1 = Permutation::random(size, rngs->rng_D1_send());
                    pi_1_p = Permutation::random(size, rngs->rng_D1_recv());
                    pi_0 = Permutation::random(size, rngs->rng_D0_recv());

                    Permutation pi_1_p_inv = pi_1_p.inverse();
                    pi_0_p = (pi_1_p_inv * pi_0 * pi_1);
                }

                perm_share->pi_0 = pi_0;
                perm_share->pi_1 = pi_1;

                for (size_t i = 0; i < size; ++i) {
                    if (recv == P1) {
                        /* Send pi_1_p to P1 */
                        D1[i] = (Ring)pi_1_p[i];
                        D1_idx++;
                    } else {
                        /* Send pi_0_p to P0 */
                        D0[i] = (Ring)pi_0_p[i];
                        D0_idx++;
                    }
                }

                std::vector<Ring> B_0(size);
                std::vector<Ring> B_1(size);

                Ring R;
                rngs->rng_self().random_data(&R, sizeof(Ring));

                Permutation pi = perm_share->pi_0 * perm_share->pi_1;
                B_0 = (pi)(R_1);
                B_1 = (pi)(R_0);

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

                for (size_t i = 0; i < size; ++i) {
                    /* Send B_0 and B_1 to the corresponding buffers */
                    D0[D0_idx] = B_0[i];
                    D1[D1_idx] = B_1[i];
                    D0_idx++;
                    D1_idx++;
                }
                auto &P0_buf = preproc_vals->at(P0);
                P0_buf.insert(P0_buf.end(), D0.begin(), D0.end());
                auto &P1_buf = preproc_vals->at(P1);
                P1_buf.insert(P1_buf.end(), D1.begin(), D1.end());
                break;
            }
            case P0: {
                if (!ssd) {
                    D0 = read_preproc(P0_recv_size);
                    if (recv == P0) {
                        /* Receive pi_0_p */
                        std::vector<Ring> perm_vec(size);
                        for (size_t i = 0; i < size; ++i) {
                            perm_vec[i] = D0[D0_idx];
                            D0_idx++;
                        }
                        perm_share->pi_0_p = Permutation(perm_vec);
                        perm_share->has_pi_0_p = true;
                    }

                    std::vector<Ring> B(size);
                    for (size_t i = 0; i < size; ++i) {
                        B[i] = D0[D0_idx];
                        D0_idx++;
                    }
                    perm_share->B = B;
                }
                break;
            }
            case P1: {
                D1 = read_preproc(P1_recv_size);
                if (ssd) {
                    shuffles_disk->write_vec(D1);
                } else {
                    if (recv == P1) {
                        /* Receive pi_1_p */
                        std::vector<Ring> perm_vec(size);
                        for (int i = 0; i < size; ++i) {
                            perm_vec[i] = D1[D1_idx];
                            D1_idx++;
                        }
                        perm_share->pi_1_p = Permutation(perm_vec);
                        perm_share->has_pi_1_p = true;
                    }

                    std::vector<Ring> B(size);
                    for (size_t i = 0; i < size; ++i) {
                        B[i] = D1[D1_idx];
                        D1_idx++;
                    }
                    perm_share->B = B;
                    perm_share->has_B = true;
                }
                break;
            }
        }

        /* Alternate receiver of larger message */
        recv = recv == P0 ? P1 : P0;
        perm_share->preprocessed = true;
    }

    void evaluate_send() override {
        std::vector<Ring> t(size);
        std::vector<Ring> R(size);
        Permutation perm;

        if (!perm_share->has_R) {
            /* Sampling 1: R_0 / R_1 */
            for (size_t i = 0; i < size; ++i) {
                if (id == P0)
                    rngs->rng_D0_send().random_data(&R[i], sizeof(Ring));
                else if (id == P1)
                    rngs->rng_D1_send().random_data(&R[i], sizeof(Ring));
            }
            perm_share->R = R;
            perm_share->has_R = true;
        } else {
            R = perm_share->R;
        }

        /* Sampling 2: pi_0_p / pi_1 */
        if (id == P0) {
            /* Reuse pi_0_p if saved earlier */
            if (perm_share->has_pi_0_p) {
                perm = perm_share->pi_0_p;
            } else {
                perm = Permutation::random(size, rngs->rng_D0_send());
                perm_share->pi_0_p = perm;
                perm_share->has_pi_0_p = true;
            }
        } else {
            /* Reuse pi_1 if saved earlier */
            if (perm_share->has_pi_1) {
                perm = perm_share->pi_1;
            } else {
                perm = Permutation::random(size, rngs->rng_D1_send());
                perm_share->pi_1 = perm;
                perm_share->has_pi_1 = true;
            }
        }

        /* Compute input + R */
#pragma omp parallel for if (size > 10000)
        for (size_t j = 0; j < size; ++j) {
            t[j] = input->at(j) + R[j];
        }

        /* Compute perm(input + R) */
        std::vector<Ring> t_permuted(size);
        t_permuted = perm(t);
        t.swap(t_permuted);

        online_vals->insert(online_vals->end(), t.begin(), t.end());
    }

    void evaluate_recv() override {
        Permutation perm;
        if (id == P0) {
            if (perm_share->has_pi_0) {
                perm = perm_share->pi_0;
            } else {
                perm = Permutation::random(size, rngs->rng_D0_recv());
                perm_share->pi_0 = perm;
                perm_share->has_pi_0 = true;
            }
        } else {
            if (perm_share->has_pi_1_p) {
                perm = perm_share->pi_1_p;
            } else {
                perm = Permutation::random(size, rngs->rng_D1_recv());
                perm_share->pi_1_p = perm;
                perm_share->has_pi_1_p = true;
            }
        }
        std::vector<Ring> t = read_online(size);  // t1
        t = perm(t);

#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            t[i] -= perm_share->B[i];
        }
        *output = t;
    }

   protected:
    Party &recv;
    bool ssd;

    FileWriter *shuffles_disk;
};
