#pragma once

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "../utils/types.h"
#include "comm.h"
#include "netmp.h"

class NetworkInterface {
   public:
    NetworkInterface() = default;
    NetworkInterface(Party id, int n_parties, int port, char *IP[], std::string certificate_path, std::string private_key_path, std::string trusted_cert_path,
                     bool localhost, size_t BLOCK_SIZE);
    ~NetworkInterface();

    std::shared_ptr<io::NetIOMP> network = nullptr;

    void init();
    void sync();
    void add_to_queue(Party dst, const std::vector<Ring> &vec);
    void send_queue(Party dst);
    void send_all();
    void send_now(Party dst, std::vector<Ring> &vec);
    std::vector<Ring> recv(Party src, size_t n_elems);

   private:
    void connection_loop();
    void sender_loop();
    bool try_send_queue(Party dst);
    void save_queue_to_disk(Party dst);
    bool all_sent();

    Party id;
    int n_parties;
    int port;
    char **IP;
    std::string certificate_path;
    std::string private_key_path;
    std::string trusted_cert_path;
    bool localhost;

    size_t BLOCK_SIZE;
    bool connection_established;

    std::mutex mutex;
    std::condition_variable cv;
    std::map<Party, std::vector<Ring>> queues;
    std::map<Party, bool> ready_to_send;
    std::map<Party, bool> in_memory;
    std::thread connection_thread;
    std::thread sender_thread;
    bool stop;
};
