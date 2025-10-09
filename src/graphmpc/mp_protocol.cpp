#include "mp_protocol.h"

void MPProtocol::preprocess() {
    if (id != D) {
        size_t n_recv;
        network->recv(D, &n_recv, sizeof(size_t));
        // std::cout << "Receiving " << n_recv << " preprocessing values." << std::endl;
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
    if (id == D) return;

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
            if (level_idx == 61 && f_idx == 6) {  // propagate-1
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 63 && f_idx == 2) {  // propagate-2
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 64 && f_idx == 2) {  // gather-1
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 65 && f_idx == 2) {  // gather-2
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 65 && f_idx == 4) {  // AddWeights
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                auto data_corr_rev = share::reveal_vec(id, network, w.mp_data_corr);
                std::cout << "";
            }
            if (level_idx == 65 && f_idx == 5) {  // update mp_data -> mp_data_corr
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 65 && f_idx == 6) {  // propagate-1
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 67 && f_idx == 2) {  // propagate-2
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 68 && f_idx == 2) {  // gather-1
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 69 && f_idx == 2) {  // gather-2
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 69 && f_idx == 4) {  // AddWeights
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 69 && f_idx == 6) {  // propagate-1
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 71 && f_idx == 2) {  // propagate-2
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 72 && f_idx == 2) {  // gather-1
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 73 && f_idx == 2) {  // gather-2
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 73 && f_idx == 4) {  // AddWeights
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 73 && f_idx == 6) {  // propagate-1
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 75 && f_idx == 2) {  // propagate-2
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 76 && f_idx == 2) {  // gather-1
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
            if (level_idx == 77 && f_idx == 2) {  // gather-2
                auto data_rev = share::reveal_vec(id, network, w.mp_data);
                std::cout << "";
            }
        }

        if (level_idx == 61 || level_idx == 62 || level_idx == 63 || level_idx == 64 || level_idx == 65) {
            auto data_rev = share::reveal_vec(id, network, g.data);
            auto data_vtx = share::reveal_vec(id, network, w.mp_data);
            std::cout << "";
        }
    }

    f_queue.clear();
}

void MPProtocol::online_communication() {
    if (id == D) return;

    size_t n_send = ctx.mult_vals.size() + ctx.and_vals.size() + ctx.shuffle_vals.size() + ctx.reveal_vals.size();
    size_t n_recv = 0;

    std::vector<Ring> send_vals;
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
}
