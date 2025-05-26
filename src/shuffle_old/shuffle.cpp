#include "shuffle.h"

Shuffle::Shuffle(ProtocolConfig &conf, size_t n_rounds) : rngs(conf.rngs), pid(conf.pid), n_rows(conf.n_rows), network(conf.network), n_rounds(n_rounds) {
    wire = std::vector<Row>(n_rows);
}

Shuffle::~Shuffle() = default;

std::vector<Row> Shuffle::reveal() {
    std::vector<Row> outvals(n_rows);
    if (wire.empty()) {
        return outvals;
    }

    if (pid != D) {
        std::vector<Row> output_share_self(wire.size());
        std::vector<Row> output_share_other(wire.size());

        std::copy(wire.begin(), wire.end(), output_share_self.begin());

        size_t partner = (pid == P0) ? 1 : 0;
        network->send(partner, output_share_self.data(), output_share_self.size() * sizeof(Row));
        network->recv(partner, output_share_other.data(), output_share_other.size() * sizeof(Row));

        for (size_t i = 0; i < wire.size(); ++i) {
            Row outmask = output_share_other[i];
            outvals[i] = output_share_self[i] + outmask;
        }
        return outvals;
    } else {
        return outvals;
    }
}

void Shuffle::set_input(std::vector<Row> &input) {
    if (input.size() != n_rows) {
        throw std::invalid_argument("Input vector has wrong size.");
    }
    wire = input;
}

void Shuffle::set_input(Permutation &input) {
    if (input.get_perm_vec().size() != n_rows) {
        throw std::invalid_argument("Input permutation has wrong size.");
    }
    wire = input.get_perm_vec();
}

std::vector<Row> Shuffle::get_output() { return wire; }

void Shuffle::run() {
    get_shuffle();
    network->sync();
    shuffle();
}

void Shuffle::unshuffle() {
    if (shuffle_idx == 0) throw std::runtime_error("Cannot unshuffle if no previous shuffle has been performed.");

    _reverse = true;

    get_shuffle();
    network->sync();
    shuffle();

    _reverse = false;
}

void Shuffle::repeat() {
    if (shuffle_idx == 0) throw std::runtime_error("Cannot repeat if no previous shuffle has been performed.");

    _new_perm = false;
    shuffle_idx -= n_rounds;

    get_shuffle();
    network->sync();
    shuffle();

    _new_perm = true;
}

void Shuffle::get_shuffle() {
    for (size_t i = 0; i < n_rounds; ++i) {
        if (_reverse) shuffle_idx--;
        preprocess();
    }
}

void Shuffle::shuffle() {
    if (_reverse) {
        shuffle_idx += n_rounds;
    }

    if (pid == D) {
        if (_reverse) {
            shuffle_idx -= n_rounds;
            for (size_t i = 0; i < n_rounds; ++i) {
                pi_0_vec.pop_back();
                pi_1_vec.pop_back();
                pi_0_p_vec.pop_back();
                pi_1_p_vec.pop_back();
            }
        } else
            shuffle_idx += n_rounds;
        return;
    }

    for (size_t i = 0; i < n_rounds; ++i) {
        if (_reverse) shuffle_idx--;
        evaluate();
        if (!_reverse) shuffle_idx++;
    }
}

void Shuffle::evaluate() {
    std::vector<Row> vals(n_rows);

    if (_reverse) {
        std::vector<Row> R;
        Row rand;

        /* Sample R_0 / R_1 */
        for (size_t i = 0; i < n_rows; ++i) {
            if (pid == P0)
                rngs.rng_D0().random_data(&rand, sizeof(Row));
            else if (pid == P1)
                rngs.rng_D1().random_data(&rand, sizeof(Row));
            R.push_back(rand);
        }

        for (size_t i = 0; i < n_rows; ++i) {
            wire[i] += R[i];
        }

        Permutation perm = pid == P0 ? pi_0_vec[shuffle_idx] : pi_1_p_vec[shuffle_idx];
        vals = perm.inverse()(wire);

    } else {
        evaluate_compute_A(vals);
    }

    /* Send and receive A_0/A_1 */
    if (pid == P0) {
        send_vec(P1, network, vals.size(), vals, BLOCK_SIZE_EVAL);
        recv_vec(P1, network, vals, BLOCK_SIZE_EVAL);
    } else {
        std::vector<Row> data_recv(n_rows);
        recv_vec(P0, network, data_recv, BLOCK_SIZE_EVAL);
        send_vec(P0, network, vals.size(), vals, BLOCK_SIZE_EVAL);
        std::copy(data_recv.begin(), data_recv.end(), vals.begin());
    }

    if (_reverse) {
        Permutation perm = pid == P0 ? pi_0_p_vec[shuffle_idx] : pi_1_vec[shuffle_idx];
        wire = perm.inverse()(vals);

        /* Last step: subtract B_0 / B_1 */
        for (size_t i = 0; i < n_rows; ++i) {
            wire[i] -= B_vec[shuffle_idx][i];
        }

        B_vec.pop_back();
        R_vec.pop_back();

        if (pid == P0) {
            pi_0_vec.pop_back();
            pi_0_p_vec.pop_back();
        } else {
            pi_1_vec.pop_back();
            pi_1_p_vec.pop_back();
        }

    } else {
        evaluate_compute_output(vals);
    }
}

