#include "codec.hpp"

// TODO: write fast FDCT and IDCT
#define float double

std::pair<uint8_t, int16_t> RLC::decode_pixel(huffman::decoder *huf, buffer *buf) {
    uint8_t res = huf->next(buf);
    if (!res)
        return std::make_pair(0, 0);

    uint8_t r = res >> 4 & 15;
    uint8_t s = res & 15;

    if (s == 0) {
        return std::make_pair(r, 0);
    }

    uint16_t offset = buf->read_bits<uint16_t>(s);
    int16_t var = (int16_t)(offset >= (1 << (int)(s - 1)) ? offset : -((int)(1 << s) - (int)offset - 1));

    // printf("%d\n", (int)var);
    return std::make_pair(r, var);
}

std::array<std::array<int16_t, 8>, 8> RLC::decode_block(huffman::decoder *huf, buffer *buf) {
    std::array<std::array<int16_t, 8>, 8> res{};

    size_t ptr = 1;
    // fprintf(stderr, "enter\n");
    while (ptr < 64) {
        std::pair<uint8_t, int16_t> RLP = decode_pixel(huf, buf);
        // fprintf(stderr, "%d %d\n", (int)RLP.first, (int)RLP.second);
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
    // printf("%d\n", (int)res);

    if (!res) {
        // printf("0\n");
        return 0;
    }

    int16_t offset = buf->read_bits<int16_t>(res);
    int16_t diff = (int16_t)(offset >= (1 << (int)(res - 1)) ? offset : -((int)(1 << res) - (int)offset - 1));

    // printf("%d\n", (int)offset);
    return diff;
}

quantizer::quantizer() {}

quantizer::quantizer(std::array<std::array<int, 8>, 8> &&qtable): qtable(qtable) {
    for (int i = 0; i < 8; ++i) {
        fprintf(stderr, "{");
        for (int j = 0; j < 8; ++j) {
            fprintf(stderr, "%d" , (int)qtable[i][j]);
            if (j == 7)
                fprintf(stderr, "}");
            else
                fprintf(stderr, ",");
        }
        fprintf(stderr, "\n");
    }
}

quantizer::quantizer(std::array<std::array<int, 8>, 8> &&qtable, uint8_t id): id(id), qtable(qtable) {
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            fprintf(stderr, "%d ", (int)qtable[i][j]);
        fprintf(stderr, "\n");
    }
}

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

void FDCT_1D(std::array<int16_t, 8> &x) {
    static const float C0 = (float)(1. / sqrt(8));
    static const float Cu = (float)(sqrt(2) / sqrt(8));
    static float y[8];
    for (int i = 0; i < 8; ++i) {
        y[i] = 0.0;
        for (int j = 0; j < 8; ++j)
            y[i] += x[j] * cosine[j][i];
        y[i] *= (i == 0 ? C0 : Cu);
    }

    for (int i = 0; i < 8; ++i)
        x[i] = (int16_t)y[i];
}

