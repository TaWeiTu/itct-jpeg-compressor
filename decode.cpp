#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <vector>

#include "buffer.hpp"
#include "codec.hpp"
#include "huffman.hpp"
#include "image.hpp"
#include "marker.hpp"
#include "parse.hpp"
#include "zigzag.hpp"


// TODO: Deal with DRI
//


int main(int argc, const char **argv) {
    std::map<std::string, std::string> args = parse(argc, argv);

    FILE *fp = fopen(args["src"].c_str(), "rb");

    if (!fp) {
        fprintf(stderr, "[Error] Image to be decoded not found\n");
        exit(1);
    }

    const char *dest = args["dest"].c_str();

    static uint8_t qt[6], fh[6], fv[6], dcid[6], acid[6];
    huffman::decoder dc[4], ac[4];
    quantizer qtz[4];
    size_t ht = 0, wd = 0, itvl = 0;
    uint8_t hmax = 0, vmax = 0;

    bool soi = false;
    bool eoi = false;

    buffer *buf = new buffer(fp);
    image *img = nullptr;

    while (!eoi) {
        uint8_t byte1 = buf->read_byte();
        uint8_t byte2 = buf->read_byte();

        if (byte1 != 0xFF) {
            fprintf(stderr, "[Error] Wrong format, expect 0xFF\n");
            exit(1);
        }

        switch (byte2) {
            case SOI: {
#ifdef DEBUG
                fprintf(stderr, "[Debug] SOI\n");
#endif
                // start of image
                soi = true;
                break;
            }

            case SOF: {
                if (!soi) {
                    fprintf(stderr, "[Error] SOI not found\n");
                    exit(1);
                }
#ifdef DEBUG
                fprintf(stderr, "[Debug] SOF\n");
#endif
                // start of frame (baseline DCT)
                // size_t leng = buf->read_bytes<size_t>(2);
                // uint8_t prec = buf->read_bytes<uint8_t>(1);
                buf->skip_bytes(3);
                ht = buf->read_bytes<size_t>(2);
                wd = buf->read_bytes<size_t>(2);

                if (args["format"] == "bmp")
                    img = new BMP(ht, wd);
                else
                    img = new PPM(ht, wd);

                uint8_t cnt = (uint8_t)buf->read_byte();
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
#ifdef DEBUG
                fprintf(stderr, "[Debug] DHT\n");
#endif
                // define huffman tables
                size_t leng = buf->read_bytes<size_t>(2);
                size_t bytes = 2;
                
                while (bytes < leng) {
                    uint8_t tc = buf->read_bits<uint8_t>(4); // type: 0 for DC and 1 for AC
                    uint8_t th = buf->read_bits<uint8_t>(4);
                    assert(tc < 2);
                    assert(th < 4);
                    bytes++;

                    std::vector<std::vector<uint8_t>> symbol(16);

                    for (int i = 0; i < 16; ++i) {
                        uint8_t codeword = buf->read_byte();
                        symbol[i].resize(codeword);
                    }
                    
                    bytes += 16;
                    for (int i = 0; i < 16; ++i) {
                        for (int j = 0; j < (int)symbol[i].size(); ++j)
                            symbol[i][j] = buf->read_byte();

                        bytes += (int)symbol[i].size();
                    }

                    // for (int i = 0; i < 16; ++i) {
                        // fprintf(stderr, "codeword[%d] = %d -> ", i, (int)codeword[i]);
                        // for (int j = 0; j < (int)symbol[i].size(); ++j)
                            // fprintf(stderr, "%d ", (int)symbol[i][j]);

                        // fprintf(stderr, "\n");
                    // }

                    if (tc) 
                        ac[th] = huffman::decoder(symbol);
                    else    
                        dc[th] = huffman::decoder(symbol);

#ifdef DEBUG
                    if (tc)
                        fprintf(stderr, "Building ac huffman decoder with ID %d\n", th);
                    else
                        fprintf(stderr, "Building dc huffman decoder with ID %d\n", th);
#endif
                }
                break;
            }

            case DQT: {
                if (!soi) {
                    fprintf(stderr, "[Error] SOI not found\n");
                    exit(1);
                }
#ifdef DEBUG
                fprintf(stderr, "[Debug] DQT\n");
#endif
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
                        qtab[i][j] = buf->read_bytes<int>((uint8_t)(pq + 1));
                        bytes += pq + 1;
                    }
                    
                    qtz[tq] = quantizer(qtab);
                }
                break;
            }

            case DRI: {
                if (!soi) {
                    fprintf(stderr, "[Error] SOI not found\n");
                    exit(1);
                }
                fprintf(stderr, "[Error] Not yet supported\n");
                exit(1);
#ifdef DEBUG
                fprintf(stderr, "[Debug] DRI\n");
#endif
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
#ifdef DEBUG
                fprintf(stderr, "[Debug] SOS\n");
#endif
                // start of scan
                // size_t leng = buf->read_bytes<size_t>(2);
                buf->skip_bytes(2);
                uint8_t cnt = buf->read_byte();

                for (int i = 0; i < (int)cnt; ++i) {
                    uint8_t cs = buf->read_byte();
                    uint8_t td = buf->read_bits<uint8_t>(4);
                    uint8_t ta = buf->read_bits<uint8_t>(4);

                    dcid[cs] = td;
                    acid[cs] = ta;
                }

                buf->skip_bytes(3);

                size_t hf = (ht + (vmax * 8) - 1) / (vmax * 8);
                size_t wf = (wd + (hmax * 8) - 1) / (hmax * 8);

                int16_t last_diff[6] = {0, 0, 0, 0, 0, 0};
                buf->start_read_mcu();

                size_t RSTn = 0;
                for (int row = 0; row < (int)hf; ++row) {
                    for (int col = 0; col < (int)wf; ++col) {
                        const int16_t EMPTY = 32767;
                        std::vector<std::vector<int16_t>> Y(8 * vmax, std::vector<int16_t>(8 * hmax, EMPTY));
                        std::vector<std::vector<int16_t>> Cb(8 * vmax, std::vector<int16_t>(8 * hmax, EMPTY));
                        std::vector<std::vector<int16_t>> Cr(8 * vmax, std::vector<int16_t>(8 * hmax, EMPTY));
                        for (int c = 1; c <= 3; ++c) {
                            for (int i = 0; i < (int)fv[c]; ++i) {
                                for (int j = 0; j < (int)fh[c]; ++j) {
                                    int16_t diff = DPCM::decode(&dc[dcid[c]], buf);
                                    std::vector<std::vector<int16_t>> block = RLC::decode_block(&ac[acid[c]], buf);

                                    int16_t real_diff = (int16_t)(last_diff[c] + diff);
                                    last_diff[c] = (int16_t)(last_diff[c] + diff);
                                    block[0][0] = real_diff;

                                    qtz[qt[c]].dequantize(block);
                                    IDCT(block);

                                    switch (c) {
                                        case 1:
                                            for (int y = 0; y < 8; ++y) {
                                                for (int x = 0; x < 8; ++x)
                                                    Y[i * 8 + y][j * 8 + x] = block[y][x];
                                            }
                                            break;

                                        case 2:
                                            for (int y = 0; y < 8; ++y) {
                                                for (int x = 0; x < 8; ++x) 
                                                    Cb[i * 8 + y][j * 8 + x] = block[y][x];
                                            }
                                            break;

                                        case 3:
                                            for (int y = 0; y < 8; ++y) {
                                                for (int x = 0; x < 8; ++x) 
                                                    Cr[i * 8 + y][j * 8 + x] = block[y][x];
                                            }
                                            break;

                                        default:
                                            fprintf(stderr, "[Error] Unexpected color, expect Y = 1, Cb = 2, Cr = 3\n");
                                            exit(1);
                                    }
                                }
                            }
                        }

                        ++RSTn;
                        if (itvl > 0 && RSTn == itvl) {
                            memset(last_diff, 0, sizeof(last_diff));
                            RSTn = 0;
                        }

                        
                        // fprintf(stderr, "Y.size() = %d\n", (int)Y.size());
                        for (int i = 0; i < (int)Y.size(); ++i) {
                            for (int j = 0; j < (int)Y.size(); ++j) {
                                if (Y[i][j] == EMPTY)
                                    Y[i][j] = Y[i % 8][j % 8];
                                if (Cb[i][j] == EMPTY)
                                    Cb[i][j] = Cb[i % 8][j % 8];
                                if (Cr[i][j] == EMPTY)
                                    Cr[i][j] = Cr[i % 8][j % 8];
                            }
                        }

                        img->add_block(row * (vmax * 8), col * (hmax * 8), Y, Cb, Cr);
                    }
                }

                buf->flush();

#ifdef DEBUG
                fprintf(stderr, "fpos = %d flen = %d\n", (int)buf->fpos, (int)buf->flen);
                fprintf(stderr, "done reading MCU\n");
#endif

                break;
            }

            case APP: {
                if (!soi) {
                    fprintf(stderr, "[Error] SOI not found\n");
                    exit(1);
                }
#ifdef DEBUG
                fprintf(stderr, "[Debug] APP\n");
#endif
                // size_t leng = buf->read_bytes<size_t>(2);
                buf->skip_bytes(2);
                uint64_t mark = buf->read_bytes<uint64_t>(5);
                
                if (mark != 0x4A46494600) {
                    fprintf(stderr, "[Error] Expect JFIF\n");
                    exit(1);
                }

                uint16_t version = buf->read_bytes<uint16_t>(2);
                uint8_t unit = buf->read_byte();
                uint16_t x_density = buf->read_bytes<uint16_t>(2);
                uint16_t y_density = buf->read_bytes<uint16_t>(2);
                // buf->skip_bytes(7);
                fprintf(stderr, "version = %d unit = %d x_density = %d y_density = %d\n", version, unit, x_density, y_density);
                uint8_t width_t  = buf->read_byte();
                uint8_t height_t = buf->read_byte();
                fprintf(stderr, "width_t = %d height_t = %d\n", (int)width_t, (int)height_t);

                
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
#ifdef DEBUG
                fprintf(stderr, "[Debug] COM\n");
#endif
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
#ifdef DEBUG
                fprintf(stderr, "[Debug] EOI\n");
#endif
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
    img->write(dest);
    delete img;

    return 0;
}
