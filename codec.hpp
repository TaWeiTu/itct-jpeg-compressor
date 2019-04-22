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
    std::array<std::array<int16_t, 8>, 8> decode_block(huffman::decoder*, buffer*);

    std::vector<std::pair<uint8_t, int16_t>> encode_block(const std::array<std::array<int16_t, 8>, 8> &);
}

namespace DPCM {
    std::vector<int16_t> decode(const std::vector<uint8_t> &, huffman::decoder *);
    int16_t decode(huffman::decoder*, buffer*);
    
}

struct quantizer {
    uint8_t id = 0;
    std::array<std::array<int, 8>, 8> qtable;
    quantizer();
    quantizer(const std::array<std::array<int, 8>, 8> &);
    quantizer(const std::array<std::array<int, 8>, 8> &, uint8_t);
    std::array<std::array<int16_t, 8>, 8> quantize(const std::array<std::array<float, 8>, 8> &);
    void dequantize(std::array<std::array<int16_t, 8>, 8> &);
};

quantizer luminance(uint8_t);
quantizer chrominance(uint8_t);
quantizer dummy(uint8_t);

std::array<std::array<float, 8>, 8> FDCT(std::array<std::array<int16_t, 8>, 8> &);
void IDCT(std::array<std::array<int16_t, 8>, 8> &);

std::array<std::array<int, 8>, 8> construct_block(std::initializer_list<int>);

#endif
