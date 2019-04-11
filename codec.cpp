#include "codec.hpp"


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
