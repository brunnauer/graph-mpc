#pragma once

#include "compaction.h"
#include "perm.h"
#include "sharing.h"
#include "shuffle.h"

class Sort {
   public:
    Sort(Party pid, size_t n_rows, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network)
        : pid(pid),
          n_rows(n_rows),
          rngs(rngs),
          network(network),
          shuffle(Shuffle(pid, n_rows, 1, rngs, network)),
          compaction(Compaction(pid, n_rows, rngs, network)) {};

    void set_input(std::vector<Row> &input) {
        if (input.size() != n_rows) throw std::invalid_argument("Specified input vector has the wrong size.");
        wire = input;
    }

    std::vector<Row> get_output() { return wire; }

    std::vector<Row> reveal() {
        std::vector<Row> outvals(n_rows);
        if (wire.empty()) {
            return outvals;
        }

        if (pid != D) {
            std::vector<Row> output_share_self(wire.size());
            std::vector<Row> output_share_other(wire.size());

            std::copy(wire.begin(), wire.end(), output_share_self.begin());

            size_t partner = (pid == P0) ? 1 : 0;
            network->send(partner, output_share_self.data(), output_share_self.size() * sizeof(Row));
            network->recv(partner, output_share_other.data(), output_share_other.size() * sizeof(Row));

            for (size_t i = 0; i < wire.size(); ++i) {
                Row outmask = output_share_other[i];
                outvals[i] = output_share_self[i] + outmask;
            }
            return outvals;
        } else {
            return outvals;
        }
    }

    Permutation get_sort() {
        Permutation rho;
        return rho;
    }

    void sort_iteration() {
        /* Shuffle compaction perm (secret_shared) */
        compaction.set_input(wire);
        std::vector<Row> perm_y = compaction.get_compaction();
        shuffle.set_input(perm_y);
        shuffle.run();

        /* Reveal shuffled perm */
        std::vector<Row> revealed = shuffle.reveal();
        Permutation perm_shuffled(revealed);
        perm_shuffled = perm_shuffled.inverse();

        /* Shuffle input using the same order */
        shuffle.set_input(wire);
        shuffle.repeat();
        std::vector<Row> input_shuffled = shuffle.get_output();

        std::vector<Row> sorted = perm_shuffled(input_shuffled);
        wire = sorted;
    }

   private:
    Party pid;
    size_t n_rows;
    size_t idx_mult = 0;
    const size_t BLOCK_SIZE = 100000;
    std::vector<Row> wire;

    RandomGenerators rngs;
    std::shared_ptr<io::NetIOMP> network;
    Shuffle shuffle;
    Compaction compaction;
};