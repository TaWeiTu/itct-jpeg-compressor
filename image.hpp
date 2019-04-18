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

struct image {
    size_t ht, wd;
    std::vector<std::vector<pixel>> pix;

    image() = default;
    image(size_t, size_t);
    virtual ~image() = default;

    virtual void write(const char*) const {}
    virtual void read(const char*) {};
    void add_pixel(size_t, size_t, pixel);
    void add_block(size_t, size_t, const std::vector<std::vector<int16_t>> &,
                                   const std::vector<std::vector<int16_t>> &,
                                   const std::vector<std::vector<int16_t>> &);

    std::vector<std::vector<int16_t>> Y_block(size_t, size_t, size_t) const;
    std::vector<std::vector<int16_t>> Cb_block(size_t, size_t, size_t) const;
    std::vector<std::vector<int16_t>> Cr_block(size_t, size_t, size_t) const;
};

struct PPM: image {
    using image::image;
    ~PPM() {}

    void write(const char*) const;
    void read(const char*);
};

struct BMP: image {
    using image::image; 
    ~BMP() {}

    void write(const char*) const;
    void read(const char*);
};


#endif
