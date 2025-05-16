#pragma once

#include <iostream>

#include "node.h"

template <typename R>
class Edge {
   public:
    Edge() = default;
    Edge(Node<R> &a, Node<R> &b) : _a(a), _b(b) {}
    Edge(Node<R> &a, Node<R> &b, R data) : _a(a), _b(b), _data(data) {}
    Node<R> a() { return _a; }
    Node<R> b() { return _b; }
    R data() { return _data; }
    void set_data(R &data) { _data = data; }
    bool operator<(const Edge<R> &n) const { return _a < n._a || _b < n._b; }
    void print() {
        std::cout << "Edge (";
        _a.print();
        std::cout << ", ";
        _b.print();
        std::cout << ")" << std::endl;
    }

   private:
    Node<R> _a;
    Node<R> _b;
    R _data;
    void set_a(Node<R> &node) { _a = node; }
    void set_b(Node<R> &node) { _b = node; }
};