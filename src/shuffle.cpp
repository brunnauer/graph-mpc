#include "shuffle.h"

Shuffle::Shuffle(Party pid, size_t n_rows, size_t n_rounds, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network)
    : rngs(rngs), pid(pid), n_rows(n_rows), n_rounds(n_rounds), network(network) {
    wire = std::vector<Row>(n_rows);
}

Shuffle::~Shuffle() = default;

std::vector<Row> Shuffle::result() {
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

std::vector<std::shared_ptr<ShufflePreprocessing<Row>>> Shuffle::get_preproc() { return preproc; }

void Shuffle::run() {
    run_offline();
    network->sync();
    run_online();
}

void Shuffle::run_offline() {
    for (size_t i = 0; i < n_rounds; ++i) {
        preprocess();
    }
}

void Shuffle::run_online() {
    if (preproc.size() != n_rounds) {
        throw std::length_error("Preprocessing has to be completed first.");
        return;
    }
    for (size_t i = 0; i < n_rounds; ++i) {
        evaluate();
        auto res = result();
        set_input(res);
        shuffle_idx++;
    }
}

void Shuffle::evaluate() {
    if (pid == D) return;

    size_t comm = n_rows;
    std::vector<Row> vals;

    evaluate_send_vals(vals);

    int seg_factor = 100000;  // 100000000; // Was 100000 during LAN benchmarks as per Graphiti, optimized to less rounds for WAN here!
    int num_comm = comm / seg_factor;
    int last_comm = comm % seg_factor;
    std::vector<Row> data_recv(comm);

    int partner = (pid == P0 ? 1 : 0);

    for (int i = 0; i < num_comm; ++i) {
        std::vector<Row> data_send_i;
        data_send_i.resize(seg_factor);
        for (int j = 0; j < seg_factor; ++j) {
            data_send_i[j] = vals[i * seg_factor + j];
        }
        network->send(partner, data_send_i.data(), sizeof(Row) * seg_factor);

        std::vector<Row> data_recv_i(seg_factor);
        network->recv(partner, data_recv_i.data(), sizeof(Row) * seg_factor);
        for (int j = 0; j < seg_factor; j++) {
            data_recv[i * seg_factor + j] = data_recv_i[j];
        }
    }

    std::vector<Row> data_send_last;
    data_send_last.resize(last_comm);
    for (int j = 0; j < last_comm; j++) {
        data_send_last[j] = vals[num_comm * seg_factor + j];
    }
    network->send(partner, data_send_last.data(), sizeof(Row) * last_comm);
    std::vector<Row> data_recv_last(last_comm);
    network->recv(partner, data_recv_last.data(), sizeof(Row) * last_comm);
    for (int j = 0; j < last_comm; j++) {
        data_recv[num_comm * seg_factor + j] = data_recv_last[j];
    }

    std::copy(data_recv.begin(), data_recv.end(), vals.begin());

    evaluate_recv_vals(vals);
}

/**
 * P0 computes: A_1 = pi_0_p(share + R_0)
 * P1 computes: A_0 = pi_1(share + R_1)
 */
void Shuffle::evaluate_send_vals(std::vector<Row> &vec_A) {
    ShufflePreprocessing<Row> pp = *preproc[shuffle_idx];
    std::vector<Row> *mask = (pid == P0) ? &pp.R_0 : &pp.R_1;
    std::vector<Row> to_send(n_rows);

    for (size_t j = 0; j < n_rows; ++j) {
        to_send[j] = wire[j] + (*mask)[j];
    }
    Permutation *perm = (pid == P0) ? &pp.pi_0_p : &pp.pi_1;
    to_send = (*perm)(to_send);

    for (size_t i = 0; i < n_rows; ++i) {
        vec_A.push_back(to_send[i]);
    }
}

/**
 * Last step of shuffle: Local computations are performed.
 * P0 computes: pi_0(A_0) - B_0
 * P1 computes: pi_1_p(A_1) - B_1
 */
void Shuffle::evaluate_recv_vals(std::vector<Row> &vec_A) {
    ShufflePreprocessing<Row> pp = *preproc[shuffle_idx];
    std::vector<Row> *B = (pid == P0) ? &pp.B_0 : &pp.B_1;
    Permutation *perm = (pid == P0) ? &pp.pi_0 : &pp.pi_1_p;

    for (size_t i = 0; i < n_rows; ++i) {
        wire[i] = vec_A[i];
    }

    wire = (*perm)(wire);

    for (size_t i = 0; i < n_rows; ++i) {
        wire[i] -= (*B)[i];
    }
}

/**
 * Performs all necessary steps to prepare the evaluation of a shuffle gate
 * Sends data from Dealer to P0 and P1
 */
void Shuffle::preprocess() {
    std::vector<Row> shared_secret_D0;
    std::vector<Row> shared_secret_D1;

    if (pid == D) {
        preprocess_compute(shared_secret_D0, shared_secret_D1);

        size_t arithmetic_comm_0 = shared_secret_D0.size();
        size_t arithmetic_comm_1 = shared_secret_D1.size();

        /* Send how many bytes of data should be received */
        network->send(0, &arithmetic_comm_0, sizeof(size_t));
        network->send(1, &arithmetic_comm_1, sizeof(size_t));

        /* Send the actual data */
        std::vector<Row> arithmetic_comm_0_offline(arithmetic_comm_0);
        std::vector<Row> arithmetic_comm_1_offline(arithmetic_comm_1);

        std::copy(shared_secret_D0.begin(), shared_secret_D0.end(), arithmetic_comm_0_offline.begin());
        std::copy(shared_secret_D1.begin(), shared_secret_D1.end(), arithmetic_comm_1_offline.begin());

        /* Send to P0 */
        int n_comm = arithmetic_comm_0 / BLOCK_SIZE;
        int last_comm = arithmetic_comm_0 % BLOCK_SIZE;
        for (int i = 0; i < n_comm; i++) {
            std::vector<Row> data_send_i;
            data_send_i.resize(BLOCK_SIZE);
            for (int j = 0; j < BLOCK_SIZE; j++) {
                data_send_i[j] = arithmetic_comm_0_offline[i * BLOCK_SIZE + j];
            }
            network->send(0, data_send_i.data(), sizeof(Row) * BLOCK_SIZE);
        }

        std::vector<Row> data_send_last;
        data_send_last.resize(last_comm);
        for (int j = 0; j < last_comm; j++) {
            data_send_last[j] = arithmetic_comm_0_offline[n_comm * BLOCK_SIZE + j];
        }
        network->send(0, data_send_last.data(), sizeof(Row) * last_comm);

        /* Send to P1 */
        n_comm = arithmetic_comm_1 / BLOCK_SIZE;
        last_comm = arithmetic_comm_1 % BLOCK_SIZE;
        for (int i = 0; i < n_comm; i++) {
            std::vector<Row> data_send_i;
            data_send_i.resize(BLOCK_SIZE);
            for (int j = 0; j < BLOCK_SIZE; j++) {
                data_send_i[j] = arithmetic_comm_1_offline[i * BLOCK_SIZE + j];
            }
            network->send(1, data_send_i.data(), sizeof(Row) * BLOCK_SIZE);
        }

        std::vector<Row> data_send_last_to_1;
        data_send_last_to_1.resize(last_comm);
        for (int j = 0; j < last_comm; j++) {
            data_send_last_to_1[j] = arithmetic_comm_1_offline[n_comm * BLOCK_SIZE + j];
        }
        network->send(1, data_send_last_to_1.data(), sizeof(Row) * last_comm);

    } else {  // pid is P0 or P1
        /* Receive length of data to receive */
        size_t arithmetic_comm;
        network->recv(2, &arithmetic_comm, sizeof(size_t));

        /* Receive actual data */
        int n_comm = arithmetic_comm / BLOCK_SIZE;
        int last_comm = arithmetic_comm % BLOCK_SIZE;
        std::vector<Row> arithmetic_comm_offline(arithmetic_comm);
        for (int i = 0; i < n_comm; i++) {
            std::vector<Row> data_recv_i(BLOCK_SIZE);
            network->recv(2, data_recv_i.data(), sizeof(Row) * BLOCK_SIZE);
            for (int j = 0; j < BLOCK_SIZE; j++) {
                arithmetic_comm_offline[i * BLOCK_SIZE + j] = data_recv_i[j];
            }
        }

        /* Receive last bytes */
        std::vector<Row> data_recv_last(last_comm);
        network->recv(2, data_recv_last.data(), sizeof(Row) * last_comm);
        for (int j = 0; j < last_comm; j++) {
            arithmetic_comm_offline[n_comm * BLOCK_SIZE + j] = data_recv_last[j];
        }

        /* Copy shared data to array */
        shared_secret_D0.resize(arithmetic_comm);
        shared_secret_D1.resize(arithmetic_comm);

        switch (pid) {
            case P0: {
                for (int i = 0; i < arithmetic_comm; ++i) {
                    shared_secret_D0[i] = arithmetic_comm_offline[i];
                }

                break;
            }
            case P1: {
                for (int i = 0; i < arithmetic_comm; ++i) {
                    shared_secret_D1[i] = arithmetic_comm_offline[i];
                }

                break;
            }
            default:
                break;
        }
        preprocess_compute(shared_secret_D0, shared_secret_D1);
    }
}

/**
 * Generates all necessary data for performing a shuffle: pi_0, pi_1, pi_0_p, pi_1_p, R_0, R_1, B_0, B_1
 * Appends result to: std::vector<std::shared_ptr<ShufflePreprocessing<Row>>> preproc
 */
void Shuffle::preprocess_compute(std::vector<Row> &shared_secret_D0, std::vector<Row> &shared_secret_D1) {
    size_t shared_secret_D0_idx = 0;
    size_t shared_secret_D1_idx = 0;

    /* Sample R_0 and R_1 */
    std::vector<Row> R_0, R_1;
    Row rand;
    if (pid != P1) {
        for (int i = 0; i < n_rows; ++i) {
            rngs.rng_D0().random_data(&rand, sizeof(Row));
            R_0.push_back(rand);
        }
    }

    if (pid != P0) {
        for (int i = 0; i < n_rows; ++i) {
            rngs.rng_D1().random_data(&rand, sizeof(Row));
            R_1.push_back(rand);
        }
    }

    /* Sample pi_0, pi_1 and pi_0_p */
    Permutation pi_0(n_rows);
    Permutation pi_1(n_rows);
    Permutation pi_0_p(n_rows);
    Permutation pi_1_p(n_rows);

    if (pid != P1) {
        pi_0 = Permutation::random(n_rows, rngs.rng_D0());
        pi_0_p = Permutation::random(n_rows, rngs.rng_D0());
    }
    if (pid != P0) {
        pi_1 = Permutation::random(n_rows, rngs.rng_D1());
    }

    if (pid == D) {  // Compute and send pi_1_p
        Permutation pi_0_p_inv = pi_0_p.inverse();
        pi_1_p = (pi_0 * pi_1 * pi_0_p_inv);

        for (int i = 0; i < n_rows; ++i) {  // Send pi_1_p to P1's shared_secret_buffer
            shared_secret_D1.push_back((Row)pi_1_p[i]);
        }
    } else if (pid == P1) {  // Receive pi_1_p from P1's shared_secret_buffer
        std::vector<size_t> pi_1_p_vec(n_rows);
        for (int i = 0; i < n_rows; ++i) {
            pi_1_p_vec[i] = shared_secret_D1[shared_secret_D1_idx];
            shared_secret_D1_idx++;
        }
        pi_1_p = Permutation(pi_1_p_vec);
    }

    std::vector<Row> B_0(n_rows);
    std::vector<Row> B_1(n_rows);

    if (pid == D) {  // Compute B_0 and B_1
        Row R;
        rngs.rng_D().random_data(&R, sizeof(Row));

        B_0 = (pi_0 * pi_1)(R_0);
        B_1 = (pi_0 * pi_1)(R_1);

        for (size_t i = 0; i < B_0.size(); ++i) {
            B_0[i] -= R;
        }

        for (size_t i = 0; i < B_1.size(); ++i) {
            B_1[i] += R;
        }

        for (int i = 0; i < n_rows; ++i) {  // Send B_0 and B_1 to the corresponding buffers
            shared_secret_D0.push_back(B_0[i]);
            shared_secret_D1.push_back(B_1[i]);
        }
    } else if (pid == P0) {  // Receive B_0 from shared_secret_D0
        for (int i = 0; i < n_rows; ++i) {
            B_0[i] = shared_secret_D0[shared_secret_D0_idx];
            shared_secret_D0_idx++;
        }
    } else {  // Receive B_1 from shared_secret_D1
        for (int i = 0; i < n_rows; ++i) {
            B_1[i] = shared_secret_D1[shared_secret_D1_idx];
            shared_secret_D1_idx++;
        }
    }

    /* Save preprocessing results */
    preproc.push_back(std::shared_ptr<ShufflePreprocessing<Row>>(new ShufflePreprocessing<Row>(pi_0, pi_1, pi_0_p, pi_1_p, R_0, R_1, B_0, B_1)));
}