/**
 * P0 computes: A_1 = pi_0_p(share + R_0)
 * P1 computes: A_0 = pi_1(share + R_1)
 */
void Shuffle::evaluate_compute_A(std::vector<Row> &vec_A) {
    std::vector<Row> R;
    Permutation perm;
    Row rand;

    if (_new_perm) {
        /* Sample R_0 / R_1 */
        for (int i = 0; i < n_rows; ++i) {
            if (pid == P0)
                rngs.rng_D0().random_data(&rand, sizeof(Row));
            else if (pid == P1)
                rngs.rng_D1().random_data(&rand, sizeof(Row));
            R.push_back(rand);
        }
        R_vec.push_back(R);
    } else {
        R = R_vec[shuffle_idx];
    }

    /* Compute input + R */
    for (size_t j = 0; j < n_rows; ++j) {
        vec_A[j] = wire[j] + R[j];
    }

    /* Sample pi_0_p / pi_1 or reuse old one */
    if (pid == P0) {
        if (_new_perm) {
            perm = Permutation::random(n_rows, rngs.rng_D0());
            pi_0_p_vec.push_back(perm);
        } else {
            perm = pi_0_p_vec[shuffle_idx];
        }
    } else if (pid == P1) {
        if (_new_perm) {
            perm = Permutation::random(n_rows, rngs.rng_D1());
            pi_1_vec.push_back(perm);
        } else {
            perm = pi_1_vec[shuffle_idx];
        }
    }

    /* P0: sample pi_0 */
    if (pid == P0 && _new_perm) pi_0_vec.push_back(Permutation::random(n_rows, rngs.rng_D0()));

    /* Compute perm(input + R) */
    vec_A = perm(vec_A);
}

/**
 * Last step of shuffle: Local computations are performed.
 * P0 computes: pi_0(A_0) - B_0
 * P1 computes: pi_1_p(A_1) - B_1
 */
void Shuffle::evaluate_compute_output(std::vector<Row> &vec_A) {
    for (size_t i = 0; i < n_rows; ++i) {
        wire[i] = vec_A[i];
    }

    Permutation perm = pid == P0 ? pi_0_vec[shuffle_idx] : pi_1_p_vec[shuffle_idx];
    wire = perm(wire);

    for (size_t i = 0; i < n_rows; ++i) {
        wire[i] -= B_vec[shuffle_idx][i];
    }
}

/**
 * Performs all necessary steps to prepare the evaluation of a shuffle gate
 * Sends data from Dealer to P0 and P1
 */
void Shuffle::preprocess() {
    std::vector<Row> shared_secret_D0;
    std::vector<Row> shared_secret_D1;

    switch (pid) {
        case D: {
            if (_reverse) {
                preprocess_compute_reverse(shared_secret_D0, shared_secret_D1);
                send_vec(P0, network, shared_secret_D0.size(), shared_secret_D0, BLOCK_SIZE_PRE);
                send_vec(P1, network, shared_secret_D1.size(), shared_secret_D1, BLOCK_SIZE_PRE);
            } else if (_new_perm) {
                preprocess_compute(shared_secret_D0, shared_secret_D1);
                send_vec(P0, network, shared_secret_D0.size(), shared_secret_D0, BLOCK_SIZE_PRE);
                send_vec(P1, network, shared_secret_D1.size(), shared_secret_D1, BLOCK_SIZE_PRE);
            }
            break;
        }
        case P0: {
            if (_new_perm || _reverse) {
                recv_vec(D, network, shared_secret_D0, BLOCK_SIZE_PRE);
                preprocess_compute(shared_secret_D0, shared_secret_D1);
            }
            break;
        }
        case P1: {
            if (_new_perm || _reverse) {
                recv_vec(D, network, shared_secret_D1, BLOCK_SIZE_PRE);
                preprocess_compute(shared_secret_D0, shared_secret_D1);
            }
            break;
        }
    }
}

