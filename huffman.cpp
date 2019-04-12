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
}

uint8_t huffman::decoder::next(buffer *buf) {
    // fprintf(stderr, "[huffman::decoder::next] Start\n");
    int mask = 0;
    uint8_t leng = 0;
    while (true) {
        mask = (mask << 1 | buf->read_bits(1));
        ++leng;
        auto it = maps.find(std::make_pair(mask, leng));

        if (it != maps.end()) 
            return it->second;

        if (leng > 16) {
            fprintf(stderr, "Codelength too long\n");
            exit(1);
        }
    }
    fprintf(stderr, "[Error] Huffman decoder insufficient buffer\n");
    exit(1);
}
