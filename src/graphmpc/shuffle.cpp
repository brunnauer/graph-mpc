#include "shuffle.h"

ShufflePre shuffle::get_shuffle_compute(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &D0, std::vector<Ring> &D1, Party recv_larger_msg) {
    ShufflePre perm_share;
    size_t D0_idx = 0;
    size_t D1_idx = 0;

    switch (id) {
        case D: {
            /* Sampling 1: R_0, R_1*/
            std::vector<Ring> R_0, R_1;
            Ring rand;

            for (size_t i = 0; i < n; ++i) {
                rngs.rng_D0().random_data(&rand, sizeof(Ring));
                R_0.push_back(rand);
            }

            for (size_t i = 0; i < n; ++i) {
                rngs.rng_D1().random_data(&rand, sizeof(Ring));
                R_1.push_back(rand);
            }

            Permutation pi_0;
            Permutation pi_1;
            Permutation pi_0_p;
            Permutation pi_1_p;

            /* Sampling 2: pi_0_p, pi_0, pi_1*/
            if (recv_larger_msg == P1) {
                pi_0_p = Permutation::random(n, rngs.rng_D0());
                pi_0 = Permutation::random(n, rngs.rng_D0());
                pi_1 = Permutation::random(n, rngs.rng_D1());

                Permutation pi_0_p_inv = pi_0_p.inverse();
                pi_1_p = (pi_0 * pi_1 * pi_0_p_inv);
            } else {
                pi_1 = Permutation::random(n, rngs.rng_D1());
                pi_1_p = Permutation::random(n, rngs.rng_D1());
                pi_0 = Permutation::random(n, rngs.rng_D0());

                Permutation pi_1_p_inv = pi_1_p.inverse();
                pi_0_p = (pi_1_p_inv * pi_0 * pi_1);
            }

            perm_share.pi_0 = pi_0;
            perm_share.pi_1 = pi_1;

            for (size_t i = 0; i < n; ++i) {
                if (recv_larger_msg == P1) {
                    /* Send pi_1_p to P1 */
                    D1[i] = (Ring)pi_1_p[i];
                    D1_idx++;
                } else {
                    /* Send pi_0_p to P0 */
                    D0[i] = (Ring)pi_0_p[i];
                    D0_idx++;
                }
            }

            /* Compute B_0, B_1 */
            std::vector<Ring> B_0(n);
            std::vector<Ring> B_1(n);

            Ring R;
            rngs.rng_self().random_data(&R, sizeof(Ring));

            B_0 = (pi_0 * pi_1)(R_1);
            B_1 = (pi_0 * pi_1)(R_0);

#pragma omp parallel for if (n > 10000)
            for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

#pragma omp parallel for if (n > 10000)
            for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

            for (size_t i = 0; i < n; ++i) {
                /* Send B_0 and B_1 to the corresponding buffers */
                D0[D0_idx] = B_0[i];
                D1[D1_idx] = B_1[i];
                D0_idx++;
                D1_idx++;
            }
            return perm_share;
        }
        case P0: {
            if (recv_larger_msg == P0) {
                /* Receive pi_0_p */
                std::vector<Ring> perm_vec(n);
                for (size_t i = 0; i < n; ++i) {
                    perm_vec[i] = D0[D0_idx];
                    D0_idx++;
                }
                perm_share.pi_0_p = Permutation(perm_vec);
            }

            std::vector<Ring> B(n);
            for (size_t i = 0; i < n; ++i) {
                B[i] = D0[D0_idx];
                D0_idx++;
            }
            perm_share.B = B;
            return perm_share;
        }
        case P1: {
            if (recv_larger_msg == P1) {
                /* Receive pi_1_p */
                std::vector<Ring> perm_vec(n);
                for (int i = 0; i < n; ++i) {
                    perm_vec[i] = D1[D1_idx];
                    D1_idx++;
                }
                perm_share.pi_1_p = Permutation(perm_vec);
            }

            std::vector<Ring> B(n);
            for (size_t i = 0; i < n; ++i) {
                B[i] = D1[D1_idx];
                D1_idx++;
            }
            perm_share.B = B;
            return perm_share;
        }
    }
}
ShufflePre shuffle::get_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, Party &recv) {
    ShufflePre perm_share;
    size_t P0_recv_size, P1_recv_size;
    /* Load balancing */
    if (recv == P1) {
        P0_recv_size = n;
        P1_recv_size = 2 * n;
    } else {
        P0_recv_size = 2 * n;
        P1_recv_size = n;
    }

    std::vector<Ring> D0(P0_recv_size);
    std::vector<Ring> D1(P1_recv_size);

    if (id == P0) D0 = network->read(D, P0_recv_size);
    if (id == P1) D1 = network->read(D, P1_recv_size);

    perm_share = get_shuffle_compute(id, rngs, n, D0, D1, recv);

    if (id == D) {
        network->add_send(P0, D0);
        network->add_send(P1, D1);
    }

    /* Alternate receiver of larger message */
    recv = recv == P0 ? P1 : P0;

    return perm_share;
}

