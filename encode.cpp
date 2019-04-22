#include <cstdio>
#include <map>
#include <string>

#include "image.hpp"
#include "codec.hpp"
#include "huffman.hpp"
#include "parse.hpp"
#include "marker.hpp"

const uint8_t acid[3] = {0, 1, 1};
const uint8_t dcid[3] = {2, 3, 3};
const uint8_t qtid[3] = {0, 1, 1};

void write_jpeg(buffer *buf, uint8_t marker, void *ptr = nullptr) {
    // write jpeg marker
    buf->write_byte(0xFF);
    buf->write_byte(marker);

    switch (marker) {
        case SOI: 
        case EOI:
            break;

        case APP: {
            uint64_t mark = 0x4A46494600;
            uint16_t version = 257;
            uint8_t unit = 0;
            uint16_t x_density = 1;
            uint16_t y_density = 1;
            uint8_t ht = 0, wd = 0;
            size_t leng = 2 + 5 + 2 + 1 + 2 + 2 + 1 + 1;
            buf->write_bytes(leng, 2);
            buf->write_bytes(mark, 5);
            buf->write_bytes(version, 2);
            buf->write_bytes(unit, 1);
            buf->write_bytes(x_density, 2);
            buf->write_bytes(y_density, 2);
            buf->write_bytes(ht, 1);
            buf->write_bytes(wd, 1);
            break;
        }

        case DQT: {
            quantizer *obj = (quantizer *)ptr;
            size_t leng = 2 + 1 + 64;
            uint8_t pq = 0;
            uint8_t tq = obj->id;
            buf->write_bytes(leng, 2);
            buf->write_bits(pq, 4);
            buf->write_bits(tq, 4);

            for (int k = 0; k < 64; ++k) 
                buf->write_byte((uint8_t)obj->qtable[zig[k]][zag[k]]);

            break;
        }

        case DHT: {
            huffman::encoder *obj = (huffman::encoder *)ptr;
            uint8_t tc = obj->id < 2;
            uint8_t th = obj->id;

            size_t leng = 2 + 1 + 16;
            for (int i = 0; i < 16; ++i)
                leng += obj->tab[i].size();

            buf->write_bytes(leng, 2);
            buf->write_bits(tc, 4);
            buf->write_bits(th, 4);
            for (int i = 0; i < 16; ++i)
                buf->write_bytes(obj->tab[i].size(), 1);

            for (int i = 0; i < 16; ++i) {
                for (int j = 0; j < (int)obj->tab[i].size(); ++j)
                    buf->write_byte(obj->tab[i][j]);
            }
            break;
        }

        case SOF: {
            image *img = (image *)ptr;
            size_t leng = 2 + 1 + 2 + 2 + 1 + 3 * 3;
            uint8_t prec = 8;
            size_t ht = img->ht;
            size_t wd = img->wd;
            uint8_t cnt = 3;

            buf->write_bytes(leng, 2);
            buf->write_bytes(prec, 1);
            buf->write_bytes(ht, 2);
            buf->write_bytes(wd, 2);
            buf->write_bytes(cnt, 1);

            for (int i = 0; i < 3; ++i) {
                uint8_t cid = (uint8_t)(i + 1);
                uint8_t hor = fh[i];
                uint8_t ver = fv[i];
                uint8_t nqt = qtid[i];
                buf->write_byte(cid);
                buf->write_bits(hor, 4);
                buf->write_bits(ver, 4);
                buf->write_byte(nqt);
            }
            break;
        }

        case SOS: {
            size_t leng = 2 + 1 + 3 * 2 + 3;
            uint8_t cnt = 3; 
            buf->write_bytes(leng, 2);
            buf->write_byte(cnt);
            for (int i = 0; i < 3; ++i) {
                uint8_t cs = (uint8_t)(i + 1);
                uint8_t td = dcid[i];
                uint8_t ta = acid[i];
                buf->write_byte(cs);
                buf->write_bits(td, 4);
                buf->write_bits(ta, 4);
            }

            buf->write_byte(0x00);
            buf->write_byte(0x3F);
            buf->write_byte(0x00);
            break;
        }

        default:
            fprintf(stderr, "[Error] Unknown marker\n");
            exit(1);
    }
}


