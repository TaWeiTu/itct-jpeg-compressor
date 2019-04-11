#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "buffer.hpp"
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


uint8_t read_MCU_byte(FILE *fp, bool &terminate) {
    uint8_t res = (uint8_t)fgetc(fp);
    if (res == 0xFF) {
        uint8_t nxt = (uint8_t)fgetc(fp);
        if (nxt != 0xFF) {
            ungetc(nxt, fp);
            ungetc(res, fp);
            terminate = true;;
            return 0x00;
        }
    }
    return res;
}


int main(int argc, const char **argv) {
    if (argc < 2) {
        fprintf(stderr, "[Usage] ./decode source [destination]\n");
        exit(1);
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "[Error] Image to be decoded not found\n");
        exit(1);
    }

    std::vector<uint8_t> qt(6), fh(6), fv(6), dcid(6), acid(6);
    std::vector<huffman::decoder> dc(4), ac(4);
    size_t ht, wd, itvl;
    uint8_t hmax = 0, vmax = 0;

    bool soi = false;
    bool eoi = false;

    buffer *buf = new buffer(fp);

    while (!eoi) {
        uint8_t byte1 = buf->read_byte();
        uint8_t byte2 = buf->read_byte();

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
                size_t leng = buf->read_bytes<size_t>(2);
                uint8_t prec = buf->read_bytes<uint8_t>(1);
                ht = buf->read_bytes<size_t>(2);
                wd = buf->read_bytes<size_t>(2);

                uint8_t cnt = (uint8_t)fgetc(fp);
                for (int i = 0; i < (int)cnt; ++i) {
                    uint8_t cid = buf->read_byte();
                    uint8_t hor = buf->read_bits<uint8_t>(4);
                    uint8_t ver = buf->read_bits<uint8_t>(4);
                    uint8_t nqt = buf->read_byte();

                    hmax = std::max(hmax, hor);
                    vmax = std::max(vmax, ver);
                    
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
                size_t leng = buf->read_bytes<size_t>(2);
                size_t bytes = 2;
                
                while (bytes < leng) {
                    uint8_t tc = buf->read_bits<uint8_t>(4); // type: 0 for DC and 1 for AC
                    uint8_t th = buf->read_bits<uint8_t>(4);
                    assert(tc < 2);
                    assert(th < 4);
                    bytes++;

                    std::vector<uint8_t> codeword(16);
                    std::vector<std::vector<uint8_t>> symbol(16);

                    for (int i = 0; i < 16; ++i) 
                        codeword[i] = buf->read_byte();
                    
                    bytes += 16;
                    for (int i = 0; i < 16; ++i) {
                        symbol[i].resize(codeword[i]);
                        for (int j = 0; j < codeword[i]; ++j)
                            symbol[i][j] = buf->read_byte();

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
                size_t leng = buf->read_bytes<size_t>(2);
                size_t bytes = 2;

                while (bytes < leng) {
                    uint8_t pq = buf->read_bits<uint8_t>(4);
                    uint8_t tq = buf->read_bits<uint8_t>(4);
                    assert(pq < 2);
                    assert(tq < 4);
                    bytes++;

                    std::vector<std::vector<int>> qtab(8, std::vector<int>(8));
                    
                    for (int k = 0; k < 64; ++k) {
                        uint8_t i = zig[k], j = zag[k];
                        qtab[i][j] = buf->read_bytes<int>(pq + 1);
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
                buf->skip_bytes(2);
                itvl = buf->read_bytes<size_t>(2);
                break;
            }

            case SOS: {
                if (!soi) {
                    fprintf(stderr, "[Error] SOI not found\n");
                    exit(1);
                }
                fprintf(stderr, "[Debug] SOS\n");
                // start of scan
                size_t leng = buf->read_bytes<size_t>(2);
                uint8_t cnt = buf->read_byte();

                fprintf(stderr, "[Debug] SOS cnt = %d\n", (int)cnt);
                fprintf(stderr, "Y: %d %d\n", (int)fh[1], (int)fv[1]);
                fprintf(stderr, "Cb: %d %d\n", (int)fh[2], (int)fv[2]);
                fprintf(stderr, "Cr: %d %d\n", (int)fh[3], (int)fv[3]);

                for (int i = 0; i < (int)cnt; ++i) {
                    uint8_t cs = buf->read_byte();
                    uint8_t ta = buf->read_bits<uint8_t>(4);
                    uint8_t td = buf->read_bits<uint8_t>(4);

                    dcid[cs] = td;
                    acid[cs] = ta;
                }

                buf->skip_bytes(3);

                // TODO: Read MCU
                while (true) {
                    // TODO: this is WRONG
                    uint8_t byte = buf->read_byte();
                    // fprintf(stderr, "[Debug] read 0x%hhx after SOS\n", byte);
                    if (byte == 0xFF) {
                        uint8_t nxt = buf->read_byte();
                        if (nxt != 0x00) {
                            buf->unread(nxt);
                            buf->unread(byte);
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
                size_t leng = buf->read_bytes<size_t>(2);
                uint64_t mark = buf->read_bytes<uint64_t>(5);
                
                if (mark != 0x4A46494600) {
                    fprintf(stderr, "[Error] Expect JFIF\n");
                    exit(1);
                }

                uint16_t version = buf->read_bytes<uint16_t>(2);
                uint8_t unit = buf->read_byte();
                uint16_t x_density = buf->read_bytes<uint16_t>(2);
                uint16_t y_density = buf->read_bytes<uint16_t>(2);
                uint8_t width_t  = buf->read_byte();
                uint8_t height_t = buf->read_byte();

                for (int i = 0; i < (int)width_t; ++i) {
                    for (int j = 0; j < (int)height_t; ++j) {
                        // TODO: make it useful
                        buf->skip_bytes(3);
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
                size_t leng = buf->read_bytes<size_t>(2);

                for (int i = 0; i < (int)leng - 2; ++i) 
                    buf->read_byte();

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
