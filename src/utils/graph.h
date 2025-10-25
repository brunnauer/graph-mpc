#pragma once

#include <emp-tool/emp-tool.h>
#include <omp.h>

#include <cassert>
#include <vector>

#include "../setup/configs.h"
#include "permutation.h"
#include "sharing.h"

class Graph {
   public:
    Graph() = default;

    Graph(std::vector<Ring> &src, std::vector<Ring> &dst, std::vector<Ring> &isV, std::vector<Ring> &data)
        : src(src), dst(dst), isV(isV), data(data), is_shared(false) {
        assert(src.size() == dst.size());
        assert(dst.size() == isV.size());
        assert(isV.size() == data.size());

        size = src.size();

        for (size_t i = 0; i < isV.size(); ++i)
            if (isV[i]) n_vertices++;
    }

    Graph(std::vector<Ring> &src, std::vector<Ring> &dst, std::vector<Ring> &isV, std::vector<Ring> &data, size_t n_vertices)
        : src(src), dst(dst), isV(isV), data(data), is_shared(true), n_vertices(n_vertices) {
        assert(src.size() == dst.size());
        assert(dst.size() == isV.size());
        assert(isV.size() == data.size());

        size = src.size();
    }

    Graph(std::vector<Ring> &src, std::vector<Ring> &dst, std::vector<Ring> &isV, std::vector<Ring> &data, std::vector<std::vector<Ring>> &src_bits,
          std::vector<std::vector<Ring>> &dst_bits, size_t n_vertices)
        : src(src), dst(dst), isV(isV), data(data), is_shared(true), src_bits(src_bits), dst_bits(dst_bits), n_vertices(n_vertices) {
        assert(src.size() == dst.size());
        assert(dst.size() == isV.size());
        assert(isV.size() == data.size());
        assert(src_bits.size() == dst_bits.size());

        size = src.size();
    }

    std::vector<Ring> src;
    std::vector<Ring> dst;
    std::vector<Ring> isV;
    std::vector<Ring> data;
    bool is_shared;

    /* Specifically for Message-Passing */
    std::vector<std::vector<Ring>> src_bits;
    std::vector<std::vector<Ring>> dst_bits;

    std::vector<Ring> isV_inv;
    size_t size = 0;
    size_t n_vertices = 0;

    std::vector<std::vector<Ring>> data_parallel;

    void add_list_entry(Ring _src, Ring _dst, Ring _isV, Ring _data) {
        src.push_back(_src);
        dst.push_back(_dst);
        isV.push_back(_isV);
        data.push_back(_data);
        size++;

        if (_isV) n_vertices++;
    }

    void add_list_entry(Ring src, Ring dst, Ring isV) { add_list_entry(src, dst, isV, 0); }

    void print() {
        for (size_t i = 0; i < size; ++i) {
            std::cout << src[i] << " " << dst[i] << " " << isV[i] << " " << data[i] << std::endl;
        }
        std::cout << std::endl;
    }

    static Graph parse(const std::string &filename) {
        Graph g;

        std::ifstream file(filename);
        if (!file) {
            throw std::invalid_argument("Error: Could not open graph file " + filename);
        }

        Ring src, dst, isV, data;
        while (file >> src >> dst >> isV >> data) {
            g.add_list_entry(src, dst, isV, data);
        }

        if (!file.eof()) {
            std::cerr << "Warning: Format of graph file may be incorrect." << std::endl;
        }

        return g;
    }

    static std::vector<std::vector<Ring>> to_bits(std::vector<Ring> &input_vec, size_t n_bits) {
        size_t n = input_vec.size();
        std::vector<std::vector<Ring>> bits(n_bits);

#pragma omp parallel for if (n > 10000)
        for (size_t i = 0; i < n_bits; ++i) {
            bits[i].resize(n);
            for (size_t j = 0; j < n; ++j) {
                bits[i][j] = (input_vec[j] & (1 << i)) >> i;
            }
        }
        return bits;
    }

