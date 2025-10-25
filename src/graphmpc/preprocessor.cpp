#include "preprocessor.h"

void Preprocessor::run(Circuit *circ) {
    if (id != D) {
        size_t n_recv;
        network->recv(D, &n_recv, sizeof(size_t));
        if (ssd) {
            network->recv_vec(D, data->preproc_disk[id], n_recv);
        } else {
            network->recv_vec(D, data->preproc[id], n_recv);
        }
    }

    preprocess(circ);

    if (id == D) {
        for (auto &party : {P0, P1}) {
            if (ssd) {
                auto &pre_disk = data->preproc_disk[party];
                auto &B_disk = data->B_disk[party];

                std::vector<Ring> data_send(pre_disk.size());
                pre_disk.read(data_send, data_send.size());

                std::vector<Ring> B(B_disk.size());
                B_disk.read(B, B_disk.size());
                data_send.insert(data_send.end(), B.begin(), B.end());

                std::vector<Ring>().swap(B);

                size_t n_send = data_send.size();
                network->send(party, &n_send, sizeof(size_t));
                network->send_vec(party, data_send, n_send);

                data->preproc_disk[party].reset();
                data->B_disk[party].reset();

                std::vector<Ring>().swap(data_send);

            } else {
                auto &data_send = data->preproc[party];
                data_send.insert(data_send.end(), data->B[party].begin(), data->B[party].end());
                data->B[party].clear();

                size_t n_send = data_send.size();
                network->send(party, &n_send, sizeof(size_t));
                network->send_vec(party, data_send, n_send);
                data->preproc[party].clear();

                std::vector<Ring>().swap(data_send);
            }
        }
    }
}

std::vector<Ring> Preprocessor::random_share_secret_vec_3P(std::vector<Ring> &secret, bool binary) {
    if (id == D) {
        std::vector<Ring> share_0(secret.size());
        std::vector<Ring> share_1(secret.size());

        if (recv == P1) {
            for (size_t i = 0; i < share_0.size(); ++i) rngs->rng_D0_prep().random_data(&share_0[i], sizeof(Ring));
        }
        if (recv == P0) {
            for (size_t i = 0; i < share_1.size(); ++i) rngs->rng_D1_prep().random_data(&share_0[i], sizeof(Ring));
        }

        if (binary) {
            for (size_t i = 0; i < secret.size(); ++i) share_1[i] = (secret[i] ^ share_0[i]);
        } else {
            for (size_t i = 0; i < secret.size(); ++i) share_1[i] = (secret[i] - share_0[i]);
        }

        data->store_vec(recv, share_1);
        return secret;
    } else if (id == recv) {
        std::vector<Ring> share = data->read_preproc(secret.size());
        return share;
    } else {
        std::vector<Ring> share(secret.size());
        if (id == P0) {
            for (size_t i = 0; i < share.size(); ++i) rngs->rng_D0_prep().random_data(&share[i], sizeof(Ring));
        }
        if (id == P1) {
            for (size_t i = 0; i < share.size(); ++i) rngs->rng_D1_prep().random_data(&share[i], sizeof(Ring));
        }
        return share;
    }
}

