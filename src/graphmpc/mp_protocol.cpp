#include "mp_protocol.h"

void MPProtocol::preprocess() {
    if (id != D) {
        size_t n_recv;
        network->recv(D, &n_recv, sizeof(size_t));
        network->recv_vec(D, n_recv, ctx.preproc.at(id));
    }

    for (size_t level_idx = 0; level_idx < f_queue.size(); ++level_idx) {
        auto &level = f_queue[level_idx];

        for (size_t f_idx = 0; f_idx < level.size(); ++f_idx) {
            auto &f = level[f_idx];
            if (f) f->preprocess();
        }
    }

    if (id == D) {
        for (auto &party : {P0, P1}) {
            auto &data_send = ctx.preproc.at(party);
            size_t n_send = data_send.size();
            // std::cout << "Sending " << n_send << " preprocessing values." << std::endl;
            network->send(party, &n_send, sizeof(size_t));
            network->send_vec(party, n_send, data_send);
        }
    }
}

void MPProtocol::evaluate() {
    if (id != D) {
        for (size_t level_idx = 0; level_idx < f_queue.size(); ++level_idx) {
            auto &level = f_queue[level_idx];

            for (size_t f_idx = 0; f_idx < level.size(); ++f_idx) {
                auto &f = level[f_idx];
                if (f) f->evaluate_send();
            }

            online_communication();

            for (size_t f_idx = 0; f_idx < level.size(); ++f_idx) {
                auto &f = level[f_idx];
                if (f) f->evaluate_recv();
            }
        }
    }
    std::cout << "Communication round: " << comm_ms << " ms" << std::endl;
    std::cout << "Sending/Receiving: " << sr_ms << " ms" << std::endl << std::endl;

    reset();
}

void MPProtocol::online_communication() {
    auto round_begin = std::chrono::high_resolution_clock::now();

    size_t n_send = ctx.mult_vals.size() + ctx.and_vals.size() + ctx.shuffle_vals.size() + ctx.reveal_vals.size();
    size_t n_recv = 0;

    std::vector<Ring> send_vals;
    send_vals.insert(send_vals.end(), ctx.mult_vals.begin(), ctx.mult_vals.end());
    send_vals.insert(send_vals.end(), ctx.and_vals.begin(), ctx.and_vals.end());
    send_vals.insert(send_vals.end(), ctx.shuffle_vals.begin(), ctx.shuffle_vals.end());
    send_vals.insert(send_vals.end(), ctx.reveal_vals.begin(), ctx.reveal_vals.end());

    auto comm_begin = std::chrono::high_resolution_clock::now();
    if (id == P0) {
        network->send(P1, &n_send, sizeof(size_t));
        network->send_vec(P1, n_send, send_vals);
        network->recv(P1, &n_recv, sizeof(size_t));
        ctx.data_recv.resize(n_recv);
        network->recv_vec(P1, n_recv, ctx.data_recv);
    } else {
        network->recv(P0, &n_recv, sizeof(size_t));
        ctx.data_recv.resize(n_recv);
        network->recv_vec(P0, n_recv, ctx.data_recv);
        network->send(P0, &n_send, sizeof(size_t));
        network->send_vec(P0, n_send, send_vals);
    }
    auto comm_end = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < ctx.mult_vals.size(); i++) {
        ctx.mult_vals[i] += ctx.data_recv[i];
    }
    for (size_t i = 0; i < ctx.and_vals.size(); i++) {
        ctx.and_vals[i] ^= ctx.data_recv[ctx.mult_vals.size() + i];
    }
    for (size_t i = 0; i < ctx.shuffle_vals.size(); i++) {
        ctx.shuffle_vals[i] = ctx.data_recv[ctx.mult_vals.size() + ctx.and_vals.size() + i];
    }
    for (size_t i = 0; i < ctx.reveal_vals.size(); i++) {
        ctx.reveal_vals[i] += ctx.data_recv[ctx.mult_vals.size() + ctx.and_vals.size() + ctx.shuffle_vals.size() + i];
    }
    ctx.data_recv.clear();

    auto round_end = std::chrono::high_resolution_clock::now();

    auto time_round = std::chrono::duration_cast<std::chrono::milliseconds>(round_end - round_begin);
    auto time_comm = std::chrono::duration_cast<std::chrono::milliseconds>(comm_end - comm_begin);

    comm_ms += time_round.count();
    sr_ms += time_comm.count();
}
