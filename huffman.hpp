#ifndef HUFFMAN_HPP_INCLUDED
#define HUFFMAN_HPP_INCLUDED

#include <cstdint>
#include <cstdio>
#include <map>
#include <vector>
#include <utility>
#include <queue>

#include "buffer.hpp"


namespace huffman {

    struct decoder {
        std::map<std::pair<int, uint8_t>, uint8_t> maps;
        // std::queue<uint8_t> buffer;
        // FILE *fp;
        // int8_t *bpos_;

        decoder() {}

        decoder(const std::vector<uint8_t> &, 
                const std::vector<std::vector<uint8_t>> &);

        std::vector<uint8_t> operator()(const std::vector<uint8_t> &);

        void feed(const std::vector<uint8_t> &);
        uint8_t next(buffer *);
        uint16_t read_bits(uint8_t);
        bool empty() const;
        void append();
        size_t size() const;
    };

    struct encoder {
        // TODO
    };
}

#endif
