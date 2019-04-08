#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "huffman.hpp"


const uint8_t zig[64] = {0, 0, 1, 2, 1, 0, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 4, 5, 6, 7, 7, 6, 5, 6, 7, 7};
const uint8_t zag[64] = {0, 1, 0, 0, 1, 2, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 5, 6, 7, 7, 6, 7};

int main(int argc, const char **argv) {
    if (argc != 2) {
        fprintf(stderr, "[Usage] ./decode filename\n");
        exit(1);
    }

    FILE *fp = fopen(argv[1], "rb");
    fseek(fp, 0, SEEK_END);
    long flen = ftell(fp);
    rewind(fp);

    char *buf = new char[flen + 1];
    fread(buf, flen, 1, fp);
    fclose(fp);

    size_t ptr = 0;

    while (ptr < (size_t)flen) {
        uint8_t byte1 = buf[ptr++];
        uint8_t byte2 = buf[ptr++];

        if (byte1 != 0xFF) {
            fprintf(stderr, "[Error] Wrong format, expect 0xFF\n");
            exit(1);
        }

        switch (byte2) {
            case 0xD8: {
                // start of image
                break;
            }

            case 0xC0: {
                // start of frame (baseline DCT)
                ptr += 3;
                size_t ht = (size_t)buf[ptr] << 8 | (size_t)buf[ptr + 1];
                ptr += 2;
                size_t wd = (size_t)buf[ptr] << 8 | (size_t)buf[ptr + 1];
                ptr += 2;

                uint8_t cnt = buf[ptr++];
                for (int i = 0; i < (int)cnt; ++i) {
                    uint8_t cid = buf[ptr++];
                    uint8_t fac = buf[ptr++];
                    uint8_t nqt = buf[ptr++];
                }
                break;
            }

            case 0xC2: {
                // start of frame (progressive DCT)
                break;
            }

            case 0xC4: {
                fprintf(stderr, "DHT\n");
                // TODO: store huffman tables
                // define huffman tables
                size_t leng = (size_t)buf[ptr] << 8 | (size_t)buf[ptr + 1];
                ptr += 2;
                
                size_t optr = ptr - 2;
                while (ptr - optr < leng) {
                    uint8_t tc = buf[ptr] >> 4 & 15; // type: 0 for DC and 1 for AC
                    assert(tc < 2);
                    uint8_t th = buf[ptr++] & 15;
                    assert(th < 4);

                    std::vector<uint8_t> codeword(16);
                    std::vector<std::vector<uint8_t>> symbol(16);

                    for (int i = 0; i < 16; ++i) 
                        codeword[i] = (uint8_t)buf[ptr++];
                    
                    for (int i = 0; i < 16; ++i) {
                        symbol[i].resize(codeword[i]);
                        for (int j = 0; j < codeword[i]; ++j)
                            symbol[i][j] = (uint8_t)buf[ptr++];
                    }
                }
                break;
            }

            case 0xDB: {
                fprintf(stderr, "DQT\n");
                // TODO: store quantization tables
                // define quantization tables
                size_t leng = (size_t)buf[ptr] << 8 | (size_t)buf[ptr + 1];
                ptr += 2;

                size_t optr = ptr - 2;
                while (ptr - optr < leng) {
                    uint8_t pq = buf[ptr] >> 4 & 15; // precision of QT
                    assert(pq < 2);
                    uint8_t tq = buf[ptr++] & 15; // number of QT
                    assert(tq < 4);

                    std::vector<std::vector<int>> qtab(8, std::vector<int>(8));
                    
                    for (int k = 0; k < 64; ++k) {
                        uint8_t i = zig[k], j = zag[k];
                        qtab[i][j] = buf[ptr++];
                        if (pq == 1)
                            qtab[i][j] = qtab[i][j] << 8 | buf[ptr++];
                    }
                }
                break;
            }

            case 0xDD: {
                // define restart interval
                break;
            }

            case 0xDA: {
                // start of scan
                break;
            }

            case 0xFE: {
                // comment
                break;
            }

            case 0xD9: {
                // end of image
                break;
            }

            default:
                fprintf(stderr, "[Error] Wrong format, unexpected byte\n");
                exit(1);

        }
    }

    return 0;
}
