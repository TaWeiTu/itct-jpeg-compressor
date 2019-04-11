#include <cstdio>

#include "image.hpp"


pixel::pixel() {}

pixel::pixel(uint8_t r, uint8_t g, uint8_t b): r(r), g(g), b(b) {
    y  = (int16_t)(  0 + 0.299    * r + 0.587    * g + 0.114    * b);
    cb = (int16_t)(128 - 0.168736 * r - 0.331264 * g + 0.5      * b);
    cr = (int16_t)(128 + 0.5      * r - 0.418688 * g - 0.081312 * b);
}

pixel::pixel(int16_t y, int16_t cb, int16_t cr): y(y), cb(cb), cr(cr) {
    r = (uint8_t)(y + 1.402 * (cr - 128));
    g = (uint8_t)(y - 0.344136 * (cb - 128) - 0.714136 * (cr - 128));
    b = (uint8_t)(y + 1.772 * (cb - 128));
}

image::image() {}

image::image(const std::string &file) {
    std::ifstream fl(file.c_str());

    if (!fl.is_open()) {
        fprintf(stderr, "[Error] Image not found.\n");
        exit(1);
    }

    std::string version, _;
    fl >> version >> wd >> ht >> _;

    if (version != "P6") {
        fprintf(stderr, "[Error] Wrong version of image.\n");
        exit(1);
    }

    const size_t bufsize = wd * ht * 3;
    unsigned char *buf = new unsigned char[bufsize];
    
    fl.get();
    fl.read((char*)buf, bufsize);

    pix = new pixel *[ht];
    for (int i = 0; i < (int)ht; ++i)
        pix[i] = new pixel[wd];

    for (int i = 0; i < (int)ht; ++i) {
        for (int j = 0; j < (int)wd; ++j) {
            uint8_t r = buf[i * j * 3 + 0];
            uint8_t g = buf[i * j * 3 + 1];
            uint8_t b = buf[i * j * 3 + 2];
            pix[i][j] = pixel(r, g, b);
        }
    }

    delete buf;
}
