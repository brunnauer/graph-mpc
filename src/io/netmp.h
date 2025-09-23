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

#include "../utils/types.h"
#include "disk.h"
#include "emp-tool/emp-tool.h"
#include "tls_net_io_channel.h"

namespace io {
using namespace emp;

struct NetworkConfig {
    Party id;
    int n_parties;
    int port;
    char **IP;
    std::string certificate_path;
    std::string private_key_path;
    std::string trusted_cert_path;
    bool localhost;
    size_t BLOCK_SIZE;
};

class NetIOMP {
   public:
    int party;
    int nP;
    std::mutex mtx;
    std::condition_variable cv;
    bool connection_established;
    bool ssd;
    FileWriter mul_disk;
    FileWriter shuffle_disk;
    FileWriter unshuffle_disk;
    FileWriter repeat_disk;
    FileWriter recv_disk;

    NetIOMP(NetworkConfig &conf, bool ssd = false)
        : conf(conf),
          ios(conf.n_parties),
          ios2(conf.n_parties),
          party(conf.id),
          nP(conf.n_parties),
          sent(conf.n_parties, false),
          BLOCK_SIZE(conf.BLOCK_SIZE),
          connection_established(false),
          ssd(ssd),
          n_send(conf.n_parties, 0),
          send_buffer(conf.n_parties),
          recv_buffer(conf.n_parties),
          tmp_buf(conf.n_parties) {
        /* Allocate recv buffers */
        for (int i = 0; i < nP; ++i) {
            if ((Party)i != party) tmp_buf[i].resize(CHUNK_SIZE);
        }

        if (party == D) {
            FileWriter P0_disk((Party)party, std::to_string(party) + "_" + std::to_string(P0) + ".bin");
            FileWriter P1_disk((Party)party, std::to_string(party) + "_" + std::to_string(P1) + ".bin");
            writers[P0] = P0_disk;
            writers[P1] = P1_disk;
        } else {
            mul_disk = FileWriter((Party)party, "mul_" + std::to_string(party) + ".bin");
            shuffle_disk = FileWriter((Party)party, "shuffle_" + std::to_string(party) + ".bin");
            unshuffle_disk = FileWriter((Party)party, "unshuffle_" + std::to_string(party) + ".bin");
            repeat_disk = FileWriter((Party)party, "repeat_" + std::to_string(party) + ".bin");
            recv_disk = FileWriter((Party)party, "recv_" + std::to_string(party) + ".bin");
        }

        init_thread = std::thread(&NetIOMP::init_network, this);
    }

    ~NetIOMP() {
        if (init_thread.joinable()) init_thread.join();
    }

