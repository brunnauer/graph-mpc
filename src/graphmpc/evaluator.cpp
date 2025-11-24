#include "evaluator.h"

std::vector<Ring> Evaluator::read_online(std::vector<Ring> &buffer, size_t n_elems, size_t &idx) {
    if (n_elems > buffer.size()) {
        throw new std::invalid_argument("Trying to read more online elements than available.");
    } else {
        std::vector<Ring> data(n_elems);
        data = {buffer.begin() + idx, buffer.begin() + idx + n_elems};

        idx += n_elems;
        if (idx >= buffer.size()) {
            idx = 0;
        }
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
        mul_vals.clear();
        and_vals.clear();
        shuffle_vals.clear();
        reveal_vals.clear();
    }
}

const std::vector<Ring> Evaluator::result() { return output; }

void Evaluator::online_communication() {
    size_t n_comm = mul_vals.size() + and_vals.size() + shuffle_vals.size() + reveal_vals.size();
    if (n_comm == 0) return;

    std::vector<Ring> data_recv(n_comm);
    std::vector<Ring> data_send;
    data_send.insert(data_send.end(), mul_vals.begin(), mul_vals.end());
    data_send.insert(data_send.end(), and_vals.begin(), and_vals.end());
    data_send.insert(data_send.end(), shuffle_vals.begin(), shuffle_vals.end());
    data_send.insert(data_send.end(), reveal_vals.begin(), reveal_vals.end());

    const size_t BLOCK_SIZE = network->BLOCK_SIZE;
    size_t n_msgs = n_comm / BLOCK_SIZE;
    size_t last_msg = n_comm % BLOCK_SIZE;

    Party partner = id == P0 ? P1 : P0;

    for (size_t i = 0; i < n_msgs; i++) {
        network->send(partner, data_send.data() + i * BLOCK_SIZE, sizeof(Ring) * BLOCK_SIZE);
        network->recv(partner, data_recv.data() + i * BLOCK_SIZE, sizeof(Ring) * BLOCK_SIZE);
    }

    network->send(partner, data_send.data() + n_msgs * BLOCK_SIZE, sizeof(Ring) * last_msg);
    network->recv(partner, data_recv.data() + n_msgs * BLOCK_SIZE, sizeof(Ring) * last_msg);

#pragma omp parallel for if (mul_vals.size() > 10000)
    for (size_t i = 0; i < mul_vals.size(); i++) {
        mul_vals[i] += data_recv[i];
    }
#pragma omp parallel for if (and_vals.size() > 10000)
    for (size_t i = 0; i < and_vals.size(); i++) {
        and_vals[i] ^= data_recv[mul_vals.size() + i];
    }
#pragma omp parallel for if (shuffle_vals.size() > 10000)
    for (size_t i = 0; i < shuffle_vals.size(); i++) {
        shuffle_vals[i] = data_recv[mul_vals.size() + and_vals.size() + i];
    }
#pragma omp parallel for if (reveal_vals.size() > 10000)
    for (size_t i = 0; i < reveal_vals.size(); i++) {
        reveal_vals[i] += data_recv[mul_vals.size() + and_vals.size() + shuffle_vals.size() + i];
    }
    data_recv.clear();
}

void Evaluator::evaluate_send(std::vector<std::shared_ptr<Gate>> &layer) {
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
                    if (data->pi_0_p[f->shuffle_idx].size() == 0) {
                        perm = Permutation::random(size, rngs->rng_D0_send());
                        data->pi_0_p[f->shuffle_idx] = perm;
                    } else {
                        perm = data->pi_0_p[f->shuffle_idx];
                    }
                } else {
                    if (data->pi_1[f->shuffle_idx].size() == 0) {
                        perm = Permutation::random(size, rngs->rng_D1_send());
                        data->pi_1[f->shuffle_idx] = perm;
                    } else {
                        perm = data->pi_1[f->shuffle_idx];
                    }
                }

