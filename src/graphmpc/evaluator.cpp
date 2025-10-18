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

void Evaluator::run(Circuit *circ, Graph &g) {
    if (id == D) return;

    wires.resize(circ->n_wires);
    set_input(g);
    for (auto &layer : circ->get()) {
        evaluate_send(layer);
        online_communication();
        evaluate_recv(layer);
    }
    g.data = output;
    wires.clear();
}

void Evaluator::set_input(Graph &g) {
    if (id != D) {
        size_t total_input = g.src_order_bits.size() * g.size + g.dst_order_bits.size() * g.size + g.isV_inv.size() + g.data.size();
        input_to_val.resize(total_input);

        size_t idx = 0;
        for (size_t i = 0; i < g.src_order_bits.size(); ++i) {
            for (size_t j = 0; j < g.size; ++j) {
                input_to_val[idx] = g.src_order_bits[i][j];
                idx++;
            }
        }
        for (size_t i = 0; i < g.dst_order_bits.size(); ++i) {
            for (size_t j = 0; j < g.size; ++j) {
                input_to_val[idx] = g.dst_order_bits[i][j];
                idx++;
            }
        }
        for (size_t i = 0; i < g.size; ++i) {
            input_to_val[idx] = g.isV_inv[i];
            idx++;
        }
        for (size_t i = 0; i < g.size; ++i) {
            input_to_val[idx] = g.data[i];
            idx++;
        }
    }
}

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
                auto perm_share = store->load_shuffle(f->shuffle_idx);

                std::vector<Ring> t(size);
                std::vector<Ring> R(size);
                Permutation perm;

                if (!perm_share->has_R) {
                    /* Sampling 1: R_0 / R_1 */
                    for (size_t i = 0; i < size; ++i) {
                        if (id == P0)
                            rngs->rng_D0_send().random_data(&R[i], sizeof(Ring));
                        else if (id == P1)
                            rngs->rng_D1_send().random_data(&R[i], sizeof(Ring));
                    }
                    perm_share->R = R;
                    perm_share->has_R = true;
                } else {
                    R = perm_share->R;
                }

                /* Sampling 2: pi_0_p / pi_1 */
                if (id == P0) {
                    if (perm_share->has_pi_0_p) {
                        perm = perm_share->pi_0_p;
                    } else {
                        perm = Permutation::random(size, rngs->rng_D0_send());
                        perm_share->pi_0_p = perm;
                        perm_share->has_pi_0_p = true;
                    }
                } else {
                    /* Reuse pi_1 if saved earlier */
                    if (perm_share->has_pi_1) {
                        perm = perm_share->pi_1;
                    } else {
                        perm = Permutation::random(size, rngs->rng_D1_send());
                        perm_share->pi_1 = perm;
                        perm_share->has_pi_1 = true;
                    }
                }

                /* Compute input + R */
