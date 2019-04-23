#ifndef HUFFMAN_HPP_INCLUDED
#define HUFFMAN_HPP_INCLUDED

#include <algorithm>
#include <array> 
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
        decoder(const std::array<std::vector<uint8_t>, 16> &);

        uint8_t next(buffer *);
    };

    struct encoder {
        uint8_t id;
        int code[256];
        size_t freq[256];
        uint8_t leng[256];
        std::vector<uint8_t> tab[16];

        encoder();
        encoder(uint8_t);

        void add_freq(uint8_t, size_t);
        void encode();
        bool decodable() const;
    };
}

#endif
