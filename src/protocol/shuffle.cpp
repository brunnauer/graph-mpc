#include "shuffle.h"

PermShare shuffle::get_shuffle_compute(ProtocolConfig &c, std::vector<Row> &shared_secret_D0, std::vector<Row> &shared_secret_D1) {
    size_t shared_secret_D0_idx = 0;
    size_t shared_secret_D1_idx = 0;

    PermShare perm_share;

    switch (c.pid) {
        case D: {
            /* Sampling 1: R_0, R_1*/
            std::vector<Row> R_0, R_1;
            Row rand;

            for (size_t i = 0; i < c.n_rows; ++i) {
                c.rngs.rng_D0().random_data(&rand, sizeof(Row));
                R_0.push_back(rand);
            }

            for (size_t i = 0; i < c.n_rows; ++i) {
                c.rngs.rng_D1().random_data(&rand, sizeof(Row));
                R_1.push_back(rand);
            }

            Permutation pi_0;
            Permutation pi_1;
            Permutation pi_0_p;
            Permutation pi_1_p;

            /* Sampling 2: pi_0_p, pi_0, pi_1*/
            pi_0_p = Permutation::random(c.n_rows, c.rngs.rng_D0());
            pi_0 = Permutation::random(c.n_rows, c.rngs.rng_D0());
            pi_1 = Permutation::random(c.n_rows, c.rngs.rng_D1());

            Permutation pi_0_p_inv = pi_0_p.inverse();
            pi_1_p = (pi_0 * pi_1 * pi_0_p_inv);

            perm_share.pi_0 = pi_0;
            perm_share.pi_0_p = pi_0_p;
            perm_share.pi_1 = pi_1;
            perm_share.pi_1_p = pi_1_p;

            for (int i = 0; i < c.n_rows; ++i) {
                /* Send pi_1_p to P1's shared_secret_buffer */
                shared_secret_D1[shared_secret_D1_idx] = (Row)pi_1_p[i];
                shared_secret_D1_idx++;
            }

            /* Compute B_0, B_1 */
            std::vector<Row> B_0(c.n_rows);
            std::vector<Row> B_1(c.n_rows);

            Row R;
            c.rngs.rng_D().random_data(&R, sizeof(Row));

            B_0 = (pi_0 * pi_1)(R_1);
            B_1 = (pi_0 * pi_1)(R_0);

            for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

            for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

            for (int i = 0; i < c.n_rows; ++i) {
                /* Send B_0 and B_1 to the corresponding buffers */
                shared_secret_D0[shared_secret_D0_idx] = B_0[i];
                shared_secret_D1[shared_secret_D1_idx] = B_1[i];
                shared_secret_D0_idx++;
                shared_secret_D1_idx++;
            }
            break;
        }
        case P0: {
            std::vector<Row> B(c.n_rows);
            for (int i = 0; i < c.n_rows; ++i) {
                B[i] = shared_secret_D0[shared_secret_D0_idx];
                shared_secret_D0_idx++;
            }
            perm_share.B = B;

            break;
        }
        case P1: {
            /* Receive pi_1_p from P1's shared_secret_buffer */
            std::vector<Row> perm_vec(c.n_rows);
            for (int i = 0; i < c.n_rows; ++i) {
                perm_vec[i] = shared_secret_D1[shared_secret_D1_idx];
                shared_secret_D1_idx++;
            }
            perm_share.pi_1_p = Permutation(perm_vec);

            std::vector<Row> B(c.n_rows);
            for (int i = 0; i < c.n_rows; ++i) {
                B[i] = shared_secret_D1[shared_secret_D1_idx];
                shared_secret_D1_idx++;
            }
            perm_share.B = B;

            break;
        }
    }
    return perm_share;
}

PermShare shuffle::get_shuffle(ProtocolConfig &c) {
    std::vector<Row> shared_secret_D0(c.n_rows);
    std::vector<Row> shared_secret_D1(2 * c.n_rows);

    switch (c.pid) {
        case D: {
            PermShare perm_share = get_shuffle_compute(c, shared_secret_D0, shared_secret_D1);
            send_vec(P0, c.network, shared_secret_D0.size(), shared_secret_D0, c.BLOCK_SIZE);
            send_vec(P1, c.network, shared_secret_D1.size(), shared_secret_D1, c.BLOCK_SIZE);
            return perm_share;
        }
        case P0: {
            recv_vec(D, c.network, shared_secret_D0, c.BLOCK_SIZE);
            return get_shuffle_compute(c, shared_secret_D0, shared_secret_D1);
        }
        case P1: {
            recv_vec(D, c.network, shared_secret_D1, c.BLOCK_SIZE);
            return get_shuffle_compute(c, shared_secret_D0, shared_secret_D1);
        }
    }
}

