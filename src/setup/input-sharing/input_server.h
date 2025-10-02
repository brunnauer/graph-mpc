#pragma once

#include <socket/TCPSSLServer.h>

#include <cassert>
#include <future>

#include "../../utils/graph.h"

class InputServer {
   public:
    struct Packet {
        size_t start;
        size_t end;
        std::vector<Ring> entries;
    };

    InputServer(Party id, std::string passwords_file, std::string port, size_t n_clients, size_t n_bits)
        : id(id), passwords_file(passwords_file), n_clients(n_clients), n_bits(n_bits), clients(n_clients) {
        auto PRINT_LOG = [](const std::string &strLogMsg) { std::cout << strLogMsg << std::endl; };
        m_pSSLTCPServer.reset(new CTCPSSLServer(PRINT_LOG, port));  // creates an SSL/TLS TCP server

        std::string SSL_CERT_FILE = "certs/socket_cert.pem";
        std::string SSL_KEY_FILE = "certs/socket_key.pem";

        m_pSSLTCPServer->SetSSLCertFile(SSL_CERT_FILE);
        m_pSSLTCPServer->SetSSLKeyFile(SSL_KEY_FILE);
    };

    ~InputServer() = default;

    void connect_clients() {
        bool success = true;
        do {
            for (auto &client : clients) {
                bool connected = m_pSSLTCPServer->Listen(client);
                success = success && connected;
            }
        } while (!success);
        std::cout << "Clients connected." << std::endl;
    }

    Graph recv_graph() {
        /* Receive passwords */
        size_t password_size;
        std::string password;
        for (auto &client : clients) {
            m_pSSLTCPServer->Receive(client, reinterpret_cast<char *>(&password_size), sizeof(size_t));
            std::cout << "Received password size: " << password_size << std::endl;

            password.resize(password_size);
            m_pSSLTCPServer->Receive(client, password.data(), password_size);
            std::cout << "Received password: " << password << std::endl;
            if (!check_password(password)) {
                throw std::runtime_error("A client failed to authenticate.");
            }
        }

        /* Receive n_vertices */
        size_t n_vertices_total = 0;
        for (auto &client : clients) {
            size_t n_vertices;
            auto n_bytes_recvd = m_pSSLTCPServer->Receive(client, reinterpret_cast<char *>(&n_vertices), sizeof(size_t));
            n_vertices_total += n_vertices;
        }
        std::cout << "n_vertices received: " << n_vertices_total << std::endl;

        /* Receive contents */
        std::vector<Packet> pkts;
        try {
            pkts = recv_packets();
        } catch (const std::exception &e) {
            std::cout << "Could not receive all packets: " << e.what() << std::endl;
            return {};
        }
        std::cout << "Packets received." << std::endl;

        size_t total_size = 0;
        for (auto &pkt : pkts) {
            size_t pkt_size = pkt.end - pkt.start;
            size_t expected_size = (4 + 2 * n_bits) * pkt_size;
            assert(pkt.start < pkt.end);
            assert(pkt.entries.size() == expected_size);
            total_size += pkt_size;
        }

        std::vector<Ring> src(total_size);
        std::vector<Ring> dst(total_size);
        std::vector<Ring> isV(total_size);
        std::vector<Ring> data(total_size);
        std::vector<std::vector<Ring>> src_bits(n_bits, std::vector<Ring>(total_size));
        std::vector<std::vector<Ring>> dst_bits(n_bits, std::vector<Ring>(total_size));

        for (auto &pkt : pkts) {
            size_t pkt_size = pkt.end - pkt.start;
            size_t pkt_idx = 0;
            size_t idx = pkt.start;

            for (size_t i = pkt.start; i < pkt.start + pkt_size; ++i) {
                src[i] = pkt.entries[pkt_idx++];
                dst[i] = pkt.entries[pkt_idx++];
                isV[i] = pkt.entries[pkt_idx++];
                data[i] = pkt.entries[pkt_idx++];
            }

            for (size_t i = 0; i < n_bits; ++i) {
                for (size_t j = pkt.start; j < pkt.start + pkt_size; ++j) {
                    src_bits[i][j] = pkt.entries[pkt_idx++];
                }
            }

            for (size_t i = 0; i < n_bits; ++i) {
                for (size_t j = pkt.start; j < pkt.start + pkt_size; ++j) {
                    dst_bits[i][j] = pkt.entries[pkt_idx++];
                }
            }
        }

        return {src, dst, isV, data, src_bits, dst_bits, n_vertices_total};
    }

   private:
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
        auto n_bytes_recvd = m_pSSLTCPServer->Receive(client, reinterpret_cast<char *>(header.data()), sizeof(header));

        if (n_bytes_recvd != sizeof(header)) {
            throw std::runtime_error("Failed to receive packet header");
        }

        pkt.start = header[0];
        pkt.end = header[1];

        if (pkt.end < pkt.start) {
            throw std::runtime_error("Invalid packet: end < start");
        }

        size_t n_entries = pkt.end - pkt.start;
        size_t elems_to_recv = 4 * n_entries + 2 * n_entries * n_bits;
        size_t bytes_to_recv = elems_to_recv * sizeof(Ring);
        pkt.entries.resize(elems_to_recv);

        if (n_entries == 0) {
            return pkt;  // nothing else to receive
        }

        n_bytes_recvd = m_pSSLTCPServer->Receive(client, reinterpret_cast<char *>(pkt.entries.data()), bytes_to_recv);

        if (n_bytes_recvd != static_cast<int>(bytes_to_recv)) {
            throw std::runtime_error("Failed to receive all ring elements.");
        }

        return pkt;
    }

    /* Not safe against side-channel attacks */
    bool check_password(std::string &pwd) {
        std::ifstream file(passwords_file);
        if (!file.is_open()) {
            throw std::invalid_argument("Could not open file.");
        }

        std::string password;
        while (std::getline(file, password)) {
            if (password == pwd) return true;
        }
        return false;
    }

    Party id;
    std::string passwords_file;
    size_t n_clients;
    size_t n_bits;
    std::vector<ASecureSocket::SSLSocket> clients;
    std::unique_ptr<CTCPSSLServer> m_pSSLTCPServer;
};