#pragma omp parallel for if (size > 10000)
                for (size_t j = 0; j < size; ++j) {
                    t[perm[j]] = wires[f->in1_idx][j] + R[j];
                }

                update_wire(f->in1_idx);
                shuffle_vals.insert(shuffle_vals.end(), t.begin(), t.end());
                break;
            }

            case Unshuffle: {
                std::vector<Ring> t(size);
                std::vector<Ring> R(size);

                /* Sampling 1: R_0 / R_1 */
                if (id == P0) {
                    for (size_t i = 0; i < size; ++i) rngs->rng_D0_send().random_data(&R[i], sizeof(Ring));
                } else {
                    for (size_t i = 0; i < size; ++i) rngs->rng_D1_send().random_data(&R[i], sizeof(Ring));
                }

                Permutation perm = id == P0 ? data->pi_0[f->shuffle_idx] : data->pi_1_p[f->shuffle_idx];

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) t[i] = wires[f->in1_idx][perm[i]] + R[perm[i]];
                update_wire(f->in1_idx);

                shuffle_vals.insert(shuffle_vals.end(), t.begin(), t.end());
                break;
            }

            case Compaction: {
                std::vector<Ring> _a, _b, _c;
                data->load_triples(_a, _b, _c, f->mult_idx);

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
                data->load_triples(_a, _b, _c, f->mult_idx, f->size);

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
                update_wires(f->in1_idx, f->in2_idx);
                break;
            }

            case EQZ: {
                std::vector<Ring> _a, _b, _c;
                data->load_triples(_a, _b, _c, f->mult_idx, f->size);

                /* x_0 == -x_1 <=> ~x_0 ^ -x_1 */
                if (id == P0 && f->layer == 0) {
#pragma omp parallel for if (size > 10000)
                    for (size_t i = 0; i < f->size; ++i) {
                        wires[f->in1_idx][i] = ~wires[f->in1_idx][i];
                    }
                }

                if (id == P1 && f->layer == 0) {
#pragma omp parallel for if (size > 10000)
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
                update_wire(f->in1_idx);
                break;
            }

            case Bit2A: {
                std::vector<Ring> _a, _b, _c;
                data->load_triples(_a, _b, _c, f->mult_idx, f->size);

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

void Evaluator::evaluate_recv(std::vector<std::shared_ptr<Gate>> &layer) {
    for (auto &f : layer) {
        switch (f->type) {
            case Input: {
                wires[f->out_idx] = input_to_val[f->out_idx];
                break;
            }
            case Output: {
                output.resize(size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    output[i] = wires[f->in1_idx][i];
                }
                update_wire(f->in1_idx);
                break;
            }
            case MergedShuffle:
            case Shuffle: {
                wires[f->out_idx].resize(size);
                Permutation perm;
                if (id == P0) {
                    if (data->pi_0[f->shuffle_idx].size() == 0) {
                        perm = Permutation::random(size, rngs->rng_D0_recv());
                        data->pi_0[f->shuffle_idx] = perm;
                    } else {
                        perm = data->pi_0[f->shuffle_idx];
                    }
                } else {
                    if (data->pi_1_p[f->shuffle_idx].size() == 0) {
                        perm = Permutation::random(size, rngs->rng_D1_recv());
                        data->pi_1_p[f->shuffle_idx] = perm;
                    } else {
                        perm = data->pi_1_p[f->shuffle_idx];
                    }
                }

                auto t = read_online(shuffle_vals, size, shuffle_idx);  // t
                std::vector<Ring> B = data->read_preproc(size);

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    wires[f->out_idx][perm[i]] = t[i] - B[perm[i]];
                }
                break;
            }

            case Unshuffle: {
                wires[f->out_idx].resize(size);
                Permutation perm;
                perm = id == P0 ? data->pi_0_p[f->shuffle_idx] : data->pi_1[f->shuffle_idx];

                std::vector<Ring> t = read_online(shuffle_vals, size, shuffle_idx);
                std::vector<Ring> unshuffle = data->read_preproc(size);  // Read directly from preprocessing

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    wires[f->out_idx][i] = t[perm[i]] - unshuffle[i];
                }
                break;
            }

            case Compaction: {
                wires[f->out_idx].resize(size);
                std::vector<Ring> _a, _b, _c;
                data->load_triples(_a, _b, _c, f->mult_idx);

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

                auto data_recv = read_online(mul_vals, 2 * size, mul_idx);

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    Ring c = _c[i];

                    auto xa = data_recv[2 * i];
                    auto yb = data_recv[2 * i + 1];

                    wires[f->out_idx][i] = (xa * yb * (id)) - (xa * b) - (yb * a) + c;
                }

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    wires[f->out_idx][i] += s_0[i];
                    if (id == P0) wires[f->out_idx][i]--;
                }
                update_wire(f->in1_idx);
                break;
            }

            case Mul: {
                wires[f->out_idx].resize(f->size);
                std::vector<Ring> _a, _b, _c;
                data->load_triples(_a, _b, _c, f->mult_idx, f->size);

                std::vector<Ring> data_recv;
                if (f->binary) {
                    data_recv = read_online(and_vals, 2 * f->size, and_idx);
                } else {
                    data_recv = read_online(mul_vals, 2 * f->size, mul_idx);
                }

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < f->size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    Ring c = _c[i];

                    auto xa = data_recv[2 * i];
                    auto yb = data_recv[2 * i + 1];

                    if (f->binary)
                        wires[f->out_idx][i] = (xa & yb) * (id) ^ (xa & b) ^ (yb & a) ^ c;
                    else
                        wires[f->out_idx][i] = (xa * yb * (id)) - (xa * b) - (yb * a) + c;
                }
                break;
            }

            case EQZ: {
                wires[f->out_idx].resize(f->size);
                std::vector<Ring> _a, _b, _c;
                data->load_triples(_a, _b, _c, f->mult_idx, f->size);

                std::vector<Ring> data_recv = read_online(and_vals, 2 * f->size, and_idx);

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < f->size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    Ring c = _c[i];

                    auto xa = data_recv[2 * i];
                    auto yb = data_recv[2 * i + 1];

                    wires[f->out_idx][i] = (xa & yb) * (id) ^ (xa & b) ^ (yb & a) ^ c;
                }
                if (f->layer == 4) {
#pragma omp parallel for if (size > 10000)
                    for (size_t i = 0; i < f->size; ++i) {
                        wires[f->out_idx][i] <<= 31;
                        wires[f->out_idx][i] >>= 31;
                    }
                }
                break;
            }

            case Bit2A: {
                wires[f->out_idx].resize(f->size);
                std::vector<Ring> _a, _b, _c;
                data->load_triples(_a, _b, _c, f->mult_idx, f->size);

                std::vector<Ring> data_recv = read_online(mul_vals, 2 * f->size, mul_idx);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < f->size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    Ring c = _c[i];
                    auto xa = data_recv[2 * i];
                    auto yb = data_recv[2 * i + 1];

                    auto mul_result = (xa * yb * (id)) - (xa * b) - (yb * a) + c;
                    wires[f->out_idx][i] = (wires[f->in1_idx][i] & 1) - 2 * mul_result;
                }
                update_wire(f->in1_idx);
                break;
            }

            case Reveal: {
                wires[f->out_idx] = read_online(reveal_vals, size, reveal_idx);
                break;
            }

            case Permute: {
                wires[f->out_idx].resize(size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) wires[f->out_idx][wires[f->in2_idx][i]] = wires[f->in1_idx][i];
                update_wires(f->in1_idx, f->in2_idx);
                break;
            }

            case ReversePermute: {
                wires[f->out_idx].resize(size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    wires[f->out_idx][i] = wires[f->in1_idx][wires[f->in2_idx][i]];
                }
                update_wires(f->in1_idx, f->in2_idx);
                break;
            }

            case AddConst: {
                wires[f->out_idx].resize(size);
#pragma omp parallel for if (nodes > 10000)
                for (size_t j = 0; j < size; ++j) {
                    if (id == P0 && j < nodes) {
                        wires[f->out_idx][j] = wires[f->in1_idx][j] + f->val;
                    } else {
                        wires[f->out_idx][j] = wires[f->in1_idx][j];
                    }
                }
                update_wire(f->in1_idx);
                break;
            }

            case Add: {
                wires[f->out_idx].resize(size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    wires[f->out_idx][i] = wires[f->in1_idx][i] + wires[f->in2_idx][i];
                }
                update_wires(f->in1_idx, f->in2_idx);
                break;
            }

            case Flip: {
                wires[f->out_idx].resize(size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    if (id == P0) {
                        wires[f->out_idx][i] = 1 - wires[f->in1_idx][i];
                    } else if (id == P1) {
                        wires[f->out_idx][i] = -wires[f->in1_idx][i];
                    }
                }
                update_wire(f->in1_idx);
                break;
            }

            case Propagate1: {
                wires[f->out_idx].resize(size);
                for (size_t i = nodes - 1; i > 0; --i) {
                    wires[f->out_idx][i] = wires[f->in1_idx][i] - wires[f->in1_idx][i - 1];
                }
                wires[f->out_idx][0] = wires[f->in1_idx][0];
                for (size_t i = nodes; i < size; ++i) {
                    wires[f->out_idx][i] = wires[f->in1_idx][i];
                }
                update_wire(f->in1_idx);
                break;
            }

            case Propagate2: {
                wires[f->out_idx].resize(size);
                Ring sum = 0;
                for (size_t i = 0; i < size; ++i) {
                    sum += wires[f->in1_idx][i];
                    wires[f->out_idx][i] = sum - wires[f->in2_idx][i];
                }
                update_wires(f->in1_idx, f->in2_idx);
                break;
            }

            case Gather1: {
                wires[f->out_idx].resize(size);
                Ring sum = 0;
                for (size_t i = 0; i < size; ++i) {
                    sum += wires[f->in1_idx][i];
                    wires[f->out_idx][i] = sum;
                }
                update_wire(f->in1_idx);
                break;
            }

            case Gather2: {
                wires[f->out_idx].resize(size);
                Ring sum = 0;
                for (size_t i = 0; i < nodes; ++i) {
                    wires[f->out_idx][i] = wires[f->in1_idx][i] - sum;
                    sum += wires[f->out_idx][i];
                }
#pragma omp parallel for if (size > 10000)
                for (size_t i = nodes; i < size; ++i) {
                    wires[f->out_idx][i] = 0;
                }
                update_wire(f->in1_idx);
                break;
            }

            case Sub: {
                wires[f->out_idx].resize(size - 1);
                for (size_t i = 1; i < size; ++i) {
                    wires[f->out_idx][i - 1] = wires[f->in1_idx][i] - wires[f->in1_idx][i - 1];
                }
                update_wire(f->in1_idx);
                break;
            }

            case Insert: {
                wires[f->out_idx] = wires[f->in1_idx];
                wires[f->out_idx].insert(wires[f->out_idx].begin(), 0);
                update_wire(f->in1_idx);
                break;
            }

            case ConstructData: {
                wires[f->out_idx].resize(size);
                for (size_t i = 0; i < nodes; ++i) {
                    auto sum = wires[f->data_parallel[i]][0];
                    for (size_t j = 1; j < nodes; ++j) {
                        sum += wires[f->data_parallel[i]][j];
                    }
                    wires[f->out_idx][i] = sum;
                }
                break;
            }
            default:
                break;
        }
    }
}