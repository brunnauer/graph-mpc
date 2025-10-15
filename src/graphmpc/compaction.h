#pragma once

#include "mul.h"

class Compaction : public Mul {
   public:
    Compaction(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
               std::vector<Ring> *output, Party &recv)
        : Mul(conf, preproc_vals, online_vals, input, {}, output, recv, false) {}

    Compaction(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
               std::vector<Ring> *output, Party &recv, FileWriter *preproc_disk, FileWriter *triples_disk)
        : Mul(conf, preproc_vals, online_vals, input, {}, output, recv, false, preproc_disk, triples_disk) {}

    void evaluate_send() override {
        if (id == D) return;
        if (ssd) {
            triples_a = triples_disk->read(size);
            triples_b = triples_disk->read(size);
            if (id == read)
                triples_c = preproc_disk->read(size);
            else
                triples_c = triples_disk->read(size);
        }

        std::vector<Ring> f_0(size);

        // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
        for (size_t i = 0; i < size; ++i) {
            f_0[i] = -input->at(i);
            if (id == P0) {
                f_0[i] += 1;  // 1 as constant only to one share
            }
        }

        std::vector<Ring> s_0(size);
        std::vector<Ring> s_1(size);
        Ring s = 0;
        // Set s_0 to prefix sum of f_0 and s_1 to prefix sum of f_1/input continuing from the prior final value
        for (size_t i = 0; i < size; ++i) {
            s += f_0[i];
            s_0[i] = s;
        }

        for (size_t i = 0; i < size; ++i) {
            s += input->at(i);
            s_1[i] = s - s_0[i];
        }

        size_t old = online_vals->size();
        online_vals->resize(old + 2 * size);
        auto send_ptr = online_vals->data() + old;

#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            Ring a = triples_a[i];
            Ring b = triples_b[i];

            auto xa = input->at(i) + a;
            auto yb = s_1[i] + b;
            send_ptr[2 * i] = xa;
            send_ptr[2 * i + 1] = yb;
        }
        return;
    }

    void evaluate_recv() override {
        std::vector<Ring> f_0(size);
        // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
        for (size_t i = 0; i < size; ++i) {
            f_0[i] = -input->at(i);
            if (id == P0) {
                f_0[i] += 1;  // 1 as constant only to one share
            }
        }

        std::vector<Ring> s_0(size);
        Ring s = 0;
        // Set s_0 to prefix sum of f_0 and s_1 to prefix sum of f_1/input continuing from the prior final value
        for (size_t i = 0; i < size; ++i) {
            s += f_0[i];
            s_0[i] = s;
        }

        auto data_recv = read_online(2 * size);

        auto outptr = output->data();
#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            Ring a = triples_a[i];
            Ring b = triples_b[i];
            Ring c = triples_c[i];

            auto xa = data_recv[2 * i];
            auto yb = data_recv[2 * i + 1];

            outptr[i] = (xa * yb * (id)) - (xa * b) - (yb * a) + c;
        }

#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < output->size(); ++i) {
            outptr[i] += s_0[i];
            if (id == P0) outptr[i]--;
        }
    }
};