// MIT License
//
// Copyright (c) 2018 Xiao Wang (wangxiao@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// The following code has been adopted from
// https://github.com/emp-toolkit/emp-agmpc. It has been modified to define the
// class within a namespace and add additional methods (sendRelative,
// recvRelative).

#pragma once

#include <errno.h>   // errno
#include <fcntl.h>   // open
#include <unistd.h>  // write, close, lseek

#include <cassert>
#include <cstdio>
#include <cstring>  // strerror
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "../setup/configs.h"
#include "disk.h"
#include "emp-tool/emp-tool.h"
#include "tls_net_io_channel.h"

namespace io {
using namespace emp;

class NetIOMP {
   public:
    int party;
    int nP, n_total;
    std::mutex mtx;
    std::condition_variable cv;
    bool connection_established;
    size_t BLOCK_SIZE;
    std::vector<size_t> start_idx;
    std::vector<size_t> end_idx;

    NetIOMP(NetworkConfig &conf, bool force_init = false)
        : party(conf.id),
          nP(conf.n_parties),
          n_total(conf.n_parties + conf.n_clients),
          connection_established(false),
          BLOCK_SIZE(conf.BLOCK_SIZE),
          start_idx(conf.n_clients),
          end_idx(conf.n_parties),
          conf(conf),
          ios(conf.n_parties + conf.n_clients),
          ios2(conf.n_parties + conf.n_clients),
          sent(conf.n_parties + conf.n_clients, false) {
        std::cout << "Establishing connection to " << nP << " parties via port " << conf.port << " and ID: " << party << ". " << std::endl;
        if (force_init) {
            init_network();
        } else {
            init_thread = std::thread(&NetIOMP::init_network, this);
        }
    }

    ~NetIOMP() {
        if (init_thread.joinable()) init_thread.join();
    }

    void init_network() {
        for (int i = 0; i < n_total; ++i) {
            for (int j = i + 1; j < n_total; ++j) {
                if (i == party) {
                    usleep(1000);
                    if (conf.localhost) {
                        ios[j] = std::make_unique<TLSNetIO>("127.0.0.1", conf.port + 2 * (i * n_total + j), conf.trusted_cert_path, BLOCK_SIZE, true);
                    } else {
                        ios[j] = std::make_unique<TLSNetIO>(conf.IP[j].c_str(), conf.port + 2 * (i * n_total + j), conf.trusted_cert_path, BLOCK_SIZE, true);
                    }
                    ios[j]->set_nodelay();

                    usleep(1000);
                    if (conf.localhost) {
                        ios2[j] =
                            std::make_unique<TLSNetIO>(conf.port + 2 * (i * n_total + j) + 1, conf.certificate_path, conf.private_key_path, BLOCK_SIZE, true);
                    } else {
                        ios2[j] =
                            std::make_unique<TLSNetIO>(conf.port + 2 * (i * n_total + j) + 1, conf.certificate_path, conf.private_key_path, BLOCK_SIZE, true);
                    }
                    ios2[j]->set_nodelay();
                } else if (j == party) {
                    usleep(1000);
                    if (conf.localhost) {
                        ios[i] = std::make_unique<TLSNetIO>(conf.port + 2 * (i * n_total + j), conf.certificate_path, conf.private_key_path, BLOCK_SIZE, true);
                    } else {
                        ios[i] = std::make_unique<TLSNetIO>(conf.port + 2 * (i * n_total + j), conf.certificate_path, conf.private_key_path, BLOCK_SIZE, true);
                    }
                    ios[i]->set_nodelay();

                    usleep(1000);
                    if (conf.localhost) {
                        ios2[i] = std::make_unique<TLSNetIO>("127.0.0.1", conf.port + 2 * (i * n_total + j) + 1, conf.trusted_cert_path, BLOCK_SIZE, true);
                    } else {
                        ios2[i] =
                            std::make_unique<TLSNetIO>(conf.IP[i].c_str(), conf.port + 2 * (i * n_total + j) + 1, conf.trusted_cert_path, BLOCK_SIZE, true);
                    }
                    ios2[i]->set_nodelay();
                }
            }
        }
        {
            std::lock_guard<std::mutex> lock(mtx);
            connection_established = true;
        }
        cv.notify_all();
    }

    int64_t count() {
        int64_t res = 0;
        for (int i = 0; i < nP; ++i)
            if (i != party) {
                res += ios[i]->counter;
                res += ios2[i]->counter;
            }
        return res;
    }

