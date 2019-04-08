#ifndef HUFFMAN_HPP_INCLUDED
#define HUFFMAN_HPP_INCLUDED

#include <cstdint>
#include <map>
#include <vector>
#include <utility>


namespace huffman {

    struct decoder {
        std::map<std::pair<int, uint8_t>, uint8_t> maps;

        decoder() {}
        decoder(const std::vector<uint8_t> &, 
                const std::vector<std::vector<uint8_t>> &);

        std::vector<uint8_t> operator()(const std::vector<uint8_t> &) const;
    };

    struct encoder {
        // TODO
    };
}

#endif
