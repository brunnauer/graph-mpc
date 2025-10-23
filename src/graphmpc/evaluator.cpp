#include "evaluator.h"

std::vector<Ring> Evaluator::read_online(std::vector<Ring> &buffer, size_t n_elems) {
    if (n_elems > buffer.size()) {
        throw new std::invalid_argument("Trying to read more online elements than available.");
    } else {
        std::vector<Ring> data(n_elems);
        data = {buffer.begin(), buffer.begin() + n_elems};

        /* Delete read values from buffer */
        buffer.erase(buffer.begin(), buffer.begin() + n_elems);
        return data;
    }
}

void Evaluator::run(Circuit *circ) {
    if (id == D) return;

    if (!initialized) {
        waiting.resize(circ->n_wires);
        wires.resize(circ->n_wires);
        initialized = true;
    }

    init_waiting(circ);  // Initialize mapping of which level outputs are no longer needed
    for (auto &layer : circ->get()) {
        evaluate_send(layer);
        online_communication();
        evaluate_recv(layer);
        update_wires(layer);
    }

    std::cout << "Communication (Entire round): " << comm_ms << std::endl;
    std::cout << "Communication (Sending/Receiving): " << sr_ms << std::endl;
    comm_ms = 0;
    sr_ms = 0;
}

const std::vector<Ring> Evaluator::result() { return output; }

void Evaluator::online_communication() {
    auto round_begin = std::chrono::high_resolution_clock::now();

    size_t n_send = mul_vals.size() + and_vals.size() + shuffle_vals.size() + reveal_vals.size();
    size_t n_recv = 0;

    std::vector<Ring> send_vals;
    send_vals.insert(send_vals.end(), mul_vals.begin(), mul_vals.end());
    send_vals.insert(send_vals.end(), and_vals.begin(), and_vals.end());
    send_vals.insert(send_vals.end(), shuffle_vals.begin(), shuffle_vals.end());
    send_vals.insert(send_vals.end(), reveal_vals.begin(), reveal_vals.end());

    auto comm_begin = std::chrono::high_resolution_clock::now();
    if (id == P0) {
        network->send(P1, &n_send, sizeof(size_t));
        network->send_vec(P1, n_send, send_vals);
        network->recv(P1, &n_recv, sizeof(size_t));
        data_recv.resize(n_recv);
        network->recv_vec(P1, n_recv, data_recv);
    } else {
        network->recv(P0, &n_recv, sizeof(size_t));
        data_recv.resize(n_recv);
        network->recv_vec(P0, n_recv, data_recv);
        network->send(P0, &n_send, sizeof(size_t));
        network->send_vec(P0, n_send, send_vals);
    }
    auto comm_end = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < mul_vals.size(); i++) {
        mul_vals[i] += data_recv[i];
    }
    for (size_t i = 0; i < and_vals.size(); i++) {
        and_vals[i] ^= data_recv[mul_vals.size() + i];
    }
    for (size_t i = 0; i < shuffle_vals.size(); i++) {
        shuffle_vals[i] = data_recv[mul_vals.size() + and_vals.size() + i];
    }
    for (size_t i = 0; i < reveal_vals.size(); i++) {
        reveal_vals[i] += data_recv[mul_vals.size() + and_vals.size() + shuffle_vals.size() + i];
    }
    data_recv.clear();

    auto round_end = std::chrono::high_resolution_clock::now();

    auto time_round = std::chrono::duration_cast<std::chrono::milliseconds>(round_end - round_begin);
    auto time_comm = std::chrono::duration_cast<std::chrono::milliseconds>(comm_end - comm_begin);

    comm_ms += time_round.count();
    sr_ms += time_comm.count();
}

