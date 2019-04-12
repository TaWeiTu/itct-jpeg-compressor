#include "codec.hpp"

// TODO: write fast FDCT and IDCT

std::pair<uint8_t, int16_t> RLC::decode_pixel(huffman::decoder *huf, buffer *buf) {
    uint8_t res = huf->next(buf);
    fprintf(stderr, "[RLC::decode_pixel] res = %d\n", (int)res);
    if (!res)
        return std::make_pair(0, 0);

    uint8_t r = res >> 4 & 15;
    uint8_t s = res & 15;
    fprintf(stderr, "[RLC::decode_pixel] r = %d s = %d\n", (int)r, (int)s);

    uint16_t offset = buf->read_bits<uint16_t>(s);
    int16_t var = (int16_t)(offset >= (1 << (s - 1)) ? offset : -((1 << s) - offset - 1));

    return std::make_pair(r, var);
}

std::vector<std::vector<int16_t>> RLC::decode_block(huffman::decoder *huf, buffer *buf) {
    std::vector<std::vector<int16_t>> res(8, std::vector<int16_t>(8, 0));

    size_t ptr = 1;
    while (true) {
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

// std::vector<std::vector<int16_t>> RLC::decode(const std::vector<uint8_t> &code, 
                                        // huffman::decoder *huf) {
    // huf->feed(code);

    // std::vector<std::vector<int16_t>> res(8, std::vector<int16_t>(8));
    // size_t ptr = 1;
    // while (!huf->empty()) {
        // uint8_t ret = huf->next();
        // if (!ret) {
            // while (ptr < 64) {
                // res[zig[ptr]][zag[ptr]] = 0;
                // ++ptr;
            // }
            // break;
        // }
        // uint8_t r = ret >> 4 & 15;
        // uint8_t s = ret & 15;
        // uint16_t offset = huf->read_bits(s);
        // int16_t var = (int16_t)(offset >= (1 << (s - 1)) ? offset : -((1 << s) - offset - 1));

        // for (int i = 0; i < (int)r; ++i) {
            // res[zig[ptr]][zag[ptr]] = 0;
            // ++ptr;
        // }
        // res[zig[ptr]][zag[ptr]] = var;
        // ++ptr;
    // }
    // return res;
// }

// std::vector<int16_t> DPCM::decode(const std::vector<uint8_t> &code,
                                 // huffman::decoder *huf) {
    // huf->feed(code);
    // std::vector<int16_t> res;

    // while (!huf->empty()) {
        // uint8_t r = huf->next();
        // uint16_t offset = huf->read_bits(r);
        // int16_t diff = r == 0 ? 0 : (int16_t)(offset >= (1 << (r - 1)) ? offset : -((1 << r) - offset - 1));

        // res.push_back(res.empty() ? diff : (int16_t)(diff + res.back()));
    // }
    // return res;
// }

quantizer::quantizer() {}

quantizer::quantizer(const std::vector<std::vector<int16_t>> &qtable): qtable(qtable) {}

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

cplx::cplx(): re(0), im(0) {}
    
cplx::cplx(double r, double i): re(r), im(i) {}

cplx cplx::operator+(const cplx &rhs) const { 
    return cplx(re + rhs.re, im + rhs.im); 
}

cplx cplx::operator-(const cplx &rhs) const { 
    return cplx(re - rhs.re, im - rhs.im); 
}

cplx cplx::operator*(const cplx &rhs) const { 
    return cplx(re * rhs.re - im * rhs.im, re * rhs.im + im * rhs.re); 
}

cplx cplx::conj() const { 
    return cplx(re, -im); 
}

void FFT(std::vector<cplx> &v, int n) {
    static const int maxn = 8;
    static cplx omega[maxn + 1];
    static const double pi = acos(-1);

    for (int i = 0; i <= maxn; ++i)
        omega[i] = cplx(cos(2 * pi * i / maxn), sin(2 * pi * i / maxn));

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
                cplx x = v[i + z + k] * omega[maxn / s * k];
                v[i + z + k] = v[i + k] - x;
                v[i + k] = v[i + k] + x;
            }
        }
    }
}

void IDCT_fast_1D(std::vector<int16_t> &x) {
    static const double pi = acos(-1);
    std::vector<cplx> fft(16);
    for (int i = 0; i < 8; ++i) fft[i] = cplx(x[i], 0);

    FFT(fft, 16);

    for (int i = 0; i < 8; ++i) {
        double coef = 4 / (i == 0 ? 1. / sqrt(2) : 1) * cos(pi * i  / 16);
        x[i] = (int16_t)(fft[i].re / coef);
    }
}

void IDCT_fast_2D(std::vector<std::vector<int16_t>> &x) {
    for (int i = 0; i < 8; ++i) {
        IDCT_fast_1D(x[i]);
    }

    for (int i = 0; i < 8; ++i) {
        std::vector<int16_t> col(8);
        for (int j = 0; j < 8; ++j) col[j] = x[j][i];

        IDCT_fast_1D(col);
        for (int j = 0; j < 8; ++j) x[j][i] = col[j];
    }
}

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

void IDCT(std::vector<std::vector<int16_t>> &x) {
    static const double pi = acos(-1);
    static const int n = (int)x.size();
    std::vector<std::vector<double>> y(n, std::vector<double>(n));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int a = 0; a < n; ++a) {
                for (int b = 0; b < n; ++b) {
                    double p = cos((2 * i + 1) * a * pi / (2 * n));
                    double q = cos((2 * j + 1) * b * pi / (2 * n));
                    double z = (a == 0 ? 1. / sqrt(2) : 1.) * (b == 0 ? 1. / sqrt(2) : 1.);
                    y[i][j] += z * x[a][b] * p * q;
                }
            }

            y[i][j] *= 2. / n;
        }
    }

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j)
            x[i][j] = (int16_t)y[i][j];
    }
}
