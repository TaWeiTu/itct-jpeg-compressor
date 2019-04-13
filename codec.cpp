#include "codec.hpp"

// TODO: write fast FDCT and IDCT

std::pair<uint8_t, int16_t> RLC::decode_pixel(huffman::decoder *huf, buffer *buf) {
    uint8_t res = huf->next(buf);
    if (!res)
        return std::make_pair(0, 0);

    uint8_t r = res >> 4 & 15;
    uint8_t s = res & 15;

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
        assert(ptr <= 64);
    }

    return res;
}

int16_t DPCM::decode(huffman::decoder *huf, buffer *buf) {
    uint8_t res = huf->next(buf); 

    if (!res)
        return 0;

    uint16_t offset = buf->read_bits<uint16_t>(res);
    int16_t diff = (int16_t)(offset >= (1 << (res - 1)) ? offset : -((1 << res) - offset - 1));

    return diff;
}

quantizer::quantizer() {}

quantizer::quantizer(const std::vector<std::vector<int>> &qtable): qtable(qtable) {}

void quantizer::quantize(std::vector<std::vector<int16_t>> &tab) {
    static const int n = (int)tab.size();

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) 
            tab[i][j] = (int16_t)(tab[i][j] / qtable[i][j]);
    }
}

void quantizer::dequantize(std::vector<std::vector<int16_t>> &tab) {
    static const int n = (int)tab.size();

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) 
            tab[i][j] = (int16_t)(tab[i][j] * qtable[i][j]);
    }
}

/* void FFT(std::vector<std::complex<double>> &v, int n) {
    static const int maxn = 16;
    static std::complex<double> omega[maxn + 1];
    static const double pi = acos(-1);

    for (int i = 0; i <= maxn; ++i)
        omega[i] = std::complex<double>(cos(2 * pi * i / maxn), sin(2 * pi * i / maxn));

    int z = __builtin_ctz(n) - 1;
    for (int i = 0; i < n; ++i) {
        int x = 0;
        for (int j = 0; (1 << j) < n; ++j) x ^= (i >> j & 1) << (z - j);
        if (x > i) std::swap(v[x], v[i]);
    }


    for (int s = 2; s <= n; s <<= 1) {
        int z = s >> 1;
        for (int i = 0; i < n; i += s) {
            for (int k = 0; k < z; ++k) {
                std::complex<double> x = v[i + z + k] * omega[maxn / s * k];
                v[i + z + k] = v[i + k] - x;
                v[i + k] = v[i + k] + x;
            }
        }
    }
} */

// void IDCT_fast_1D(std::vector<int16_t> &x) {
    // static const double pi = acos(-1);
    // std::vector<cplx> fft(16);
    // for (int i = 0; i < 8; ++i) fft[i] = cplx(x[i], 0);

    // FFT(fft, 16);

    // for (int i = 0; i < 8; ++i) {
        // double coef = 4 / (i == 0 ? 1. / sqrt(2) : 1) * cos(pi * i  / 16);
        // x[i] = (int16_t)(fft[i].re / coef);
    // }
// }

// void IDCT_fast_2D(std::vector<std::vector<int16_t>> &x) {
    // for (int i = 0; i < 8; ++i) {
        // IDCT_fast_1D(x[i]);
    // }

    // for (int i = 0; i < 8; ++i) {
        // std::vector<int16_t> col(8);
        // for (int j = 0; j < 8; ++j) col[j] = x[j][i];

        // IDCT_fast_1D(col);
        // for (int j = 0; j < 8; ++j) x[j][i] = col[j];
    // }
// }

/* void FDCT_naive_1D(std::vector<int16_t> &x) {
    static const double pi = acos(-1);
    static const std::complex<double> I(0, 1);

    // std::vector<double> y(8, 0);
    // for (int i = 0; i < 8; ++i) {
        // for (int j = 0; j < 8; ++j) {
            // y[i] += x[j] * cos((2 * j + 1) * i * pi / 16);
        // }
        // y[i] *= (i == 0 ? 1. / sqrt(2) : 1) / 2;
    // }
    // for (int i = 0; i < 8; ++i) x[i] = (int16_t)y[i];

    std::vector<std::complex<double>> y(16);
    for (int i = 0; i < 8; ++i) y[i] = std::complex<double>(x[i], 0);
    FFT(y, 16);
    for (int i = 0; i < 16; ++i) 
        y[i] = std::complex<double>(y[i].real(), -y[i].imag());
    for (int i = 0; i < 16; ++i) fprintf(stderr, "(%.5lf, %.5lf) ", y[i].real(), y[i].imag());

    const std::complex<double> fact = -I * pi / 16.;
    fprintf(stderr, "\n");
    for (int i = 0; i < 8; ++i) {
        double coef = 4 / (i == 0 ? 1. / sqrt(2) : 1) * cos(pi * i / 16);
        // y[i] *= exp(fact * (double)i);
        x[i] = (int16_t)(y[i].real() / coef);
    }
} */

/* void FDCT_naive_2D(std::vector<std::vector<int16_t>> &x) {
    for (int i = 0; i < 8; ++i) {
        FDCT_naive_1D(x[i]); 
    }
    for (int i = 0; i < 8; ++i) {
        std::vector<int16_t> y(8);
        for (int j = 0; j < 8; ++j) y[j] = x[j][i];

        FDCT_naive_1D(y);
        for (int j = 0; j < 8; ++j) x[j][i] = y[j];
    }
} */

void FDCT(std::vector<std::vector<int16_t>> &x) {
    static const double pi = acos(-1);
    static const int n = (int)x.size();
    std::vector<std::vector<double>> y(n, std::vector<double>(n));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int a = 0; a < n; ++a) {
                for (int b = 0; b < n; ++b) {
                    double p = cos((2 * a + 1) * i * pi / (2 * n));
                    double q = cos((2 * b + 1) * j * pi / (2 * n));
                    y[i][j] += x[a][b] * p * q;
                }
            }
            y[i][j] *= 2.0 / n * (i == 0 ? 1. / sqrt(2) : 1) * (j == 0 ? 1. / sqrt(2) : 1);
        }
    }

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) 
            x[i][j] = (int16_t)y[i][j];
    }
}

static const float cosine[8][8] = {
    {1.00000, 0.98079, 0.92388, 0.83147, 0.70711, 0.55557, 0.38268, 0.19509},
    {1.00000, 0.83147, 0.38268, -0.19509, -0.70711, -0.98079, -0.92388, -0.55557},
    {1.00000, 0.55557, -0.38268, -0.98079, -0.70711, 0.19509, 0.92388, 0.83147},
    {1.00000, 0.19509, -0.92388, -0.55557, 0.70711, 0.83147, -0.38268, -0.98079},
    {1.00000, -0.19509, -0.92388, 0.55557, 0.70711, -0.83147, -0.38268, 0.98079},
    {1.00000, -0.55557, -0.38268, 0.98079, -0.70711, -0.19509, 0.92388, -0.83147},
    {1.00000, -0.83147, 0.38268, 0.19509, -0.70711, 0.98079, -0.92388, 0.55557},
    {1.00000, -0.98079, 0.92388, -0.83147, 0.70711, -0.55557, 0.38268, -0.19509}
};

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

            y[i][j] *= 0.25;
        }
    }

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            x[i][j] = (int16_t)y[i][j];
    }
}