    std::tuple<Graph, Graph> secret_share(emp::PRG &rng, size_t n_bits) {
        std::vector<Ring> src_0, src_1;
        std::vector<Ring> dst_0, dst_1;
        std::vector<Ring> isV_0, isV_1;
        std::vector<Ring> data_0, data_1;

        /* Secret share src */
        for (size_t i = 0; i < size; ++i) {
            Ring r;
            rng.random_data(&r, sizeof(Ring));
            src_0.push_back(r);
            src_1.push_back(src[i] - r);
        }
        for (size_t i = 0; i < size; ++i) {
            Ring r;
            rng.random_data(&r, sizeof(Ring));
            dst_0.push_back(r);
            dst_1.push_back(dst[i] - r);
        }
        for (size_t i = 0; i < size; ++i) {
            Ring r;
            rng.random_data(&r, sizeof(Ring));
            isV_0.push_back(r);
            isV_1.push_back(isV[i] - r);
        }
        for (size_t i = 0; i < size; ++i) {
            Ring r;
            rng.random_data(&r, sizeof(Ring));
            data_0.push_back(r);
            data_1.push_back(data[i] - r);
        }

        std::vector<std::vector<Ring>> src_bits_0, src_bits_1, dst_bits_0, dst_bits_1;
        auto src_bits = to_bits(src, n_bits);
        auto dst_bits = to_bits(dst, n_bits);

        src_bits_0.resize(n_bits);
        src_bits_1.resize(n_bits);
        dst_bits_0.resize(n_bits);
        dst_bits_1.resize(n_bits);

        for (size_t i = 0; i < n_bits; ++i) {
            for (size_t j = 0; j < size; ++j) {
                Ring r;
                rng.random_data(&r, sizeof(Ring));
                src_bits_0[i].push_back(r);
                src_bits_1[i].push_back(src_bits[i][j] - r);
            }
        }

        for (size_t i = 0; i < n_bits; ++i) {
            for (size_t j = 0; j < size; ++j) {
                Ring r;
                rng.random_data(&r, sizeof(Ring));
                dst_bits_0[i].push_back(r);
                dst_bits_1[i].push_back(dst_bits[i][j] - r);
            }
        }

        return {{src_0, dst_0, isV_0, data_0, src_bits_0, dst_bits_0, n_vertices}, {src_1, dst_1, isV_1, data_1, src_bits_1, dst_bits_1, n_vertices}};
    }

    Graph secret_share_parties(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n_bits, Party sender) {
        if (id == D) {
            return {};
        }

        std::vector<Ring> _src;
        std::vector<Ring> _dst;
        std::vector<Ring> _isV;
        std::vector<Ring> _data;
        size_t _size;
        size_t _n_vertices;

        std::vector<std::vector<Ring>> _src_bits(n_bits);
        std::vector<std::vector<Ring>> _dst_bits(n_bits);

        Party partner = (id == P0) ? P1 : P0;
        if (id == sender) {
            _src = src;
            _dst = dst;
            _isV = isV;
            _data = data;
            _size = size;
            _n_vertices = n_vertices;
            network->send(partner, &_size, sizeof(size_t));
            network->send(partner, &_n_vertices, sizeof(size_t));
            _src_bits = to_bits(_src, n_bits);
            _dst_bits = to_bits(_dst, n_bits);
        } else {
            network->recv(partner, &_size, sizeof(size_t));
            network->recv(partner, &_n_vertices, sizeof(size_t));

            _src.resize(_size);
            _dst.resize(_size);
            _isV.resize(_size);
            _data.resize(_size);

            for (size_t i = 0; i < n_bits; ++i) {
                _src_bits[i].resize(_size);
                _dst_bits[i].resize(_size);
            }
        }

        std::vector<std::vector<Ring>> src_bits_shared(n_bits);
        std::vector<std::vector<Ring>> dst_bits_shared(n_bits);
        std::vector<Ring> src_shared(_size);
        std::vector<Ring> dst_shared(_size);
        std::vector<Ring> isV_shared(_size);
        std::vector<Ring> data_shared(_size);

        for (size_t i = 0; i < n_bits; ++i) {
            src_bits_shared[i].resize(_size);
            dst_bits_shared[i].resize(_size);
        }

        for (size_t i = 0; i < n_bits; ++i) {
            src_bits_shared[i] = share::random_share_secret_vec_2P(id, rngs, _src_bits[i], sender);
            dst_bits_shared[i] = share::random_share_secret_vec_2P(id, rngs, _dst_bits[i], sender);
        }

        src_shared = share::random_share_secret_vec_2P(id, rngs, _src, sender);
        dst_shared = share::random_share_secret_vec_2P(id, rngs, _dst, sender);
        isV_shared = share::random_share_secret_vec_2P(id, rngs, _isV, sender);
        data_shared = share::random_share_secret_vec_2P(id, rngs, _data, sender);

        return {src_shared, dst_shared, isV_shared, data_shared, src_bits_shared, dst_bits_shared, _n_vertices};
    }