void Evaluator::evaluate_send(std::vector<std::shared_ptr<Function>> &layer) {
    for (auto &f : layer) {
        switch (f->type) {
            case Input: {
                break;
            }

            case MergedShuffle:
            case Shuffle: {
                std::vector<Ring> t(size);
                std::vector<Ring> R(size);
                Permutation perm;

                /* Sampling 1: R_0 / R_1 */
                for (size_t i = 0; i < size; ++i) {
                    if (id == P0)
                        rngs->rng_D0_send().random_data(&R[i], sizeof(Ring));
                    else if (id == P1)
                        rngs->rng_D1_send().random_data(&R[i], sizeof(Ring));
                }

                /* Sampling 2: pi_0_p / pi_1 */
                if (id == P0) {
                    if (store->pi_0_p[f->shuffle_idx].size() == 0) {
                        perm = Permutation::random(size, rngs->rng_D0_send());
                        store->pi_0_p[f->shuffle_idx] = perm;
                    } else {
                        perm = store->pi_0_p[f->shuffle_idx];
                    }
                } else {
                    if (store->pi_1[f->shuffle_idx].size() == 0) {
                        perm = Permutation::random(size, rngs->rng_D1_send());
                        store->pi_1[f->shuffle_idx] = perm;
                    } else {
                        perm = store->pi_1[f->shuffle_idx];
                    }
                }

                /* Compute input + R */
#pragma omp parallel for if (size > 10000)
                for (size_t j = 0; j < size; ++j) {
                    t[j] = wires[f->in1_idx][j] + R[j];
                }

                /* Compute perm(input + R) */
                std::vector<Ring> t_permuted(size);
                t_permuted = perm(t);
                t.swap(t_permuted);

                shuffle_vals.insert(shuffle_vals.end(), t.begin(), t.end());
                break;
            }

            case Unshuffle: {
                std::vector<Ring> vec_t(size);
                std::vector<Ring> R(size);

                /* Sampling 1: R_0 / R_1 */
                if (id == P0) {
                    for (size_t i = 0; i < size; ++i) rngs->rng_D0_send().random_data(&R[i], sizeof(Ring));
                } else {
                    for (size_t i = 0; i < size; ++i) rngs->rng_D1_send().random_data(&R[i], sizeof(Ring));
                }

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) vec_t[i] = wires[f->in1_idx][i] + R[i];

                Permutation perm = id == P0 ? store->pi_0[f->shuffle_idx] : store->pi_1_p[f->shuffle_idx];

                std::vector<Ring> vec_t_permuted(size);
                vec_t_permuted = perm.inverse()(vec_t);
                vec_t.swap(vec_t_permuted);

                shuffle_vals.insert(shuffle_vals.end(), vec_t.begin(), vec_t.end());
                break;
            }

            case Compaction: {
                std::vector<Ring> _a, _b, _c;
                store->load_triples(_a, _b, _c, f->mult_idx);

                std::vector<Ring> f_0(size);

                // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
                for (size_t i = 0; i < size; ++i) {
                    f_0[i] = -wires[f->in1_idx][i];
                    if (id == P0) {
                        f_0[i] += 1;  // 1 as constant only to one share
                    }
                }

                std::vector<Ring> s_0(size);
                std::vector<Ring> s_1(size);
                Ring s = 0;
                // Set s_0 to prefix sum of f_0 and s_1 to prefix sum of f_1/input continuing from the prior final value
                for (size_t i = 0; i < size; ++i) {
                    s += f_0[i];
                    s_0[i] = s;
                }

                for (size_t i = 0; i < size; ++i) {
                    s += wires[f->in1_idx][i];
                    s_1[i] = s - s_0[i];
                }

                size_t old = mul_vals.size();
                mul_vals.resize(old + 2 * size);
                auto send_ptr = mul_vals.data() + old;

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];

                    auto xa = wires[f->in1_idx][i] + a;
                    auto yb = s_1[i] + b;
                    send_ptr[2 * i] = xa;
                    send_ptr[2 * i + 1] = yb;
                }
                break;
            }

            case Mul: {
                std::vector<Ring> _a, _b, _c;
                store->load_triples(_a, _b, _c, f->mult_idx, f->size);

                std::vector<Ring> data_send(2 * f->size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < f->size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    Ring xa, yb;
                    if (f->binary) {
                        xa = wires[f->in1_idx][i] ^ a;
                        yb = wires[f->in2_idx][i] ^ b;
                    } else {
                        xa = wires[f->in1_idx][i] + a;
                        yb = wires[f->in2_idx][i] + b;
                    }
                    data_send[2 * i] = xa;
                    data_send[2 * i + 1] = yb;
                }
                if (f->binary) {
                    and_vals.insert(and_vals.end(), data_send.begin(), data_send.end());
                } else {
                    mul_vals.insert(mul_vals.end(), data_send.begin(), data_send.end());
                }
                break;
            }

            case EQZ: {
                std::vector<Ring> _a, _b, _c;
                store->load_triples(_a, _b, _c, f->mult_idx, f->size);

                /* x_0 == -x_1 <=> ~x_0 ^ -x_1 */
                if (id == P0 && f->layer == 0) {
#pragma omp parallel for if (size > 10000)
                    // for (auto &elem : f->in1) elem = ~elem;
                    for (size_t i = 0; i < f->size; ++i) {
                        wires[f->in1_idx][i] = ~wires[f->in1_idx][i];
                    }
                }

                if (id == P1 && f->layer == 0) {
#pragma omp parallel for if (size > 10000)
                    // for (auto &elem : f->in1) elem = -elem;
                    for (size_t i = 0; i < f->size; ++i) {
                        wires[f->in1_idx][i] = -wires[f->in1_idx][i];
                    }
                }

                std::vector<Ring> shares_left(f->size);
                std::vector<Ring> shares_right(f->size);
                for (size_t i = 0; i < f->size; ++i) {
                    shares_left[i] = wires[f->in1_idx][i];
                }
                for (size_t i = 0; i < f->size; ++i) {
                    shares_right[i] = wires[f->in1_idx][i];
                }

                size_t width = 1 << (4 - f->layer);

#pragma omp parallel for if (size > 10000)
                for (auto &elem : shares_left) elem >>= width;

                size_t old = and_vals.size();
                and_vals.resize(old + 2 * f->size);
                auto send_ptr = and_vals.data() + old;

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < f->size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    auto xa = shares_left[i] ^ a;
                    auto yb = shares_right[i] ^ b;
                    send_ptr[2 * i] = xa;
                    send_ptr[2 * i + 1] = yb;
                }
                break;
            }

            case Bit2A: {
                std::vector<Ring> _a, _b, _c;
                store->load_triples(_a, _b, _c, f->mult_idx, f->size);

                size_t old = mul_vals.size();
                mul_vals.resize(old + 2 * f->size);
                auto send_ptr = mul_vals.data() + old;

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < f->size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    auto xa = (id == P0 ? 1 : 0) * (wires[f->in1_idx][i] & 1) + a;
                    auto yb = (id == P0 ? 0 : 1) * (wires[f->in1_idx][i] & 1) + b;
                    send_ptr[2 * i] = xa;
                    send_ptr[2 * i + 1] = yb;
                }
                break;
            }

            case Reveal: {
                for (size_t i = 0; i < size; ++i) {
                    reveal_vals.push_back(wires[f->in1_idx][i]);
                }
                break;
            }
            default:
                break;
        }
    }
}

