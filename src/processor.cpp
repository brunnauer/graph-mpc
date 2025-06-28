#include "processor.h"

#include "../setup/utils.h"

std::vector<Ring> apply(std::vector<Ring> &old_payload, std::vector<Ring> &new_payload) {
    std::vector<Ring> result(old_payload.size());

    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = old_payload[i] + new_payload[i];
    }
    return result;
}

void pre_mp_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, MPPreprocessing &preproc) {
    return;
}

void post_mp_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, MPPreprocessing &preproc) {
    return;
}

void pre_mp_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, MPPreprocessing &preproc,
                     SecretSharedGraph &g) {
    return;
}

void post_mp_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, SecretSharedGraph &g,
                      MPPreprocessing &preproc, std::vector<Ring> &payload) {
    g.payload = payload;
    g.payload_bits = to_bits(payload, sizeof(Ring) * 8);
}

void Processor::add(FunctionToken function) { queue.push_back(function); }

void Processor::run_mp_preprocessing(size_t n_iterations) {
    pre_completed = false;

    queue.push_back(SORT);
    queue.push_back(SORT);
    queue.push_back(SORT_ITERATION);

    queue.push_back(APPLY_PERM);

    for (size_t i = 0; i < n_iterations; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            queue.push_back(SWITCH_PERM);
        }
    }

    queue.push_back(CLIP);

    run_preprocessing_dealer();
    run_preprocessing_parties();
    pre_completed = true;
}

void Processor::run_mp_evaluation(size_t n_iterations, size_t n_vertices) {
    if (id == D) return;

    if (!pre_completed) throw std::logic_error("Preprocessing has to complete before evaluation.");

    std::vector<Ring> weights(n_iterations);
    size_t n_bits = sizeof(Ring) * 8;
    mp::evaluate(id, rngs, network, g.size, BLOCK_SIZE, n_bits, n_iterations, n_vertices, g, weights, apply, pre_mp_evaluate, post_mp_evaluate, mp_preproc);
}

size_t Processor::preprocessing_data_size(Party id) {
    size_t data_recv_P0 = 0;
    size_t data_recv_P1 = 0;

    size_t sort_idx = 0;

    for (auto &func : queue) {
        switch (func) {
            case SORT: {
                size_t n_bits = sizeof(Ring) * 8 + 1;
                data_recv_P0 += (n_bits - 1) * 2 * n;      // P0 receives n_bits-1 B_0's for shuffle and unshuffle respectively
                data_recv_P1 += (n_bits - 1) * 4 * n + n;  // P1 receives the same as P0, but n_bits triple shares on top
                sort_idx++;
                break;
            }
            case SORT_ITERATION: {
                data_recv_P0 += 2 * n;
                data_recv_P1 += 4 * n;
                break;
            }
            case SWITCH_PERM: {
                data_recv_P0 += 4 * n;  // pi_B0, omega_B0, merged_B0, sigma_0_p
                data_recv_P1 += 6 * n;  // pi_shares_P1 (2n), omega_shares_P1 (2n), merged_B0, sigma_1
                break;
            }
            case APPLY_PERM: {
                data_recv_P0 += n;      // P0 receives B_0
                data_recv_P1 += 2 * n;  // P1 receives B_1 and pi_1_p
                break;
            }
            case CLIP: {
                data_recv_P1 += n + (5 * n);  // P1 receives secret shares for n triples (B2A) and 5 * n triples (eqz);
                break;
            }
            default:
                break;
        }
    }
    if (id == P0) {
        return data_recv_P0;
    } else {
        return data_recv_P1;
    }
}

void Processor::run_preprocessing_parties() {
    if (id == D) return;

    std::vector<Ring> vals;
    size_t idx = 0;
    size_t n_elems = preprocessing_data_size(id);

    recv_vec(D, network, n_elems, vals, BLOCK_SIZE);

    size_t sort_idx = 0;

    for (auto &func : queue) {
        switch (func) {
            case SORT: {
                size_t n_bits = sizeof(Ring) * 8 + 1;
                SortPreprocessing sort_preproc = sort::get_sort_preprocess_Parties(id, rngs, n, n_bits, vals, idx);

                if (sort_idx == 0) {
                    mp_preproc.src_order_pre = sort_preproc;
                } else if (sort_idx == 1) {
                    mp_preproc.dst_order_pre = sort_preproc;
                }

                sort_idx++;
                break;
            }
            case SORT_ITERATION: {
                SortIterationPreprocessing sort_iteration_pre = sort::sort_iteration_preprocess_Parties(id, rngs, n, vals, idx);
                mp_preproc.vtx_order_pre = sort_iteration_pre;
                break;
            }
            case SWITCH_PERM: {
                SwitchPermPreprocessing sw_perm_preproc = permute::switch_perm_preprocess_Parties(id, rngs, n, vals, idx);
                mp_preproc.sw_perm_pre.push_back(sw_perm_preproc);
                break;
            }
            case APPLY_PERM: {
                break;
            }
            case CLIP: {
                auto eqz_triples = clip::equals_zero_preprocess_Parties(id, rngs, n, vals, idx);
                mp_preproc.eqz_triples = eqz_triples;
                mp_preproc.B2A_triples = clip::B2A_preprocess_Parties(id, rngs, n, vals, idx);
                break;
            }
            default:
                break;
        }
    }
}