    void init_network() {
        for (int i = 0; i < nP; ++i) {
            for (int j = i + 1; j < nP; ++j) {
                if (i == party) {
                    usleep(1000);
                    if (conf.localhost) {
                        ios[j] = std::make_unique<TLSNetIO>("127.0.0.1", conf.port + 2 * (i * nP + j), conf.trusted_cert_path, true);
                    } else {
                        ios[j] = std::make_unique<TLSNetIO>(conf.IP[j], conf.port + 2 * (i * nP + j), conf.trusted_cert_path, true);
                    }
                    ios[j]->set_nodelay();

                    usleep(1000);
                    if (conf.localhost) {
                        ios2[j] = std::make_unique<TLSNetIO>(conf.port + 2 * (i * nP + j) + 1, conf.certificate_path, conf.private_key_path, true);
                    } else {
                        ios2[j] = std::make_unique<TLSNetIO>(conf.port + 2 * (i * nP + j) + 1, conf.certificate_path, conf.private_key_path, true);
                    }
                    ios2[j]->set_nodelay();
                } else if (j == party) {
                    usleep(1000);
                    if (conf.localhost) {
                        ios[i] = std::make_unique<TLSNetIO>(conf.port + 2 * (i * nP + j), conf.certificate_path, conf.private_key_path, true);
                    } else {
                        ios[i] = std::make_unique<TLSNetIO>(conf.port + 2 * (i * nP + j), conf.certificate_path, conf.private_key_path, true);
                    }
                    ios[i]->set_nodelay();

                    usleep(1000);
                    if (conf.localhost) {
                        ios2[i] = std::make_unique<TLSNetIO>("127.0.0.1", conf.port + 2 * (i * nP + j) + 1, conf.trusted_cert_path, true);
                    } else {
                        ios2[i] = std::make_unique<TLSNetIO>(conf.IP[i], conf.port + 2 * (i * nP + j) + 1, conf.trusted_cert_path, true);
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

    void add_send(Party dst, Ring &data) {
        auto vec = std::vector<Ring>({data});
        add_send(dst, vec);
    }

    void add_send(Party dst, std::vector<Ring> &data) {
        /* Count the total number of elements */
        n_send[dst] += data.size();

        if (party == D && ssd) {
            writers[dst].write_vec(data);
        } else {
            auto &buf = send_buffer[dst];
            buf.insert(buf.end(), data.begin(), data.end());
        }
    }

    void send_all() {
        for (int i = 0; i < nP; ++i) {
            Party dst = (Party)i;
            size_t n_elems = n_send[(Party)i];
            if (dst != party && n_elems > 0) {
                /* Send number of elements to receive */
                send(dst, &n_elems, sizeof(size_t));

                /* Sending actual data */
                if (party == D && ssd) {
                    size_t remaining = n_elems;
                    while (remaining > 0) {
                        size_t n_send;
                        if ((int)(remaining - CHUNK_SIZE) < 0) {
                            n_send = remaining;
                        } else {
                            n_send = CHUNK_SIZE;
                        }
                        auto vals = writers[dst].read(n_send);
                        send_vec((Party)i, n_send, vals);
                        remaining -= n_send;
                    }
                } else {
                    send_vec((Party)i, send_buffer[i].size(), send_buffer[i]);
                }
                /* Clear n_send */
                n_send[(Party)i] = 0;
            }
        }
    }

    void send(Party dst, const void *data, size_t len) {
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
        // #ifdef __clang__
        flush(dst);
        // #endif
    }

    void send_vec(Party dst, size_t n_elems, std::vector<Ring> &data) {
        size_t n_msgs = n_elems / BLOCK_SIZE;
        size_t last_msg_size = n_elems % BLOCK_SIZE;
        for (size_t i = 0; i < n_msgs; i++) {
            std::vector<Ring> data_send_i;
            data_send_i.resize(BLOCK_SIZE);
            for (size_t j = 0; j < BLOCK_SIZE; j++) {
                data_send_i[j] = data[i * BLOCK_SIZE + j];
            }
            send(dst, data_send_i.data(), sizeof(Ring) * BLOCK_SIZE);
        }

        std::vector<Ring> data_send_last;
        data_send_last.resize(last_msg_size);
        for (size_t j = 0; j < last_msg_size; j++) {
            data_send_last[j] = data[n_msgs * BLOCK_SIZE + j];
        }
        send(dst, data_send_last.data(), sizeof(Ring) * last_msg_size);
    }

    void recv(Party src, void *data, size_t len) {
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

    void recv_buffered(Party src) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return connection_established; });
        }
        size_t n_elems = 0;
        recv(src, &n_elems, sizeof(size_t));
        if (n_elems > 0) {
            // std::cout << "Receiving " << n_elems << " from " << src << std::endl;
            recv_vec(src, n_elems, recv_buffer[src]);
            // std::cout << "Done receiving." << std::endl;
        }
    }

    void recv_vec(Party src, size_t n_elems, std::vector<Ring> &buffer) {
        const size_t BLOCK_SIZE_MIN = std::min(CHUNK_SIZE, BLOCK_SIZE);
        size_t n_msgs = n_elems / BLOCK_SIZE_MIN;
        size_t last_msg_size = n_elems % BLOCK_SIZE_MIN;

        if (!(ssd && src == D)) buffer.resize(n_elems);

        auto &tmp = tmp_buf[src];

        for (size_t i = 0; i < n_msgs; i++) {
            recv(src, tmp.data(), sizeof(Ring) * BLOCK_SIZE_MIN);
            if (ssd && src == D) {
                recv_disk.write_vec({tmp.begin(), tmp.begin() + BLOCK_SIZE_MIN});
            } else {
                std::memcpy(buffer.data() + (i * BLOCK_SIZE_MIN), tmp.data(), sizeof(Ring) * BLOCK_SIZE_MIN);
            }
        }

        /* Receive last elements */
        if (last_msg_size > 0) {
            recv(src, tmp.data(), sizeof(Ring) * last_msg_size);

            if (ssd && src == D) {
                recv_disk.write_vec({tmp.begin(), tmp.begin() + last_msg_size});
            } else {
                std::memcpy(buffer.data() + (n_msgs * BLOCK_SIZE_MIN), tmp.data(), sizeof(Ring) * last_msg_size);
            }
        }
    }

    Ring read_one(Party src) {
        auto result = read(src, 1);
        return result[0];
    }

    std::vector<Ring> read(Party src, size_t n_elems) {
        if (src == party) {
            throw std::logic_error("Cannot receive data from yourself.");
        }

        std::vector<Ring> chunk;

        if (ssd) {  // Read from file
            chunk = recv_disk.read(n_elems);
        } else {  // Read from buffer
            auto &buffer = recv_buffer[src];
            assert(buffer.size() >= n_elems);

            chunk.reserve(n_elems);
            std::move(buffer.begin(), buffer.begin() + n_elems, std::back_inserter(chunk));
            buffer.erase(buffer.begin(), buffer.begin() + n_elems);
        }

        return chunk;
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
        for (int i = 0; i < nP; ++i) {
            for (int j = 0; j < nP; ++j) {
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

   private:
    NetworkConfig conf;
    std::vector<std::unique_ptr<TLSNetIO>> ios;
    std::vector<std::unique_ptr<TLSNetIO>> ios2;
    std::vector<bool> sent;
    size_t BLOCK_SIZE;
    const size_t CHUNK_SIZE = 10000;

    std::thread init_thread;

    std::vector<std::vector<Ring>> send_buffer;
    std::vector<std::vector<Ring>> recv_buffer;
    std::vector<std::vector<Ring>> tmp_buf;
    std::vector<size_t> n_send;

    std::unordered_map<Party, FileWriter> writers;
    // size_t read_offset = 0;  // For reading preprocessing vals from file
};
};  // namespace io
