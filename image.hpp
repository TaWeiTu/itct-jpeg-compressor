#ifndef IMAGE_HPP_INCLUDED
#define IMAGE_HPP_INCLUDED

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>


struct pixel {
    uint8_t r, g, b;
    int16_t y, cb, cr;

    pixel();
    pixel(uint8_t, uint8_t, uint8_t);
    pixel(int16_t, int16_t, int16_t);
};

namespace image {

    struct base {
        size_t ht, wd;
        std::vector<std::vector<pixel>> pix;

        base() = default;
        base(size_t, size_t);
        virtual ~base() = default;

        virtual void write(const char*) const {}
        void add_pixel(size_t, size_t, pixel);
        void add_block(size_t, size_t, const std::vector<std::vector<int16_t>> &,
                                       const std::vector<std::vector<int16_t>> &,
                                       const std::vector<std::vector<int16_t>> &);
    };

    struct PPM: base {
        using base::base;
        ~PPM() {}

        void write(const char*) const;
    };

    struct BMP: base {
        using base::base; 
        ~BMP() {}

        void write(const char*) const;
    };

    // PPM *read(const char *);
    // BMP *read(const char *);
}

#endif
