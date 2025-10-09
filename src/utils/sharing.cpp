#include "sharing.h"

std::vector<Ring> share::random_share_secret_vec_2P(Party id, RandomGenerators &rngs, std::vector<Ring> &secret_vec, Party sender) {
    std::vector<Ring> share(secret_vec.size());
    if (id == D)
        return share;
    else if (id == sender) {
        for (size_t i = 0; i < share.size(); ++i) {
            rngs.rng_01().random_data(&share[i], sizeof(Ring));
            share[i] = secret_vec[i] - share[i];
        }
    } else {
        for (size_t i = 0; i < share.size(); ++i) {
            rngs.rng_01().random_data(&share[i], sizeof(Ring));
        }
    }
    return share;
}

std::vector<Ring> share::random_share_secret_vec_2P_bin(Party id, RandomGenerators &rngs, std::vector<Ring> &secret_vec) {
    std::vector<Ring> share(secret_vec.size());
    switch (id) {
        case P0: {
            for (size_t i = 0; i < share.size(); ++i) {
                rngs.rng_01().random_data(&share[i], sizeof(Ring));
                share[i] = secret_vec[i] ^ share[i];
            }
            break;
        }
        case P1: {
            for (size_t i = 0; i < share.size(); ++i) {
                rngs.rng_01().random_data(&share[i], sizeof(Ring));
            }
            break;
        }
        default:
            break;
    }
    return share;
}

std::vector<Ring> share::random_share_vec_3P(Party id, RandomGenerators &rngs, size_t n, bool binary) {
    switch (id) {
        case P0: {
            std::vector<Ring> share(n);
            for (size_t i = 0; i < n; ++i) rngs.rng_D0_comp().random_data(&share[i], sizeof(Ring));
            return share;
        }
        case P1: {
            std::vector<Ring> share(n);
            for (size_t i = 0; i < n; ++i) rngs.rng_D1_comp().random_data(&share[i], sizeof(Ring));
            return share;
        }
        case D: {
            std::vector<Ring> share_1(n);
            std::vector<Ring> share_2(n);
            std::vector<Ring> sum(n);
            for (size_t i = 0; i < n; ++i) rngs.rng_D0_comp().random_data(&share_1[i], sizeof(Ring));
            for (size_t i = 0; i < n; ++i) rngs.rng_D1_comp().random_data(&share_2[i], sizeof(Ring));

            if (binary) {
#pragma omp parallel for if (n > 10000)
                for (size_t i = 0; i < n; ++i) sum[i] = share_1[i] ^ share_2[i];
            } else {
#pragma omp parallel for if (n > 10000)
                for (size_t i = 0; i < n; ++i) sum[i] = share_1[i] + share_2[i];
            }
            return sum;
        }
    }
}

std::vector<Ring> share::reveal_vec(Party id, std::shared_ptr<io::NetIOMP> network, std::vector<Ring> &share) {
    std::vector<Ring> result(share.size());

    switch (id) {
        case P0: {
            std::vector<Ring> share_other(share.size());

            network->send_vec(P1, share.size(), share);
            network->recv_vec(P1, share.size(), share_other);

            for (size_t i = 0; i < result.size(); ++i) {
                result[i] = share[i] + share_other[i];
            }
            return result;
        }
        case P1: {
            std::vector<Ring> share_other(share.size());

            network->recv_vec(P0, share.size(), share_other);
            network->send_vec(P0, share.size(), share);

            for (size_t i = 0; i < result.size(); ++i) {
                result[i] = share[i] + share_other[i];
            }
            return result;
        }
        default:
            return result;
    }
}

std::vector<Ring> share::reveal_vec_bin(Party id, std::shared_ptr<io::NetIOMP> network, std::vector<Ring> &share) {
    std::vector<Ring> result(share.size());
    std::vector<Ring> share_other(share.size());

    switch (id) {
        case P0: {
            network->send_vec(P1, share.size(), share);
            network->recv_vec(P1, share.size(), share_other);
            break;
        }
        case P1: {
            network->recv_vec(P0, share.size(), share_other);
            network->send_vec(P0, share.size(), share);
            break;
        }
        default:
            return result;
    }

    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = share[i] ^ share_other[i];
    }
    return result;
}

Permutation share::reveal_perm(Party id, std::shared_ptr<io::NetIOMP> network, Permutation &share) {
    std::vector<Ring> revealed_perm_vec = reveal_vec(id, network, share.perm_vec);
    return Permutation(revealed_perm_vec);
}
