#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "codec.hpp"
#include "huffman.hpp"
#include "zigzag.hpp"

#define SOI 0xD8
#define SOF 0xC0
#define DHT 0xC4
#define DQT 0xDB
#define DRI 0xDD
#define SOS 0xDA
#define APP 0xE0
#define COM 0xFE
#define EOI 0xD9


int main(int argc, const char **argv) {
    if (argc < 2) {
        fprintf(stderr, "[Usage] ./decode source [destination]\n");
        exit(1);
    }

    FILE *fp = fopen(argv[1], "rb");

    std::vector<uint8_t> qt(6), fh(6), fv(6), dcid(6), acid(6);
    std::vector<huffman::decoder> dc(4), ac(4);
    size_t ht, wd, itvl;

    bool soi = false;
    bool eoi = false;

    while (!eoi) {
        uint8_t byte1 = (uint8_t)fgetc(fp);
        uint8_t byte2 = (uint8_t)fgetc(fp);

        if (byte1 != 0xFF) {
            fprintf(stderr, "[Error] Wrong format, expect 0xFF\n");
            exit(1);
        }

        switch (byte2) {
            case SOI: {
                fprintf(stderr, "[Debug] SOI\n");
                // start of image
                soi = true;
                break;
            }

            case SOF: {
                if (!soi) {
                    fprintf(stderr, "[Error] SOI not found\n");
                    exit(1);
                }
                fprintf(stderr, "[Debug] SOF\n");
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

            case DHT: {
                if (!soi) {
                    fprintf(stderr, "[Error] SOI not found\n");
                    exit(1);
                }
                fprintf(stderr, "[Debug] DHT\n");
                // TODO: store huffman tables
                // define huffman tables
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

            case DQT: {
                if (!soi) {
                    fprintf(stderr, "[Error] SOI not found\n");
                    exit(1);
                }
                fprintf(stderr, "[Debug] DQT\n");
                // TODO: store quantization tables
                // define quantization tables
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

            case DRI: {
                if (!soi) {
                    fprintf(stderr, "[Error] SOI not found\n");
                    exit(1);
                }
                fprintf(stderr, "[Debug] DRI\n");
                // define restart interval
                fgetc(fp), fgetc(fp);
                itvl = (size_t)fgetc(fp) << 8 | (size_t)fgetc(fp);
                break;
            }

            case SOS: {
                if (!soi) {
                    fprintf(stderr, "[Error] SOI not found\n");
                    exit(1);
                }
                fprintf(stderr, "[Debug] SOS\n");
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
                while (true) {
                    // TODO: this is WRONG
                    uint8_t byte = (uint8_t)fgetc(fp);
                    fprintf(stderr, "[DEBUG] read 0x%hhx after SOS\n", byte);
                    if (byte == 0xFF) {
                        uint8_t nxt = (uint8_t)fgetc(fp);
                        if (nxt == 0x00) {
                            ungetc(nxt, fp);
                        } else {
                            ungetc(nxt,  fp);
                            ungetc(byte, fp);
                            break;
                        }
                    }
                }

                break;
            }

            case APP: {
                if (!soi) {
                    fprintf(stderr, "[Error] SOI not found\n");
                    exit(1);
                }
                fprintf(stderr, "[Debug] APP\n");
                size_t leng = (size_t)fgetc(fp) << 8 | (size_t)fgetc(fp);
                uint64_t mark = 0;
                for (int i = 0; i < 5; ++i)
                    mark = mark << 8 | fgetc(fp);
                
                if (mark != 0x4A46494600) {
                    fprintf(stderr, "[Error] Expect JFIF\n");
                    exit(1);
                }

                uint16_t version = fgetc(fp) << 8 | fgetc(fp);
                uint8_t unit = (uint8_t)fgetc(fp);
                uint16_t x_density = fgetc(fp) << 8 | fgetc(fp);
                uint16_t y_density = fgetc(fp) << 8 | fgetc(fp);
                uint8_t width_t  = (uint8_t)fgetc(fp);
                uint8_t height_t = (uint8_t)fgetc(fp);

                for (int i = 0; i < (int)width_t; ++i) {
                    for (int j = 0; j < (int)height_t; ++j) {
                        // TODO: make it useful
                        fgetc(fp);
                        fgetc(fp);
                        fgetc(fp);
                    }
                }
                break;
            }

            case COM: {
                if (!soi) {
                    fprintf(stderr, "[Error] SOI not found\n");
                    exit(1);
                }
                fprintf(stderr, "[Debug] COM\n");
                // comment
                fgetc(fp), fgetc(fp), fgetc(fp);
                break;
            }

            case EOI: {
                if (!soi) {
                    fprintf(stderr, "[Error] SOI not found\n");
                    exit(1);
                }
                fprintf(stderr, "[Debug] EOI\n");
                // end of image
                eoi = true;
                break;
            }

            default:
                fprintf(stderr, "[Error] Wrong format, unexpected byte 0x%hhx\n", byte2);
                exit(1);
        }
    }

    fclose(fp);

    return 0;
}
