#ifndef HUFFMAN_HPP_INCLUDED
#define HUFFMAN_HPP_INCLUDED

#include <cstdint>
#include <cstdio>
#include <map>
#include <queue>
#include <utility>
#include <vector>

#include "buffer.hpp"


namespace huffman {

    struct decoder {
        std::map<std::pair<int, uint8_t>, uint8_t> maps;

        decoder();
        decoder(const std::vector<std::vector<uint8_t>> &);

        std::vector<uint8_t> operator()(const std::vector<uint8_t> &);
        uint8_t next(buffer *);
    };

    struct encoder {
        int code[256];
        uint8_t leng[256];

        encoder();
        encoder(const std::vector<size_t> &);
    };
}

#endif