void shuffle::get_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, bool save_to_disk) {
    // if (save_to_disk) {
    //  network->preproc_disk.write_shuffle();
    //} else {
    preproc.shuffles.push(get_shuffle(id, rngs, network, n));
    //}
}

ShufflePre shuffle::get_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n) {
    ShufflePre perm_share;
    size_t P0_recv_size, P1_recv_size;

    /* Load balancing */
    Party recv_larger;
    int coin;
    rngs.rng_D().random_data(&coin, sizeof(int));
    if (coin < 0) coin = -coin;
    coin %= 2;
    if (coin) {
        recv_larger = P0;
        P0_recv_size = 2 * n;
        P1_recv_size = n;
    } else {
        recv_larger = P1;
        P0_recv_size = n;
        P1_recv_size = 2 * n;
    }

    std::vector<Ring> D0(P0_recv_size);
    std::vector<Ring> D1(P1_recv_size);

    if (id == P0) D0 = network->read(D, P0_recv_size);
    if (id == P1) D1 = network->read(D, P1_recv_size);

    perm_share = get_shuffle_compute(id, rngs, n, D0, D1, recv_larger);

    if (id == D) {
        network->add_send(P0, D0);
        network->add_send(P1, D1);
    }

    return perm_share;
}

void shuffle::get_unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &perm_share,
                            MPPreprocessing &preproc) {
    std::vector<Ring> unshuffle = get_unshuffle(id, rngs, network, n, perm_share);
    preproc.unshuffles.push(unshuffle);
}

std::vector<Ring> shuffle::get_unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &perm_share) {
    std::vector<Ring> B_0(n);
    std::vector<Ring> B_1(n);

    switch (id) {
        case D: {
            /* Sampling 1: R_0, R_1*/
            std::vector<Ring> R_0, R_1;
            Ring rand;

            for (size_t i = 0; i < n; ++i) {
                rngs.rng_D0_unshuffle().random_data(&rand, sizeof(Ring));
                R_0.push_back(rand);
            }

            for (size_t i = 0; i < n; ++i) {
                rngs.rng_D1_unshuffle().random_data(&rand, sizeof(Ring));
                R_1.push_back(rand);
            }

            /* Compute B_0, B_1 */
            Ring R;
            rngs.rng_self().random_data(&R, sizeof(Ring));

            Permutation pi_inv = (perm_share.pi_0 * perm_share.pi_1).inverse();

            B_0 = pi_inv(R_1);
            B_1 = pi_inv(R_0);

#pragma omp parallel for if (n > 10000)
            for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

#pragma omp parallel for if (n > 10000)
            for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

            network->add_send(P0, B_0);
            network->add_send(P1, B_1);

            return std::vector<Ring>(n);
        }
        case P0: {
            B_0 = network->read(D, n);
            return B_0;
        }
        case P1: {
            B_1 = network->read(D, n);
            return B_1;
        }
    }
}

