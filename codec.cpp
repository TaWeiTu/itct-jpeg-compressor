#include "codec.hpp"


std::pair<uint8_t, int16_t> RLC::decode_pair(huffman::decoder *huf, buffer *buf) {
    // decode 
    uint8_t res = huf->next(buf);
    if (!res)
        return std::make_pair(0, 0);

    uint8_t r = res >> 4 & 15;
    uint8_t s = res & 15;

    if (s == 0)
        return std::make_pair(r, 0);

    uint16_t offset = buf->read_bits<uint16_t>(s);
    int16_t var = (int16_t)(offset >= (1 << (int)(s - 1)) ? offset : -((int)(1 << s) - (int)offset - 1));
    return std::make_pair(r, var);
}

std::array<std::array<int16_t, 8>, 8> RLC::decode_block(huffman::decoder *huf, buffer *buf) {
    // decode a 8x8 block of run-length encoded AC signal
    std::array<std::array<int16_t, 8>, 8> res{};

    size_t ptr = 1;
    while (ptr < 64) {
        std::pair<uint8_t, int16_t> RLP = decode_pair(huf, buf);
        if (RLP == std::make_pair((uint8_t)0, (int16_t)0))
            break;

        for (int i = 0; i < (int)RLP.first; ++i) {
            res[zig[ptr]][zag[ptr]] = 0;
            ++ptr;
        }
        res[zig[ptr]][zag[ptr]] = RLP.second;
        ++ptr;
    }

    if (ptr > 64) {
        fprintf(stderr, "[Error] variable ptr = %d exceed 64\n", (int)ptr);
        exit(1);
    }

    return res;
}

std::vector<std::pair<uint8_t, int16_t>> RLC::encode_block(const std::array<std::array<int16_t, 8>, 8> &block) {
    // encode a 8x8 block of run-length encoded AC signal
    std::vector<std::pair<uint8_t, int16_t>> code;

    for (int i = 1, j = 0; i < 64; ) {
        for (j = i; j < 64 && block[zig[j]][zag[j]] == 0; ++j);

        if (j == 64) {
            code.emplace_back(0, 0);
            break;
        } else if (j - i > 15) {
            code.emplace_back(15, 0);
            i += 16;
        } else {
            uint8_t leng = (uint8_t)(j - i);
            code.emplace_back(leng, block[zig[j]][zag[j]]);
            i = j + 1;
        }
    }

    return code;
}

int16_t DPCM::decode(huffman::decoder *huf, buffer *buf) {
    // decode a DC signal
    uint8_t res = huf->next(buf); 

    if (!res) 
        return 0;

    int16_t offset = buf->read_bits<int16_t>(res);
    int16_t diff = (int16_t)(offset >= (1 << (int)(res - 1)) ? offset : -((int)(1 << res) - (int)offset - 1));
    return diff;
}

quantizer::quantizer() {}

quantizer::quantizer(std::array<std::array<int, 8>, 8> &&qtable): 
    qtable(qtable) {}

quantizer::quantizer(std::array<std::array<int, 8>, 8> &&qtable, uint8_t id): 
    id(id), qtable(qtable) {}

void quantizer::quantize(std::array<std::array<int16_t, 8>, 8> &tab) {
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) 
            tab[i][j] = (int16_t)(1.0 * tab[i][j] / qtable[i][j] + 0.5);
    }
}

void quantizer::dequantize(std::array<std::array<int16_t, 8>, 8> &tab) {
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) 
            tab[i][j] = (int16_t)(tab[i][j] * qtable[i][j]);
    }
}

static const int cosine[8][8] = {
    {23170, 32138, 30273, 27245, 23170, 18204, 12539, 6392},
    {23170, 27245, 12539, -6392, -23170, -32138, -30273, -18204},
    {23170, 18204, -12539, -32138, -23170, 6392, 30273, 27245},
    {23170, 6392, -30273, -18204, 23170, 27245, -12539, -32138},
    {23170, -6392, -30273, 18204, 23170, -27245, -12539, 32138},
    {23170, -18204, -12539, 32138, -23170, -6392, 30273, -27245},
    {23170, -27245, 12539, 6392, -23170, 32138, -30273, 18204},
    {23170, -32138, 30273, -27245, 23170, -18204, 12539, -6392}
};