std::vector<Row> shuffle::shuffle(ProtocolConfig &c, std::vector<Row> &input_share, PermShare &perm_share, bool save) {
    if (c.pid == D) return std::vector<Row>(c.n_rows);

    std::vector<Row> shuffled_share(c.n_rows);
    std::vector<Row> vec_A(c.n_rows);

    Permutation perm;

    std::vector<Row> R;

    if (perm_share.R.empty()) {
        Row rand;

        /* Sampling 1: R_0 / R_1 */
        for (size_t i = 0; i < c.n_rows; ++i) {
            if (c.pid == P0)
                c.rngs.rng_D0().random_data(&rand, sizeof(Row));
            else if (c.pid == P1)
                c.rngs.rng_D1().random_data(&rand, sizeof(Row));
            R.push_back(rand);
        }
        if (save) perm_share.R = R;
    } else {
        R = perm_share.R;
    }

    /* Sampling 2: pi_0_p / pi_1 */
    if (c.pid == P0) {
        /* Reuse pi_0_p if saved earlier */
        if (perm_share.pi_0_p.not_null())
            perm = perm_share.pi_0_p;
        else
            perm = Permutation::random(c.n_rows, c.rngs.rng_D0());
        if (save) perm_share.pi_0_p = perm;
    } else {
        /* Reuse pi_1 if saved earlier */
        if (perm_share.pi_1.not_null())
            perm = perm_share.pi_1;
        else
            perm = Permutation::random(c.n_rows, c.rngs.rng_D1());
        if (save) perm_share.pi_1 = perm;
    }

    /* Compute input + R */
    for (size_t j = 0; j < c.n_rows; ++j) {
        vec_A[j] = input_share[j] + R[j];
    }

    /* Compute perm(input + R) */
    vec_A = perm(vec_A);

    /* Send A to the other party */
    if (c.pid == P0) {
        send_vec(P1, c.network, vec_A.size(), vec_A, c.BLOCK_SIZE);
        recv_vec(P1, c.network, vec_A, c.BLOCK_SIZE);
    } else {
        std::vector<Row> data_recv(c.n_rows);
        recv_vec(P0, c.network, data_recv, c.BLOCK_SIZE);
        send_vec(P0, c.network, vec_A.size(), vec_A, c.BLOCK_SIZE);
        std::copy(data_recv.begin(), data_recv.end(), vec_A.begin());
    }

    std::copy(vec_A.begin(), vec_A.end(), shuffled_share.begin());

    if (c.pid == P0) {
        /* Sampling 3: pi_0 */
        if (perm_share.pi_0.not_null())
            perm = perm_share.pi_0;
        else
            perm = Permutation::random(c.n_rows, c.rngs.rng_D0());
        if (save) perm_share.pi_0 = perm;
    } else {
        if (perm_share.pi_1_p.not_null())
            perm = perm_share.pi_1_p;
        else
            perm = Permutation::random(c.n_rows, c.rngs.rng_D1());
    }

    shuffled_share = perm(shuffled_share);

    for (size_t i = 0; i < c.n_rows; ++i) {
        shuffled_share[i] -= perm_share.B[i];
    }

    return shuffled_share;
}

Permutation shuffle::shuffle(ProtocolConfig &c, Permutation &perm, PermShare &perm_share, bool save) {
    std::vector<Row> perm_vec = perm.get_perm_vec();
    auto shuffled = shuffle(c, perm_vec, perm_share, save);
    return Permutation(shuffled);
}

std::vector<Row> shuffle::unshuffle(ProtocolConfig &c, std::vector<Row> &input_share, PermShare &perm_share) {
    std::vector<Row> B_0(c.n_rows);
    std::vector<Row> B_1(c.n_rows);

    std::vector<Row> output_share(c.n_rows);

    if (c.pid == D) {
        Permutation pi_inv = (perm_share.pi_0 * perm_share.pi_1).inverse();

        /* Sampling 1: R_0, R_1*/
        std::vector<Row> R_0, R_1;
        Row rand;

        for (size_t i = 0; i < c.n_rows; ++i) {
            c.rngs.rng_D0_unshuffle().random_data(&rand, sizeof(Row));
            R_0.push_back(rand);
        }

        for (size_t i = 0; i < c.n_rows; ++i) {
            c.rngs.rng_D1_unshuffle().random_data(&rand, sizeof(Row));
            R_1.push_back(rand);
        }

        /* Compute B_0, B_1 */
        Row R;
        c.rngs.rng_D().random_data(&R, sizeof(Row));

        B_0 = pi_inv(R_1);
        B_1 = pi_inv(R_0);

        for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

        for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

        send_vec(P0, c.network, B_0.size(), B_0, c.BLOCK_SIZE);
        send_vec(P1, c.network, B_1.size(), B_1, c.BLOCK_SIZE);
    } else {
        std::vector<Row> vec_t(c.n_rows);
        std::vector<Row> R;
        Row rand;

        if (c.pid == P0)
            recv_vec(D, c.network, B_0, c.BLOCK_SIZE);
        else
            recv_vec(D, c.network, B_1, c.BLOCK_SIZE);

        /* Sampling 1: R_0 / R_1 */
        for (size_t i = 0; i < c.n_rows; ++i) {
            if (c.pid == P0)
                c.rngs.rng_D0_unshuffle().random_data(&rand, sizeof(Row));
            else
                c.rngs.rng_D1_unshuffle().random_data(&rand, sizeof(Row));
            R.push_back(rand);
        }

        for (size_t i = 0; i < c.n_rows; ++i) vec_t[i] = input_share[i] + R[i];

        Permutation perm = c.pid == P0 ? perm_share.pi_0 : perm_share.pi_1_p;
        vec_t = perm.inverse()(vec_t);

        /* Send and receive t_0/t_1 */
        if (c.pid == P0) {
            send_vec(P1, c.network, vec_t.size(), vec_t, c.BLOCK_SIZE);
            recv_vec(P1, c.network, vec_t, c.BLOCK_SIZE);
        } else {
            std::vector<Row> data_recv(c.n_rows);
            recv_vec(P0, c.network, data_recv, c.BLOCK_SIZE);
            send_vec(P0, c.network, vec_t.size(), vec_t, c.BLOCK_SIZE);
            std::copy(data_recv.begin(), data_recv.end(), vec_t.begin());
        }

        /* Apply inverse */
        perm = c.pid == P0 ? perm_share.pi_0_p : perm_share.pi_1;
        output_share = perm.inverse()(vec_t);

        /* Last step: subtract B_0 / B_1 */
        for (size_t i = 0; i < c.n_rows; ++i) {
            if (c.pid == P0)
                output_share[i] -= B_0[i];
            else
                output_share[i] -= B_1[i];
        }
    }

    return output_share;
}