ShufflePre shuffle::get_merged_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &pi_share,
                                       ShufflePre &omega_share) {
    std::vector<Ring> sigma_0_p_vec(n);
    std::vector<Ring> sigma_1_vec(n);

    std::vector<Ring> B_0(n);
    std::vector<Ring> B_1(n);
    std::vector<Ring> R_0, R_1;

    ShufflePre perm_share;

    switch (id) {
        case D: {
            Ring rand;

            /* Sampling 1: R_0 / R_1 */
            for (int i = 0; i < n; ++i) {
                rngs.rng_D0().random_data(&rand, sizeof(Ring));
                R_0.push_back(rand);
            }

            for (int i = 0; i < n; ++i) {
                rngs.rng_D1().random_data(&rand, sizeof(Ring));
                R_1.push_back(rand);
            }

            /* Computing / Sampling merged permutation */
            Permutation pi = pi_share.pi_0 * pi_share.pi_1;
            Permutation omega = omega_share.pi_0 * omega_share.pi_1;

            Permutation merged = pi(omega.get_perm_vec());

            Permutation sigma_0 = Permutation::random(n, rngs.rng_D0());
            Permutation sigma_1_p = Permutation::random(n, rngs.rng_D1());

            Permutation sigma_0_p = sigma_1_p.inverse() * merged;
            Permutation sigma_1 = sigma_0.inverse() * merged;

            sigma_0_p_vec = sigma_0_p.get_perm_vec();
            sigma_1_vec = sigma_1.get_perm_vec();

            perm_share.pi_0 = sigma_0;
            perm_share.pi_1 = sigma_1;

            Ring R;
            rngs.rng_self().random_data(&R, sizeof(Ring));

            /* Compute B_0 / B_1 */
            B_0 = (sigma_0 * sigma_1)(R_1);
            B_1 = (sigma_0 * sigma_1)(R_0);

#pragma omp parallel for if (n > 10000)
            for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;
#pragma omp parallel for if (n > 10000)
            for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

            network->add_send(P0, sigma_0_p_vec);
            network->add_send(P0, B_0);

            network->add_send(P1, sigma_1_vec);
            network->add_send(P1, B_1);
            break;
        }
        case P0: {
            sigma_0_p_vec = network->read(D, n);
            B_0 = network->read(D, n);
            perm_share.pi_0_p = Permutation(sigma_0_p_vec);
            perm_share.B = B_0;
            break;
        }
        case P1: {
            sigma_1_vec = network->read(D, n);
            B_1 = network->read(D, n);
            perm_share.pi_1 = Permutation(sigma_1_vec);
            perm_share.B = B_1;

            break;
        }
    }
    return perm_share;
}

std::vector<Ring> shuffle::shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                                   std::vector<Ring> &input_share) {
    ShufflePre shuffle_pre = preproc.shuffles.front();
    preproc.shuffles.pop();
    return shuffle(id, rngs, network, n, shuffle_pre, input_share);
}

Permutation shuffle::shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                             Permutation &input_share) {
    ShufflePre shuffle_pre = preproc.shuffles.front();
    preproc.shuffles.pop();
    return shuffle(id, rngs, network, n, shuffle_pre, input_share);
}