void FDCT_1D(std::array<int16_t, 8> &x) {
    // Chen's butterfly algorithm of 8-point 1D DCT
    static int c1 = 32138;
    static int c2 = 30273;
    static int c3 = 27245;
    static int c4 = 23170;
    static int c5 = 18204;
    static int c6 = 12539;
    static int c7 = 6392;
    static int16_t x0, x1, x2, x3, x4, x5, x6, x7;
    static int16_t x00, x11, x22, x33;

    x0 = x[0] + x[7];
    x1 = x[1] + x[6];
    x2 = x[2] + x[5];
    x3 = x[3] + x[4];
    x4 = x[3] - x[4];
    x5 = x[2] - x[5];
    x6 = x[1] - x[6];
    x7 = x[0] - x[7];

    x00 = x0 + x3;
    x11 = x1 + x2;
    x22 = x1 - x2;
    x33 = x0 - x3;

    x[0] = ((x00 + x11) * c4) >> 16;
    x[4] = ((x00 - x11) * c4) >> 16;
    x[2] = (x33 * c2 + x22 * c6) >> 16;
    x[6] = (x33 * c6 - x22 * c2) >> 16;

    x[1] = (c1 * x7 + c3 * x6 + c5 * x5 + c7 * x4) >> 16;
    x[3] = (c3 * x7 - c7 * x6 - c1 * x5 - c5 * x4) >> 16;
    x[5] = (c5 * x7 - c1 * x6 + c7 * x5 + c3 * x4) >> 16;
    x[7] = (c7 * x7 - c5 * x6 + c3 * x5 - c1 * x4) >> 16;
}

void IDCT_1D(std::array<int16_t, 8> &x) {
    // inverse DCT
    static int y[8];
    memset(y, 0, sizeof(y));
    y[0] += x[0] * cosine[0][0];
    y[0] += x[1] * cosine[0][1];
    y[0] += x[2] * cosine[0][2];
    y[0] += x[3] * cosine[0][3];
    y[0] += x[4] * cosine[0][4];
    y[0] += x[5] * cosine[0][5];
    y[0] += x[6] * cosine[0][6];
    y[0] += x[7] * cosine[0][7];

    y[1] += x[0] * cosine[1][0];
    y[1] += x[1] * cosine[1][1];
    y[1] += x[2] * cosine[1][2];
    y[1] += x[3] * cosine[1][3];
    y[1] += x[4] * cosine[1][4];
    y[1] += x[5] * cosine[1][5];
    y[1] += x[6] * cosine[1][6];
    y[1] += x[7] * cosine[1][7];

    y[2] += x[0] * cosine[2][0];
    y[2] += x[1] * cosine[2][1];
    y[2] += x[2] * cosine[2][2];
    y[2] += x[3] * cosine[2][3];
    y[2] += x[4] * cosine[2][4];
    y[2] += x[5] * cosine[2][5];
    y[2] += x[6] * cosine[2][6];
    y[2] += x[7] * cosine[2][7];
    
    y[3] += x[0] * cosine[3][0];
    y[3] += x[1] * cosine[3][1];
    y[3] += x[2] * cosine[3][2];
    y[3] += x[3] * cosine[3][3];
    y[3] += x[4] * cosine[3][4];
    y[3] += x[5] * cosine[3][5];
    y[3] += x[6] * cosine[3][6];
    y[3] += x[7] * cosine[3][7];

    y[4] += x[0] * cosine[4][0];
    y[4] += x[1] * cosine[4][1];
    y[4] += x[2] * cosine[4][2];
    y[4] += x[3] * cosine[4][3];
    y[4] += x[4] * cosine[4][4];
    y[4] += x[5] * cosine[4][5];
    y[4] += x[6] * cosine[4][6];
    y[4] += x[7] * cosine[4][7];

    y[5] += x[0] * cosine[5][0];
    y[5] += x[1] * cosine[5][1];
    y[5] += x[2] * cosine[5][2];
    y[5] += x[3] * cosine[5][3];
    y[5] += x[4] * cosine[5][4];
    y[5] += x[5] * cosine[5][5];
    y[5] += x[6] * cosine[5][6];
    y[5] += x[7] * cosine[5][7];
    
    y[6] += x[0] * cosine[6][0];
    y[6] += x[1] * cosine[6][1];
    y[6] += x[2] * cosine[6][2];
    y[6] += x[3] * cosine[6][3];
    y[6] += x[4] * cosine[6][4];
    y[6] += x[5] * cosine[6][5];
    y[6] += x[6] * cosine[6][6];
    y[6] += x[7] * cosine[6][7];

    y[7] += x[0] * cosine[7][0];
    y[7] += x[1] * cosine[7][1];
    y[7] += x[2] * cosine[7][2];
    y[7] += x[3] * cosine[7][3];
    y[7] += x[4] * cosine[7][4];
    y[7] += x[5] * cosine[7][5];
    y[7] += x[6] * cosine[7][6];
    y[7] += x[7] * cosine[7][7];

    x[0] = (int16_t)(y[0] >> 16);
    x[1] = (int16_t)(y[1] >> 16);
    x[2] = (int16_t)(y[2] >> 16);
    x[3] = (int16_t)(y[3] >> 16);
    x[4] = (int16_t)(y[4] >> 16);
    x[5] = (int16_t)(y[5] >> 16);
    x[6] = (int16_t)(y[6] >> 16);
    x[7] = (int16_t)(y[7] >> 16);
}

