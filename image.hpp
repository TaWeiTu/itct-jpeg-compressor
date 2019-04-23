#ifndef IMAGE_HPP_INCLUDED
#define IMAGE_HPP_INCLUDED

#include <algorithm>
#include <array>
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
    FILE *fp;
    std::vector<std::vector<pixel>> pix;
    static const size_t SIZE = 65536;
    uint8_t buffer[SIZE];
    uint8_t *ptr = buffer;

    image() = default;
    image(size_t, size_t);
    virtual ~image() = default;

    virtual void write(const char*) {}
    virtual void read(const char*) {};
    void add_pixel(size_t, size_t, pixel);
    void add_block(size_t, size_t, const std::vector<std::vector<int16_t>> &,
                                   const std::vector<std::vector<int16_t>> &,
                                   const std::vector<std::vector<int16_t>> &);

    std::array<std::array<int16_t, 8>, 8>  Y_block(size_t, size_t, size_t, size_t) const;
    std::array<std::array<int16_t, 8>, 8> Cb_block(size_t, size_t, size_t, size_t) const;
    std::array<std::array<int16_t, 8>, 8> Cr_block(size_t, size_t, size_t, size_t) const;

    void WRITE(uint8_t);
    uint8_t READ();
};

struct PPM: image {
    using image::image;
    ~PPM() {
        if (ptr != buffer)
            fwrite(buffer, 1, ptr - buffer, fp);
    }

    void write(const char*);
    void read(const char*);
};

struct BMP: image {
    using image::image; 
    ~BMP() {
        if (ptr != buffer)
            fwrite(buffer, 1, ptr - buffer, fp);
        fclose(fp);
    }

    void write(const char*);
    void read(const char*);
};


#endif
