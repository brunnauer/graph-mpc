#pragma once

#include <omp.h>

#include <cassert>
#include <vector>

#include "../graphmpc/sort.h"
#include "permutation.h"
#include "sharing.h"
#include "types.h"

class Graph {
   public:
    Graph() = default;

    Graph(std::vector<Ring> &src, std::vector<Ring> &dst, std::vector<Ring> &isV, std::vector<Ring> &data)
        : _src(src), _dst(dst), _isV(isV), _data(data), is_shared(false) {
        assert(src.size() == dst.size());
        assert(dst.size() == isV.size());
        assert(isV.size() == data.size());

        _size = src.size();

        for (size_t i = 0; i < isV.size(); ++i)
            if (isV[i]) _n_vertices++;
    }

    Graph(std::vector<Ring> &src, std::vector<Ring> &dst, std::vector<Ring> &isV, std::vector<Ring> &data, std::vector<std::vector<Ring>> &src_bits,
          std::vector<std::vector<Ring>> &dst_bits, size_t n_vertices)
        : _src(src), _dst(dst), _isV(isV), _data(data), _src_bits(src_bits), _dst_bits(dst_bits), _n_vertices(n_vertices), is_shared(true) {
        assert(_src.size() == _dst.size());
        assert(_dst.size() == _isV.size());
        assert(_isV.size() == _data.size());
        assert(_src_bits.size() == _dst_bits.size());

        _size = _src.size();
    }

    std::vector<std::vector<Ring>> src_order_bits;
    std::vector<std::vector<Ring>> dst_order_bits;
    std::vector<Ring> isV_inv;
    std::vector<Ring> _data;

    std::vector<Ring> src() const { return _src; }
    std::vector<Ring> dst() const { return _dst; }
    std::vector<Ring> isV() const { return _isV; }
    size_t size() { return _size; }
    size_t n_vertices() { return _n_vertices; }
    std::vector<std::vector<Ring>> src_bits() { return _src_bits; }
    std::vector<std::vector<Ring>> dst_bits() { return _dst_bits; }

    void add_list_entry(Ring src, Ring dst, Ring isV, Ring data) {
        _src.push_back(src);
        _dst.push_back(dst);
        _isV.push_back(isV);
        _data.push_back(data);
        _size++;

        if (isV) _n_vertices++;
    }

    void add_list_entry(Ring src, Ring dst, Ring isV) { add_list_entry(src, dst, isV, 0); }

    void print() {
        for (size_t i = 0; i < _size; ++i) {
            std::cout << _src[i] << " " << _dst[i] << " " << _isV[i] << " " << _data[i] << std::endl;
        }
        std::cout << std::endl;
    }

