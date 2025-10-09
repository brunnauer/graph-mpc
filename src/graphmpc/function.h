#pragma once

#include "../io/disk.h"
#include "../setup/configs.h"
#include "../utils/sharing.h"
#include "../utils/types.h"

class Function {
   public:
    Function(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
             std::vector<Ring> *output)
        : id(conf->id),
          rngs(&conf->rngs),
          size(conf->size),
          ssd(conf->ssd),
          preproc_vals(preproc_vals),
          online_vals(online_vals),
          input(input),
          output(output) {}

    Function(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
             std::vector<Ring> *input2, std::vector<Ring> *output)
        : id(conf->id),
          rngs(&conf->rngs),
          size(conf->size),
          ssd(conf->ssd),
          preproc_vals(preproc_vals),
          online_vals(online_vals),
          input(input),
          input2(input2),
          output(output) {}

    Function(ProtocolConfig *conf, std::unordered_map<Party, std::vector<Ring>> *preproc_vals, std::vector<Ring> *online_vals, std::vector<Ring> *input,
             std::vector<Ring> *input2, std::vector<Ring> *output, size_t &size)
        : id(conf->id),
          rngs(&conf->rngs),
          size(size),
          ssd(conf->ssd),
          preproc_vals(preproc_vals),
          online_vals(online_vals),
          input(input),
          input2(input2),
          output(output) {}

    virtual ~Function() = default;

    std::vector<Ring> *input = nullptr;
    std::vector<Ring> *input2 = nullptr;
    std::vector<Ring> *output = nullptr;

    virtual void preprocess() = 0;
    virtual void evaluate_send() = 0;
    virtual void evaluate_recv() = 0;

    std::vector<Ring> read_preproc(size_t n_elems) {
        std::vector<Ring> data(n_elems);

        data = {preproc_vals->at(id).begin(), preproc_vals->at(id).begin() + n_elems};

        /* Delete read values from buffer */
        preproc_vals->at(id).erase(preproc_vals->at(id).begin(), preproc_vals->at(id).begin() + n_elems);

        return data;
    }

    std::vector<Ring> read_online(size_t n_elems) {
        std::vector<Ring> data(n_elems);

        data = {online_vals->begin(), online_vals->begin() + n_elems};

        /* Delete read values from buffer */
        online_vals->erase(online_vals->begin(), online_vals->begin() + n_elems);

        return data;
    }

   protected:
    Party id;
    RandomGenerators *rngs;
    size_t size;
    bool ssd;

    std::unordered_map<Party, std::vector<Ring>> *preproc_vals;
    std::vector<Ring> *online_vals;
};
