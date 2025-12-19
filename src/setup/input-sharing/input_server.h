#pragma once

#include <cassert>
#include <future>

#include "../../io/netmp.h"
#include "../../utils/graph.h"

class InputServer {
   public:
    struct Packet {
        size_t start;
        size_t end;
        std::vector<Ring> entries;
    };

    InputServer(std::shared_ptr<io::NetIOMP> client_network, size_t n_clients) : client_network(client_network), n_clients(n_clients) {};

    ~InputServer() = default;

    Graph recv_graph() {
        /* Receive n_vertices */
        size_t nodes_total = recv_nodes();
        bits = std::ceil(std::log2(nodes_total + 2));

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
            assert(pkt.start < pkt.end);
            assert(pkt.entries.size() == (4 + 2 * bits) * pkt_size);
            total_size += pkt_size;
        }

        std::vector<Ring> src(total_size);
        std::vector<Ring> dst(total_size);
        std::vector<Ring> isV(total_size);
        std::vector<Ring> data(total_size);
        std::vector<std::vector<Ring>> src_bits(bits, std::vector<Ring>(total_size));
        std::vector<std::vector<Ring>> dst_bits(bits, std::vector<Ring>(total_size));

        for (auto &pkt : pkts) {
            size_t pkt_size = pkt.end - pkt.start;
            size_t pkt_idx = 0;

            for (size_t i = pkt.start; i < pkt.start + pkt_size; ++i) {
                src[i] = pkt.entries[pkt_idx++];
                dst[i] = pkt.entries[pkt_idx++];
                isV[i] = pkt.entries[pkt_idx++];
                data[i] = pkt.entries[pkt_idx++];
            }

            for (size_t i = 0; i < bits; ++i) {
                for (size_t j = pkt.start; j < pkt.start + pkt_size; ++j) {
                    src_bits[i][j] = pkt.entries[pkt_idx++];
                }
            }

            for (size_t i = 0; i < bits; ++i) {
                for (size_t j = pkt.start; j < pkt.start + pkt_size; ++j) {
                    dst_bits[i][j] = pkt.entries[pkt_idx++];
                }
            }
        }

        return {src, dst, isV, data, src_bits, dst_bits, nodes_total};
    }

    size_t recv_nodes() {
        size_t nodes_total = 0;
        for (size_t client = 3; client < n_clients + 3; ++client) {
            size_t nodes;
            client_network->recv(client, &nodes, sizeof(size_t));
            nodes_total += nodes;
        }
        std::cout << "Received nodes: " << nodes_total << std::endl;
        return nodes_total;
    }

    void recv_size(size_t &size_total) {
        size_total = 0;
        for (size_t client = 3; client < n_clients + 3; ++client) {
            size_t size;
            client_network->recv(client, &size, sizeof(size_t));
            size_total += size;
        }
        std::cout << "Received nodes: " << size_total << std::endl;
    }

    void send_result(std::vector<Ring> &data, int client_id) { client_network->send(client_id, &data, sizeof(Ring) * data.size()); }

   private:
    std::vector<Packet> recv_packets() {
        std::vector<Packet> packets;
        for (size_t client = 3; client < n_clients + 3; ++client) {
            auto packet = recv_packet(client);
            packets.push_back(packet);
        }
        return packets;
    }

    Packet recv_packet(size_t &client) {
        Packet pkt;

        std::array<size_t, 2> header{};
        client_network->recv(client, header.data(), sizeof(header));

        pkt.start = header[0];
        pkt.end = header[1];

        if (pkt.end < pkt.start) {
            throw std::runtime_error("Invalid packet: end < start");
        }

        size_t n_entries = pkt.end - pkt.start;
        size_t elems_to_recv = 4 * n_entries + 2 * n_entries * bits;
        size_t bytes_to_recv = elems_to_recv * sizeof(Ring);
        pkt.entries.resize(elems_to_recv);

        if (n_entries == 0) {
            return pkt;  // nothing else to receive
        }

        client_network->recv(client, pkt.entries.data(), bytes_to_recv);

        return pkt;
    }

    std::shared_ptr<io::NetIOMP> client_network;
    size_t bits = 0;
    size_t n_clients;
};
