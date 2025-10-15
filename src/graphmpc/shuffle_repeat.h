#pragma once

#include "shuffle.h"

class ShuffleRepeat : public Shuffle {
   public:
    ShuffleRepeat(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
                  std::vector<Ring> *output, ShufflePre *perm_share, Party &recv)
        : Shuffle(conf, preproc_vals, online_vals, input, output, recv, perm_share) {}

    ShuffleRepeat(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
                  std::vector<Ring> *output, ShufflePre *perm_share, Party &recv, FileWriter *disk)
        : Shuffle(conf, preproc_vals, online_vals, input, output, recv, perm_share, disk) {}

    void preprocess() override {}
};