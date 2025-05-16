#include <graph/graph.h>

#include <cassert>
#include <exception>
#include <iostream>
#include <set>

#include "../../src/utils/types.h"

void test_graph() {
    std::set<Node<int>> nodes;
    std::set<Edge<int>> edges;

    Node<int> n0(0, 0);
    nodes.insert(n0);

    Node<int> n1(1, 1);
    nodes.insert(n1);

    Edge<int> e0(n0, n1, 3);
    edges.insert(e0);

    Node<int> n2(2, 2);
    nodes.insert(n2);

    Edge<int> e1(n0, n2, 4);
    edges.insert(e1);

    Edge<int> e2(n2, n1, 5);
    edges.insert(e2);

    Graph<int> g(nodes, edges);
    g.print();
}

int main(int argc, char **argv) {
    try {
        test_graph();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}