    static Graph parse(const std::string &filename) {
        Graph g;

        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Error: Could not open graph file " << filename << std::endl;
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

    Graph secret_share(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n_bits, Party sender) {
        if (id == D) {
            return {};
        }

        std::vector<Ring> src;
        std::vector<Ring> dst;
        std::vector<Ring> isV;
        std::vector<Ring> data;
        size_t size;
        size_t n_vertices;

        std::vector<std::vector<Ring>> src_bits(n_bits);
        std::vector<std::vector<Ring>> dst_bits(n_bits);

        Party partner = (id == P0) ? P1 : P0;
        if (id == sender) {
            src = _src;
            dst = _dst;
            isV = _isV;
            data = _data;
            size = _size;
            n_vertices = _n_vertices;
            network->send(partner, &size, sizeof(size_t));
            network->send(partner, &n_vertices, sizeof(size_t));
            src_bits = to_bits(src, n_bits);
            dst_bits = to_bits(dst, n_bits);
        } else {
            network->recv(partner, &size, sizeof(size_t));
            network->recv(partner, &n_vertices, sizeof(size_t));

            src.resize(size);
            dst.resize(size);
            isV.resize(size);
            data.resize(size);

            for (size_t i = 0; i < n_bits; ++i) {
                src_bits[i].resize(size);
                dst_bits[i].resize(size);
            }
        }

        std::vector<std::vector<Ring>> src_bits_shared(n_bits);
        std::vector<std::vector<Ring>> dst_bits_shared(n_bits);
        std::vector<Ring> src_shared(size);
        std::vector<Ring> dst_shared(size);
        std::vector<Ring> isV_shared(size);
        std::vector<Ring> data_shared(size);

        for (size_t i = 0; i < n_bits; ++i) {
            src_bits_shared[i].resize(size);
            dst_bits_shared[i].resize(size);
        }

        for (size_t i = 0; i < n_bits; ++i) {
            src_bits_shared[i] = share::random_share_secret_vec_2P(id, rngs, src_bits[i], sender);
            dst_bits_shared[i] = share::random_share_secret_vec_2P(id, rngs, dst_bits[i], sender);
        }

        src_shared = share::random_share_secret_vec_2P(id, rngs, src, sender);
        dst_shared = share::random_share_secret_vec_2P(id, rngs, dst, sender);
        isV_shared = share::random_share_secret_vec_2P(id, rngs, isV, sender);
        data_shared = share::random_share_secret_vec_2P(id, rngs, data, sender);

        return {src_shared, dst_shared, isV_shared, data_shared, src_bits_shared, dst_bits_shared, n_vertices};
    }

    Graph share_subgraphs(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n_bits) {
        size_t size_other;

        if (id == P0) {
            auto g1 = secret_share(id, rngs, network, n_bits, id);
            auto g2 = secret_share(id, rngs, network, n_bits, P1);

            auto concat = g1 + g2;
            return concat;
        }
        if (id == P1) {
            auto g1 = secret_share(id, rngs, network, n_bits, P0);
            auto g2 = secret_share(id, rngs, network, n_bits, id);
            auto concat = g1 + g2;
            return concat;
        }

        return {};
    }

    Graph reveal(Party id, std::shared_ptr<io::NetIOMP> network) {
        if (id == D) return {};

        std::vector<Ring> src_revealed = share::reveal_vec(id, network, _src);
        std::vector<Ring> dst_revealed = share::reveal_vec(id, network, _dst);
        std::vector<Ring> isV_revealed = share::reveal_vec(id, network, _isV);
        std::vector<Ring> data_revealed = share::reveal_vec(id, network, _data);

        return {src_revealed, dst_revealed, isV_revealed, data_revealed};
    }

    Graph operator+(const Graph &other) const {
        if (is_shared ^ other.is_shared) throw new std::logic_error("Cannot concatenate a shared graph with a non-shared graph.");

        Graph g;
        if (other._size <= 0) return g;
        if (_size <= 0) return other;
        g.is_shared = is_shared;

        size_t n_bits = _src_bits.size();
        assert(other._src_bits.size() == n_bits);
        assert(other._dst_bits.size() == n_bits);

        g._src_bits.resize(n_bits);
        g._dst_bits.resize(n_bits);

        g._size = _size + other._size;
        g._n_vertices = _n_vertices + other._n_vertices;

        /* Add values of this graph */
        for (size_t i = 0; i < n_bits; ++i) {
            for (size_t j = 0; j < _size; ++j) {
                g._src_bits[i].push_back(_src_bits[i][j]);
                g._dst_bits[i].push_back(_dst_bits[i][j]);
            }
        }
        /* Add values of other graph*/
        for (size_t i = 0; i < n_bits; ++i) {
            for (size_t j = 0; j < other._size; ++j) {
                g._src_bits[i].push_back(other._src_bits[i][j]);
                g._dst_bits[i].push_back(other._dst_bits[i][j]);
            }
        }

        for (size_t i = 0; i < _size; ++i) {
            g._src.push_back(_src[i]);
            g._dst.push_back(_dst[i]);
            g._isV.push_back(_isV[i]);
            g._data.push_back(_data[i]);
        }

        for (size_t i = 0; i < other._size; ++i) {
            g._src.push_back(other._src[i]);
            g._dst.push_back(other._dst[i]);
            g._isV.push_back(other._isV[i]);
            g._data.push_back(other._data[i]);
        }

        return g;
    }

    void init_mp(Party id) {
        /* Generate vector containing { 1-isV[0], 1-isV[1], ... 1-isV[n-1]} */
        isV_inv.resize(_isV.size());
        for (size_t i = 0; i < isV_inv.size(); ++i) {
            isV_inv[i] = -_isV[i];
            if (id == P0) isV_inv[i] += 1;
        }

        /* Generate vector containing { 1-isV, src_bits[0], src_bits[1], ..., src_bits[n_bits - 1] } */
        src_order_bits.resize(_src_bits.size() + 1);
        std::copy(_src_bits.begin(), _src_bits.end(), src_order_bits.begin() + 1);
        src_order_bits[0] = isV_inv;

        /* Generate vector containing { isV, dst_bits[0], dst_bits[1], ..., dst_bits[n_bits - 1] } */
        dst_order_bits.resize(_dst_bits.size() + 1);
        std::copy(_dst_bits.begin(), _dst_bits.end(), dst_order_bits.begin() + 1);
        dst_order_bits[0] = _isV;
    }

   private:
    std::vector<Ring> _src;
    std::vector<Ring> _dst;
    std::vector<Ring> _isV;
    size_t _size = 0;
    size_t _n_vertices = 0;
    bool is_shared = false;

    /* Specifically for Message-Passing */
    std::vector<std::vector<Ring>> _src_bits;
    std::vector<std::vector<Ring>> _dst_bits;
};