#ifndef HUFFMAN_HPP_INCLUDED
#define HUFFMAN_HPP_INCLUDED

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <map>
#include <queue>
#include <utility>
#include <vector>
#include "buffer.hpp"


namespace huffman {

    struct decoder {
        std::map<std::pair<int, uint8_t>, uint8_t> maps;

        decoder() = default;
        decoder(const std::vector<std::vector<uint8_t>> &);

        std::vector<uint8_t> operator()(const std::vector<uint8_t> &);
        uint8_t next(buffer *);
    };

    struct encoder {
        int code[512];
        size_t freq[512];
        uint8_t leng[512];

        encoder();
        encoder(const std::vector<size_t> &);

        void add_freq(uint8_t, size_t);
        void encode();
        bool decodable() const;
    };
}

#endif
