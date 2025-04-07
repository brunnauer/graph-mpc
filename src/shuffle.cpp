#include "shuffle.h"

Shuffle::Shuffle(size_t _n_rows, size_t _n_rounds, RandomGenerators &_rngs) : rngs(_rngs) {
    n_rows = _n_rows;
    n_rounds = _n_rounds;
    pis_0 = std::vector<std::shared_ptr<Permutation>>();
    pis_1 = std::vector<std::shared_ptr<Permutation>>();
    pis_0_p = std::vector<std::shared_ptr<Permutation>>();
    pis_1_p = std::vector<std::shared_ptr<Permutation>>();
}

PreprocessingResult Shuffle::preprocess() {
    /* Sample R_0 and R_1 */
    std::vector<Row> R_0, R_1;
    for (int i = 0; i < n_rows; ++i) {
        Row rand_0 = rngs.rng_D0()(Row());
        Row rand_1 = rngs.rng_D1()(Row());
        R_0.push_back(rand_0);
        R_1.push_back(rand_1);
    }

    /* Sample pi_0, pi_1 and pi_0_p */
    Permutation pi_0 = Permutation::random(n_rows, rngs.rng_D0());
    Permutation pi_1 = Permutation::random(n_rows, rngs.rng_D1());
    Permutation pi_0_p = Permutation::random(n_rows, rngs.rng_D0());
    Permutation pi_0_p_inv = pi_0_p.inverse();
    Permutation pi_1_p = (pi_0 * pi_1 * pi_0_p_inv);

    pis_0.push_back(std::shared_ptr<Permutation>(&pi_0));
    pis_1.push_back(std::shared_ptr<Permutation>(&pi_1));
    pis_0_p.push_back(std::shared_ptr<Permutation>(&pi_0_p));
    pis_1_p.push_back(std::shared_ptr<Permutation>(&pi_1_p));

    Row R = rngs.rng_D()(Row());

    std::vector<Row> B_0 = (pi_0 * pi_1)(R_1);
    std::vector<Row> B_1 = pi_0_p(R_0);

    for (auto &element : B_0) {
        element -= R;
    }

    for (auto &element : B_1) {
        element += R;
    }

    PreprocessingResult result;
    result.pi_0 = pi_0;
    result.pi_1 = pi_1;
    result.pi_0_p = pi_0_p;
    result.pi_1_p = pi_1_p;
    result.R_0 = R_0;
    result.R_1 = R_1;
    result.B_0 = B_0;
    result.B_1 = B_1;

    return result;
}

std::tuple<std::vector<Row>, std::vector<Row>> Shuffle::shuffle(std::vector<Row> T_0, std::vector<Row> T_1) {
    PreprocessingResult pp_res = preprocess();

    /* 1) P_1 computes A_0 and sends it to P_0 */
    std::vector<Row> T_1_cpy(T_1.size());
    for (int i = 0; i < T_1.size(); ++i) {
        T_1_cpy[i] += pp_res.R_1[i];
    }
    std::vector<Row> A_0 = pp_res.pi_1(T_1_cpy);

    /* 2) P_0 computes A_1 and sends it to P_1 */
    std::vector<Row> T_0_cpy(T_1.size());
    for (int i = 0; i < T_0.size(); ++i) {
        T_0_cpy[i] += pp_res.R_0[i];
    }
    std::vector<Row> A_1 = pp_res.pi_0_p(T_0_cpy);

    /* Local computations */
    /* 1) P_0 computes T_O'_0 and T_O_0*/
    std::vector<Row> T_O_p_0 = pp_res.pi_0(A_0);
    std::vector<Row> T_O_0 = std::vector<Row>(T_O_p_0.size());
    for (int i = 0; i < T_O_0.size(); ++i) {
        T_O_0[i] = T_O_p_0[i] - pp_res.B_0[i];
    }

    /* 2) P_1 computes T_0_p_1 and T_0_1 */
    std::vector<Row> T_O_p_1 = pp_res.pi_0(A_0);
    std::vector<Row> T_O_1 = std::vector<Row>(T_O_p_1.size());
    for (int i = 0; i < T_O_1.size(); ++i) {
        T_O_1[i] = T_O_p_1[i] - pp_res.B_1[i];
    }

    /* 3) P_0 and P_1 locally sample a random permutation pi_2 */
    Permutation pi_2 = Permutation::random(n_rows, rngs.rng_self());
    T_O_0 = pi_2(T_O_0);
    T_O_1 = pi_2(T_O_1);

    return std::tuple(T_O_0, T_O_1);
}
