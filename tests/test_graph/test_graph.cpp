#include <graph.h>

#include <cassert>
#include <exception>
#include <iostream>
#include <set>

#include "../../src/utils/types.h"

void test_graph() {}

int main(int argc, char **argv) {
    try {
        test_graph();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}