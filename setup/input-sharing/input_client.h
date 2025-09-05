#pragma once

#include <emp-tool/emp-tool.h>
#include <socket/TCPSSLClient.h>

#include <future>

#include "../../src/utils/graph.h"
#include "../../src/utils/types.h"

class InputClient {
   public:
    struct Packet {
        size_t start;
        size_t end;
        std::vector<Ring> entries;
    };

    InputClient(int id, size_t n_bits, std::string password) : id(id), n_bits(n_bits), password(password), connected(false) {
        auto PRINT_LOG = [](const std::string &strLogMsg) { std::cout << strLogMsg << std::endl; };

        m_pSSLTCPClient.reset(new CTCPSSLClient(PRINT_LOG));
    };

    ~InputClient() { m_pSSLTCPClient->Disconnect(); };

    void connect(std::string ip, int port) {
        m_pSSLTCPClient->Disconnect();
        connected = m_pSSLTCPClient->Connect(ip, std::to_string(port));
        if (!connected) {
            throw std::runtime_error("Could not connect to " + ip + ":" + std::to_string(port));
        }
    }

    void send_graph(Graph &g, size_t start) {
        Packet pkt;
        pkt.start = start;
        pkt.end = start + g.size();
        pkt.entries = g.serialize(n_bits);

        if (connected) {
            size_t password_size = password.size();
            send(password_size);
            send(password);
            auto n_vertices = g.n_vertices();
            send(n_vertices);
            send_packet(pkt);
            std::cout << "Graph sent successfully." << std::endl;
        } else {
            std::cout << "Could not send packet since client is not connected to server." << std::endl;
        }
    }

   private:
    void send(size_t &elem) { m_pSSLTCPClient->Send(reinterpret_cast<const char *>(&elem), sizeof(size_t)); }

    void send(std::string &str) { m_pSSLTCPClient->Send(reinterpret_cast<const char *>(str.data()), str.size()); }

    void send_packet(Packet &pkt) {
        std::array<size_t, 2> header{pkt.start, pkt.end};
        m_pSSLTCPClient->Send(reinterpret_cast<const char *>(header.data()), sizeof(header));

        m_pSSLTCPClient->Send(reinterpret_cast<const char *>(pkt.entries.data()), pkt.entries.size() * sizeof(Ring));
    }

   private:
    int id;
    size_t n_bits;
    bool connected;
    std::string password;
    std::unique_ptr<CTCPSSLClient> m_pSSLTCPClient;
};
