#include <array>
#include <cstdio>
#include <cinttypes>
#include <random>
#include <vector>

#include "../codec.hpp"

int main() {
    // std::vector<std::vector<int16_t>> v(8, std::vector<int16_t>(8));
    // std::vector<std::vector<int16_t>> w(8, std::vector<int16_t>(8));
    std::array<std::array<int16_t, 8>, 8> v;
    std::mt19937 rng(7122);
    std::uniform_int_distribution<int16_t> dis(-256, 256);
    quantizer qtz = dummy(0);
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) 
            v[i][j] = dis(rng);
    }

    printf("Input matrix: \n");
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            printf("%" PRId16 " ", v[i][j]);
        printf("\n");
    }

    v = qtz.quantize(FDCT(v));

    printf("FDCT: \n");
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            printf("%" PRId16 " ", v[i][j]);
        printf("\n");
    }

    qtz.dequantize(v);
    IDCT(v);

    printf("IDCT: \n");
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            printf("%" PRId16 " ", v[i][j]);
        printf("\n");
    }
}