    void resetStats() {
        for (int i = 0; i < nP; ++i) {
            if (i != party) {
                ios[i]->counter = 0;
                ios2[i]->counter = 0;
            }
        }
    }

    void send(int dst, const void *data, size_t len) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return connection_established; });
        }
        if (dst != party) {
            if (party < dst)
                ios[dst]->send_data(data, len);
            else
                ios2[dst]->send_data(data, len);
            sent[dst] = true;
        }
        flush(dst);
    }

    void send_vec(Party dst, std::vector<Ring> &data, size_t n_elems) {
        size_t n_msgs = n_elems / BLOCK_SIZE;
        size_t last_msg_size = n_elems % BLOCK_SIZE;

        for (size_t i = 0; i < n_msgs; i++) {
            send(dst, data.data() + i * BLOCK_SIZE, sizeof(Ring) * BLOCK_SIZE);
        }
        if (last_msg_size > 0) {
            send(dst, data.data() + n_msgs * BLOCK_SIZE, sizeof(Ring) * last_msg_size);
        }
    }

    void recv(int src, void *data, size_t len) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return connection_established; });
        }
        if (src != party) {
            if (sent[src]) flush(src);
            if (src < party)
                ios[src]->recv_data(data, len);
            else
                ios2[src]->recv_data(data, len);
        }
    }

    void recv_vec(Party src, FileWriter &disk, size_t n_elems) {
        size_t n_msgs = n_elems / BLOCK_SIZE;
        size_t last_msg_size = n_elems % BLOCK_SIZE;

        std::vector<Ring> tmp(BLOCK_SIZE);

        for (size_t i = 0; i < n_msgs; ++i) {
            recv(src, tmp.data(), sizeof(Ring) * BLOCK_SIZE);
            disk.write(tmp.data(), BLOCK_SIZE);
        }

        if (last_msg_size > 0) {
            tmp.resize(last_msg_size);
            recv(src, tmp.data(), sizeof(Ring) * last_msg_size);
            disk.write(tmp.data(), last_msg_size);
        }
    }

    void recv_vec(Party src, std::vector<Ring> &buffer, size_t n_elems) {
        size_t n_msgs = n_elems / BLOCK_SIZE;
        size_t last_msg_size = n_elems % BLOCK_SIZE;

        buffer.resize(n_elems);

        for (size_t i = 0; i < n_msgs; i++) {
            recv(src, buffer.data() + (i * BLOCK_SIZE), sizeof(Ring) * BLOCK_SIZE);
        }

        /* Receive last elements */
        if (last_msg_size > 0) {
            recv(src, buffer.data() + (n_msgs * BLOCK_SIZE), sizeof(Ring) * last_msg_size);
        }
    }

    TLSNetIO *get(size_t idx, bool b = false) {
        if (b)
            return ios[idx].get();
        else
            return ios2[idx].get();
    }

    TLSNetIO *getSendChannel(size_t idx) {
        if ((size_t)party < idx) {
            return ios[idx].get();
        }

        return ios2[idx].get();
    }

    TLSNetIO *getRecvChannel(size_t idx) {
        if (idx < (size_t)party) {
            return ios[idx].get();
        }

        return ios2[idx].get();
    }

    void flush(int idx = -1) {
        if (idx == -1) {
            for (int i = 0; i < nP; ++i) {
                if (i != party) {
                    ios[i]->flush();
                    ios2[i]->flush();
                }
            }
        } else {
            if (party < idx) {
                ios[idx]->flush();
            } else {
                ios2[idx]->flush();
            }
        }
    }

    void sync() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return connection_established; });
        }
        for (int i = 0; i < n_total; ++i) {
            for (int j = 0; j < n_total; ++j) {
                if (i < j) {
                    if (i == party) {
                        ios[j]->sync();
                        ios2[j]->sync();
                    } else if (j == party) {
                        ios[i]->sync();
                        ios2[i]->sync();
                    }
                }
            }
        }
    }

    void send_result(int client_id, std::vector<Ring> &result) {
        size_t start = start_idx[client_id];
        size_t end = end_idx[client_id];
        auto share = std::vector<Ring>({result.begin() + start, result.begin() + end});
        send(client_id + 3, &share, share.size() * sizeof(Ring));
    }

   private:
    NetworkConfig conf;
    std::vector<std::unique_ptr<TLSNetIO>> ios;
    std::vector<std::unique_ptr<TLSNetIO>> ios2;
    std::vector<bool> sent;

    std::thread init_thread;
};
};  // namespace io
