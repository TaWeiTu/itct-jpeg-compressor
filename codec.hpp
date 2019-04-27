#ifndef CODEC_HPP_INCLUDED
#define CODEC_HPP_INCLUDED

#include <array>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#include "huffman.hpp"
#include "zigzag.hpp"

namespace RLC {
    std::pair<uint8_t, int16_t> decode_pair(huffman::decoder*, buffer*);
    std::array<std::array<int16_t, 8>, 8> decode_block(huffman::decoder*, buffer*);
    std::vector<std::pair<uint8_t, int16_t>> encode_block(const std::array<std::array<int16_t, 8>, 8> &);
}

namespace DPCM {
    int16_t decode(huffman::decoder*, buffer*);
}

struct quantizer {
    uint8_t id = 0;
    std::array<std::array<int, 8>, 8> qtable;
    quantizer();
    quantizer(std::array<std::array<int, 8>, 8> &&);
    quantizer(std::array<std::array<int, 8>, 8> &&, uint8_t);
    void quantize(std::array<std::array<int16_t, 8>, 8> &);
    void dequantize(std::array<std::array<int16_t, 8>, 8> &);
};

quantizer luminance_low(uint8_t);
quantizer chrominance_low(uint8_t);
quantizer luminance_high(uint8_t);
quantizer chrominance_high(uint8_t);
quantizer lossless(uint8_t);

void FDCT(std::array<std::array<int16_t, 8>, 8> &);
void IDCT(std::array<std::array<int16_t, 8>, 8> &);
void FDCT_1D(std::array<int16_t, 8> &);
void IDCT_1D(std::array<int16_t, 8> &);


std::array<std::array<int, 8>, 8> construct_block(std::initializer_list<int>);

#endif