std::vector<Ring> shuffle::shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &perm_share,
                                   std::vector<Ring> &input_share) {
    std::vector<Ring> vec_A(n);

    if (id == D) return vec_A;

    Permutation perm;
    std::vector<Ring> R;

    if (perm_share.R.empty()) {
        Ring rand;

        /* Sampling 1: R_0 / R_1 */
        for (size_t i = 0; i < n; ++i) {
            if (id == P0)
                rngs.rng_D0().random_data(&rand, sizeof(Ring));
            else if (id == P1)
                rngs.rng_D1().random_data(&rand, sizeof(Ring));
            R.push_back(rand);
        }
        perm_share.R = R;
    } else {
        R = perm_share.R;
    }

    /* Sampling 2: pi_0_p / pi_1 */
    if (id == P0) {
        /* Reuse pi_0_p if saved earlier */
        if (perm_share.pi_0_p.not_null()) {
            perm = perm_share.pi_0_p;
        } else {
            perm = Permutation::random(n, rngs.rng_D0());
            perm_share.pi_0_p = perm;
        }
    } else {
        /* Reuse pi_1 if saved earlier */
        if (perm_share.pi_1.not_null()) {
            perm = perm_share.pi_1;
        } else {
            perm = Permutation::random(n, rngs.rng_D1());
            perm_share.pi_1 = perm;
        }
    }

    /* Compute input + R */
#pragma omp parallel for if (n > 10000)
    for (size_t j = 0; j < n; ++j) {
        vec_A[j] = input_share[j] + R[j];
    }

    /* Compute perm(input + R) */
    vec_A = perm(vec_A);

    /* Send A to the other party */
    if (id == P0) {
        network->send_vec(P1, n, vec_A);
        network->recv_vec(P1, n, vec_A);
    } else {
        std::vector<Ring> data_recv(n);
        network->recv_vec(P0, n, data_recv);
        network->send_vec(P0, n, vec_A);
        std::copy(data_recv.begin(), data_recv.end(), vec_A.begin());
    }

    if (id == P0) {
        /* Sampling 3: pi_0 */
        if (perm_share.pi_0.not_null()) {
            perm = perm_share.pi_0;
        } else {
            perm = Permutation::random(n, rngs.rng_D0());
            perm_share.pi_0 = perm;
        }
    } else {
        if (perm_share.pi_1_p.not_null()) {
            perm = perm_share.pi_1_p;
        } else {
            perm = Permutation::random(n, rngs.rng_D1());
            perm_share.pi_1_p = perm;
        }
    }

    vec_A = perm(vec_A);

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < n; ++i) {
        vec_A[i] -= perm_share.B[i];
    }

    return vec_A;
}

Permutation shuffle::shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &perm_share,
                             Permutation &input_share) {
    auto perm_vec = input_share.get_perm_vec();
    return Permutation(shuffle(id, rngs, network, n, perm_share, perm_vec));
}

std::vector<Ring> shuffle::unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                                     ShufflePre &shuffle_pre, std::vector<Ring> &input_share) {
    auto unshuffle_B = preproc.unshuffles.front();
    preproc.unshuffles.pop();
    return unshuffle(id, rngs, network, n, shuffle_pre, unshuffle_B, input_share);
}

Permutation shuffle::unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                               ShufflePre &shuffle_pre, Permutation &input_share) {
    auto unshuffle_B = preproc.unshuffles.front();
    preproc.unshuffles.pop();
    return unshuffle(id, rngs, network, n, shuffle_pre, unshuffle_B, input_share);
}

std::vector<Ring> shuffle::unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &shuffle_share,
                                     std::vector<Ring> &B, std::vector<Ring> &input_share) {
    std::vector<Ring> output_share(n);

    if (id == D) return output_share;

    std::vector<Ring> vec_t(n);
    std::vector<Ring> R;
    Ring rand;

    /* Sampling 1: R_0 / R_1 */
    for (size_t i = 0; i < n; ++i) {
        if (id == P0)
            rngs.rng_D0_unshuffle().random_data(&rand, sizeof(Ring));
        else
            rngs.rng_D1_unshuffle().random_data(&rand, sizeof(Ring));
        R.push_back(rand);
    }

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < n; ++i) vec_t[i] = input_share[i] + R[i];

    Permutation perm = id == P0 ? shuffle_share.pi_0 : shuffle_share.pi_1_p;
    vec_t = perm.inverse()(vec_t);

    /* Send and receive t_0/t_1 */
    if (id == P0) {
        network->send_vec(P1, n, vec_t);
        network->recv_vec(P1, n, vec_t);
    } else {
        std::vector<Ring> data_recv;
        network->recv_vec(P0, n, data_recv);
        network->send_vec(P0, n, vec_t);
        std::copy(data_recv.begin(), data_recv.end(), vec_t.begin());
    }

    /* Apply inverse */
    perm = id == P0 ? shuffle_share.pi_0_p : shuffle_share.pi_1;
    output_share = perm.inverse()(vec_t);

    /* Last step: subtract B_0 / B_1 */
#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < n; ++i) output_share[i] -= B[i];

    return output_share;
}

Permutation shuffle::unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, ShufflePre &shuffle_share,
                               std::vector<Ring> &B, Permutation &input_share) {
    auto perm_vec = input_share.get_perm_vec();
    return Permutation(unshuffle(id, rngs, network, n, shuffle_share, B, perm_vec));
}