Permutation shuffle::unshuffle(ProtocolConfig &c, Permutation &perm, PermShare &perm_share) {
    std::vector<Row> perm_vec = perm.get_perm_vec();
    std::vector<Row> unshuffled = unshuffle(c, perm_vec, perm_share);
    return Permutation(unshuffled);
}

PermShare shuffle::get_merged_shuffle(ProtocolConfig &c, PermShare &pi_share, PermShare &omega_share) {
    std::vector<Row> sigma_0_p_vec(c.n_rows);
    std::vector<Row> sigma_1_vec(c.n_rows);

    std::vector<Row> B_0(c.n_rows);
    std::vector<Row> B_1(c.n_rows);
    std::vector<Row> R_0, R_1;

    PermShare perm_share;

    switch (c.pid) {
        case D: {
            Row rand;

            /* Sampling 1: R_0 / R_1 */
            for (int i = 0; i < c.n_rows; ++i) {
                c.rngs.rng_D0().random_data(&rand, sizeof(Row));
                R_0.push_back(rand);
            }

            for (int i = 0; i < c.n_rows; ++i) {
                c.rngs.rng_D1().random_data(&rand, sizeof(Row));
                R_1.push_back(rand);
            }

            /* Computing / Sampling merged permutation */
            Permutation pi = pi_share.pi_0 * pi_share.pi_1;
            Permutation omega = omega_share.pi_0 * omega_share.pi_1;

            Permutation merged = pi(omega.get_perm_vec());

            Permutation sigma_0 = Permutation::random(c.n_rows, c.rngs.rng_D0());
            Permutation sigma_1_p = Permutation::random(c.n_rows, c.rngs.rng_D1());

            Permutation sigma_0_p = sigma_1_p.inverse() * merged;
            Permutation sigma_1 = sigma_0.inverse() * merged;

            sigma_0_p_vec = sigma_0_p.get_perm_vec();
            sigma_1_vec = sigma_1.get_perm_vec();

            perm_share.pi_0 = sigma_0;
            perm_share.pi_1 = sigma_1;
            perm_share.pi_0_p = sigma_0_p;
            perm_share.pi_1_p = sigma_1_p;

            Row R;
            c.rngs.rng_D().random_data(&R, sizeof(Row));

            /* Compute B_0 / B_1 */
            B_0 = (sigma_0 * sigma_1)(R_1);
            B_1 = (sigma_0 * sigma_1)(R_0);

            for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

            for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

            send_vec(P0, c.network, sigma_0_p_vec.size(), sigma_0_p_vec, c.BLOCK_SIZE);
            send_vec(P1, c.network, sigma_1_vec.size(), sigma_1_vec, c.BLOCK_SIZE);
            send_vec(P0, c.network, B_0.size(), B_0, c.BLOCK_SIZE);
            send_vec(P1, c.network, B_1.size(), B_1, c.BLOCK_SIZE);
            break;
        }
        case P0: {
            recv_vec(D, c.network, sigma_0_p_vec, c.BLOCK_SIZE);
            recv_vec(D, c.network, B_0, c.BLOCK_SIZE);
            /* Sample sigma_0 */
            // perm_share.pi_0 = Permutation::random(c.n_rows, c.rngs.rng_D0());
            perm_share.pi_0_p = Permutation(sigma_0_p_vec);
            perm_share.B = B_0;
            break;
        }
        case P1: {
            recv_vec(D, c.network, sigma_1_vec, c.BLOCK_SIZE);
            recv_vec(D, c.network, B_1, c.BLOCK_SIZE);
            /* Sample sigma_1 */
            perm_share.pi_1 = Permutation(sigma_1_vec);
            perm_share.B = B_1;

            break;
        }
    }
    return perm_share;
}