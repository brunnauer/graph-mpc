#pragma once

#include "../io/disk.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

class Function {
   public:
    Function(FType type, size_t f_id, std::vector<Ring> output) : type(type), f_id(f_id), output(output) {}

    Function(FType type, size_t f_id, Ring in1, Ring in2, Ring output) : type(type), f_id(f_id), _in1(std::move(in1)), _in2(std::move(in2)), _output(output) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> output) : type(type), f_id(f_id), in1(std::move(in1)), output(output) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> output, bool inverse)
        : type(type), f_id(f_id), in1(std::move(in1)), output(output), inverse(inverse) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> output, size_t size, size_t layer)
        : type(type), f_id(f_id), in1(std::move(in1)), output(output), size(size), layer(layer) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> output, size_t shuffle_idx)
        : type(type), f_id(f_id), in1(std::move(in1)), output(output), shuffle_idx(shuffle_idx) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> output, size_t shuffle_idx, size_t pi_idx, size_t omega_idx)
        : type(type), f_id(f_id), in1(std::move(in1)), output(output), shuffle_idx(shuffle_idx), pi_idx(pi_idx), omega_idx(omega_idx) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, Ring in2, std::vector<Ring> output)
        : type(type), f_id(f_id), in1(std::move(in1)), _in2(std::move(in2)), output(output) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> in2, std::vector<Ring> output)
        : type(type), f_id(f_id), in1(std::move(in1)), in2(std::move(in2)), output(output) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> in2, std::vector<Ring> output, bool binary)
        : type(type), f_id(f_id), in1(std::move(in1)), in2(std::move(in2)), output(output), binary(binary) {}

    Function(FType type, size_t f_id, std::vector<Ring> in1, std::vector<Ring> in2, std::vector<Ring> output, size_t size, bool binary)
        : type(type), f_id(f_id), in1(std::move(in1)), in2(std::move(in2)), output(output), size(size), binary(binary) {}

    virtual ~Function() = default;

    inline bool interactive() {
        return type == Shuffle || type == MergedShuffle || type == Unshuffle || type == Compaction || type == Mul || type == EQZ || type == Bit2A ||
               type == Reveal;
    }

    size_t f_id;
    FType type;
    std::vector<Ring> in1;
    std::vector<Ring> in2;
    std::vector<Ring> output;
    Ring _in1;
    Ring _in2;
    Ring _output;

    bool binary;
    bool inverse;
    Party recv;

    size_t size;
    size_t layer;

    size_t shuffle_idx;
    size_t pi_idx;
    size_t omega_idx;
};
