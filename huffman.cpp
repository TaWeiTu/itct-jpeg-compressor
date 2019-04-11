#include "huffman.hpp"


huffman::decoder::decoder(const std::vector<uint8_t> &codeword,
                          const std::vector<std::vector<uint8_t>> &symbol) {
    int mask = 0;
    for (int i = 0; i < 16; ++i) {
        mask = mask << 1;
        for (int j = 0; j < (int)codeword[i]; ++j) {
            maps[std::make_pair(mask, i + 1)] = symbol[i][j];
            mask++;
        }
    }
}

// std::vector<uint8_t> huffman::decoder::operator()(const std::vector<uint8_t> &text) {
    // size_t ptr = 0;
    // int mask = 0;
    // int8_t leng = 0, bpos = 7;

    // std::vector<uint8_t> res;
    // while (!buffer.empty()) {
        // mask = mask << 1 | (buffer.front() >> bpos & 1);

        // if (--bpos < 0) {
            // bpos = 7;
            // buffer.pop();
        // }

        // auto it = maps.find(std::make_pair(mask, leng));

        // if (it != maps.end()) {
            // res.push_back(it->second);
            // leng = 0;
            // mask = 0;
        // }
    // }

    // return res;
// }

// void huffman::decoder::feed(const std::vector<uint8_t> &text) {
    // while (!buffer.empty()) buffer.pop();
    // for (int i = 0; i < (int)text.size(); ++i) 
        // buffer.push(text[i]);
    // bpos_ = 7;
// }

uint8_t huffman::decoder::next(buffer *buf) {
    int mask = 0;
    uint8_t leng = 0;
    while (true) {
        mask = mask << 1 | buf->read_bits(1);
        auto it = maps.find(std::make_pair(mask, leng));

        if (it != maps.end()) 
            return it->second;
    }
    fprintf(stderr, "[Error] Huffman decoder insufficient buffer\n");
    exit(1);
}

// uint16_t huffman::decoder::read_bits(uint8_t s) {
    // uint16_t res = 0;
    // for (int i = 0; i < (int)s; ++i) {
        // if (buffer.empty()) {
            // fprintf(stderr, "[Error] Huffman decoder insufficient buffer\n");
            // exit(1);
        // }
        // res = (uint16_t)(res << 1 | (buffer.front() >> bpos_ & 1));
        // if (--bpos_ < 0) {
            // bpos_ = 7;
            // buffer.pop();
        // }
    // }
    // return res;
// }

// bool huffman::decoder::empty() const {
    // return buffer.empty();
// }
