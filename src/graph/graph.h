#pragma once

#include <set>

#include "edge.h"
#include "node.h"

template <typename R>
class Graph {
   public:
    Graph(std::set<Node<R>> &nodes, std::set<Edge<R>> &edges) : nodes(nodes), edges(edges) { set_dag_list(); }
    std::vector<R> src() { return _src; }
    std::vector<R> dest() { return _dst; }
    std::vector<bool> isV() { return _isV; }
    std::vector<R> payload() { return _payload; }
    void print() {
        std::cout << "src  dst  isV  payload" << std::endl;
        for (size_t i = 0; i < nodes.size() + edges.size(); ++i) {
            std::cout << _src[i] << " " << _dst[i] << " " << _isV[i] << " " << _payload[i] << std::endl;
        }
    }

    static Graph from_file() {}

   private:
    std::set<Node<R>> nodes;
    std::set<Edge<R>> edges;
    std::vector<R> _src;
    std::vector<R> _dst;
    std::vector<bool> _isV;
    std::vector<R> _payload;

    void set_dag_list() {
        /* Graph vectors are created in source-order */
        for (auto node : nodes) {
            _src.push_back(node.name());
            std::cout << "Pushed back node " << node.name() << std::endl;
            _dst.push_back(node.name());
            _isV.push_back(true);
            _payload.push_back(node.data());
        }
        for (auto edge : edges) {
            _src.push_back(edge.a().name());
            std::cout << "Pushed back edge src " << edge.a().name() << std::endl;
            _dst.push_back(edge.b().name());
            std::cout << "Pushed back edge dst " << edge.b().name() << std::endl;
            _isV.push_back(false);
            _payload.push_back(edge.data());
        }
    }
};