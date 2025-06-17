#pragma once

#include <omp.h>

#include <algorithm>
#include <functional>
#include <tuple>

#include "../setup/utils.h"
#include "../utils/graph.h"
#include "../utils/protocol_config.h"
#include "clip.h"
#include "sorting.h"

struct MPPreprocessing_Dealer {
    SortPreprocessing_Dealer src_order_pre;
    SortPreprocessing_Dealer dst_order_pre;
    SortIterationPreprocessing_Dealer vtx_order_pre;
    std::tuple<std::vector<Ring>, std::vector<Ring>> apply_perm_pre;
    std::vector<SwitchPermPreprocessing_Dealer> sw_perm_pre;
    std::vector<Ring> clip_vals_to_P1;
    std::vector<Ring> B2A_vals_to_P1;

    std::tuple<std::vector<Ring>, std::vector<Ring>> to_vals() {
        std::vector<Ring> vals_P0;
        std::vector<Ring> vals_P1;

        auto [src_order_vals_P0, src_order_vals_P1] = src_order_pre.to_vals();
        auto [dst_order_vals_P0, dst_order_vals_P1] = dst_order_pre.to_vals();
        auto [vtx_order_vals_P0, vtx_order_vals_P1] = vtx_order_pre.to_vals();
        auto [apply_B0, apply_B1] = apply_perm_pre;

        vals_P0.insert(vals_P0.end(), src_order_vals_P0.begin(), src_order_vals_P0.end());
        vals_P0.insert(vals_P0.end(), dst_order_vals_P0.begin(), dst_order_vals_P0.end());
        vals_P0.insert(vals_P0.end(), vtx_order_vals_P0.begin(), vtx_order_vals_P0.end());
        vals_P0.insert(vals_P0.end(), apply_B0.begin(), apply_B0.end());
        for (auto &pre : sw_perm_pre) {
            vals_P0.insert(vals_P0.end(), pre.pi_B_0.begin(), pre.pi_B_0.end());
            vals_P0.insert(vals_P0.end(), pre.omega_B_0.begin(), pre.omega_B_0.end());
            vals_P0.insert(vals_P0.end(), pre.merged_B_0.begin(), pre.merged_B_0.end());
            vals_P0.insert(vals_P0.end(), pre.sigma_0_p.begin(), pre.sigma_0_p.end());
        }

        vals_P1.insert(vals_P1.end(), src_order_vals_P1.begin(), src_order_vals_P1.end());
        vals_P1.insert(vals_P1.end(), dst_order_vals_P1.begin(), dst_order_vals_P1.end());
        vals_P1.insert(vals_P1.end(), vtx_order_vals_P1.begin(), vtx_order_vals_P1.end());
        vals_P1.insert(vals_P1.end(), apply_B1.begin(), apply_B1.end());
        for (auto &pre : sw_perm_pre) {
            vals_P1.insert(vals_P1.end(), pre.pi_shares_P1.begin(), pre.pi_shares_P1.end());
            vals_P1.insert(vals_P1.end(), pre.omega_shares_P1.begin(), pre.omega_shares_P1.end());
            vals_P1.insert(vals_P1.end(), pre.merged_B_1.begin(), pre.merged_B_1.end());
            vals_P1.insert(vals_P1.end(), pre.sigma_1.begin(), pre.sigma_1.end());
        }

        for (auto &val : clip_vals_to_P1) vals_P1.push_back(val);

        return {vals_P0, vals_P1};
    }
};

struct MPPreprocessing {
    SortPreprocessing src_order_pre;                        // size: (3 + (5 resp. 6) * (n_bits-1)) * n
    SortPreprocessing dst_order_pre;                        // size: (3 + (5 resp. 6) * (n_bits-1)) * n
    SortIterationPreprocessing vtx_order_pre;               // size: 5n resp. 6n
    ShufflePre apply_perm_pre;                              // size: n resp. 2n
    std::vector<SwitchPermPreprocessing> sw_perm_pre;       // size: 5n resp. 7n
    std::vector<std::tuple<Ring, Ring, Ring>> eqz_triples;  // size: 3n
    std::vector<std::tuple<Ring, Ring, Ring>> B2A_triples;
};

namespace mp {

using F_apply = std::function<std::vector<Ring>(std::vector<Ring> &, std::vector<Ring> &)>;

std::vector<Ring> propagate_1(std::vector<Ring> &input_vector, size_t n_vertices);

std::vector<Ring> propagate_2(std::vector<Ring> &input_vector, std::vector<Ring> &correction_vector);

std::vector<Ring> gather_1(std::vector<Ring> &input_vector);

std::vector<Ring> gather_2(std::vector<Ring> &input_vector, size_t n_vertices);

std::tuple<std::vector<std::vector<Ring>>, std::vector<std::vector<Ring>>, std::vector<Ring>> init(Party id, SecretSharedGraph &g);

MPPreprocessing_Dealer preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n, size_t n_iterations);

MPPreprocessing preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, size_t n_iterations, std::vector<Ring> &vals, size_t &idx);

MPPreprocessing preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, size_t n_iterations);

void evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, SecretSharedGraph &g, size_t n_iterations,
              size_t n_vertices, MPPreprocessing &preproc, F_apply f_apply);

void run(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, SecretSharedGraph &g, size_t n_iterations,
         size_t n_vertices, F_apply f_apply);

}  // namespace mp