#include <cassert>
#include <iostream>
#include <numeric>

#include "../src/io/disk.h"
#include "../src/setup/configs.h"
#include "../src/utils/permutation.h"
#include "../src/utils/shuffle_preproc.h"

const std::string filename = "test_disk.bin";
const size_t size = 10;
RandomGenerators rngs(std::vector<uint64_t>({123456789, 123456789, 123456789, 123456789, 123456789, 123456789, 123456789, 123456789, 123456789}),
                      std::vector<uint64_t>({123456789, 123456789, 123456789, 123456789, 123456789, 123456789, 123456789, 123456789, 123456789}));

void test() {
    FileWriter disk(filename);
    ShufflePre perm_share;
    perm_share.pi_0 = Permutation::random(size, rngs.rng_self());
    perm_share.pi_1 = Permutation::random(size, rngs.rng_self());
    perm_share.has_pi_0 = true;
    perm_share.has_pi_1 = true;
    perm_share.preprocessed = true;
}

int main() {
    test();
    std::cout << "Passed test." << std::endl;
    return 0;
}