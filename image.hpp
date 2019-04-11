#ifndef IMAGE_HPP_INCLUDED
#define IMAGE_HPP_INCLUDED

#include <string>
#include <fstream>


struct pixel {
    uint8_t r, g, b;
    int16_t y, cb, cr;

    pixel();
    pixel(uint8_t, uint8_t, uint8_t);
    pixel(int16_t, int16_t, int16_t);
};

struct image {
    pixel **pix;
    size_t wd, ht;

    image();
    image(const std::string &);
};


#endif
