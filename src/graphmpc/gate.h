#pragma once

#include "../io/disk.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

class Gate {
   public:
    /* Used by Input */
    Gate(GType type, size_t g_id, size_t out_idx);

    /* Used by ConstructData */
    Gate(GType type, size_t g_id, size_t in1_idx, size_t out_idx, std::vector<size_t> &data_parallel);

    /* Used by Output, Propagate-1, Gather-1, Gather-2, Reveal, Sub */
    Gate(GType type, size_t g_id, size_t in1_idx, size_t out_idx);

    /* Used by AddConst */
    Gate(GType type, size_t g_id, size_t in1_idx, Ring val, size_t out_idx);

    /* Used by Propagate-2, Shuffle, Unshuffle, Bit2A, Compaction, Add, Sub */
    Gate(GType type, size_t g_id, size_t param1, size_t param2, size_t param3);

    /* Used by Bit2A */
    Gate(GType type, size_t g_id, size_t param1, size_t param2, size_t param3, size_t param4);

    /* Used by MergedShuffle and EQZ */
    Gate(GType type, size_t g_id, size_t in1_idx, size_t out_idx, size_t n1, size_t n2, size_t n3);

    /* Used by Mul */
    Gate(GType type, size_t g_id, size_t in1_idx, size_t in2_idx, size_t out_idx, size_t size, size_t mult_idx, bool binary);

    bool interactive();

    GType type;
    size_t g_id;
    size_t in1_idx;
    size_t in2_idx;
    Ring val;
    size_t out_idx;

    size_t size;
    size_t layer;

    size_t shuffle_idx;
    size_t pi_idx;
    size_t omega_idx;
    size_t mult_idx;

    bool inverse;
    bool binary;

    std::vector<size_t> data_parallel;
};
