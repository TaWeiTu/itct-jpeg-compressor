#ifndef IMAGE_HPP_INCLUDED
#define IMAGE_HPP_INCLUDED

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>


struct pixel {
    bool active;
    uint8_t r, g, b;
    int16_t y, cb, cr;

    pixel();
    pixel(uint8_t, uint8_t, uint8_t);
    pixel(int16_t, int16_t, int16_t);
};

namespace image {

    struct PPM {
        size_t ht, wd, rptr, cptr;
        std::vector<std::vector<pixel>> pix;
        std::vector<std::vector<bool>> filled;
        FILE *fp;

        PPM();
        PPM(size_t, size_t, const char *);

        void add_pixel(size_t, size_t, pixel);
        void add_block(size_t, size_t, const std::vector<std::vector<int16_t>> &,
                                       const std::vector<std::vector<int16_t>> &,
                                       const std::vector<std::vector<int16_t>> &);
    };

}

#endif
