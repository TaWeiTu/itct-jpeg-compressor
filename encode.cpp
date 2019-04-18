#include <cstdio>
#include <map>
#include <string>

#include "image.hpp"
#include "codec.hpp"
#include "huffman.hpp"
#include "parse.hpp"
#include "marker.hpp"


int main(int argc, const char **argv) {
    std::map<std::string, std::string> args = parse(argc, argv);

    image *img = nullptr;
    if (args["format"] == "bmp")
        img = new BMP();
    else
        img = new PPM();

    img->read(args["src"].c_str());
    fprintf(stderr, "source = %s\n", args["src"].c_str());
    size_t vbk = (img->ht + 7) / 8;
    size_t hbk = (img->wd + 7) / 8; 
    fprintf(stderr, "vbk = %d hbk = %d\n", (int)vbk, (int)hbk);

    huffman::encoder huff[4];
    int acid[3] = {0, 1, 1};
    int dcid[3] = {2, 3, 3};

    for (int i = 0; i < (int)vbk; ++i) {
        for (int j = 0; j < (int)hbk; ++j) {
            std::vector<std::vector<int16_t>> block[3] = {
                img->Y_block(i * 8, j * 8, 8),
                img->Cb_block(i * 8, j * 8, 8),
                img->Cr_block(i * 8, j * 8, 8)
            };
            for (int c = 0; c < 3; ++c) {
                FDCT(block[c]);
                std::vector<std::pair<uint8_t, int16_t>> RLP = RLC::encode_block(block[c]);
                for (int k = 0; k < (int)RLP.size(); ++k) {
                    uint8_t r = RLP[k].first;
                    int16_t v = RLP[k].second;
                    uint8_t s = (v == 0 ? 0 : (uint8_t)(32 - __builtin_clz(abs(v))));
#ifdef DEBUG
                    fprintf(stderr, "[Debug] var = %d s = %d\n", (int)v, (int)s);
#endif

                    huff[acid[c]].add_freq((uint8_t)(r << 4 | s), 1);
                }
            }

        }
    }

    huff[0].encode();
    huff[1].encode();

    delete img;
    return 0;
}
