#include "preprocessor.h"

void Preprocessor::run(Circuit *circ) {
    if (id != D) {
        size_t n_recv;
        network->recv(D, &n_recv, sizeof(size_t));
        if (ssd) {
            network->recv_vec(D, n_recv, store->preproc_disk);
        } else {
            network->recv_vec(D, n_recv, store->preproc[id]);
        }
    }

    preprocess(circ);

    if (id == D) {
        for (auto &party : {P0, P1}) {
            auto &data_send = store->preproc[party];
            data_send.insert(data_send.end(), store->B[party].begin(), store->B[party].end());
            store->B[party].clear();

            size_t n_send = data_send.size();
            network->send(party, &n_send, sizeof(size_t));
            network->send_vec(party, n_send, data_send);
            store->preproc[party].clear();

            std::vector<Ring>().swap(data_send);
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

        store->preproc[recv].insert(store->preproc[recv].end(), share_1.begin(), share_1.end());
        return secret;
    } else if (id == recv) {
        std::vector<Ring> share(secret.size());
        store->read_preproc(share, secret.size());
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

                            if (store->pi_0[f->shuffle_idx].size() == 0) {
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
                                    store->preproc[P0].insert(store->preproc[P0].end(), pi_0_p.perm_vec.begin(), pi_0_p.perm_vec.end());
                                } else {
                                    store->preproc[P1].insert(store->preproc[P1].end(), pi_1_p.perm_vec.begin(), pi_1_p.perm_vec.end());
                                }

                                store->pi_0[f->shuffle_idx] = pi_0;
                                store->pi_1[f->shuffle_idx] = pi_1;
                            }

                            /* Compute B0 / B1 */
                            std::vector<Ring> B_0(size);
                            std::vector<Ring> B_1(size);

                            Ring R;
                            rngs->rng_self().random_data(&R, sizeof(Ring));

                            Permutation pi = store->pi_0[f->shuffle_idx] * store->pi_1[f->shuffle_idx];
                            B_0 = (pi)(R_1);
                            B_1 = (pi)(R_0);

#pragma omp parallel for if (size > 10000)
                            for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

#pragma omp parallel for if (size > 10000)
                            for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

                            store->B[P0].insert(store->B[P0].end(), B_0.begin(), B_0.end());
                            store->B[P1].insert(store->B[P1].end(), B_1.begin(), B_1.end());
                            break;
                        }
                        case P0: {
                            if (recv == P0 && (!store->preprocessed[f->shuffle_idx])) {
                                /* Save pi_0_p */
                                std::vector<Ring> perm_vec;
                                store->read_preproc(perm_vec, size);
                                store->pi_0_p[f->shuffle_idx] = Permutation(perm_vec);
                            }
                            break;
                        }
                        case P1: {
                            if (recv == P1 && !store->preprocessed[f->shuffle_idx]) {
                                /* Save pi_1_p */
                                std::vector<Ring> perm_vec;
                                store->read_preproc(perm_vec, size);
                                store->pi_1_p[f->shuffle_idx] = Permutation(perm_vec);
                            }
                            break;
                        }
                    }

                    /* Alternate receiver of larger message */
                    store->preprocessed[f->shuffle_idx] = true;
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

                            if (store->pi_0[f->shuffle_idx].size() == 0) {
                                /* Computing / Sampling merged permutation */
                                std::vector<Ring> sigma_0_p_vec(size);
                                std::vector<Ring> sigma_1_vec(size);

                                Permutation pi = store->pi_0[f->pi_idx] * store->pi_1[f->pi_idx];
                                Permutation omega = store->pi_0[f->omega_idx] * store->pi_1[f->omega_idx];

                                Permutation merged = pi(omega.perm_vec);

                                Permutation sigma_0 = Permutation::random(size, rngs->rng_D0_recv());
                                Permutation sigma_1_p = Permutation::random(size, rngs->rng_D1_recv());

                                Permutation sigma_0_p = sigma_1_p.inverse() * merged;
                                Permutation sigma_1 = sigma_0.inverse() * merged;

                                sigma_0_p_vec = sigma_0_p.perm_vec;
                                sigma_1_vec = sigma_1.perm_vec;

                                store->pi_0[f->shuffle_idx] = sigma_0;
                                store->pi_1[f->shuffle_idx] = sigma_1;

                                store->preproc[P0].insert(store->preproc[P0].end(), sigma_0_p_vec.begin(), sigma_0_p_vec.end());
                                store->preproc[P1].insert(store->preproc[P1].end(), sigma_1_vec.begin(), sigma_1_vec.end());
                            }

                            std::vector<Ring> B_0(size);
                            std::vector<Ring> B_1(size);

                            Ring R;
                            rngs->rng_self().random_data(&R, sizeof(Ring));

                            /* Compute B_0 / B_1 */
                            Permutation pi = store->pi_0[f->shuffle_idx] * store->pi_1[f->shuffle_idx];
                            B_0 = pi(R_1);
                            B_1 = pi(R_0);

#pragma omp parallel for if (size > 10000)
                            for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;
#pragma omp parallel for if (size > 10000)
                            for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

                            store->B[P0].insert(store->B[P0].end(), B_0.begin(), B_0.end());
                            store->B[P1].insert(store->B[P1].end(), B_1.begin(), B_1.end());
                            break;
                        }
                        case P0: {
                            /* Save pi_0_p */
                            if (!store->preprocessed[f->shuffle_idx]) {
                                std::vector<Ring> sigma_0_p_vec(size);
                                store->read_preproc(sigma_0_p_vec, size);
                                store->pi_0_p[f->shuffle_idx] = Permutation(sigma_0_p_vec);
                            }
                            break;
                        }
                        case P1: {
                            /* Save pi_1 */
                            if (!store->preprocessed[f->shuffle_idx]) {
                                std::vector<Ring> sigma_1_vec(size);
                                store->read_preproc(sigma_1_vec, size);
                                store->pi_1[f->shuffle_idx] = Permutation(sigma_1_vec);
                            }
                            break;
                        }
                    }
                    store->preprocessed[f->shuffle_idx] = true;
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

                        Permutation pi_0 = store->pi_0[f->shuffle_idx];
                        Permutation pi_1 = store->pi_1[f->shuffle_idx];
                        Permutation pi_inv = (pi_0 * pi_1).inverse();

                        B_0 = pi_inv(R_1);
                        B_1 = pi_inv(R_0);

#pragma omp parallel for if (size > 10000)
                        for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

#pragma omp parallel for if (size > 10000)
                        for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

                        store->B[P0].insert(store->B[P0].end(), B_0.begin(), B_0.end());
                        store->B[P1].insert(store->B[P1].end(), B_1.begin(), B_1.end());
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
                        store->store_triples(a, b, c_final, f->mult_idx);
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