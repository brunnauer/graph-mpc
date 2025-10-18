#include "preprocessor.h"

void Preprocessor::run(Circuit *circ) {
    if (id != D) {
        size_t n_recv;
        network->recv(D, &n_recv, sizeof(size_t));
        if (ssd) {
            network->recv_vec(D, n_recv, preproc_disk);
        } else {
            network->recv_vec(D, n_recv, preproc[id]);
        }
    }

    preprocess(circ);
    preproc_disk.reset_idx();

    if (id == D) {
        for (auto &party : {P0, P1}) {
            auto &data_send = preproc[party];
            size_t n_send = data_send.size();
            network->send(party, &n_send, sizeof(size_t));
            network->send_vec(party, n_send, data_send);
            preproc[party].clear();
        }
    }
}

std::vector<Ring> Preprocessor::read_preproc(size_t n_elems) {
    if (ssd) {
        return preproc_disk.read(n_elems);
    } else {
        if (n_elems > preproc[id].size()) {
            throw new std::invalid_argument("Cannot read more preproc values than available.");
        } else {
            std::vector<Ring> data(n_elems);
            data = {preproc[id].begin(), preproc[id].begin() + n_elems};

            /* Delete read values from buffer */
            preproc[id].erase(preproc[id].begin(), preproc[id].begin() + n_elems);

            return data;
        }
    }
}

std::vector<Ring> Preprocessor::random_share_secret_vec_3P(std::vector<Ring> &secret, bool binary) {
    if (id == D) {
        assert(secret.size() == size);
        std::vector<Ring> share_0(size);
        std::vector<Ring> share_1(size);

        if (recv == P1) {
            for (size_t i = 0; i < size; ++i) rngs->rng_D0_prep().random_data(&share_0[i], sizeof(Ring));
        }
        if (recv == P0) {
            for (size_t i = 0; i < size; ++i) rngs->rng_D1_prep().random_data(&share_0[i], sizeof(Ring));
        }

        if (binary) {
            for (size_t i = 0; i < size; ++i) share_1[i] = (secret[i] ^ share_0[i]);
        } else {
            for (size_t i = 0; i < size; ++i) share_1[i] = (secret[i] - share_0[i]);
        }

        preproc[recv].insert(preproc[recv].end(), share_1.begin(), share_1.end());
        return secret;
    } else if (id == recv) {
        return read_preproc(size);
    } else {
        std::vector<Ring> share(size);
        if (id == P0) {
            for (size_t i = 0; i < size; ++i) rngs->rng_D0_prep().random_data(&share[i], sizeof(Ring));
        }
        if (id == P1) {
            for (size_t i = 0; i < size; ++i) rngs->rng_D1_prep().random_data(&share[i], sizeof(Ring));
        }
        return share;
    }
}