/* void FDCT_1D_chen(std::array<int16_t, 8> &x) {
    static const float pi = (float)acos(-1);
    static float sq2 = (float)sqrt(2);
    static float c1 = (float)cos(1 * pi / 16) / sq2;
    static float s1 = (float)sin(1 * pi / 16) / sq2;
    static float c2 = (float)cos(2 * pi / 16) / sq2;
    static float s2 = (float)sin(2 * pi / 16) / sq2;
    static float c3 = (float)cos(3 * pi / 16) / sq2;
    static float s3 = (float)sin(3 * pi / 16) / sq2;
    static float c4 = (float)cos(4 * pi / 16) / sq2;
    static float s4 = (float)sin(4 * pi / 16) / sq2;
    static float c5 = (float)cos(5 * pi / 16) / sq2;
    static float s5 = (float)sin(5 * pi / 16) / sq2;
    static float c6 = (float)cos(6 * pi / 16) / sq2;
    static float s6 = (float)sin(6 * pi / 16) / sq2;
    static float c7 = (float)cos(7 * pi / 16) / sq2;
    static float s7 = (float)sin(7 * pi / 16) / sq2;
    // printf("c5 = %.3f s5 = %.3f c6 = %.3f s6 = %.3f c7 = %.3f s7 = %.3f\n", 
            // c5, s5, c6, s6, c7, s7);

    float x0 = x[0] + x[7];
    float x1 = x[1] + x[8];
    float x2 = x[2] + x[5];
    float x3 = x[3] + x[4];
    float x4 = x[3] - x[4];
    float x5 = x[2] - x[5];
    float x6 = x[1] - x[6];
    float x7 = x[0] - x[7];

    float x00 = x0 + x3;
    float x11 = x1 + x2;
    float x22 = x1 - x2;
    float x33 = x0 - x3;

    x[0] = (x00 + x11) * c4;
    x[4] = (x00 - x11) * c4;
    x[2] = (x22 * c6 + x33 * s6) * sq2;
    x[6] = (x22 * -s6 + x33 * c6) * sq2;

    float x44 = x4 * c7 + x7 * s7;
    float x55 = x5 * c5 + x6 * s5;
    float x66 = x5 * -s5 + x6 * c5;
    float x77 = x4 * -s7 + x7 * c7;

    float x444 = x44 + x55;
    float x555 = x44 - x55;
    float x666 = x66 + x77;
    float x777 = x66 - x77;

    x[1] = x444 * sq2;
    x[7] = x777 * -sq2;
    x[3] = x555 + x666;
    x[5] = x555 - x666;
} */

void IDCT_1D(std::array<int16_t, 8> &x) {
    static const float C0 = (float)(1. / sqrt(8));
    static const float Cu = (float)(sqrt(2) / sqrt(8));
    static float y[8];
    for (int i = 0; i < 8; ++i) {
        y[i] = 0.0;
        for (int j = 0; j < 8; ++j)
            y[i] += (j == 0 ? C0 : Cu) * x[j] * cosine[i][j];
    }

    for (int i = 0; i < 8; ++i)
        x[i] = (int16_t)y[i];
}

void FDCT2(std::array<std::array<int16_t, 8>, 8> &x) {
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

void IDCT2(std::array<std::array<int16_t, 8>, 8> &x) {
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

void FDCT(std::array<std::array<int16_t, 8>, 8> &x) {
    return FDCT2(x);
    static const float C0 = (float)(1. / sqrt(2));
    static const float Cu = 1.;
    static float y[8][8];

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
            y[i][j] += x[1][7] * cosine[1][i] * cosine[7][j];

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
            
            y[i][j] = (float)(y[i][j] * 0.25 * (i == 0 ? C0 : Cu) * (j == 0 ? C0 : Cu));
        }
    }

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            x[i][j] = (int16_t)(y[i][j] + 0.5);
    }
}

void IDCT(std::array<std::array<int16_t, 8>, 8> &x) {
    return IDCT2(x);
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
            x[i][j] = (int16_t)(y[i][j]);
    }
}

quantizer luminance(uint8_t id) {
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

quantizer chrominance(uint8_t id) {
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

quantizer dummy(uint8_t id) {
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

quantizer hey1(uint8_t id) {
    return quantizer(std::array<std::array<int, 8>, 8>{{
        {3,1,1,3,4,6,7,9},
        {1,1,1,3,4,8,8,8},
        {1,1,3,4,6,8,10,8},
        {1,3,3,4,7,12,11,10},
        {3,3,5,8,10,15,15,12},
        {4,5,8,9,11,15,16,13},
        {7,9,11,12,15,17,17,15},
        {10,13,13,14,16,14,15,14}
    }}, id);
}
quantizer hey2(uint8_t id) {
    return quantizer(std::array<std::array<int, 8>, 8>{{
        {1,1,3,5,11,11,11,11},
        {1,3,3,7,11,11,11,11},
        {3,3,6,11,11,11,11,11},
        {5,7,11,11,11,11,11,11},
        {11,11,11,11,11,11,11,11},
        {11,11,11,11,11,11,11,11},
        {11,11,11,11,11,11,11,11},
        {11,11,11,11,11,11,11,11}
    }}, id);
}
