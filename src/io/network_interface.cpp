#include "network_interface.h"

NetworkInterface::NetworkInterface(Party id, int n_parties, int port, char *IP[], std::string certificate_path, std::string private_key_path,
                                   std::string trusted_cert_path, bool localhost, size_t BLOCK_SIZE)
    : id(id),
      n_parties(n_parties),
      port(port),
      IP(IP),
      certificate_path(certificate_path),
      private_key_path(private_key_path),
      trusted_cert_path(trusted_cert_path),
      localhost(localhost),
      BLOCK_SIZE(BLOCK_SIZE),
      network(nullptr),
      connection_established(false),
      stop(false) {
    for (int i = 0; i < 3; ++i) {
        Party p = (Party)i;
        if (i != id) {
            queues[p] = std::vector<Ring>();
            ready_to_send[p] = false;
            in_memory[p] = true;
        }
    }
    sender_thread = std::thread(&NetworkInterface::sender_loop, this);
    connection_thread = std::thread(&NetworkInterface::connection_loop, this);
}

NetworkInterface::~NetworkInterface() {
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&]() { return all_sent(); });
    }

    stop = true;
    cv.notify_all();
    if (sender_thread.joinable()) sender_thread.join();
    if (connection_thread.joinable()) connection_thread.join();
}

void NetworkInterface::init() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&]() { return connection_established || stop; });
}

void NetworkInterface::sync() {
    send_all();

    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&]() { return connection_established && all_sent() || stop; });
    network->sync();
}

void NetworkInterface::add_to_queue(Party dst, const std::vector<Ring> &vec) {
    std::lock_guard<std::mutex> lock(mutex);

    queues[dst].insert(queues[dst].end(), vec.begin(), vec.end());

    cv.notify_one();
}

void NetworkInterface::send_queue(Party dst) {
    if (queues[dst].empty()) return;

    std::lock_guard<std::mutex> lock(mutex);

    /* If the connection is not yet established, save the queue to disk */
    if (!connection_established) {
        save_queue_to_disk(dst);
        in_memory[dst] = false;
    }

    /* Mark as ready to send */
    ready_to_send[dst] = true;

    cv.notify_one();
}

void NetworkInterface::send_all() {
    for (auto const &q : queues) {
        send_queue(q.first);
    }
}

void NetworkInterface::send_now(Party dst, std::vector<Ring> &vec) {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&]() { return connection_established && all_sent(); });

    send_vec(dst, network, vec.size(), vec, BLOCK_SIZE);
}

std::vector<Ring> NetworkInterface::recv(Party src, size_t n_elems) {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&]() { return connection_established || stop; });

    std::vector<Ring> data_recv(n_elems);
    recv_vec(src, network, data_recv, BLOCK_SIZE);

    return data_recv;
}

void NetworkInterface::connection_loop() {
    while (!stop && !connection_established) {
        auto net = std::make_shared<io::NetIOMP>((size_t)id, (size_t)n_parties, port, IP, certificate_path, private_key_path, trusted_cert_path, localhost);
        {
            std::lock_guard<std::mutex> lock(mutex);
            network = net;
            connection_established = true;
        }
        cv.notify_all();
        std::cout << "Connection established!\n";
    }
}

void NetworkInterface::sender_loop() {
    while (!stop) {
        /* Waits until connection is established or stop */
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&]() { return (connection_established && !all_sent()) || stop; });

        if (stop) break;

        /* Try to send queues that are marked as ready */
        for (auto const &q : queues) {
            if (ready_to_send[q.first]) {
                if (try_send_queue(q.first)) {
                    ready_to_send[q.first] = false;
                    cv.notify_all();
                } else
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
}

bool NetworkInterface::try_send_queue(Party dst) {
    try {
        if (!in_memory[dst]) {
            std::string filename = "buffer_" + std::to_string(id) + "_to_" + std::to_string(dst) + ".bin";
            std::ifstream ifs(filename, std::ios::binary);
            size_t size;
            ifs.read(reinterpret_cast<char *>(&size), sizeof(size));
            std::vector<Ring> vec(size);
            ifs.read(reinterpret_cast<char *>(vec.data()), sizeof(Ring) * size);

            send_vec(dst, network, size, vec, BLOCK_SIZE);
            std::filesystem::remove(filename);
        } else {
            send_vec(dst, network, queues[dst].size(), const_cast<std::vector<Ring> &>(queues[dst]), BLOCK_SIZE);

            queues[dst].clear();
        }
        if (id == D) std::cout << "Sent vec." << std::endl;
        return true;
    } catch (...) {
        return false;
    }
}

void NetworkInterface::save_queue_to_disk(Party dst) {
    std::string filename = "buffer_" + std::to_string(id) + "_to_" + std::to_string(dst) + ".bin";
    std::ofstream ofs(filename, std::ios::binary | std::ios::app);
    size_t size = queues[dst].size();
    ofs.write(reinterpret_cast<const char *>(&size), sizeof(size));
    ofs.write(reinterpret_cast<const char *>(queues[dst].data()), sizeof(Ring) * size);
    /* Clear queue */
    queues[dst].clear();
}

bool NetworkInterface::all_sent() {
    bool _all_sent = true;
    for (auto const &q : queues) {
        if (!queues[q.first].empty() || ready_to_send[q.first]) {
            _all_sent = false;
        }
    }
    return _all_sent;
}
