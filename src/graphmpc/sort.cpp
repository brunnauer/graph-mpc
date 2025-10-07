#include "mp_protocol.h"

void MPProtocol::add_sort(std::vector<std::vector<Ring>> &bit_keys, std::vector<Ring> &output) {
    add_compaction(bit_keys[0], output);
    for (size_t bit = 1; bit < bits; ++bit) {
        add_sort_iteration(output, bit_keys[bit], output);
    }
}

void MPProtocol::add_sort_iteration(std::vector<Ring> &perm, std::vector<Ring> &keys, std::vector<Ring> &output) {
    add_shuffle(perm, w.sort_perm);
    repeat_shuffle(keys, w.sort_bits);

    add_reveal(w.sort_perm, w.sort_perm);
    add_permute(w.sort_bits, w.sort_bits, w.sort_perm);

    add_compaction(w.sort_bits, w.sort_perm);

    add_permute(w.sort_bits, w.sort_bits, w.sort_perm, true);
    add_unshuffle(w.sort_bits, output);
}
