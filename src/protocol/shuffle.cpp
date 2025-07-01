#include "shuffle.h"

/**
 * Preprocessing functions for performing different variants of shuffle
 * 1. Helper function
 * 2. Get Shuffle
 * 3. Get Repeat
 * 4. Get Unshuffle
 * 4. Get Merged Shuffle
 * Afterwards: The same functions split into Dealer and Parties
 */
ShufflePre shuffle::get_shuffle_compute(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &shared_secret_D0, std::vector<Ring> &shared_secret_D1,
                                        bool save) {
    size_t shared_secret_D0_idx = 0;
    size_t shared_secret_D1_idx = 0;

    ShufflePre perm_share;

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
            pi_0_p = Permutation::random(n, rngs.rng_D0());
            pi_0 = Permutation::random(n, rngs.rng_D0());
            pi_1 = Permutation::random(n, rngs.rng_D1());

            Permutation pi_0_p_inv = pi_0_p.inverse();
            pi_1_p = (pi_0 * pi_1 * pi_0_p_inv);

            perm_share.pi_0 = pi_0;
            perm_share.pi_0_p = pi_0_p;
            perm_share.pi_1 = pi_1;
            perm_share.pi_1_p = pi_1_p;

            for (int i = 0; i < n; ++i) {
                /* Send pi_1_p to P1's shared_secret_buffer */
                shared_secret_D1[shared_secret_D1_idx] = (Ring)pi_1_p[i];
                shared_secret_D1_idx++;
            }

            /* Compute B_0, B_1 */
            std::vector<Ring> B_0(n);
            std::vector<Ring> B_1(n);

            Ring R;
            rngs.rng_D().random_data(&R, sizeof(Ring));

            B_0 = (pi_0 * pi_1)(R_1);
            B_1 = (pi_0 * pi_1)(R_0);

#pragma omp parallel for if (n > 10000)
            for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

#pragma omp parallel for if (n > 10000)
            for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

            for (int i = 0; i < n; ++i) {
                /* Send B_0 and B_1 to the corresponding buffers */
                shared_secret_D0[shared_secret_D0_idx] = B_0[i];
                shared_secret_D1[shared_secret_D1_idx] = B_1[i];
                shared_secret_D0_idx++;
                shared_secret_D1_idx++;
            }
            break;
        }
        case P0: {
            std::vector<Ring> B(n);
            for (int i = 0; i < n; ++i) {
                B[i] = shared_secret_D0[shared_secret_D0_idx];
                shared_secret_D0_idx++;
            }
            perm_share.B = B;
            perm_share.save = save;
            break;
        }
        case P1: {
            /* Receive pi_1_p from P1's shared_secret_buffer */
            std::vector<Ring> perm_vec(n);
            for (int i = 0; i < n; ++i) {
                perm_vec[i] = shared_secret_D1[shared_secret_D1_idx];
                shared_secret_D1_idx++;
            }
            perm_share.pi_1_p = Permutation(perm_vec);

            std::vector<Ring> B(n);
            for (int i = 0; i < n; ++i) {
                B[i] = shared_secret_D1[shared_secret_D1_idx];
                shared_secret_D1_idx++;
            }
            perm_share.B = B;
            perm_share.save = save;
            break;
        }
    }
    return perm_share;
}

ShufflePre shuffle::get_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, bool save) {
    std::vector<Ring> shared_secret_D0(n);
    std::vector<Ring> shared_secret_D1(2 * n);

    switch (id) {
        case D: {
            ShufflePre perm_share = get_shuffle_compute(id, rngs, n, shared_secret_D0, shared_secret_D1, save);
            network->add_to_queue(P0, shared_secret_D0);
            network->add_to_queue(P1, shared_secret_D1);
            // send_vec(P0, network, shared_secret_D0.size(), shared_secret_D0, BLOCK_SIZE);
            // send_vec(P1, network, shared_secret_D1.size(), shared_secret_D1, BLOCK_SIZE);
            return perm_share;
        }
        case P0: {
            shared_secret_D0 = network->recv(D, n);
            // recv_vec(D, network, shared_secret_D0, BLOCK_SIZE);
            return get_shuffle_compute(id, rngs, n, shared_secret_D0, shared_secret_D1, save);
        }
        case P1: {
            shared_secret_D1 = network->recv(D, 2 * n);
            // recv_vec(D, network, shared_secret_D1, BLOCK_SIZE);
            return get_shuffle_compute(id, rngs, n, shared_secret_D0, shared_secret_D1, save);
        }
    }
}

