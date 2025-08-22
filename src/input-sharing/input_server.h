#pragma once

#include <socket/TCPSSLServer.h>

#include <future>

#include "../src/utils/types.h"

class InputServer {
   public:
    struct Packet {
        size_t start;
        size_t end;
        std::vector<Ring> entries;
    };

    InputServer(Party id, std::string port, size_t n_clients = 1) : id(id), n_clients(n_clients), clients(n_clients) {
        auto PRINT_LOG = [](const std::string &strLogMsg) { std::cout << strLogMsg << std::endl; };
        m_pSSLTCPServer.reset(new CTCPSSLServer(PRINT_LOG, port));  // creates an SSL/TLS TCP server to listen on port 1000

        std::string SSL_CERT_FILE = "certs/socket_cert.pem";
        std::string SSL_KEY_FILE = "certs/socket_key.pem";

        m_pSSLTCPServer->SetSSLCertFile(SSL_CERT_FILE);
        m_pSSLTCPServer->SetSSLKeyFile(SSL_KEY_FILE);
    };

    ~InputServer() {
        for (auto &client : clients) {
            m_pSSLTCPServer->Disconnect(client);
        }
    };

    void connect_clients() {
        for (auto &client : clients) {
            m_pSSLTCPServer->Listen(client);
        }
    }

    Graph recv_graph() {
        auto pkts = recv_packets();

        std::vector<Ring> entries;
        for (auto &pkt : pkts) {
            entries.insert(entries.end() + pkt.start, pkt.entries.begin(), pkt.entries.end());
        }

        Graph g;
        for (size_t i = 0; i < entries.size() / 4; ++i) {
            g.add_list_entry(entries[i * 4], entries[i * 4 + 1], entries[i * 4 + 2], entries[i * 4 + 3]);
        }

        return g;
    }

   private:
    std::vector<char> recv(size_t client_id, size_t n_elems) {
        std::vector<char> recv_buffer(n_elems);
        auto HeaderReceive = [&]() { return m_pSSLTCPServer->Receive(clients[client_id], recv_buffer.data(), n_elems); };
        std::future<int> futHeaderReceive = std::async(std::launch::async, HeaderReceive);
        auto n_bytes_recvd = futHeaderReceive.get();
        return recv_buffer;
    }

    std::vector<Packet> recv_packets() {
        std::vector<Packet> packets;
        for (auto &client : clients) {
            auto packet = recv_packet(client);
            packets.push_back(packet);
        }
        return packets;
    }

    Packet recv_packet(ASecureSocket::SSLSocket &client) {
        Packet pkt;

        std::array<size_t, 2> header{};
        auto HeaderReceive = [&]() { return m_pSSLTCPServer->Receive(client, reinterpret_cast<char *>(header.data()), sizeof(header)); };
        std::future<int> futHeaderReceive = std::async(std::launch::async, HeaderReceive);
        auto n_bytes_recvd = futHeaderReceive.get();

        if (n_bytes_recvd != sizeof(header)) {
            throw std::runtime_error("Failed to receive packet header");
        }

        pkt.start = header[0];
        pkt.end = header[1];

        if (pkt.end < pkt.start) {
            throw std::runtime_error("Invalid packet: end < start");
        }

        size_t n_entries = pkt.end - pkt.start;
        pkt.entries.resize(n_entries);

        if (n_entries == 0) {
            return pkt;  // nothing else to receive
        }

        auto entriesReceive = [&]() { return m_pSSLTCPServer->Receive(client, reinterpret_cast<char *>(pkt.entries.data()), n_entries * sizeof(Ring)); };
        std::future<int> futEntriesReceive = std::async(std::launch::async, entriesReceive);
        n_bytes_recvd = futEntriesReceive.get();

        if (n_bytes_recvd != static_cast<int>(n_entries * sizeof(Ring))) {
            throw std::runtime_error("Failed to receive all Ring entries");
        }

        return pkt;
    }

    Party id;
    size_t n_clients;
    std::vector<ASecureSocket::SSLSocket> clients;
    std::unique_ptr<CTCPSSLServer> m_pSSLTCPServer;
};
