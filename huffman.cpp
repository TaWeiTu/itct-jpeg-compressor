#include "huffman.hpp"


huffman::decoder::decoder() {
    maps.clear();
}

huffman::decoder::decoder(const std::vector<std::vector<uint8_t>> &symbol) {
    maps.clear();
    int mask = 0;
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < (int)symbol[i].size(); ++j) {
            maps[std::make_pair(mask, i + 1)] = symbol[i][j];
            mask++;
        }
        mask <<= 1;
    }
}

uint8_t huffman::decoder::next(buffer *buf) {
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

huffman::encoder::encoder() {}

// huffman::encoder(const std::vector<size_t> &freq) {
    // std::priority_queue<std::pair<size_t, int>, std::vector<std::pair<size_t, int>>, std::greater<std::pair<size_t, int>>> pq;

    // assert((int)freq.size() == 256);
    // for (int i = 0; i < 256; ++i) pq.emplace(freq[i], i);
    // std::vector<int> lc(), rc();

    // while (!pq.empty()) {
        // int x = 
    // }
// }
