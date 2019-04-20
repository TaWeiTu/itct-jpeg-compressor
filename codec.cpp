#include "codec.hpp"

// TODO: write fast FDCT and IDCT

std::pair<uint8_t, int16_t> RLC::decode_pixel(huffman::decoder *huf, buffer *buf) {
    uint8_t res = huf->next(buf);
    if (!res)
        return std::make_pair(0, 0);

    uint8_t r = res >> 4 & 15;
    uint8_t s = res & 15;

    if (s == 0)
        return std::make_pair(15, 0);

    uint16_t offset = buf->read_bits<uint16_t>(s);
    int16_t var = (int16_t)(offset >= (1 << (s - 1)) ? offset : -((1 << s) - offset - 1));

    return std::make_pair(r, var);
}

std::vector<std::vector<int16_t>> RLC::decode_block(huffman::decoder *huf, buffer *buf) {
    std::vector<std::vector<int16_t>> res(8, std::vector<int16_t>(8, 0));

    size_t ptr = 1;
    while (ptr < 64) {
        std::pair<uint8_t, int16_t> RLP = decode_pixel(huf, buf);
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

std::vector<std::pair<uint8_t, int16_t>> RLC::encode_block(const std::vector<std::vector<int16_t>> &block) {
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
    uint8_t res = huf->next(buf); 

    if (!res)
        return 0;

#ifdef DEBUG
    fprintf(stderr, "res = %d\n", (int)res);
#endif
    uint16_t offset = buf->read_bits<uint16_t>(res);
    int16_t diff = (int16_t)(offset >= (1 << (res - 1)) ? offset : -((1 << res) - offset - 1));

    return diff;
}

quantizer::quantizer() {}

quantizer::quantizer(const std::vector<std::vector<int>> &qtable): qtable(qtable) {}

quantizer::quantizer(const std::vector<std::vector<int>> &qtable, uint8_t id): id(id), qtable(qtable){}

std::vector<std::vector<int16_t>> quantizer::quantize(const std::vector<std::vector<float>> &tab) {
    static const int n = (int)tab.size();
    std::vector<std::vector<int16_t>> res(n, std::vector<int16_t>(n));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) 
            res[i][j] = (int16_t)(tab[i][j] / (float)qtable[i][j] + 0.5);
    }
    return res;
}

void quantizer::dequantize(std::vector<std::vector<int16_t>> &tab) {
    static const int n = (int)tab.size();

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) 
            tab[i][j] = (int16_t)(tab[i][j] * qtable[i][j]);
    }
}

static const float cosine[8][8] = {
    {1.00000f, 0.98079f, 0.92388f, 0.83147f, 0.70711f, 0.55557f, 0.38268f, 0.19509f},
    {1.00000f, 0.83147f, 0.38268f, -0.19509f, -0.70711f, -0.98079f, -0.92388f, -0.55557f},
    {1.00000f, 0.55557f, -0.38268f, -0.98079f, -0.70711f, 0.19509f, 0.92388f, 0.83147f},
    {1.00000f, 0.19509f, -0.92388f, -0.55557f, 0.70711f, 0.83147f, -0.38268f, -0.98079f},
    {1.00000f, -0.19509f, -0.92388f, 0.55557f, 0.70711f, -0.83147f, -0.38268f, 0.98079f},
    {1.00000f, -0.55557f, -0.38268f, 0.98079f, -0.70711f, -0.19509f, 0.92388f, -0.83147f},
    {1.00000f, -0.83147f, 0.38268f, 0.19509f, -0.70711f, 0.98079f, -0.92388f, 0.55557f},
    {1.00000f, -0.98079f, 0.92388f, -0.83147f, 0.70711f, -0.55557f, 0.38268f, -0.19509f}
};

