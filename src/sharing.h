#pragma once

#include "./utils/types.h"
#include "io/netmp.h"
#include "random_generators.h"

class Share {
   public:
    static void random_share_secret_send(Party dst_pid, RandomGenerators &rngs, io::NetIOMP &network, Row &share, Row &secret) {
        Row val_1;
        Row val_2;

        rngs.rng_self().random_data(&val_1, sizeof(Row));
        share = val_1;
        val_2 = secret - val_1;
        network.send(dst_pid, &val_2, sizeof(Row));
    }

    static void random_share_secret_recv(Party src_pid, io::NetIOMP &network, Row &share) { network.recv(src_pid, &share, sizeof(Row)); }

    static void random_share_secret_vec_send(Party dst_pid, RandomGenerators &rngs, io::NetIOMP &network, std::vector<Row> &share_vec,
                                             std::vector<Row> &secret_vec) {
        for (size_t i = 0; i < secret_vec.size(); ++i) {
            random_share_secret_send(dst_pid, rngs, network, share_vec[i], secret_vec[i]);
        }
    }

    static void random_share_secret_vec_recv(Party src_pid, io::NetIOMP &network, std::vector<Row> &share_vec) {
        for (size_t i = 0; i < share_vec.size(); ++i) {
            random_share_secret_recv(src_pid, network, share_vec[i]);
        }
    }

    static Row reconstruct(Party partner, io::NetIOMP &network, Row &share) {
        Row share_other;
        network.send(partner, &share, sizeof(Row));
        network.recv(partner, &share_other, sizeof(Row));
        return share + share_other;
    }

    static std::vector<Row> reconstruct_vec(Party partner, io::NetIOMP &network, std::vector<Row> &share_vec) {
        std::vector<Row> res(share_vec.size());
        for (int i = 0; i < res.size(); ++i) {
            res[i] = reconstruct(partner, network, share_vec[i]);
        }
        return res;
    }
};