    Graph share_subgraphs(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n_bits) {
        if (id == P0) {
            auto g1 = secret_share_parties(id, rngs, network, n_bits, id);
            auto g2 = secret_share_parties(id, rngs, network, n_bits, P1);

            auto concat = g1 + g2;
            return concat;
        }
        if (id == P1) {
            auto g1 = secret_share_parties(id, rngs, network, n_bits, P0);
            auto g2 = secret_share_parties(id, rngs, network, n_bits, id);
            auto concat = g1 + g2;
            return concat;
        }

        return {};
    }

    Graph reveal(Party id, std::shared_ptr<io::NetIOMP> network) {
        if (id == D) return {};

        std::vector<Ring> src_revealed = share::reveal_vec(id, network, src);
        std::vector<Ring> dst_revealed = share::reveal_vec(id, network, dst);
        std::vector<Ring> isV_revealed = share::reveal_vec(id, network, isV);
        std::vector<Ring> data_revealed = share::reveal_vec(id, network, data);

        Graph revealed(src_revealed, dst_revealed, isV_revealed, data_revealed);
        revealed.n_vertices = n_vertices;

        return revealed;
    }

    Graph operator+(const Graph &other) const {
        if (is_shared ^ other.is_shared) throw new std::logic_error("Cannot concatenate a shared graph with a non-shared graph.");

        Graph g;
        if (other.size <= 0) return g;
        if (size <= 0) return other;
        g.is_shared = is_shared;

        size_t n_bits = src_bits.size();
        assert(other.src_bits.size() == n_bits);
        assert(other.dst_bits.size() == n_bits);

        g.src_bits.resize(n_bits);
        g.dst_bits.resize(n_bits);

        g.size = size + other.size;
        g.n_vertices = n_vertices + other.n_vertices;

        /* Add values of this graph */
        for (size_t i = 0; i < n_bits; ++i) {
            for (size_t j = 0; j < size; ++j) {
                g.src_bits[i].push_back(src_bits[i][j]);
                g.dst_bits[i].push_back(dst_bits[i][j]);
            }
        }
        /* Add values of other graph*/
        for (size_t i = 0; i < n_bits; ++i) {
            for (size_t j = 0; j < other.size; ++j) {
                g.src_bits[i].push_back(other.src_bits[i][j]);
                g.dst_bits[i].push_back(other.dst_bits[i][j]);
            }
        }

        for (size_t i = 0; i < size; ++i) {
            g.src.push_back(src[i]);
            g.dst.push_back(dst[i]);
            g.isV.push_back(isV[i]);
            g.data.push_back(data[i]);
        }

        for (size_t i = 0; i < other.size; ++i) {
            g.src.push_back(other.src[i]);
            g.dst.push_back(other.dst[i]);
            g.isV.push_back(other.isV[i]);
            g.data.push_back(other.data[i]);
        }

        return g;
    }