std::vector<std::vector<float>> FDCT(std::vector<std::vector<int16_t>> &x) {
    static const float C0 = (float)(1. / sqrt(2));
    static const float Cu = 1.;
    std::vector<std::vector<float>> y(8, std::vector<float>(8));

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            y[i][j] = 0.0;
            y[i][j] += x[0][0] * cosine[0][i] * cosine[0][j];
            y[i][j] += x[0][1] * cosine[0][i] * cosine[1][j];
            y[i][j] += x[0][2] * cosine[0][i] * cosine[2][j];
            y[i][j] += x[0][3] * cosine[0][i] * cosine[3][j];
            y[i][j] += x[0][4] * cosine[0][i] * cosine[4][j];
            y[i][j] += x[0][5] * cosine[0][i] * cosine[5][j];
            y[i][j] += x[0][6] * cosine[0][i] * cosine[6][j];
            y[i][j] += x[0][7] * cosine[0][i] * cosine[7][j];

            y[i][j] += x[1][0] * cosine[1][i] * cosine[0][j];
            y[i][j] += x[1][1] * cosine[1][i] * cosine[1][j];
            y[i][j] += x[1][2] * cosine[1][i] * cosine[2][j];
            y[i][j] += x[1][3] * cosine[1][i] * cosine[3][j];
            y[i][j] += x[1][4] * cosine[1][i] * cosine[4][j];
            y[i][j] += x[1][5] * cosine[1][i] * cosine[5][j];
            y[i][j] += x[1][6] * cosine[1][i] * cosine[6][j];
            y[i][j] += x[1][7] * cosine[0][i] * cosine[7][j];

            y[i][j] += x[2][0] * cosine[2][i] * cosine[0][j];
            y[i][j] += x[2][1] * cosine[2][i] * cosine[1][j];
            y[i][j] += x[2][2] * cosine[2][i] * cosine[2][j];
            y[i][j] += x[2][3] * cosine[2][i] * cosine[3][j];
            y[i][j] += x[2][4] * cosine[2][i] * cosine[4][j];
            y[i][j] += x[2][5] * cosine[2][i] * cosine[5][j];
            y[i][j] += x[2][6] * cosine[2][i] * cosine[6][j];
            y[i][j] += x[2][7] * cosine[2][i] * cosine[7][j];

            y[i][j] += x[3][0] * cosine[3][i] * cosine[0][j];
            y[i][j] += x[3][1] * cosine[3][i] * cosine[1][j];
            y[i][j] += x[3][2] * cosine[3][i] * cosine[2][j];
            y[i][j] += x[3][3] * cosine[3][i] * cosine[3][j];
            y[i][j] += x[3][4] * cosine[3][i] * cosine[4][j];
            y[i][j] += x[3][5] * cosine[3][i] * cosine[5][j];
            y[i][j] += x[3][6] * cosine[3][i] * cosine[6][j];
            y[i][j] += x[3][7] * cosine[3][i] * cosine[7][j];

            y[i][j] += x[4][0] * cosine[4][i] * cosine[0][j];
            y[i][j] += x[4][1] * cosine[4][i] * cosine[1][j];
            y[i][j] += x[4][2] * cosine[4][i] * cosine[2][j];
            y[i][j] += x[4][3] * cosine[4][i] * cosine[3][j];
            y[i][j] += x[4][4] * cosine[4][i] * cosine[4][j];
            y[i][j] += x[4][5] * cosine[4][i] * cosine[5][j];
            y[i][j] += x[4][6] * cosine[4][i] * cosine[6][j];
            y[i][j] += x[4][7] * cosine[4][i] * cosine[7][j];

            y[i][j] += x[5][0] * cosine[5][i] * cosine[0][j];
            y[i][j] += x[5][1] * cosine[5][i] * cosine[1][j];
            y[i][j] += x[5][2] * cosine[5][i] * cosine[2][j];
            y[i][j] += x[5][3] * cosine[5][i] * cosine[3][j];
            y[i][j] += x[5][4] * cosine[5][i] * cosine[4][j];
            y[i][j] += x[5][5] * cosine[5][i] * cosine[5][j];
            y[i][j] += x[5][6] * cosine[5][i] * cosine[6][j];
            y[i][j] += x[5][7] * cosine[5][i] * cosine[7][j];

            y[i][j] += x[6][0] * cosine[6][i] * cosine[0][j];
            y[i][j] += x[6][1] * cosine[6][i] * cosine[1][j];
            y[i][j] += x[6][2] * cosine[6][i] * cosine[2][j];
            y[i][j] += x[6][3] * cosine[6][i] * cosine[3][j];
            y[i][j] += x[6][4] * cosine[6][i] * cosine[4][j];
            y[i][j] += x[6][5] * cosine[6][i] * cosine[5][j];
            y[i][j] += x[6][6] * cosine[6][i] * cosine[6][j];
            y[i][j] += x[6][7] * cosine[6][i] * cosine[7][j];

            y[i][j] += x[7][0] * cosine[7][i] * cosine[0][j];
            y[i][j] += x[7][1] * cosine[7][i] * cosine[1][j];
            y[i][j] += x[7][2] * cosine[7][i] * cosine[2][j];
            y[i][j] += x[7][3] * cosine[7][i] * cosine[3][j];
            y[i][j] += x[7][4] * cosine[7][i] * cosine[4][j];
            y[i][j] += x[7][5] * cosine[7][i] * cosine[5][j];
            y[i][j] += x[7][6] * cosine[7][i] * cosine[6][j];
            y[i][j] += x[7][7] * cosine[7][i] * cosine[7][j];

            // for (int a = 0; a < 8; ++a) {
                // for (int b = 0; b < 8; ++b) {
                    // double p = cos((2 * a + 1) * i * pi / (2 * n));
                    // double q = cos((2 * b + 1) * j * pi / (2 * n));
                    // y[i][j] += x[a][b] * p * q;
                // }
            // }
            
            y[i][j] = (float)(y[i][j] * 0.25 * (i == 0 ? C0 : Cu) * (j == 0 ? C0 : Cu));
        }
    }
    return y;
}

