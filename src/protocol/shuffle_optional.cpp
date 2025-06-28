
std::vector<Ring> shuffle::shuffle_1(Party id, RandomGenerators &rngs, size_t n, Permutation &input_share, ShufflePre &perm_share) {
    std::vector<Ring> perm_vec = input_share.get_perm_vec();
    return shuffle_1(id, rngs, n, perm_vec, perm_share);
}

std::vector<Ring> shuffle::shuffle_1(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &input_share, ShufflePre &perm_share) {
    std::vector<Ring> shuffled_share(n);
    std::vector<Ring> vec_A(n);

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
    for (size_t j = 0; j < n; ++j) {
        vec_A[j] = input_share[j] + R[j];
    }

    /* Compute perm(input + R) */
    vec_A = perm(vec_A);

    return vec_A;
}

std::vector<Ring> shuffle::shuffle_2(Party id, RandomGenerators &rngs, std::vector<Ring> &vec_A, ShufflePre &perm_share, size_t n) {
    Permutation perm;

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
    }

    vec_A = perm(vec_A);

    for (size_t i = 0; i < n; ++i) {
        vec_A[i] -= perm_share.B[i];
    }

    return vec_A;
}

std::vector<Ring> shuffle::unshuffle_1(Party id, RandomGenerators &rngs, ShufflePre &shuffle_share, std::vector<Ring> &input_share, size_t n) {
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

    for (size_t i = 0; i < n; ++i) vec_t[i] = input_share[i] + R[i];

    Permutation perm = id == P0 ? shuffle_share.pi_0 : shuffle_share.pi_1_p;
    vec_t = perm.inverse()(vec_t);

    return vec_t;
}

std::vector<Ring> shuffle::unshuffle_2(Party id, ShufflePre &shuffle_share, std::vector<Ring> vec_t, std::vector<Ring> &B, size_t n) {
    /* Apply inverse */
    Permutation perm;
    perm = id == P0 ? shuffle_share.pi_0_p : shuffle_share.pi_1;
    vec_t = perm.inverse()(vec_t);

    /* Last step: subtract B_0 / B_1 */
    for (size_t i = 0; i < n; ++i) vec_t[i] -= B[i];
    return vec_t;
}

std::vector<Ring> shuffle::shuffle_1(Party id, RandomGenerators &rngs, size_t n, Permutation &input_share, ShufflePre &perm_share) {
    std::vector<Ring> perm_vec = input_share.get_perm_vec();
    return shuffle_1(id, rngs, n, perm_vec, perm_share);
}

std::vector<Ring> shuffle::shuffle_1(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &input_share, ShufflePre &perm_share) {
    std::vector<Ring> shuffled_share(n);
    std::vector<Ring> vec_A(n);

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
    for (size_t j = 0; j < n; ++j) {
        vec_A[j] = input_share[j] + R[j];
    }

    /* Compute perm(input + R) */
    vec_A = perm(vec_A);

    return vec_A;
}

std::vector<Ring> shuffle::shuffle_2(Party id, RandomGenerators &rngs, std::vector<Ring> &vec_A, ShufflePre &perm_share, size_t n) {
    Permutation perm;

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
    }

    vec_A = perm(vec_A);

    for (size_t i = 0; i < n; ++i) {
        vec_A[i] -= perm_share.B[i];
    }

    return vec_A;
}

std::vector<Ring> shuffle::unshuffle_1(Party id, RandomGenerators &rngs, ShufflePre &shuffle_share, std::vector<Ring> &input_share, size_t n) {
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

    for (size_t i = 0; i < n; ++i) vec_t[i] = input_share[i] + R[i];

    Permutation perm = id == P0 ? shuffle_share.pi_0 : shuffle_share.pi_1_p;
    vec_t = perm.inverse()(vec_t);

    return vec_t;
}

std::vector<Ring> shuffle::unshuffle_2(Party id, ShufflePre &shuffle_share, std::vector<Ring> vec_t, std::vector<Ring> &B, size_t n) {
    /* Apply inverse */
    Permutation perm;
    perm = id == P0 ? shuffle_share.pi_0_p : shuffle_share.pi_1;
    vec_t = perm.inverse()(vec_t);

    /* Last step: subtract B_0 / B_1 */
    for (size_t i = 0; i < n; ++i) vec_t[i] -= B[i];
    return vec_t;
}