/* Only executed by Dealer */
void Shuffle::preprocess_compute_reverse(std::vector<Row> &shared_secret_D0, std::vector<Row> &shared_secret_D1) {
    size_t shared_secret_D0_idx = 0;
    size_t shared_secret_D1_idx = 0;

    std::vector<Row> R_0, R_1;
    Row rand;

    Permutation pi_0(n_rows);
    Permutation pi_1(n_rows);

    for (int i = 0; i < n_rows; ++i) {
        rngs.rng_D0().random_data(&rand, sizeof(Row));
        R_0.push_back(rand);
    }

    for (int i = 0; i < n_rows; ++i) {
        rngs.rng_D1().random_data(&rand, sizeof(Row));
        R_1.push_back(rand);
    }

    pi_0 = pi_0_vec[shuffle_idx];
    pi_1 = pi_1_vec[shuffle_idx];

    std::vector<Row> B_0(n_rows);
    std::vector<Row> B_1(n_rows);

    Row R;
    rngs.rng_D().random_data(&R, sizeof(Row));

    B_0 = (pi_0 * pi_1).inverse()(R_1);
    B_1 = (pi_0 * pi_1).inverse()(R_0);

    for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

    for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

    for (int i = 0; i < n_rows; ++i) {
        /* Send B_0 and B_1 to the corresponding buffers */
        shared_secret_D0.push_back(B_0[i]);
        shared_secret_D1.push_back(B_1[i]);
    }
}

void Shuffle::preprocess_compute(std::vector<Row> &shared_secret_D0, std::vector<Row> &shared_secret_D1) {
    size_t shared_secret_D0_idx = 0;
    size_t shared_secret_D1_idx = 0;

    switch (pid) {
        case D: {
            std::vector<Row> R_0, R_1;
            Row rand;

            Permutation pi_0;
            Permutation pi_1;
            Permutation pi_0_p;
            Permutation pi_1_p;

            for (int i = 0; i < n_rows; ++i) {
                rngs.rng_D0().random_data(&rand, sizeof(Row));
                R_0.push_back(rand);
            }

            for (int i = 0; i < n_rows; ++i) {
                rngs.rng_D1().random_data(&rand, sizeof(Row));
                R_1.push_back(rand);
            }

            if (_new_perm) {
                pi_0_p = Permutation::random(n_rows, rngs.rng_D0());
                pi_0 = Permutation::random(n_rows, rngs.rng_D0());
                pi_1 = Permutation::random(n_rows, rngs.rng_D1());

                Permutation pi_0_p_inv = pi_0_p.inverse();
                pi_1_p = (pi_0 * pi_1 * pi_0_p_inv);

                pi_0_vec.push_back(pi_0);
                pi_0_p_vec.push_back(pi_0_p);
                pi_1_vec.push_back(pi_1);
                pi_1_p_vec.push_back(pi_1_p);

                for (int i = 0; i < n_rows; ++i) {
                    /* Send pi_1_p to P1's shared_secret_buffer */
                    shared_secret_D1.push_back((Row)pi_1_p[i]);
                }

                std::vector<Row> B_0(n_rows);
                std::vector<Row> B_1(n_rows);

                Row R;
                rngs.rng_D().random_data(&R, sizeof(Row));

                B_0 = (pi_0 * pi_1)(R_1);
                B_1 = (pi_0 * pi_1)(R_0);

                for (size_t i = 0; i < B_0.size(); ++i) B_0[i] -= R;

                for (size_t i = 0; i < B_1.size(); ++i) B_1[i] += R;

                for (int i = 0; i < n_rows; ++i) {
                    /* Send B_0 and B_1 to the corresponding buffers */
                    shared_secret_D0.push_back(B_0[i]);
                    shared_secret_D1.push_back(B_1[i]);
                }
            }
            break;
        }
        case P0: {
            if (!_new_perm) break;

            /* Receive B_0 from shared_secret_D0 */
            std::vector<Row> B(n_rows);
            for (int i = 0; i < n_rows; ++i) {
                B[i] = shared_secret_D0[shared_secret_D0_idx];
                shared_secret_D0_idx++;
            }
            if (_reverse)
                B_vec[shuffle_idx] = B;
            else
                B_vec.push_back(B);
            break;
        }

        case P1: {
            /* Receive pi_1_p from P1's shared_secret_buffer (only if new permutation is sampled) */
            if (!_new_perm && !_reverse) break;
            if (!_reverse) {
                std::vector<Row> perm_vec(n_rows);
                for (int i = 0; i < n_rows; ++i) {
                    perm_vec[i] = shared_secret_D1[shared_secret_D1_idx];
                    shared_secret_D1_idx++;
                }

                Permutation pi_1_p = Permutation(perm_vec);
                pi_1_p_vec.push_back(pi_1_p);
            }

            /* Receive B_1 */
            std::vector<Row> B(n_rows);
            for (int i = 0; i < n_rows; ++i) {
                B[i] = shared_secret_D1[shared_secret_D1_idx];
                shared_secret_D1_idx++;
            }
            if (_reverse)
                B_vec[shuffle_idx] = B;
            else
                B_vec.push_back(B);
            break;
        }
    }
}