void Preprocessor::preprocess(Circuit *circ) {
    for (auto &level : circ->get()) {
        for (auto &f : level) {
            switch (f->type) {
                case Shuffle: {
                    switch (id) {
                        case D: {
                            /* Sampling 1: R_0, R_1*/
                            std::vector<Ring> R_0(size);
                            std::vector<Ring> R_1(size);

                            for (size_t i = 0; i < size; ++i) rngs->rng_D0_send().random_data(&R_0[i], sizeof(Ring));

                            for (size_t i = 0; i < size; ++i) rngs->rng_D1_send().random_data(&R_1[i], sizeof(Ring));

                            if (data->pi_0[f->shuffle_idx].size() == 0) {
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

                                if (recv == P0) {
                                    data->store_vec(P0, pi_0_p.perm_vec);
                                } else {
                                    data->store_vec(P1, pi_1_p.perm_vec);
                                }

                                data->pi_0[f->shuffle_idx] = pi_0;
                                data->pi_1[f->shuffle_idx] = pi_1;
                            }

                            /* Compute B0 / B1 */
                            std::vector<Ring> B_0(size);
                            std::vector<Ring> B_1(size);

                            Ring R;
                            rngs->rng_self().random_data(&R, sizeof(Ring));

                            Permutation pi = data->pi_0[f->shuffle_idx] * data->pi_1[f->shuffle_idx];
                            B_0 = (pi)(R_1);
                            B_1 = (pi)(R_0);

#pragma omp parallel for if (size > 10000)
                            for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

#pragma omp parallel for if (size > 10000)
                            for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

                            data->store_B(P0, B_0);
                            data->store_B(P1, B_1);
                            break;
                        }
                        case P0: {
                            if (recv == P0 && (!data->preprocessed[f->shuffle_idx])) {
                                /* Save pi_0_p */
                                std::vector<Ring> perm_vec = data->read_preproc(size);
                                data->pi_0_p[f->shuffle_idx] = Permutation(perm_vec);
                            }
                            break;
                        }
                        case P1: {
                            if (recv == P1 && !data->preprocessed[f->shuffle_idx]) {
                                /* Save pi_1_p */
                                std::vector<Ring> perm_vec = data->read_preproc(size);
                                data->pi_1_p[f->shuffle_idx] = Permutation(perm_vec);
                            }
                            break;
                        }
                    }

                    /* Alternate receiver of larger message */
                    data->preprocessed[f->shuffle_idx] = true;
                    recv = recv == P0 ? P1 : P0;
                    break;
                }

                case MergedShuffle: {
                    switch (id) {
                        case D: {
                            std::vector<Ring> R_0(size);
                            std::vector<Ring> R_1(size);

                            /* Sampling 1: R_0 / R_1 */
                            for (size_t i = 0; i < size; ++i) rngs->rng_D0_send().random_data(&R_0[i], sizeof(Ring));

                            for (size_t i = 0; i < size; ++i) rngs->rng_D1_send().random_data(&R_1[i], sizeof(Ring));

                            if (data->pi_0[f->shuffle_idx].size() == 0) {
                                /* Computing / Sampling merged permutation */
                                std::vector<Ring> sigma_0_p_vec(size);
                                std::vector<Ring> sigma_1_vec(size);

                                Permutation pi = data->pi_0[f->pi_idx] * data->pi_1[f->pi_idx];
                                Permutation omega = data->pi_0[f->omega_idx] * data->pi_1[f->omega_idx];

                                Permutation merged = pi(omega.perm_vec);

                                Permutation sigma_0 = Permutation::random(size, rngs->rng_D0_recv());
                                Permutation sigma_1_p = Permutation::random(size, rngs->rng_D1_recv());

                                Permutation sigma_0_p = sigma_1_p.inverse() * merged;
                                Permutation sigma_1 = sigma_0.inverse() * merged;

                                sigma_0_p_vec = sigma_0_p.perm_vec;
                                sigma_1_vec = sigma_1.perm_vec;

                                data->pi_0[f->shuffle_idx] = sigma_0;
                                data->pi_1[f->shuffle_idx] = sigma_1;

                                data->store_vec(P0, sigma_0_p_vec);
                                data->store_vec(P1, sigma_1_vec);
                            }

                            std::vector<Ring> B_0(size);
                            std::vector<Ring> B_1(size);

                            Ring R;
                            rngs->rng_self().random_data(&R, sizeof(Ring));

                            /* Compute B_0 / B_1 */
                            Permutation pi = data->pi_0[f->shuffle_idx] * data->pi_1[f->shuffle_idx];
                            B_0 = pi(R_1);
                            B_1 = pi(R_0);

#pragma omp parallel for if (size > 10000)
                            for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;
#pragma omp parallel for if (size > 10000)
                            for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

                            data->store_B(P0, B_0);
                            data->store_B(P1, B_1);
                            break;
                        }
                        case P0: {
                            /* Save pi_0_p */
                            if (!data->preprocessed[f->shuffle_idx]) {
                                std::vector<Ring> sigma_0_p_vec = data->read_preproc(size);
                                data->pi_0_p[f->shuffle_idx] = Permutation(sigma_0_p_vec);
                            }
                            break;
                        }
                        case P1: {
                            /* Save pi_1 */
                            if (!data->preprocessed[f->shuffle_idx]) {
                                std::vector<Ring> sigma_1_vec = data->read_preproc(size);
                                data->pi_1[f->shuffle_idx] = Permutation(sigma_1_vec);
                            }
                            break;
                        }
                    }
                    data->preprocessed[f->shuffle_idx] = true;
                    break;
                }
                case Unshuffle: {
                    if (id == D) {
                        std::vector<Ring> B_0(size);
                        std::vector<Ring> B_1(size);

                        /* Sampling 1: R_0, R_1*/
                        std::vector<Ring> R_0(size);
                        std::vector<Ring> R_1(size);

                        for (size_t i = 0; i < size; ++i) rngs->rng_D0_send().random_data(&R_0[i], sizeof(Ring));

                        for (size_t i = 0; i < size; ++i) rngs->rng_D1_send().random_data(&R_1[i], sizeof(Ring));

                        /* Compute B_0, B_1 */
                        Ring R;
                        rngs->rng_self().random_data(&R, sizeof(Ring));

                        Permutation pi_0 = data->pi_0[f->shuffle_idx];
                        Permutation pi_1 = data->pi_1[f->shuffle_idx];
                        Permutation pi_inv = (pi_0 * pi_1).inverse();

                        B_0 = pi_inv(R_1);
                        B_1 = pi_inv(R_0);

#pragma omp parallel for if (size > 10000)
                        for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

#pragma omp parallel for if (size > 10000)
                        for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

                        data->store_B(P0, B_0);
                        data->store_B(P1, B_1);
                    }
                    break;
                }
                case Compaction:
                case Mul:
                case EQZ:
                case Bit2A: {
                    size_t triple_size = size;
                    if (f->type != Compaction) {
                        triple_size = f->size;
                    }
                    std::vector<Ring> a(triple_size);
                    std::vector<Ring> b(triple_size);
                    std::vector<Ring> c(triple_size);
                    std::vector<Ring> c_final(triple_size);
                    a = share::random_share_vec_3P(id, *rngs, triple_size, f->binary);
                    b = share::random_share_vec_3P(id, *rngs, triple_size, f->binary);

                    if (f->binary) {
                        for (size_t i = 0; i < size; ++i) c[i] = (a[i] & b[i]);
                    } else {
                        for (size_t i = 0; i < size; ++i) c[i] = (a[i] * b[i]);
                    }

                    c_final = random_share_secret_vec_3P(c, f->binary);

                    if (id != D) {
                        data->store_triples(a, b, c_final, f->mult_idx);
                    }
                    /* Alternate receiver */
                    recv = recv == P0 ? P1 : P0;
                    break;
                }
                default:
                    break;
            }
        }
    }
}