int main(int argc, const char **argv) {
    std::map<std::string, std::string> args = parse(argc, argv);

    image *img = nullptr;
    if (args["format"] == "bmp")
        img = new BMP();
    else
        img = new PPM();

    img->read(args["src"].c_str());

    uint8_t vmax = *std::max_element(fv, fv + 3);
    uint8_t hmax = *std::max_element(fh, fh + 3);
    size_t block_per_mcu = 0;
    for (int i = 0; i < 3; ++i)
        block_per_mcu += (size_t)(fv[i] * fh[i]);

    size_t hf = (img->ht + (vmax * 8) - 1) / (vmax * 8);
    size_t wf = (img->wd + (hmax * 8) - 1) / (hmax * 8);

    huffman::encoder huff[4];
    for (int i = 0; i < 4; ++i)
        huff[i] = huffman::encoder((uint8_t)i);

    quantizer qtz[2] = {luminance(0), chrominance(1)};
    // quantizer qtz[2] = {dummy(0), dummy(1)};

    using block_type = std::vector<std::array<std::array<int16_t, 8>, 8>>;
    std::vector<std::vector<block_type>> blk(hf, std::vector<block_type>(wf));

    int16_t last[3] = {0, 0, 0};

    for (int i = 0; i < (int)hf; ++i) {
        for (int j = 0; j < (int)wf; ++j) {
            blk[i][j].resize(block_per_mcu);
            for (int c = 0, p = 0; c < 3; ++c) {
                for (int x = 0; x < (int)fv[c]; ++x) {
                    for (int y = 0; y < (int)fh[c]; ++y) {
                        std::array<std::array<int16_t, 8>, 8> block = 
                            c == 0 ? img->Y_block(i * vmax * 8 + x * 8, j * hmax * 8 + y * 8) : 
                            c == 1 ? img->Cb_block(i * vmax * 8 + x * 8, j * hmax * 8 + y * 8) :
                            img->Cr_block(i * vmax * 8 + x * 8, j * hmax * 8 + y * 8);
                        blk[i][j][p] = qtz[qtid[c]].quantize(FDCT(block));
                        // for (int x = 0; x < 8; ++x) {
                            // for (int y = 0; y < 8; ++y)
                                // printf("%d ", (x == 0 && y == 0 ? 0 : (int)blk[i][j][p][x][y]));
                            // printf("\n");
                        // }
                        std::vector<std::pair<uint8_t, int16_t>> RLP = RLC::encode_block(blk[i][j][p]);
                        for (int k = 0; k < (int)RLP.size(); ++k) {
                            uint8_t r = RLP[k].first;
                            int16_t v = RLP[k].second;
                            uint8_t s = (v == 0 ? 0 : (uint8_t)(32 - __builtin_clz(abs(v))));
                            huff[acid[c]].add_freq((uint8_t)(r << 4 | s), 1);
                        }
                        uint8_t s = (blk[i][j][p][0][0] - last[c] == 0 ? 0 : (uint8_t)(32 - __builtin_clz(abs(blk[i][j][p][0][0] - last[c]))));
                        huff[dcid[c]].add_freq(s, 1);
                        last[c] = blk[i][j][p][0][0];
                        p++;
                    }
                }
            }
        }
    }

    for (int i = 0; i < 4; ++i)
        huff[i].encode();

#ifdef DEBUG
    fprintf(stderr, "done all encoding!\n");
#endif

    buffer *buf = new buffer(fopen(args["dest"].c_str(), "wb"));
    write_jpeg(buf, SOI);
    write_jpeg(buf, APP);
    for (int i = 0; i < 2; ++i)
        write_jpeg(buf, DQT, &qtz[i]);

    write_jpeg(buf, SOF, img);
    for (int i = 0; i < 4; ++i)
        write_jpeg(buf, DHT, &huff[i]);

    write_jpeg(buf, SOS);
    memset(last, 0, sizeof(last));
    
    buf->start_processing_mcu();

    for (int i = 0; i < (int)hf; ++i) {
        for (int j = 0; j < (int)wf; ++j) {
            for (int c = 0, p = 0; c < 3; ++c) {
                for (int x = 0; x < (int)fv[c]; ++x) {
                    for (int y = 0; y < (int)fh[c]; ++y) {
                        int16_t dc = blk[i][j][p][0][0];
                        uint8_t s = (dc - last[c] == 0 ? 0 : (uint8_t)(32 - __builtin_clz(abs(dc - last[c]))));

                        buf->write_bits(huff[dcid[c]].code[s], huff[dcid[c]].leng[s]);
                        uint8_t diff = 0;
                        if (dc != last[c]) {
                            if (dc < last[c])
                                diff = (uint8_t)((1 << s) - 1 - (last[c] - dc));
                            else
                                diff = (uint8_t)(dc - last[c]);
                        }
                        buf->write_bits(diff, s);
                        last[c] = dc;

                        std::vector<std::pair<uint8_t, int16_t>> RLP = RLC::encode_block(blk[i][j][p]);
                        for (int k = 0; k < (int)RLP.size(); ++k) {
                            uint8_t r = RLP[k].first;
                            int16_t v = RLP[k].second;
                            uint8_t s = (v == 0 ? 0 : (uint8_t)(32 - __builtin_clz(abs(v))));
                            uint8_t diff = 0;
                            if (v != 0) {
                                if (v < 0)
                                    diff = (uint8_t)((1 << s) - 1 + v);
                                else
                                    diff = (uint8_t)v;
                            }
                            buf->write_bits(huff[acid[c]].code[r << 4 | s], huff[acid[c]].leng[r << 4 | s]);
                            buf->write_bits(diff, s);
                        }
                        p++;
                    }
                }
            }
        }
    }

    buf->end_processing_mcu();
    buf->finish();
    write_jpeg(buf, EOI);
    delete img;
    delete buf;

    return 0;
}
