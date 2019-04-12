#ifndef CODEC_HPP_INCLUDED
#define CODEC_HPP_INCLUDED

#include <cassert>
#include <cmath>
#include <cstdint>
#include <complex>
#include <utility>
#include <vector>

#include "huffman.hpp"
#include "zigzag.hpp"


namespace RLC {
    std::vector<std::vector<int16_t>> decode(const std::vector<uint8_t> &, huffman::decoder *);
    std::pair<uint8_t, int16_t> decode_pixel(huffman::decoder*, buffer*);
    std::vector<std::vector<int16_t>> decode_block(huffman::decoder*, buffer*);
}

namespace DPCM {
    std::vector<int16_t> decode(const std::vector<uint8_t> &, huffman::decoder *);
    int16_t decode(huffman::decoder*, buffer*);
}

struct quantizer {
    std::vector<std::vector<int>> qtable;
    quantizer();
    quantizer(const std::vector<std::vector<int>> &);
    void quantize(std::vector<std::vector<int16_t>> &);
    void dequantize(std::vector<std::vector<int16_t>> &);
};

// void FFT(std::vector<std::complex<double>> &, int);
// void IDCT_fast_1D(std::vector<int16_t> &);
// void IDCT_fast_2D(std::vector<std::vector<int16_t>> &);
// void FDCT_naive_1D(std::vector<int16_t> &);
// void FDCT_naive_2D(std::vector<std::vector<int16_t>> &);
void FDCT(std::vector<std::vector<int16_t>> &);
void IDCT(std::vector<std::vector<int16_t>> &);

#endif
