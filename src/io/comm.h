#pragma once

#include <memory>

#include "../utils/types.h"
#include "netmp.h"

static void send_vec(Party dest, std::shared_ptr<io::NetIOMP> network, size_t n_elems, std::vector<Row> &data, const size_t BLOCK_SIZE) {
    /* Send amount of elements to receive */
    network->send(dest, &n_elems, sizeof(size_t));

    /* Send elements of the vector */
    int n_msgs = n_elems / BLOCK_SIZE;
    int last_msg_size = n_elems % BLOCK_SIZE;
    for (int i = 0; i < n_msgs; i++) {
        std::vector<Row> data_send_i;
        data_send_i.resize(BLOCK_SIZE);
        for (int j = 0; j < BLOCK_SIZE; j++) {
            data_send_i[j] = data[i * BLOCK_SIZE + j];
        }
        network->send(dest, data_send_i.data(), sizeof(Row) * BLOCK_SIZE);
    }

    std::vector<Row> data_send_last;
    data_send_last.resize(last_msg_size);
    for (int j = 0; j < last_msg_size; j++) {
        data_send_last[j] = data[n_msgs * BLOCK_SIZE + j];
    }
    network->send(dest, data_send_last.data(), sizeof(Row) * last_msg_size);
}

static void recv_vec(Party src, std::shared_ptr<io::NetIOMP> network, std::vector<Row> &buffer, const size_t BLOCK_SIZE) {
    /* Receive amount of elements */
    size_t n_elems;
    network->recv(src, &n_elems, sizeof(size_t));

    buffer.resize(n_elems);

    /* Receive actual data */
    size_t n_msgs = n_elems / BLOCK_SIZE;
    size_t last_msg_size = n_elems % BLOCK_SIZE;
    for (int i = 0; i < n_msgs; i++) {
        std::vector<Row> data_recv_i(BLOCK_SIZE);
        network->recv(src, data_recv_i.data(), sizeof(Row) * BLOCK_SIZE);
        for (int j = 0; j < BLOCK_SIZE; j++) {
            buffer[i * BLOCK_SIZE + j] = data_recv_i[j];
        }
    }

    /* Receive last elements */
    std::vector<Row> data_recv_last(last_msg_size);
    network->recv(src, data_recv_last.data(), sizeof(Row) * last_msg_size);
    for (int j = 0; j < last_msg_size; j++) {
        buffer[n_msgs * BLOCK_SIZE + j] = data_recv_last[j];
    }
}
