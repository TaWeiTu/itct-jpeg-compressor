#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <chrono>
#include <ctime>
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

#ifdef DEBUG
FILE *dbg = fopen("decode.out", "w");

void debug(std::array<std::array<int16_t, 8>, 8> block) {
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            fprintf(dbg, "%d ", (int)block[i][j]);
    }
    fprintf(dbg, "\n");
}
#else
void debug(...) {

}
#endif

int main(int argc, const char **argv) {
    std::map<std::string, std::string> args = parse(argc, argv);

    FILE *fp = fopen(args["src"].c_str(), "rb");

    if (!fp) {
        fprintf(stderr, "[Error] Image to be decoded not found\n");
        exit(1);
    }

    const char *dest = args["dest"].c_str();

    // quantization table id, horizontal sampling rate, vertical sampling rate
    // DC/AC huffman table id
    static uint8_t qt[6], fh[6], fv[6], dcid[6], acid[6];
    huffman::decoder dc[4], ac[4];
    quantizer qtz[4];

    // height and width of the image, reset interval
    size_t ht = 0, wd = 0, itvl = 0;
    uint8_t hmax = 0, vmax = 0;

    bool soi = false;
    bool eoi = false;

    buffer *buf = new buffer(fp);
    image *img = nullptr;

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    while (!eoi) {
        // reading marker
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
                buf->skip_bytes(3);
                ht = buf->read_bytes<size_t>(2);
                wd = buf->read_bytes<size_t>(2);

                if (args["format"] == "bmp")
                    img = new BMP(ht, wd);
                else
                    img = new PPM(ht, wd);

                uint8_t cnt = (uint8_t)buf->read_byte();
                for (int i = 0; i < (int)cnt; ++i) {
                    uint8_t cid = buf->read_byte(); // color ID
                    uint8_t hor = buf->read_bits<uint8_t>(4); // horizontal subsampling rate
                    uint8_t ver = buf->read_bits<uint8_t>(4); // vertical subsampling rate
                    uint8_t nqt = buf->read_byte(); // quantization table ID

                    hmax = std::max(hmax, hor);
                    vmax = std::max(vmax, ver);
                    
                    qt[cid] = nqt;
                    fh[cid] = hor;
                    fv[cid] = ver;
                    fprintf(stderr, "qt[%d] = %d fh[%d] = %d fv[%d] = %d\n", cid, (int)qt[cid], cid, (int)fh[cid], cid, (int)fv[cid]);
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
                    uint8_t th = buf->read_bits<uint8_t>(4); // huffman table ID
                    assert(tc < 2);
                    assert(th < 4);
                    bytes++;

                    std::array<std::vector<uint8_t>, 16> symbol;

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

                    std::array<std::array<int, 8>, 8> qtab;
                    
                    for (int k = 0; k < 64; ++k) {
                        uint8_t i = zig[k], j = zag[k];
                        qtab[i][j] = buf->read_bytes<int>((uint8_t)(pq + 1));
                        bytes += pq + 1;
                    }
                    
                    qtz[tq] = quantizer(std::move(qtab));
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

                size_t hf = (ht + (vmax * 8) - 1) / (vmax * 8);
                size_t wf = (wd + (hmax * 8) - 1) / (hmax * 8);

                // printf("hf = %d wf = %d\n", (int)hf, (int)wf);
                buf->start_processing_mcu();
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

                int16_t last[6];
                memset(last, 0, sizeof(last));

                size_t RSTn = 0;
                const int16_t EMPTY = 32767;
                std::vector<std::vector<int16_t>>  Y(8 * vmax, std::vector<int16_t>(8 * hmax, EMPTY));
                std::vector<std::vector<int16_t>> Cb(8 * vmax, std::vector<int16_t>(8 * hmax, EMPTY));
                std::vector<std::vector<int16_t>> Cr(8 * vmax, std::vector<int16_t>(8 * hmax, EMPTY));

                for (int row = 0; row < (int)hf; ++row) {
                    for (int col = 0; col < (int)wf; ++col) {
                        std::fill( Y.begin(),  Y.end(), std::vector<int16_t>(8 * hmax, EMPTY));
                        std::fill(Cb.begin(), Cb.end(), std::vector<int16_t>(8 * hmax, EMPTY));
                        std::fill(Cr.begin(), Cr.end(), std::vector<int16_t>(8 * hmax, EMPTY));
                        for (int c = 1; c <= 3; ++c) {
                            for (int i = 0; i < (int)fv[c]; ++i) {
                                for (int j = 0; j < (int)fh[c]; ++j) {
                                    int16_t diff = DPCM::decode(&dc[dcid[c]], buf);
                                    int16_t dc = (int16_t)(last[c] + diff);
                                    // printf("%d\n", (int)last[c]);
                                    // printf("%d\n", (int)diff);
                                    // printf("%d\n", (int)dc);
                                    std::array<std::array<int16_t, 8>, 8> block = RLC::decode_block(&ac[acid[c]], buf);
                                    block[0][0] = last[c] = dc;
                                    /* for (int y = 0; y < 8; ++y) {
                                        for (int x = 0; x < 8; ++x)
                                            printf("%d ", (int)block[y][x]);
                                        puts("");
                                    } */
                                    qtz[qt[c]].dequantize(block);
                                    IDCT(block);
                                    // debug(block);

                                    std::vector<std::vector<int16_t>> *dest = c == 1 ? &Y : c == 2 ? &Cb : &Cr;
                                    for (int y = 0; y < 8; ++y) {
                                        for (int x = 0; x < 8; ++x) {
                                            for (int p = 0; p < vmax / fv[c]; ++p) {
                                                for (int q = 0; q < hmax / fh[c]; ++q)
                                                    (*dest)[i * 8 + y * vmax / fv[c] + p][j * 8 + x * hmax / fh[c] + q] = block[y][x];
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        ++RSTn;
                        if (itvl > 0 && RSTn == itvl) {
                            memset(last, 0, sizeof(last));
                            RSTn = 0;
                        }
                        img->add_block(row * (vmax * 8), col * (hmax * 8), Y, Cb, Cr);
                    }
                }

                buf->end_processing_mcu();
                buf->flush();

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
                buf->skip_bytes(2);
                uint64_t mark = buf->read_bytes<uint64_t>(5);
                
                if (mark != 0x4A46494600) {
                    fprintf(stderr, "[Error] Expect JFIF\n");
                    exit(1);
                }

#ifdef DEBUG
                uint16_t version = buf->read_bytes<uint16_t>(2);
                uint8_t unit = buf->read_byte();
                uint16_t x_density = buf->read_bytes<uint16_t>(2);
                uint16_t y_density = buf->read_bytes<uint16_t>(2);
                fprintf(stderr, "version = %d unit = %d x_density = %d y_density = %d\n", 
                        (int)version, (int)unit, (int)x_density, (int)y_density);
#else
                buf->skip_bytes(7);
#endif
                uint8_t width_t  = buf->read_byte();
                uint8_t height_t = buf->read_byte();
                fprintf(stderr, "width = %d height = %d\n", (int)width_t, (int)height_t);

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
                    buf->skip_byte();

                break;
            }

            case EOI: {
                if (!soi) {
                    fprintf(stderr, "[Error] SOI not found\n");
                    exit(1);
                }
                // end of image
                eoi = true;
                break;
            }

            case 0xE1 ... 0xEF: {
                fprintf(stderr, "[Warning] Unsupported marker (ignored): APPn\n");
                size_t leng = buf->read_bytes<size_t>(2);
                for (int i = 0; i < (int)leng - 2; ++i)
                    buf->skip_byte();
                break;
            }

            default: 
                fprintf(stderr, "[Error] Wrong format, unexpected byte 0x%hhx\n", byte2);
                exit(1);
        }
    }
    std::chrono::high_resolution_clock::time_point stop = std::chrono::high_resolution_clock::now();
    fprintf(stderr, "[Info] Time elapsed: %d (ms)\n", (int)std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count());

    fclose(fp);
    img->write(dest);
    delete img;

    return 0;
}
