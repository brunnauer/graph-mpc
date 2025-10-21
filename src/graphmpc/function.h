#pragma once

#include "../io/disk.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

class Function {
   public:
    /* Used by Input */
    Function(FType type, size_t f_id, size_t out_idx)
        : type(type),
          f_id(f_id),
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
    Function(FType type, size_t f_id, size_t in1_idx, size_t out_idx, std::vector<size_t> &data_parallel)
        : type(type),
          f_id(f_id),
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
    Function(FType type, size_t f_id, size_t in1_idx, size_t out_idx)
        : type(type),
          f_id(f_id),
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
    Function(FType type, size_t f_id, size_t in1_idx, Ring val, size_t out_idx)
        : type(type),
          f_id(f_id),
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

    Function(FType type, size_t f_id, size_t in1_idx, size_t out_idx, bool inverse)
        : type(type),
          f_id(f_id),
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
          inverse(inverse),
          binary(false) {}

    /* Used by Propagate-2, Shuffle, Unshuffle, Bit2A, Compaction, Add, Sub */
    Function(FType type, size_t f_id, size_t param1, size_t param2, size_t param3)
        : type(type), f_id(f_id), val(0), size(0), layer(0), pi_idx(0), omega_idx(0), inverse(false), binary(false) {
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
    Function(FType type, size_t f_id, size_t param1, size_t param2, size_t param3, size_t param4)
        : type(type), f_id(f_id), val(0), layer(0), pi_idx(0), omega_idx(0), inverse(false), binary(false) {
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
    Function(FType type, size_t f_id, size_t in1_idx, size_t out_idx, size_t n1, size_t n2, size_t n3)
        : type(type), f_id(f_id), in1_idx(in1_idx), in2_idx(0), val(0), out_idx(out_idx) {
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
    Function(FType type, size_t f_id, size_t in1_idx, size_t in2_idx, size_t out_idx, size_t size, size_t mult_idx, bool binary)
        : type(type),
          f_id(f_id),
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

    virtual ~Function() = default;

    inline bool interactive() {
        return type == Shuffle || type == MergedShuffle || type == Unshuffle || type == Compaction || type == Mul || type == EQZ || type == Bit2A ||
               type == Reveal;
    }

    FType type;
    size_t f_id;
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
