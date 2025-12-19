#pragma once

#include <emp-tool/emp-tool.h>
#include <socket/TCPSSLClient.h>

#include <future>

#include "../../utils/graph.h"

class InputClient {
   public:
    struct Packet {
        size_t start;
        size_t end;
        std::vector<Ring> entries;
    };

    InputClient(std::shared_ptr<io::NetIOMP> client_network, size_t bits) : client_network(client_network), bits(bits) {};

    ~InputClient() {};

    void send_graph(emp::PRG &rng, Graph &g, size_t start) {
        auto [share_0, share_1] = g.secret_share(rng, bits);
        for (auto dst : {P0, P1}) {
            auto share = dst == P0 ? share_0 : share_1;
            Packet pkt;
            pkt.start = start;
            pkt.end = start + share.size;
            pkt.entries = share.serialize(bits);
            size_t n_vertices = share.nodes;
            send(dst, n_vertices);
            send_packet(dst, pkt);
        }
        /* Dealer needs to know |V| and n */
        send_nodes(D, g.nodes);
        send_size(D, g.size);
        std::cout << "Graph sent successfully." << std::endl;
    }

    void send_nodes(int dst, size_t nodes) { send(dst, nodes); }

    void send_size(int dst, size_t size) { send(dst, size); }

    std::vector<Ring> recv_result(size_t &size) {
        std::vector<Ring> share_0, share_1, result;
        share_0.resize(size);
        client_network->recv(P0, share_0.data(), sizeof(Ring) * size);
        share_1.resize(size);
        client_network->recv(P1, share_1.data(), sizeof(Ring) * size);

        result.resize(size);
        for (size_t i = 0; i < size; ++i) {
            result[i] = share_0[i] + share_1[i];
        }
        return result;
    }

   private:
    void send(int dst, size_t &elem) { client_network->send(dst, &elem, sizeof(size_t)); }

    void send(int dst, std::string &str) { client_network->send(dst, str.data(), str.size()); }

    void send_packet(int dst, Packet &pkt) {
        std::array<size_t, 2> header{pkt.start, pkt.end};
        client_network->send(dst, header.data(), sizeof(header));
        client_network->send(dst, pkt.entries.data(), pkt.entries.size() * sizeof(Ring));
    }

   private:
    std::shared_ptr<io::NetIOMP> client_network;
    size_t bits;
};
