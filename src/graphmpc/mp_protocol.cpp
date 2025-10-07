#include "mp_protocol.h"

void MPProtocol::preprocess() {
    if (id != D) {
        size_t n_recv;
        network->recv(D, &n_recv, sizeof(size_t));
        network->recv_vec(D, n_recv, ctx.preproc.at(id));
    }

    for (auto &level : f_queue) {
        for (auto &f : level) {
            if (f) f->preprocess();
        }
    }

    if (id == D) {
        for (auto &party : {P0, P1}) {
            auto &data_send = ctx.preproc.at(party);
            size_t n_send = data_send.size();
            network->send(party, &n_send, sizeof(size_t));
            network->send_vec(party, n_send, data_send);
        }
    }
}

void MPProtocol::evaluate() {
    for (auto &level : f_queue) {
        for (auto &f : level) {
            if (f) f->evaluate_send();
        }

        online_communication();

        for (auto &f : level) {
            if (f) f->evaluate_recv();
        }
    }
}

void MPProtocol::online_communication() {
    if (id == D) return;

    size_t n_send = ctx.mult_vals.size() + ctx.and_vals.size() + ctx.shuffle_vals.size() + ctx.reveal_vals.size();
    size_t n_recv = 0;

    std::vector<Ring> send_vals(n_send);
    send_vals.insert(send_vals.end(), ctx.mult_vals.begin(), ctx.mult_vals.end());
    send_vals.insert(send_vals.end(), ctx.and_vals.begin(), ctx.and_vals.end());
    send_vals.insert(send_vals.end(), ctx.shuffle_vals.begin(), ctx.shuffle_vals.end());
    send_vals.insert(send_vals.end(), ctx.reveal_vals.begin(), ctx.reveal_vals.end());

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

    for (int i = 0; i < ctx.mult_vals.size(); i++) {
        ctx.mult_vals[i] += ctx.data_recv[i];
    }
    for (int i = 0; i < ctx.and_vals.size(); i++) {
        ctx.and_vals[i] ^= ctx.data_recv[ctx.mult_vals.size() + i];
    }
    for (int i = 0; i < ctx.shuffle_vals.size(); i++) {
        ctx.shuffle_vals[i] = ctx.data_recv[ctx.mult_vals.size() + ctx.and_vals.size() + i];
    }
    for (int i = 0; i < ctx.reveal_vals.size(); i++) {
        ctx.reveal_vals[i] += ctx.data_recv[ctx.mult_vals.size() + ctx.and_vals.size() + ctx.shuffle_vals.size() + i];
    }
}