void FDCT(std::array<std::array<int16_t, 8>, 8> &x) {
    // calculate 2D DCT by first applying 1D DCT to each row then apply 1D DCT to columns
    static std::array<std::array<int16_t, 8>, 8> y;
    for (int i = 0; i < 8; ++i) {
        FDCT_1D(x[i]);
    }
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            y[i][j] = x[j][i];
        FDCT_1D(y[i]);
    }

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            x[i][j] = y[j][i];
    }
}

void IDCT(std::array<std::array<int16_t, 8>, 8> &x) {
    // Calculate 2D IDCT by first applying 1D IDCT to each column then apply 1D DCT to rows
    static std::array<std::array<int16_t, 8>, 8> y;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            y[i][j] = x[j][i];
        IDCT_1D(y[i]);
    }
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            x[i][j] = y[j][i];
        IDCT_1D(x[i]);
    }
}

quantizer luminance_low(uint8_t id) {
    // low quality luminance quantization table
    return quantizer(std::array<std::array<int, 8>, 8>{{
        {16, 11, 10, 16, 24, 40, 51, 61},
        {12, 12, 14, 19, 26, 58, 60, 55},
        {14, 13, 16, 24, 40, 67, 69, 56},
        {14, 17, 22, 29, 51, 87, 80, 62},
        {18, 22, 37, 56, 68, 109, 103, 77},
        {24, 35, 55, 64, 81, 104, 113, 92},
        {49, 64, 78, 87, 103, 121, 120, 101},
        {72, 92, 95, 98, 112, 100, 103, 99} 
    }}, id);
}

quantizer chrominance_low(uint8_t id) {
    // low quality chrominance quantization table
    return quantizer(std::array<std::array<int, 8>, 8>{{
        {17, 18, 24, 47, 99, 99, 99, 99},
        {18, 21, 26, 66, 99, 99, 99, 99},
        {24, 26, 56, 99, 99, 99, 99, 99},
        {47, 66, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99}
    }}, id);
}

quantizer lossless(uint8_t id) {
    // lossless quantization table
    return quantizer(std::array<std::array<int, 8>, 8>{{
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1}
    }}, id);
}

quantizer luminance_high(uint8_t id) {
    // high quality luminance quantization table
    return quantizer(std::array<std::array<int, 8>, 8>{{
        {3, 1, 1, 3, 4, 6, 7, 9},
        {1, 1, 1, 3, 4, 8, 8, 8},
        {1, 1, 3, 4, 6, 8, 10, 8},
        {1, 3, 3, 4, 7, 12, 11, 10},
        {3, 3, 5, 8, 10, 15, 15, 12},
        {4, 5, 8, 9, 11, 15, 16, 13},
        {7, 9, 11, 12, 15, 17, 17, 15},
        {10 ,13, 13, 14, 16, 14, 15, 14}
    }}, id);
}
quantizer chrominance_high(uint8_t id) {
    // high quality chrominance quantization table
    return quantizer(std::array<std::array<int, 8>, 8>{{
        {1, 1, 3, 5, 11, 11, 11, 11},
        {1, 3, 3, 7, 11, 11, 11, 11},
        {3, 3, 6, 11, 11, 11, 11, 11},
        {5, 7, 11, 11, 11, 11, 11, 11},
        {11, 11, 11, 11, 11, 11, 11, 11},
        {11, 11, 11, 11, 11, 11, 11, 11},
        {11, 11, 11, 11, 11, 11, 11, 11},
        {11, 11, 11, 11, 11, 11, 11, 11}
    }}, id);
}