std::vector<Ring> shuffle::get_unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, ShufflePre &perm_share) {
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
            rngs.rng_D().random_data(&R, sizeof(Ring));

            Permutation pi_inv = (perm_share.pi_0 * perm_share.pi_1).inverse();

            B_0 = pi_inv(R_1);
            B_1 = pi_inv(R_0);

#pragma omp parallel for if (n > 10000)
            for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

#pragma omp parallel for if (n > 10000)
            for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

            // send_vec(P0, network, B_0.size(), B_0, BLOCK_SIZE);
            // send_vec(P1, network, B_1.size(), B_1, BLOCK_SIZE);
            network->add_to_queue(P0, B_0);
            network->add_to_queue(P1, B_1);

            return std::vector<Ring>(n);
        }
        case P0: {
            // recv_vec(D, network, B_0, BLOCK_SIZE);
            B_0 = network->recv(D, n);
            return B_0;
        }
        case P1: {
            // recv_vec(D, network, B_1, BLOCK_SIZE);
            B_1 = network->recv(D, n);
            return B_1;
        }
    }
}

ShufflePre shuffle::get_merged_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, ShufflePre &pi_share,
                                       ShufflePre &omega_share) {
    std::vector<Ring> sigma_0_p_vec(n);
    std::vector<Ring> sigma_1_vec(n);

    std::vector<Ring> B_0(n);
    std::vector<Ring> B_1(n);
    std::vector<Ring> R_0, R_1;

    ShufflePre perm_share;
    perm_share.save = true;

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
            rngs.rng_D().random_data(&R, sizeof(Ring));

            /* Compute B_0 / B_1 */
            B_0 = (sigma_0 * sigma_1)(R_1);
            B_1 = (sigma_0 * sigma_1)(R_0);

#pragma omp parallel for if (n > 10000)
            for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;
#pragma omp parallel for if (n > 10000)
            for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

            network->add_to_queue(P0, sigma_0_p_vec);
            network->add_to_queue(P0, B_0);

            network->add_to_queue(P1, sigma_1_vec);
            network->add_to_queue(P1, B_1);
            // send_vec(P0, network, sigma_0_p_vec.size(), sigma_0_p_vec, BLOCK_SIZE);
            // send_vec(P1, network, sigma_1_vec.size(), sigma_1_vec, BLOCK_SIZE);
            // send_vec(P0, network, B_0.size(), B_0, BLOCK_SIZE);
            // send_vec(P1, network, B_1.size(), B_1, BLOCK_SIZE);
            break;
        }
        case P0: {
            sigma_0_p_vec = network->recv(D, n);
            B_0 = network->recv(D, n);
            // recv_vec(D, network, sigma_0_p_vec, BLOCK_SIZE);
            // recv_vec(D, network, B_0, BLOCK_SIZE);
            perm_share.pi_0_p = Permutation(sigma_0_p_vec);
            perm_share.B = B_0;
            break;
        }
        case P1: {
            sigma_1_vec = network->recv(D, n);
            B_1 = network->recv(D, n);
            // recv_vec(D, network, sigma_1_vec, BLOCK_SIZE);
            // recv_vec(D, network, B_1, BLOCK_SIZE);
            perm_share.pi_1 = Permutation(sigma_1_vec);
            perm_share.B = B_1;

            break;
        }
    }
    return perm_share;
}