void Evaluator::evaluate_recv(std::vector<std::shared_ptr<Function>> &layer) {
    for (auto &f : layer) {
        switch (f->type) {
            case Input: {
                wires[f->out_idx] = input_to_val[f->out_idx];
                break;
            }
            case Output: {
                output.resize(size);
                for (size_t i = 0; i < size; ++i) {
                    output[i] = wires[f->in1_idx][i];
                }
                break;
            }
            case MergedShuffle:
            case Shuffle: {
                std::vector<Ring> output(size);
                Permutation perm;
                if (id == P0) {
                    if (store->pi_0[f->shuffle_idx].size() == 0) {
                        perm = Permutation::random(size, rngs->rng_D0_recv());
                        store->pi_0[f->shuffle_idx] = perm;
                    } else {
                        perm = store->pi_0[f->shuffle_idx];
                    }
                } else {
                    if (store->pi_1_p[f->shuffle_idx].size() == 0) {
                        perm = Permutation::random(size, rngs->rng_D1_recv());
                        store->pi_1_p[f->shuffle_idx] = perm;
                    } else {
                        perm = store->pi_1_p[f->shuffle_idx];
                    }
                }

                auto t = read_online(shuffle_vals, size);  // t
                t = perm(t);

                std::vector<Ring> B;
                store->read_preproc(B, size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    output[i] = t[i] - B[i];  // Read B directly from preprocessing (needs recv also in evaluator)
                }
                wires[f->out_idx] = output;
                break;
            }

            case Unshuffle: {
                std::vector<Ring> output(size);
                Permutation perm;
                perm = id == P0 ? store->pi_0_p[f->shuffle_idx] : store->pi_1[f->shuffle_idx];

                std::vector<Ring> vec_t = read_online(shuffle_vals, size);

                /* Apply inverse */
                vec_t = perm.inverse()(vec_t);

                /* Last step: subtract B_0 / B_1 */
                std::vector<Ring> unshuffle;
                store->read_preproc(unshuffle, size);  // Read directly from preprocessing
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) vec_t[i] -= unshuffle[i];

                for (size_t i = 0; i < size; ++i) output[i] = vec_t[i];

                wires[f->out_idx] = output;
                break;
            }

            case Compaction: {
                std::vector<Ring> output(size);
                std::vector<Ring> _a, _b, _c;
                store->load_triples(_a, _b, _c, f->mult_idx);

                std::vector<Ring> f_0(size);
                // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
                for (size_t i = 0; i < size; ++i) {
                    f_0[i] = -wires[f->in1_idx][i];
                    if (id == P0) {
                        f_0[i] += 1;  // 1 as constant only to one share
                    }
                }

                std::vector<Ring> s_0(size);
                Ring s = 0;
                // Set s_0 to prefix sum of f_0 and s_1 to prefix sum of f_1/input continuing from the prior final value
                for (size_t i = 0; i < size; ++i) {
                    s += f_0[i];
                    s_0[i] = s;
                }

                auto data_recv = read_online(mul_vals, 2 * size);

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    Ring c = _c[i];

                    auto xa = data_recv[2 * i];
                    auto yb = data_recv[2 * i + 1];

                    output[i] = (xa * yb * (id)) - (xa * b) - (yb * a) + c;
                }

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    output[i] += s_0[i];
                    if (id == P0) output[i]--;
                }

                wires[f->out_idx] = output;
                break;
            }

            case Mul: {
                std::vector<Ring> output(f->size);
                std::vector<Ring> _a, _b, _c;
                store->load_triples(_a, _b, _c, f->mult_idx, f->size);

                std::vector<Ring> data_recv;
                if (f->binary) {
                    data_recv = read_online(and_vals, 2 * f->size);
                } else {
                    data_recv = read_online(mul_vals, 2 * f->size);
                }

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < f->size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    Ring c = _c[i];

                    auto xa = data_recv[2 * i];
                    auto yb = data_recv[2 * i + 1];

                    if (f->binary)
                        output[i] = (xa & yb) * (id) ^ (xa & b) ^ (yb & a) ^ c;
                    else
                        output[i] = (xa * yb * (id)) - (xa * b) - (yb * a) + c;
                }
                wires[f->out_idx] = output;
                break;
            }

            case EQZ: {
                std::vector<Ring> output(f->size);
                std::vector<Ring> _a, _b, _c;
                store->load_triples(_a, _b, _c, f->mult_idx, f->size);

                std::vector<Ring> data_recv = read_online(and_vals, 2 * f->size);

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < f->size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    Ring c = _c[i];

                    auto xa = data_recv[2 * i];
                    auto yb = data_recv[2 * i + 1];

                    output[i] = (xa & yb) * (id) ^ (xa & b) ^ (yb & a) ^ c;
                }
                if (f->layer == 4) {
#pragma omp parallel for if (size > 10000)
                    for (size_t i = 0; i < f->size; ++i) {
                        output[i] <<= 31;
                        output[i] >>= 31;
                    }
                }
                wires[f->out_idx] = output;
                break;
            }

            case Bit2A: {
                std::vector<Ring> output(f->size);
                std::vector<Ring> _a, _b, _c;
                store->load_triples(_a, _b, _c, f->mult_idx, f->size);

                std::vector<Ring> data_recv = read_online(mul_vals, 2 * f->size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < f->size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    Ring c = _c[i];
                    auto xa = data_recv[2 * i];
                    auto yb = data_recv[2 * i + 1];

                    auto mul_result = (xa * yb * (id)) - (xa * b) - (yb * a) + c;
                    output[i] = (wires[f->in1_idx][i] & 1) - 2 * mul_result;
                }
                wires[f->out_idx] = output;
                break;
            }

            case Reveal: {
                auto data_revealed = read_online(reveal_vals, size);
                wires[f->out_idx] = data_revealed;
                break;
            }

            case Permute: {
                std::vector<Ring> output(size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) output[wires[f->in2_idx][i]] = wires[f->in1_idx][i];
                wires[f->out_idx] = output;
                break;
            }

            case ReversePermute: {
                std::vector<Ring> output(size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    output[i] = wires[f->in1_idx][wires[f->in2_idx][i]];
                }
                wires[f->out_idx] = output;
                break;
            }

            case AddConst: {
                std::vector<Ring> output(size);
#pragma omp parallel for if (nodes > 10000)
                for (size_t j = 0; j < size; ++j) {
                    if (id == P0 && j < nodes) {
                        output[j] = wires[f->in1_idx][j] + f->val;
                    } else {
                        output[j] = wires[f->in1_idx][j];
                    }
                }
                wires[f->out_idx] = output;
                break;
            }

            case Add: {
                std::vector<Ring> output(size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    output[i] = wires[f->in1_idx][i] + wires[f->in2_idx][i];
                }
                wires[f->out_idx] = output;
                break;
            }

            case Flip: {
                std::vector<Ring> output(size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    if (id == P0) {
                        output[i] = 1 - wires[f->in1_idx][i];
                    } else if (id == P1) {
                        output[i] = -wires[f->in1_idx][i];
                    }
                }
                wires[f->out_idx] = output;
                break;
            }

            case Propagate1: {
                std::vector<Ring> output(size);
                for (size_t i = nodes - 1; i > 0; --i) {
                    output[i] = wires[f->in1_idx][i] - wires[f->in1_idx][i - 1];
                }
                output[0] = wires[f->in1_idx][0];
                for (size_t i = nodes; i < size; ++i) {
                    output[i] = wires[f->in1_idx][i];
                }
                wires[f->out_idx] = output;
                break;
            }

            case Propagate2: {
                std::vector<Ring> output(size);
                Ring sum = 0;
                for (size_t i = 0; i < size; ++i) {
                    sum += wires[f->in1_idx][i];
                    output[i] = sum - wires[f->in2_idx][i];
                }
                wires[f->out_idx] = output;
                break;
            }

            case Gather1: {
                std::vector<Ring> output(size);
                Ring sum = 0;
                for (size_t i = 0; i < size; ++i) {
                    sum += wires[f->in1_idx][i];
                    output[i] = sum;
                }
                wires[f->out_idx] = output;
                break;
            }

            case Gather2: {
                std::vector<Ring> output(size);
                Ring sum = 0;
                for (size_t i = 0; i < nodes; ++i) {
                    output[i] = wires[f->in1_idx][i] - sum;
                    sum += output[i];
                }
#pragma omp parallel for if (size > 10000)
                for (size_t i = nodes; i < size; ++i) {
                    output[i] = 0;
                }
                wires[f->out_idx] = output;
                break;
            }

            case Sub: {
                std::vector<Ring> output(size - 1);
                for (size_t i = 1; i < size; ++i) {
                    output[i - 1] = wires[f->in1_idx][i] - wires[f->in1_idx][i - 1];
                }
                wires[f->out_idx] = output;
                break;
            }

            case Insert: {
                std::vector<Ring> output({wires[f->in1_idx].begin(), wires[f->in1_idx].end()});
                output.insert(output.begin(), 0);
                wires[f->out_idx] = output;
                break;
            }

            case ConstructData: {
                std::vector<Ring> output(size);
                for (size_t i = 0; i < nodes; ++i) {
                    auto sum = wires[f->data_parallel[i]][0];
                    for (size_t j = 1; j < nodes; ++j) {
                        sum += wires[f->data_parallel[i]][j];
                    }
                    output[i] = sum;
                }
                wires[f->out_idx] = output;
                break;
            }
            default:
                break;
        }
    }
}