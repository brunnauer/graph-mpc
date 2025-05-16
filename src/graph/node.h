#pragma once

template <typename R>
class Node {
   public:
    Node() = default;
    Node(R &data) : _data(data) {}
    Node(R name, R data) : _name(name), _data(data) {}
    R data() { return _data; }
    R name() { return _name; }
    bool operator<(const Node<R> &n) const { return _name < n._name; }
    void print() { std::cout << _data; }

   private:
    R _name;
    R _data;
};