std::tuple<ShufflePre, std::vector<Ring>, std::vector<Ring>> shuffle::get_shuffle_1(Party id, RandomGenerators &rngs, size_t n) {
    /* P1 receives pi_1_p and B_1 */
    std::vector<Ring> shares_P1(2 * n);

    ShufflePre perm_share;

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
    pi_0_p = Permutation::random(n, rngs.rng_D0());
    pi_0 = Permutation::random(n, rngs.rng_D0());
    pi_1 = Permutation::random(n, rngs.rng_D1());

    Permutation pi_0_p_inv = pi_0_p.inverse();
    pi_1_p = (pi_0 * pi_1 * pi_0_p_inv);

    perm_share.pi_0 = pi_0;
    perm_share.pi_0_p = pi_0_p;
    perm_share.pi_1 = pi_1;
    perm_share.pi_1_p = pi_1_p;

    /* Compute B_0, B_1 */
    std::vector<Ring> B_0(n);
    std::vector<Ring> B_1(n);

    Ring R;
    rngs.rng_D().random_data(&R, sizeof(Ring));

    B_0 = (pi_0 * pi_1)(R_1);
    B_1 = (pi_0 * pi_1)(R_0);

    for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

    for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

    for (int i = 0; i < n; ++i) {
        /* Send B_0 and B_1 to the corresponding buffers */
        shares_P1[i] = B_1[i];
    }

    for (int i = n; i < 2 * n; ++i) {
        /* Send pi_1_p to P1's shared_secret_buffer */
        shares_P1[i] = (Ring)pi_1_p[i - n];
    }

    return {perm_share, B_0, shares_P1};
}

ShufflePre shuffle::get_shuffle_2(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> vals, size_t &idx, bool save) {
    ShufflePre perm_share;
    std::vector<Ring> B(n);

    for (size_t i = 0; i < n; ++i) {
        B[i] = vals[idx];
        idx++;
    }

    perm_share.B = B;

    if (id == P1) {
        std::vector<Ring> pi_1_p_vec(n);
        for (size_t i = 0; i < n; ++i) {
            pi_1_p_vec[i] = vals[idx];
            idx++;
        }
        perm_share.pi_1_p = Permutation(pi_1_p_vec);
    }

    perm_share.save = save;
    return perm_share;
}

std::tuple<std::vector<Ring>, std::vector<Ring>> shuffle::get_unshuffle_1(Party id, RandomGenerators &rngs, size_t n, ShufflePre &perm_share) {
    std::vector<Ring> B_0(n);
    std::vector<Ring> B_1(n);

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
    rngs.rng_D().random_data(&R, sizeof(Ring));

    Permutation pi_inv = (perm_share.pi_0 * perm_share.pi_1).inverse();

    B_0 = pi_inv(R_1);
    B_1 = pi_inv(R_0);

    for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

    for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

    return {B_0, B_1};
}

std::vector<Ring> shuffle::get_unshuffle_2(Party id, size_t n, std::vector<Ring> &vals, size_t &idx) {
    std::vector<Ring> unshuffle_B(n);
    for (size_t i = 0; i < n; ++i) {
        unshuffle_B[i] = vals[idx];
        ++idx;
    }
    return unshuffle_B;
}

std::tuple<ShufflePre, std::vector<Ring>, std::vector<Ring>, std::vector<Ring>, std::vector<Ring>> shuffle::get_merged_shuffle_1(Party id,
                                                                                                                                 RandomGenerators &rngs,
                                                                                                                                 size_t n, ShufflePre &pi_share,
                                                                                                                                 ShufflePre &omega_share) {
    std::vector<Ring> sigma_0_p_vec(n);
    std::vector<Ring> sigma_1_vec(n);

    std::vector<Ring> B_0(n);
    std::vector<Ring> B_1(n);
    std::vector<Ring> R_0, R_1;

    ShufflePre perm_share;

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
    perm_share.pi_0_p = sigma_0_p;
    perm_share.pi_1_p = sigma_1_p;

    Ring R;
    rngs.rng_D().random_data(&R, sizeof(Ring));

    /* Compute B_0 / B_1 */
    B_0 = (sigma_0 * sigma_1)(R_1);
    B_1 = (sigma_0 * sigma_1)(R_0);

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

    return {perm_share, B_0, B_1, sigma_0_p_vec, sigma_1_vec};
}