void Preprocessor::preprocess(Circuit *circ) {
    for (auto &level : circ->get()) {
        for (auto &f : level) {
            switch (f->type) {
                case Shuffle: {
                    auto perm_share = store->load_shuffle(f->shuffle_idx);
                    if (perm_share->preprocessed) break;  // Already preprocessed

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

                            for (size_t i = 0; i < size; ++i) rngs->rng_D0_send().random_data(&R_0[i], sizeof(Ring));

                            for (size_t i = 0; i < size; ++i) rngs->rng_D1_send().random_data(&R_1[i], sizeof(Ring));

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
                            auto &P0_buf = preproc[P0];
                            P0_buf.insert(P0_buf.end(), D0.begin(), D0.end());
                            auto &P1_buf = preproc[P1];
                            P1_buf.insert(P1_buf.end(), D1.begin(), D1.end());
                            break;
                        }
                        case P0: {
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
                            store->store_shuffle(f->shuffle_idx);
                            break;
                        }
                        case P1: {
                            D1 = read_preproc(P1_recv_size);
                            if (recv == P1) {
                                /* Receive pi_1_p */
                                std::vector<Ring> perm_vec(size);
                                for (size_t i = 0; i < size; ++i) {
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
                            store->store_shuffle(f->shuffle_idx);
                            break;
                        }
                    }

                    /* Alternate receiver of larger message */
                    perm_share->preprocessed = true;
                    recv = recv == P0 ? P1 : P0;
                    break;
                }

                case MergedShuffle: {
                    auto perm_share = store->load_shuffle(f->shuffle_idx);
                    if (perm_share->preprocessed) break;  // Already preprocessed

                    auto pi_share = store->load_shuffle(f->pi_idx);
                    auto omega_share = store->load_shuffle(f->omega_idx);

                    switch (id) {
                        case D: {
                            std::vector<Ring> sigma_0_p_vec(size);
                            std::vector<Ring> sigma_1_vec(size);

                            std::vector<Ring> B_0(size);
                            std::vector<Ring> B_1(size);

                            std::vector<Ring> R_0(size);
                            std::vector<Ring> R_1(size);

                            /* Sampling 1: R_0 / R_1 */
                            for (size_t i = 0; i < size; ++i) {
                                rngs->rng_D0_send().random_data(&R_0[i], sizeof(Ring));
                            }

                            for (size_t i = 0; i < size; ++i) {
                                rngs->rng_D1_send().random_data(&R_1[i], sizeof(Ring));
                            }

                            /* Computing / Sampling merged permutation */
                            Permutation pi = pi_share->pi_0 * pi_share->pi_1;
                            Permutation omega = omega_share->pi_0 * omega_share->pi_1;

                            Permutation merged = pi(omega.perm_vec);

                            Permutation sigma_0 = Permutation::random(size, rngs->rng_D0_recv());
                            Permutation sigma_1_p = Permutation::random(size, rngs->rng_D1_recv());

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

                            auto &P0_buf = preproc[P0];
                            P0_buf.insert(P0_buf.end(), sigma_0_p_vec.begin(), sigma_0_p_vec.end());
                            P0_buf.insert(P0_buf.end(), B_0.begin(), B_0.end());
                            auto &P1_buf = preproc[P1];
                            P1_buf.insert(P1_buf.end(), sigma_1_vec.begin(), sigma_1_vec.end());
                            P1_buf.insert(P1_buf.end(), B_1.begin(), B_1.end());

                            break;
                        }
                        case P0: {
                            std::vector<Ring> sigma_0_p_vec(size);
                            std::vector<Ring> B_0(size);
                            sigma_0_p_vec = read_preproc(size);
                            B_0 = read_preproc(size);
                            perm_share->pi_0_p = Permutation(sigma_0_p_vec);
                            perm_share->has_pi_0_p = true;
                            perm_share->B = B_0;
                            store->store_shuffle(f->shuffle_idx);
                            break;
                        }
                        case P1: {
                            std::vector<Ring> sigma_1_vec(size);
                            std::vector<Ring> B_1(size);
                            sigma_1_vec = read_preproc(size);
                            B_1 = read_preproc(size);
                            perm_share->pi_1 = Permutation(sigma_1_vec);
                            perm_share->has_pi_1 = true;
                            perm_share->B = B_1;
                            store->store_shuffle(f->shuffle_idx);
                            break;
                        }
                    }
                    perm_share->preprocessed = true;
                    break;
                }
                case Unshuffle: {
                    auto perm_share = store->load_shuffle(f->shuffle_idx);
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

                        preproc[P0].insert(preproc[P0].end(), B_0.begin(), B_0.end());
                        preproc[P1].insert(preproc[P1].end(), B_1.begin(), B_1.end());
                    } else {
                        auto unshuffle = read_preproc(size);
                        store->store_unshuffle(unshuffle);
                    }
                    break;
                }
                case Compaction:
                case Mul:
                case EQZ:
                case Bit2A: {
                    std::vector<Ring> a(size);
                    std::vector<Ring> b(size);
                    std::vector<Ring> c(size);
                    std::vector<Ring> c_final(size);
                    a = share::random_share_vec_3P(id, *rngs, size, f->binary);
                    b = share::random_share_vec_3P(id, *rngs, size, f->binary);

                    if (f->binary) {
                        for (size_t i = 0; i < size; ++i) c[i] = (a[i] & b[i]);
                    } else {
                        for (size_t i = 0; i < size; ++i) c[i] = (a[i] * b[i]);
                    }

                    c_final = random_share_secret_vec_3P(c, f->binary);

                    if (id != D) {
                        store->store_triples(a, b, c_final, f->triples_idx);
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