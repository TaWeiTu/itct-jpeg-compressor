#include <array>
#include <cstdio>
#include <cinttypes>
#include <random>
#include <vector>
#include <cassert>

#include "../codec.hpp"

int main() {
    // std::vector<std::vector<int16_t>> v(8, std::vector<int16_t>(8));
    // std::vector<std::vector<int16_t>> w(8, std::vector<int16_t>(8));
    std::array<std::array<int16_t, 8>, 8> v, w;
    std::array<int16_t, 8> vv, ww;
    std::mt19937 rng(7122);
    std::uniform_int_distribution<int16_t> dis(-256, 256);
    // quantizer qtz = dummy(0);
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) 
            v[i][j] = dis(rng),
            w[i][j] = v[i][j];
    }

    for (int i = 0; i < 8; ++i)
        ww[i] = vv[i] = dis(rng);

    printf("input vector: \n");
    for (int i = 0; i < 8; ++i) printf("%d ", (int)vv[i]); puts("");

    IDCT_1D(vv);
    IDCT_1D_chen(ww);

    printf("IDCT1: \n");
    for (int i = 0; i < 8; ++i) printf("%d ", (int)vv[i]); puts("");

    printf("IDCT2: \n");
    for (int i = 0; i < 8; ++i) printf("%d ", (int)ww[i]); puts("");
    /* printf("Input matrix: \n");
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            printf("%" PRId16 " ", v[i][j]);
        printf("\n");
    }

    FDCT(v);
    FDCT2(w);

    printf("FDCT1: \n");
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            printf("%" PRId16 " ", v[i][j]);
        printf("\n");
    }


    printf("FDCT2: \n");
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            printf("%" PRId16 " ", w[i][j]),
            assert((v[i][j] - w[i][j]) <= 3);
        printf("\n");
    }

    IDCT(v);
    IDCT2(w);

    printf("IDCT1: \n");
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            printf("%" PRId16 " ", v[i][j]);
        printf("\n");
    }


    printf("jDCT2: \n");
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            printf("%" PRId16 " ", w[i][j]);
        printf("\n");
    } */
}