void IDCT(std::vector<std::vector<int16_t>> &x) {
    static const float C0 = (float)(1. / sqrt(2));
    static const float Cu = 1.;
    static const float C0u = C0 * Cu;
    static const float C00 = C0 * C0;
    static const float Cuu = Cu * Cu;
    static float y[8][8];

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            y[i][j] = 0;
            y[i][j] += C00 * x[0][0] * cosine[i][0] * cosine[j][0];
            y[i][j] += C0u * x[0][1] * cosine[i][0] * cosine[j][1];
            y[i][j] += C0u * x[0][2] * cosine[i][0] * cosine[j][2];
            y[i][j] += C0u * x[0][3] * cosine[i][0] * cosine[j][3];
            y[i][j] += C0u * x[0][4] * cosine[i][0] * cosine[j][4];
            y[i][j] += C0u * x[0][5] * cosine[i][0] * cosine[j][5];
            y[i][j] += C0u * x[0][6] * cosine[i][0] * cosine[j][6];
            y[i][j] += C0u * x[0][7] * cosine[i][0] * cosine[j][7];

            y[i][j] += C0u * x[1][0] * cosine[i][1] * cosine[j][0];
            y[i][j] += Cuu * x[1][1] * cosine[i][1] * cosine[j][1];
            y[i][j] += Cuu * x[1][2] * cosine[i][1] * cosine[j][2];
            y[i][j] += Cuu * x[1][3] * cosine[i][1] * cosine[j][3];
            y[i][j] += Cuu * x[1][4] * cosine[i][1] * cosine[j][4];
            y[i][j] += Cuu * x[1][5] * cosine[i][1] * cosine[j][5];
            y[i][j] += Cuu * x[1][6] * cosine[i][1] * cosine[j][6];
            y[i][j] += Cuu * x[1][7] * cosine[i][1] * cosine[j][7];

            y[i][j] += C0u * x[2][0] * cosine[i][2] * cosine[j][0];
            y[i][j] += Cuu * x[2][1] * cosine[i][2] * cosine[j][1];
            y[i][j] += Cuu * x[2][2] * cosine[i][2] * cosine[j][2];
            y[i][j] += Cuu * x[2][3] * cosine[i][2] * cosine[j][3];
            y[i][j] += Cuu * x[2][4] * cosine[i][2] * cosine[j][4];
            y[i][j] += Cuu * x[2][5] * cosine[i][2] * cosine[j][5];
            y[i][j] += Cuu * x[2][6] * cosine[i][2] * cosine[j][6];
            y[i][j] += Cuu * x[2][7] * cosine[i][2] * cosine[j][7];
            
            y[i][j] += C0u * x[3][0] * cosine[i][3] * cosine[j][0];
            y[i][j] += Cuu * x[3][1] * cosine[i][3] * cosine[j][1];
            y[i][j] += Cuu * x[3][2] * cosine[i][3] * cosine[j][2];
            y[i][j] += Cuu * x[3][3] * cosine[i][3] * cosine[j][3];
            y[i][j] += Cuu * x[3][4] * cosine[i][3] * cosine[j][4];
            y[i][j] += Cuu * x[3][5] * cosine[i][3] * cosine[j][5];
            y[i][j] += Cuu * x[3][6] * cosine[i][3] * cosine[j][6];
            y[i][j] += Cuu * x[3][7] * cosine[i][3] * cosine[j][7];

            y[i][j] += C0u * x[4][0] * cosine[i][4] * cosine[j][0];
            y[i][j] += Cuu * x[4][1] * cosine[i][4] * cosine[j][1];
            y[i][j] += Cuu * x[4][2] * cosine[i][4] * cosine[j][2];
            y[i][j] += Cuu * x[4][3] * cosine[i][4] * cosine[j][3];
            y[i][j] += Cuu * x[4][4] * cosine[i][4] * cosine[j][4];
            y[i][j] += Cuu * x[4][5] * cosine[i][4] * cosine[j][5];
            y[i][j] += Cuu * x[4][6] * cosine[i][4] * cosine[j][6];
            y[i][j] += Cuu * x[4][7] * cosine[i][4] * cosine[j][7];

            y[i][j] += C0u * x[5][0] * cosine[i][5] * cosine[j][0];
            y[i][j] += Cuu * x[5][1] * cosine[i][5] * cosine[j][1];
            y[i][j] += Cuu * x[5][2] * cosine[i][5] * cosine[j][2];
            y[i][j] += Cuu * x[5][3] * cosine[i][5] * cosine[j][3];
            y[i][j] += Cuu * x[5][4] * cosine[i][5] * cosine[j][4];
            y[i][j] += Cuu * x[5][5] * cosine[i][5] * cosine[j][5];
            y[i][j] += Cuu * x[5][6] * cosine[i][5] * cosine[j][6];
            y[i][j] += Cuu * x[5][7] * cosine[i][5] * cosine[j][7];

            y[i][j] += C0u * x[6][0] * cosine[i][6] * cosine[j][0];
            y[i][j] += Cuu * x[6][1] * cosine[i][6] * cosine[j][1];
            y[i][j] += Cuu * x[6][2] * cosine[i][6] * cosine[j][2];
            y[i][j] += Cuu * x[6][3] * cosine[i][6] * cosine[j][3];
            y[i][j] += Cuu * x[6][4] * cosine[i][6] * cosine[j][4];
            y[i][j] += Cuu * x[6][5] * cosine[i][6] * cosine[j][5];
            y[i][j] += Cuu * x[6][6] * cosine[i][6] * cosine[j][6];
            y[i][j] += Cuu * x[6][7] * cosine[i][6] * cosine[j][7];

            y[i][j] += C0u * x[7][0] * cosine[i][7] * cosine[j][0];
            y[i][j] += Cuu * x[7][1] * cosine[i][7] * cosine[j][1];
            y[i][j] += Cuu * x[7][2] * cosine[i][7] * cosine[j][2];
            y[i][j] += Cuu * x[7][3] * cosine[i][7] * cosine[j][3];
            y[i][j] += Cuu * x[7][4] * cosine[i][7] * cosine[j][4];
            y[i][j] += Cuu * x[7][5] * cosine[i][7] * cosine[j][5];
            y[i][j] += Cuu * x[7][6] * cosine[i][7] * cosine[j][6];
            y[i][j] += Cuu * x[7][7] * cosine[i][7] * cosine[j][7];

            y[i][j] = (float)(y[i][j] * 0.25);
        }
    }

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            x[i][j] = (int16_t)(y[i][j] + 0.5);
    }
}

quantizer luminance(uint8_t id) {
    return quantizer({
        {16, 11, 10, 16, 24, 40, 51, 61},
        {12, 12, 14, 19, 26, 58, 60, 55},
        {14, 13, 16, 24, 40, 67, 69, 56},
        {14, 17, 22, 29, 51, 87, 80, 62},
        {18, 22, 37, 56, 68, 109, 103, 77},
        {24, 35, 55, 64, 81, 104, 113, 92},
        {49, 64, 78, 87, 103, 121, 120, 101},
        {72, 92, 95, 98, 112, 100, 103, 99} 
    }, id);
}

quantizer chrominance(uint8_t id) {
    return quantizer({
        {17, 18, 24, 47, 99, 99, 99, 99},
        {18, 21, 26, 66, 99, 99, 99, 99},
        {24, 26, 56, 99, 99, 99, 99, 99},
        {47, 66, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99}
    }, id);
}