    std::vector<Ring> serialize(size_t n_bits = 32) {
        std::vector<Ring> entries(4 * size);
        for (size_t i = 0; i < size; ++i) {
            entries[i * 4 + 0] = src[i];
            entries[i * 4 + 1] = dst[i];
            entries[i * 4 + 2] = isV[i];
            entries[i * 4 + 3] = data[i];
        }

        for (size_t i = 0; i < n_bits; ++i) {
            for (size_t j = 0; j < size; ++j) {
                entries.push_back(src_bits[i][j]);
            }
        }
        for (size_t i = 0; i < n_bits; ++i) {
            for (size_t j = 0; j < size; ++j) {
                entries.push_back(dst_bits[i][j]);
            }
        }
        return entries;
    }

    void init_mp(Party id) {
        /* Generate vector containing { 1-isV[0], 1-isV[1], ... 1-isV[n-1]} */
        isV_inv.resize(isV.size());
        for (size_t i = 0; i < isV_inv.size(); ++i) {
            isV_inv[i] = -isV[i];
            if (id == P0) isV_inv[i] += 1;
        }

        /* Generate vector containing { 1-isV, src_bits[0], src_bits[1], ..., src_bits[n_bits - 1] } */
        src_bits.insert(src_bits.begin(), isV_inv);

        /* Generate vector containing { isV, dst_bits[0], dst_bits[1], ..., dst_bits[n_bits - 1] } */
        dst_bits.insert(dst_bits.begin(), isV);
    }

    static Graph benchmark_graph(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> network) {
        Graph g;
        if (conf.id == P0) {
            for (size_t i = 0; i < conf.nodes / 2; i++) g.add_list_entry(i + 1, i + 1, 1);
            for (size_t i = 0; i < (conf.size - conf.nodes) / 2; i++) g.add_list_entry(1, 2, 0);
        }
        if (conf.id == P1) {
            for (size_t i = conf.nodes / 2; i < conf.nodes; i++) g.add_list_entry(i + 1, i + 1, 1);
            for (size_t i = (conf.size - conf.nodes) / 2; i < conf.size - conf.nodes; i++) g.add_list_entry(1, 2, 0);
        }
        auto g_P0 = g.secret_share_parties(conf.id, conf.rngs, network, conf.bits, P0);
        auto g_P1 = g.secret_share_parties(conf.id, conf.rngs, network, conf.bits, P1);
        auto g_shared = g_P0 + g_P1;
        g_shared.init_mp(conf.id);
        return g_shared;
    }

    static Graph benchmark_graph_PiR(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> network) {
        Graph g;
        if (conf.id == P0) {
            for (size_t i = 0; i < conf.nodes / 2; i++) g.add_list_entry(i + 1, i + 1, 1);
            for (size_t i = 0; i < (conf.size - conf.nodes) / 2; i++) g.add_list_entry(1, 2, 0);
        }
        if (conf.id == P1) {
            for (size_t i = conf.nodes / 2; i < conf.nodes; i++) g.add_list_entry(i + 1, i + 1, 1);
            for (size_t i = (conf.size - conf.nodes) / 2; i < conf.size - conf.nodes; i++) g.add_list_entry(1, 2, 0);
        }

        auto g_shared = g.share_subgraphs(conf.id, conf.rngs, network, conf.bits);
        g_shared.init_mp(conf.id);

        for (size_t i = 0; i < conf.nodes; ++i) {
            std::vector<Ring> data(conf.size);
            data[i] = 1;
            auto data_shared = share::random_share_secret_vec_2P(conf.id, conf.rngs, data);
            g_shared.data_parallel.push_back(data_shared);
        }

        return g_shared;
    }
};