#pragma omp parallel for if (size > 10000)
                for (size_t j = 0; j < size; ++j) {
                    t[j] = wires[f->in1[j]] + R[j];
                }

                /* Compute perm(input + R) */
                std::vector<Ring> t_permuted(size);
                t_permuted = perm(t);
                t.swap(t_permuted);

                shuffle_vals.insert(shuffle_vals.end(), t.begin(), t.end());
                break;
            }

            case Unshuffle: {
                auto perm_share = store->load_shuffle(f->shuffle_idx);

                std::vector<Ring> vec_t(size);
                std::vector<Ring> R(size);

                /* Sampling 1: R_0 / R_1 */
                if (id == P0) {
                    for (size_t i = 0; i < size; ++i) rngs->rng_D0_send().random_data(&R[i], sizeof(Ring));
                } else {
                    for (size_t i = 0; i < size; ++i) rngs->rng_D1_send().random_data(&R[i], sizeof(Ring));
                }

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) vec_t[i] = wires[f->in1[i]] + R[i];

                Permutation perm = id == P0 ? perm_share->pi_0 : perm_share->pi_1_p;

                std::vector<Ring> vec_t_permuted(size);
                vec_t_permuted = perm.inverse()(vec_t);
                vec_t.swap(vec_t_permuted);

                shuffle_vals.insert(shuffle_vals.end(), vec_t.begin(), vec_t.end());
                break;
            }

            case Compaction: {
                std::vector<Ring> _a, _b, _c;
                store->load_triples(_a, _b, _c, f->triples_idx);

                std::vector<Ring> f_0(size);

                // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
                for (size_t i = 0; i < size; ++i) {
                    f_0[i] = -wires[f->in1[i]];
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
                    s += wires[f->in1[i]];
                    s_1[i] = s - s_0[i];
                }

                std::vector<Ring> data(2 * size);

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];

                    auto xa = wires[f->in1[i]] + a;
                    auto yb = s_1[i] + b;
                    data[2 * i] = xa;
                    data[2 * i + 1] = yb;
                }

                mul_vals.insert(mul_vals.end(), data.begin(), data.end());
                break;
            }

            case Mul: {
                std::vector<Ring> _a, _b, _c;
                store->load_triples(_a, _b, _c, f->triples_idx);

                std::vector<Ring> data_send(2 * size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    Ring xa, yb;
                    if (f->binary) {
                        xa = wires[f->in1[i]] ^ a;
                        yb = wires[f->in2[i]] ^ b;
                    } else {
                        xa = wires[f->in1[i]] + a;
                        yb = wires[f->in2[i]] + b;
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
                store->load_triples(_a, _b, _c, f->triples_idx);

                /* x_0 == -x_1 <=> ~x_0 ^ -x_1 */
                if (id == P0 && f->layer == 0) {
#pragma omp parallel for if (size > 10000)
                    // for (auto &elem : f->in1) elem = ~elem;
                    for (size_t i = 0; i < f->size; ++i) {
                        wires[f->in1[i]] = ~wires[f->in1[i]];
                    }
                }

                if (id == P1 && f->layer == 0) {
#pragma omp parallel for if (size > 10000)
                    // for (auto &elem : f->in1) elem = -elem;
                    for (size_t i = 0; i < f->size; ++i) {
                        wires[f->in1[i]] = -wires[f->in1[i]];
                    }
                }

                std::vector<Ring> shares_left(f->size);
                std::vector<Ring> shares_right(f->size);
                for (size_t i = 0; i < f->size; ++i) {
                    shares_left[i] = wires[f->in1[i]];
                }
                for (size_t i = 0; i < f->size; ++i) {
                    shares_right[i] = wires[f->in1[i]];
                }

                size_t width = 1 << (4 - f->layer);

#pragma omp parallel for if (size > 10000)
                for (auto &elem : shares_left) elem >>= width;

                size_t old = and_vals.size();
                and_vals.resize(old + 2 * size);
                auto send_ptr = and_vals.data() + old;

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
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
                store->load_triples(_a, _b, _c, f->triples_idx);

                size_t old = mul_vals.size();
                mul_vals.resize(old + 2 * size);
                auto send_ptr = mul_vals.data() + old;

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    auto xa = (id == P0 ? 1 : 0) * (wires[f->in1[i]] & 1) + a;
                    auto yb = (id == P0 ? 0 : 1) * (wires[f->in1[i]] & 1) + b;
                    send_ptr[2 * i] = xa;
                    send_ptr[2 * i + 1] = yb;
                }
                break;
            }

            case Reveal: {
                for (size_t i = 0; i < size; ++i) {
                    reveal_vals.push_back(wires[f->in1[i]]);
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
                for (size_t i = 0; i < size; ++i) {
                    wires[f->output[i]] = input_to_val[f->output[i]];
                }
                break;
            }
            case Output: {
                output.resize(size);
                for (size_t i = 0; i < size; ++i) {
                    output[i] = wires[f->in1[i]];
                }
                break;
            }
            case MergedShuffle:
            case Shuffle: {
                auto perm_share = store->load_shuffle(f->shuffle_idx);
                Permutation perm;
                if (id == P0) {
                    if (perm_share->has_pi_0) {
                        perm = perm_share->pi_0;
                    } else {
                        perm = Permutation::random(size, rngs->rng_D0_recv());
                        perm_share->pi_0 = perm;
                        perm_share->has_pi_0 = true;
                    }
                } else {
                    if (perm_share->has_pi_1_p) {
                        perm = perm_share->pi_1_p;
                    } else {
                        perm = Permutation::random(size, rngs->rng_D1_recv());
                        perm_share->pi_1_p = perm;
                        perm_share->has_pi_1_p = true;
                    }
                }
                auto t = read_online(shuffle_vals, size);  // t1
                t = perm(t);

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    wires[f->output[i]] = t[i] - perm_share->B[i];
                }
                break;
            }

            case Unshuffle: {
                auto perm_share = store->load_shuffle(f->shuffle_idx);
                Permutation perm;
                perm = id == P0 ? perm_share->pi_0_p : perm_share->pi_1;
                std::vector<Ring> vec_t = read_online(shuffle_vals, size);

                /* Apply inverse */
                vec_t = perm.inverse()(vec_t);

                /* Last step: subtract B_0 / B_1 */
                std::vector<Ring> unshuffle = store->load_unshuffle();
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) vec_t[i] -= unshuffle[i];

                for (size_t i = 0; i < size; ++i) wires[f->output[i]] = vec_t[i];

                break;
            }

            case Compaction: {
                std::vector<Ring> _a, _b, _c;
                store->load_triples(_a, _b, _c, f->triples_idx);

                std::vector<Ring> f_0(size);
                // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
                for (size_t i = 0; i < size; ++i) {
                    f_0[i] = -wires[f->in1[i]];
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

                std::vector<Ring> output(size);
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
                for (size_t i = 0; i < size; ++i) {
                    wires[f->output[i]] = output[i];
                }
                break;
            }

            case Mul: {
                std::vector<Ring> _a, _b, _c;
                store->load_triples(_a, _b, _c, f->triples_idx);

                std::vector<Ring> data_recv;
                if (f->binary) {
                    data_recv = read_online(and_vals, 2 * size);
                } else {
                    data_recv = read_online(mul_vals, 2 * size);
                }

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    Ring c = _c[i];

                    auto xa = data_recv[2 * i];
                    auto yb = data_recv[2 * i + 1];

                    if (f->binary)
                        wires[f->output[i]] = (xa & yb) * (id) ^ (xa & b) ^ (yb & a) ^ c;
                    else
                        wires[f->output[i]] = (xa * yb * (id)) - (xa * b) - (yb * a) + c;
                }
                break;
            }

            case EQZ: {
                std::vector<Ring> _a, _b, _c;
                store->load_triples(_a, _b, _c, f->triples_idx);

                std::vector<Ring> data_recv = read_online(and_vals, 2 * size);

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    Ring c = _c[i];

                    auto xa = data_recv[2 * i];
                    auto yb = data_recv[2 * i + 1];

                    wires[f->output[i]] = (xa & yb) * (id) ^ (xa & b) ^ (yb & a) ^ c;
                }
                if (f->layer == 4) {
#pragma omp parallel for if (size > 10000)
                    for (size_t i = 0; i < f->size; ++i) {
                        wires[f->output[i]] <<= 31;
                        wires[f->output[i]] >>= 31;
                    }
                }
                break;
            }

            case Bit2A: {
                std::vector<Ring> _a, _b, _c;
                store->load_triples(_a, _b, _c, f->triples_idx);

                std::vector<Ring> data_recv = read_online(mul_vals, 2 * size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    Ring a = _a[i];
                    Ring b = _b[i];
                    Ring c = _c[i];
                    auto xa = data_recv[2 * i];
                    auto yb = data_recv[2 * i + 1];

                    auto mul_result = (xa * yb * (id)) - (xa * b) - (yb * a) + c;
                    wires[f->output[i]] = (wires[f->in1[i]] & 1) - 2 * mul_result;
                }
                break;
            }

            case Reveal: {
                auto data_revealed = read_online(reveal_vals, size);
                for (size_t i = 0; i < size; ++i) {
                    wires[f->output[i]] = data_revealed[i];
                }
                break;
            }

            case Permute: {
                std::vector<Ring> perm(size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    perm[i] = wires[f->in2[i]];
                }

#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    wires[f->output[perm[i]]] = wires[f->in1[i]];
                }
            }

            case ReversePermute: {
                std::vector<Ring> inverse_vec(size);
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    inverse_vec[wires[f->in2[i]]] = i;
                }
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < inverse_vec.size(); ++i) {
                    wires[f->output[inverse_vec[i]]] = wires[f->in1[i]];
                }
                break;
            }

            case AddConst: {
#pragma omp parallel for if (nodes > 10000)
                for (size_t j = 0; j < size; ++j) {
                    if (id == P0 && j < nodes) {
                        wires[f->output[j]] = wires[f->in1[j]] + f->_in2;
                    } else {
                        wires[f->output[j]] = wires[f->in1[j]];
                    }
                }
                break;
            }

            case Add: {
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    wires[f->output[i]] = wires[f->in1[i]] + wires[f->in2[i]];
                }
                break;
            }

            case Flip: {
#pragma omp parallel for if (size > 10000)
                for (size_t i = 0; i < size; ++i) {
                    if (id == P0) {
                        wires[f->output[i]] = 1 - wires[f->in1[i]];
                    } else if (id == P1) {
                        wires[f->output[i]] = -wires[f->in1[i]];
                    }
                }
                break;
            }

            case Propagate1: {
                for (size_t i = nodes - 1; i > 0; --i) {
                    wires[f->output[i]] = wires[f->in1[i]] - wires[f->in1[i - 1]];
                }
                wires[f->output[0]] = wires[f->in1[0]];
                for (size_t i = nodes; i < size; ++i) {
                    wires[f->output[i]] = wires[f->in1[i]];
                }
                break;
            }

            case Propagate2: {
                Ring sum = 0;
                for (size_t i = 0; i < size; ++i) {
                    sum += wires[f->in1[i]];
                    wires[f->output[i]] = sum - wires[f->in2[i]];
                }
                break;
            }

            case Gather1: {
                Ring sum = 0;
                for (size_t i = 0; i < size; ++i) {
                    sum += wires[f->in1[i]];
                    wires[f->output[i]] = sum;
                }
                break;
            }

            case Gather2: {
                Ring sum = 0;

                for (size_t i = 0; i < nodes; ++i) {
                    wires[f->output[i]] = wires[f->in1[i]] - sum;
                    sum += wires[f->output[i]];
                }
#pragma omp parallel for if (size > 10000)
                for (size_t i = nodes; i < size; ++i) {
                    wires[f->output[i]] = 0;
                }
                break;
            }
            default:
                break;
        }
    }
}