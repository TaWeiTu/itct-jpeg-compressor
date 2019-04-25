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
        std::array<uint8_t, 1 << 16> leng{};
        std::array<uint8_t, 1 << 16> code{};

        decoder() = default;
        decoder(const std::array<std::vector<uint8_t>, 16> &);

        uint8_t next(buffer *);
    };

    struct encoder {
        uint8_t id;
        std::array<int, 256> code{};
        std::array<size_t, 256> freq{};
        std::array<uint8_t, 256> leng{};
        std::vector<uint8_t> tab[16];

        encoder() = default;
        encoder(uint8_t);

        void add_freq(uint8_t, size_t);
        void encode();
        bool decodable() const;
    };
}

#endif
