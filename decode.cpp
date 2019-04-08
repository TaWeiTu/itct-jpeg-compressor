#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "huffman.hpp"


const uint8_t zig[64] = {0, 0, 1, 2, 1, 0, 0, 1, 
                         2, 3, 4, 3, 2, 1, 0, 0, 
                         1, 2, 3, 4, 5, 6, 5, 4, 
                         3, 2, 1, 0, 0, 1, 2, 3, 
                         4, 5, 6, 7, 7, 6, 5, 4, 
                         3, 2, 1, 2, 3, 4, 5, 6, 
                         7, 7, 6, 5, 4, 3, 4, 5, 
                         6, 7, 7, 6, 5, 6, 7, 7};

const uint8_t zag[64] = {0, 1, 0, 0, 1, 2, 3, 2, 
                         1, 0, 0, 1, 2, 3, 4, 5, 
                         4, 3, 2, 1, 0, 0, 1, 2, 
                         3, 4, 5, 6, 7, 6, 5, 4, 
                         3, 2, 1, 0, 1, 2, 3, 4, 
                         5, 6, 7, 7, 6, 5, 4, 3, 
                         2, 3, 4, 5, 6, 7, 7, 6, 
                         5, 4, 5, 6, 7, 7, 6, 7};


int main(int argc, const char **argv) {
    if (argc != 2) {
        fprintf(stderr, "[Usage] ./decode filename\n");
        exit(1);
    }

    FILE *fp = fopen(argv[1], "rb");

    std::vector<uint8_t> qt(6), fh(6), fv(6), dcid(6), acid(6);
    std::vector<huffman::decoder> dc(4), ac(4);
    size_t ht, wd, itvl;

    bool eoi = false;

    while (!eoi) {
        uint8_t byte1 = (uint8_t)fgetc(fp);
        uint8_t byte2 = (uint8_t)fgetc(fp);

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
                fgetc(fp), fgetc(fp), fgetc(fp);
                ht = (size_t)fgetc(fp) << 8 | (size_t)fgetc(fp);
                wd = (size_t)fgetc(fp) << 8 | (size_t)fgetc(fp);

                uint8_t cnt = (uint8_t)fgetc(fp);
                for (int i = 0; i < (int)cnt; ++i) {
                    uint8_t cid = (uint8_t)fgetc(fp);
                    uint8_t fac = (uint8_t)fgetc(fp);
                    uint8_t hor = fac & 15, ver = fac >> 4 & 15;
                    uint8_t nqt = (uint8_t)fgetc(fp);
                    
                    qt[cid] = nqt;
                    fh[cid] = hor;
                    fv[cid] = ver;
                }
                break;
            }

            case 0xC2: {
                // start of frame (progressive DCT)
                break;
            }

            case 0xC4: {
                // TODO: store huffman tables
                // define huffman tables (DHT)
                size_t leng = (size_t)fgetc(fp) << 8 | (size_t)fgetc(fp);
                size_t bytes = 2;
                
                while (bytes < leng) {
                    uint8_t tc = (uint8_t)fgetc(fp); // type: 0 for DC and 1 for AC
                    uint8_t th = tc & 15;
                    tc = tc >> 4 & 15;
                    assert(tc < 2);
                    assert(th < 4);
                    bytes++;

                    std::vector<uint8_t> codeword(16);
                    std::vector<std::vector<uint8_t>> symbol(16);

                    for (int i = 0; i < 16; ++i) 
                        codeword[i] = (uint8_t)fgetc(fp);
                    
                    bytes += 16;
                    for (int i = 0; i < 16; ++i) {
                        symbol[i].resize(codeword[i]);
                        for (int j = 0; j < codeword[i]; ++j)
                            symbol[i][j] = (uint8_t)fgetc(fp);

                        bytes += codeword[i];
                    }

                    if (tc) ac[th] = huffman::decoder(codeword, symbol);
                    else    dc[th] = huffman::decoder(codeword, symbol);
                }
                break;
            }

            case 0xDB: {
                // TODO: store quantization tables
                // define quantization tables (DQT)
                size_t leng = (size_t)fgetc(fp) << 8 | (size_t)fgetc(fp);
                size_t bytes = 2;

                while (bytes < leng) {
                    uint8_t pq = (uint8_t)fgetc(fp); // precision of QT
                    uint8_t tq = pq & 15;
                    pq = pq >> 4 & 15;
                    assert(pq < 2);
                    assert(tq < 4);
                    bytes++;

                    std::vector<std::vector<int>> qtab(8, std::vector<int>(8));
                    
                    for (int k = 0; k < 64; ++k) {
                        uint8_t i = zig[k], j = zag[k];
                        qtab[i][j] = fgetc(fp);
                        if (pq == 1)
                            qtab[i][j] = qtab[i][j] << 8 | fgetc(fp);
                        bytes += pq + 1;
                    }
                }
                break;
            }

            case 0xDD: {
                // define restart interval
                fgetc(fp), fgetc(fp);
                itvl = (size_t)fgetc(fp) << 8 | (size_t)fgetc(fp);
                break;
            }

            case 0xDA: {
                // start of scan
                size_t leng = (size_t)fgetc(fp) << 8 | (size_t)fgetc(fp);
                uint8_t cnt = (uint8_t)fgetc(fp);

                for (int i = 0; i < (int)cnt; ++i) {
                    uint8_t cs = (uint8_t)fgetc(fp);
                    uint8_t td = (uint8_t)fgetc(fp);
                    uint8_t ta = td & 15;
                    td = td >> 4 & 15;

                    dcid[cs] = td;
                    acid[cs] = ta;
                }
                fgetc(fp), fgetc(fp), fgetc(fp);
                
                // TODO: Read MCU

                break;
            }

            case 0xFE: {
                // comment
                fgetc(fp), fgetc(fp), fgetc(fp);
                break;
            }

            case 0xD9: {
                // end of image
                eoi = true;
                break;
            }

            default:
                fprintf(stderr, "[Error] Wrong format, unexpected byte\n");
                exit(1);
        }
    }

    return 0;
}
