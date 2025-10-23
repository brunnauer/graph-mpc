#include "gate.h"

/* Used by Input */
Gate::Gate(GType type, size_t g_id, size_t out_idx)
    : type(type),
      g_id(g_id),
      in1_idx(0),
      in2_idx(0),
      val(0),
      out_idx(out_idx),
      size(0),
      layer(0),
      shuffle_idx(0),
      pi_idx(0),
      omega_idx(0),
      mult_idx(0),
      inverse(false),
      binary(false) {}

/* Used by ConstructData */
Gate::Gate(GType type, size_t g_id, size_t in1_idx, size_t out_idx, std::vector<size_t> &data_parallel)
    : type(type),
      g_id(g_id),
      in1_idx(in1_idx),
      in2_idx(0),
      val(0),
      out_idx(out_idx),
      size(0),
      layer(0),
      shuffle_idx(0),
      pi_idx(0),
      omega_idx(0),
      mult_idx(0),
      inverse(false),
      binary(false),
      data_parallel(data_parallel) {}

/* Used by Output, Propagate-1, Gather-1, Gather-2, Reveal, Sub */
Gate::Gate(GType type, size_t g_id, size_t in1_idx, size_t out_idx)
    : type(type),
      g_id(g_id),
      in1_idx(in1_idx),
      in2_idx(0),
      val(0),
      out_idx(out_idx),
      size(0),
      layer(0),
      shuffle_idx(0),
      pi_idx(0),
      omega_idx(0),
      mult_idx(0),
      inverse(false),
      binary(false) {}

/* Used by AddConst */
Gate::Gate(GType type, size_t g_id, size_t in1_idx, Ring val, size_t out_idx)
    : type(type),
      g_id(g_id),
      in1_idx(in1_idx),
      in2_idx(0),
      val(val),
      out_idx(out_idx),
      size(0),
      layer(0),
      shuffle_idx(0),
      pi_idx(0),
      omega_idx(0),
      mult_idx(0),
      inverse(false),
      binary(false) {}

/* Used by Propagate-2, Shuffle, Unshuffle, Bit2A, Compaction, Add, Sub */
Gate::Gate(GType type, size_t g_id, size_t param1, size_t param2, size_t param3)
    : type(type), g_id(g_id), val(0), size(0), layer(0), pi_idx(0), omega_idx(0), inverse(false), binary(false) {
    if (type == Propagate2 || type == Permute || type == ReversePermute || type == Add) {
        in1_idx = param1;
        in2_idx = param2;
        out_idx = param3;
        shuffle_idx = 0;
        mult_idx = 0;
    } else if (type == Shuffle || type == Unshuffle) {
        in1_idx = param1;
        in2_idx = 0;
        out_idx = param2;
        shuffle_idx = param3;
        mult_idx = 0;
    } else if (type == Compaction) {
        in1_idx = param1;
        in2_idx = 0;
        out_idx = param2;
        shuffle_idx = param3;
        mult_idx = param3;
    }
}

/* Used by Bit2A */
Gate::Gate(GType type, size_t g_id, size_t param1, size_t param2, size_t param3, size_t param4)
    : type(type), g_id(g_id), val(0), layer(0), pi_idx(0), omega_idx(0), inverse(false), binary(false) {
    if (type == Bit2A) {
        in1_idx = param1;
        in2_idx = 0;
        out_idx = param2;
        size = param3;
        shuffle_idx = 0;
        mult_idx = param4;
    }
}

/* Used by MergedShuffle and EQZ */
Gate::Gate(GType type, size_t g_id, size_t in1_idx, size_t out_idx, size_t n1, size_t n2, size_t n3)
    : type(type), g_id(g_id), in1_idx(in1_idx), in2_idx(0), val(0), out_idx(out_idx) {
    if (type == MergedShuffle) {
        size = 0;
        layer = 0;
        shuffle_idx = n1;
        pi_idx = n2;
        omega_idx = n3;
        mult_idx = 0;
        inverse = false;
        binary = false;
    } else if (type == EQZ) {
        size = n1;
        layer = n2;
        shuffle_idx = 0;
        pi_idx = 0;
        omega_idx = 0;
        mult_idx = n3;
        inverse = false;
        binary = true;
    }
}

/* Used by Mul */
Gate::Gate(GType type, size_t g_id, size_t in1_idx, size_t in2_idx, size_t out_idx, size_t size, size_t mult_idx, bool binary)
    : type(type),
      g_id(g_id),
      in1_idx(in1_idx),
      in2_idx(in2_idx),
      out_idx(out_idx),
      size(size),
      layer(0),
      shuffle_idx(0),
      pi_idx(0),
      omega_idx(0),
      mult_idx(mult_idx),
      inverse(false),
      binary(binary) {}

bool Gate::interactive() {
    return type == Shuffle || type == MergedShuffle || type == Unshuffle || type == Compaction || type == Mul || type == EQZ || type == Bit2A || type == Reveal;
}