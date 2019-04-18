#ifndef CODEC_HPP_INCLUDED
#define CODEC_HPP_INCLUDED

#include <cassert>
#include <cmath>
#include <complex>
#include <cstdint>
#include <utility>
#include <vector>

#include "huffman.hpp"
#include "zigzag.hpp"


namespace RLC {
    std::pair<uint8_t, int16_t> decode_pixel(huffman::decoder*, buffer*);
    std::vector<std::vector<int16_t>> decode_block(huffman::decoder*, buffer*);

    std::vector<std::pair<uint8_t, int16_t>> encode_block(const std::vector<std::vector<int16_t>> &);
}

namespace DPCM {
    std::vector<int16_t> decode(const std::vector<uint8_t> &, huffman::decoder *);
    int16_t decode(huffman::decoder*, buffer*);
    
}

struct quantizer {
    std::vector<std::vector<int>> qtable;
    quantizer();
    quantizer(const std::vector<std::vector<int>> &);
    void quantize(std::vector<std::vector<float>> &);
    void dequantize(std::vector<std::vector<int16_t>> &);
};

quantizer luminance();
quantizer chrominance();

std::vector<std::vector<float>> FDCT(std::vector<std::vector<int16_t>> &);
void IDCT(std::vector<std::vector<int16_t>> &);

#endif