void Processor::run_preprocessing_dealer() {
    if (id != D) return;

    std::vector<Ring> vals_P0;
    std::vector<Ring> vals_P1;

    size_t sort_idx = 0;

    for (auto &func : queue) {
        switch (func) {
            /*case COMPACTION: {
                auto shares_P1 = compaction::preprocess_Dealer(id, rngs, n);
                for (auto &val : shares_P1) vals_P1.push_back(val);
                break;
            }
            case SHUFFLE: {
                auto [perm_share, B_0, shares_P1] = shuffle::get_shuffle_1(id, rngs, n);
                for (auto &val : B_0) vals_P0.push_back(val);
                for (auto &val : shares_P1) vals_P1.push_back(val);
                last_shuffle = perm_share;
                break;
            }
            case UNSHUFFLE: {
                auto [B_0, B_1] = shuffle::get_unshuffle_1(id, rngs, n, last_shuffle);
                for (auto &val : B_0) vals_P0.push_back(val);
                for (auto &val : B_1) vals_P1.push_back(val);
                break;
            }*/
            case SORT: {
                size_t n_bits = sizeof(Ring) * 8 + 1;

                auto preproc = sort::get_sort_preprocess_Dealer(id, rngs, n, n_bits);
                auto [sort_vals_P0, sort_vals_P1] = preproc.to_vals();
                vals_P0.insert(vals_P0.end(), sort_vals_P0.begin(), sort_vals_P0.end());
                vals_P1.insert(vals_P1.end(), sort_vals_P1.begin(), sort_vals_P1.end());

                sort_idx++;
                break;
            }
            case SORT_ITERATION: {
                auto preproc = sort::sort_iteration_preprocess_Dealer(id, rngs, n);
                auto [sort_it_vals_P0, sort_it_vals_P1] = preproc.to_vals();
                vals_P0.insert(vals_P0.end(), sort_it_vals_P0.begin(), sort_it_vals_P0.end());
                vals_P1.insert(vals_P1.end(), sort_it_vals_P1.begin(), sort_it_vals_P1.end());
                break;
            }
            case SWITCH_PERM: {
                SwitchPermPreprocessing_Dealer preproc = permute::switch_perm_preprocess_Dealer(id, rngs, n);
                auto [sw_perm_vals_P0, sw_perm_vals_P1] = preproc.to_vals();
                vals_P0.insert(vals_P0.end(), sw_perm_vals_P0.begin(), sw_perm_vals_P0.end());
                vals_P1.insert(vals_P1.end(), sw_perm_vals_P1.begin(), sw_perm_vals_P1.end());
                break;
            }
            case APPLY_PERM: {
                auto [perm_share, B_0, shares_P1] = shuffle::get_shuffle_1(id, rngs, n);
                for (auto &val : B_0) vals_P0.push_back(val);
                for (auto &val : shares_P1) vals_P1.push_back(val);
                break;
            }
            case CLIP: {
                std::vector<Ring> vals_to_P1 = clip::equals_zero_preprocess_Dealer(id, rngs, n);
                vals_P1.insert(vals_P1.end(), vals_to_P1.begin(), vals_to_P1.end());

                vals_to_P1 = clip::B2A_preprocess_Dealer(id, rngs, n);
                vals_P1.insert(vals_P1.end(), vals_to_P1.begin(), vals_to_P1.end());
                break;
            }
            default:
                break;
        }
    }

    send_vec(P0, network, vals_P0.size(), vals_P0, BLOCK_SIZE);
    send_vec(P1, network, vals_P1.size(), vals_P1, BLOCK_SIZE);
}