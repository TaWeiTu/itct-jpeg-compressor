#include "huffman.hpp"


huffman::decoder::decoder() {
    maps.clear();
}

huffman::decoder::decoder(const std::vector<uint8_t> &codeword,
                          const std::vector<std::vector<uint8_t>> &symbol) {
    maps.clear();
    int mask = 0;
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < (int)codeword[i]; ++j) {
            maps[std::make_pair(mask, i + 1)] = symbol[i][j];
            mask++;
        }
        mask <<= 1;
    }

    // for (auto it : maps) {
        // fprintf(stderr, "[huffman::decoder] %04X (%d) -> 0x%02X\n", it.first.first, it.first.second, it.second);
    // }
}

uint8_t huffman::decoder::next(buffer *buf) {
    // fprintf(stderr, "[huffman::decoder::next] Start\n");
    int mask = 0;
    uint8_t leng = 0;
    while (true) {
        /* if (buf->fpeek() == EOF) {
            fprintf(stderr, "[Error] EOF\n");
            exit(1);
        } */
        mask = (mask << 1 | buf->read_bits(1));
        ++leng;
        auto it = maps.find(std::make_pair(mask, leng));

        if (it != maps.end()) {
            // fprintf(stderr, "[huffman::decoder::next] Successfully decode %04X (%d) -> %02X\n", mask, (int)leng, it->second);
            return it->second;
        }

        if (leng > 16) {
            fprintf(stderr, "Codelength too long\n");
            exit(1);
        }
    }
    fprintf(stderr, "[Error] Huffman decoder insufficient buffer\n");
    exit(1);
}
