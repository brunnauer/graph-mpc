#pragma once

#include "perm.h"
#include "protocol_config.h"

namespace share {
Row get_random_share(Party pid, RandomGenerators &rngs);

Row get_random_share_secret(Party pid, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, Row &secret);

void random_share_secret_send(Party dst_pid, RandomGenerators &rngs, io::NetIOMP &network, Row &share, Row &secret);

void random_share_secret_recv(Party src_pid, io::NetIOMP &network, Row &share);

void random_share_secret_vec_send(Party dst_pid, RandomGenerators &rngs, io::NetIOMP &network, std::vector<Row> &share_vec, std::vector<Row> &secret_vec);

void random_share_secret_vec_recv(Party src_pid, io::NetIOMP &network, std::vector<Row> &share_vec);

Row reconstruct(Party partner, std::shared_ptr<io::NetIOMP> network, Row &share);

std::vector<Row> reconstruct_vec(Party partner, std::shared_ptr<io::NetIOMP> network, std::vector<Row> &share_vec);

std::vector<Row> reveal(ProtocolConfig &conf, std::vector<Row> &share);

Permutation reveal(ProtocolConfig &conf, Permutation &share);

};  // namespace share