ShufflePre shuffle::get_merged_shuffle_2(Party id, size_t n, std::vector<Ring> &vals, size_t &idx) {
    ShufflePre perm_share;
    std::vector<Ring> B(n);

    for (size_t i = 0; i < n; ++i) {
        B[i] = vals[idx];
        idx++;
    }

    perm_share.B = B;

    std::vector<Ring> perm_vec(n);
    for (size_t i = 0; i < n; ++i) {
        perm_vec[i] = vals[idx];
        idx++;
    }

    if (id == P0) {
        perm_share.pi_0_p = Permutation(perm_vec);
    } else {
        perm_share.pi_1 = Permutation(perm_vec);
    }

    return perm_share;
}

/**
 * Evaluation functions for performing different shuffles
 * 1. Shuffle
 * 2. Prepare Repeat
 * 3. Unshuffle
 */
Permutation shuffle::shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, Permutation &input_share, ShufflePre &perm_share,
                             size_t n) {
    auto perm_vec = input_share.get_perm_vec();
    return Permutation(shuffle(id, rngs, network, perm_vec, perm_share, n));
}

std::vector<Ring> shuffle::shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, std::vector<Ring> &input_share,
                                   ShufflePre &perm_share, size_t n) {
    std::vector<Ring> shuffled_share(n);
    std::vector<Ring> vec_A(n);

    if (id == D) return shuffled_share;

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
        if (perm_share.save) perm_share.R = R;
    } else {
        R = perm_share.R;
    }

    /* Sampling 2: pi_0_p / pi_1 */
    if (id == P0) {
        /* Reuse pi_0_p if saved earlier */
        if (perm_share.pi_0_p.not_null())
            perm = perm_share.pi_0_p;
        else
            perm = Permutation::random(n, rngs.rng_D0());
        if (perm_share.save) perm_share.pi_0_p = perm;
    } else {
        /* Reuse pi_1 if saved earlier */
        if (perm_share.pi_1.not_null())
            perm = perm_share.pi_1;
        else
            perm = Permutation::random(n, rngs.rng_D1());
        if (perm_share.save) perm_share.pi_1 = perm;
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
        network->send_now(P1, vec_A);
        vec_A = network->recv(P1, n);
    } else {
        std::vector<Ring> data_recv = network->recv(P0, n);
        network->send_now(P0, vec_A);
        std::copy(data_recv.begin(), data_recv.end(), vec_A.begin());
    }

    std::copy(vec_A.begin(), vec_A.end(), shuffled_share.begin());

    if (id == P0) {
        /* Sampling 3: pi_0 */
        if (perm_share.pi_0.not_null())
            perm = perm_share.pi_0;
        else
            perm = Permutation::random(n, rngs.rng_D0());
        if (perm_share.save) perm_share.pi_0 = perm;
    } else {
        if (perm_share.pi_1_p.not_null())
            perm = perm_share.pi_1_p;
        else
            perm = Permutation::random(n, rngs.rng_D1());
        if (perm_share.save) perm_share.pi_1_p = perm;
    }

    shuffled_share = perm(shuffled_share);

#pragma omp parallel for if (n > 10000)
    for (size_t i = 0; i < n; ++i) {
        shuffled_share[i] -= perm_share.B[i];
    }

    return shuffled_share;
}

std::vector<Ring> shuffle::unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, ShufflePre &shuffle_share,
                                     std::vector<Ring> &B, std::vector<Ring> &input_share, size_t n) {
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
        network->send_now(P1, vec_t);
        vec_t = network->recv(P1, n);
    } else {
        std::vector<Ring> data_recv = network->recv(P0, n);
        network->send_now(P0, vec_t);
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

Permutation shuffle::unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, ShufflePre &shuffle_share,
                               std::vector<Ring> &B, Permutation &input_share) {
    auto perm_vec = input_share.get_perm_vec();
    return Permutation(unshuffle(id, rngs, network, shuffle_share, B, perm_vec, n));
}
