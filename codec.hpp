#ifndef CODEC_HPP_INCLUDED
#define CODEC_HPP_INCLUDED

#include <cstdint>
#include <cassert>
#include <cmath>
#include <vector>

#include "huffman.hpp"
#include "zigzag.hpp"


namespace RLC {
    std::vector<std::vector<int16_t>> decode(const std::vector<uint8_t> &,
                                            huffman::decoder *);

}

namespace DPCM {
    std::vector<int16_t> decode(const std::vector<uint8_t> &,
                                huffman::decoder *);
}

struct quantizer {
    std::vector<std::vector<int16_t>> qtable;
    quantizer();
    quantizer(const std::vector<std::vector<int16_t>> &);
    void quantize(std::vector<std::vector<int16_t>> &);
    void dequantize(std::vector<std::vector<int16_t>> &);
};

void FDCT(std::vector<std::vector<int16_t>> &);
void IDCT(std::vector<std::vector<int16_t>> &);

#endif
