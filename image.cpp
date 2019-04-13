#include "image.hpp"


pixel::pixel() {}

pixel::pixel(uint8_t r, uint8_t g, uint8_t b): r(r), g(g), b(b) {
    y  = (int16_t)(  0 + 0.299    * r + 0.587    * g + 0.114    * b);
    cb = (int16_t)(128 - 0.168736 * r - 0.331264 * g + 0.5      * b);
    cr = (int16_t)(128 + 0.5      * r - 0.418688 * g - 0.081312 * b);
}

pixel::pixel(int16_t y, int16_t cb, int16_t cr): y(y), cb(cb), cr(cr) {
    int16_t r_ = (int16_t)(y + 1.402 * cr + 128);
    int16_t g_ = (int16_t)(y - 0.344136 * cb - 0.714136 * cr + 128);
    int16_t b_ = (int16_t)(y + 1.772 * cb + 128);
    r_ = std::clamp(r_, (int16_t)0, (int16_t)255);
    g_ = std::clamp(g_, (int16_t)0, (int16_t)255);
    b_ = std::clamp(b_, (int16_t)0, (int16_t)255);
    r = (uint8_t)r_;
    g = (uint8_t)g_;
    b = (uint8_t)b_;
}

image::PPM::PPM(): fp(nullptr) {}

image::PPM::PPM(size_t ht, size_t wd, const char *filename): ht(ht), wd(wd), rptr(0), cptr(0) {
    pix.resize(ht, std::vector<pixel>(wd));
    filled.resize(ht, std::vector<bool>(wd));
    fp = fopen(filename, "wb");

    if (!fp) {
        fprintf(stderr, "[Error] Can't write PPM.\n");
        exit(1);
    }

    fprintf(fp, "P6\n");
    char buf[20];
    sprintf(buf, "%d", (int)wd);
    fprintf(fp, "%s ", buf);
    sprintf(buf, "%d", (int)ht);
    fprintf(fp, "%s\n", buf);
    fprintf(fp, "255\n");
}

void image::PPM::add_pixel(size_t r, size_t c, pixel px) {
    pix[r][c] = px; 
    filled[r][c] = true;
    while ((cptr < wd && rptr < ht) && filled[rptr][cptr]) {
        fwrite(&(pix[rptr][cptr].r), 1, 1, fp);
        fwrite(&(pix[rptr][cptr].g), 1, 1, fp);
        fwrite(&(pix[rptr][cptr].b), 1, 1, fp);

        if (++cptr == wd) {
            cptr = 0;
            rptr++;
        }
    }
}

void image::PPM::add_block(size_t topmost, size_t leftmost, 
                           const std::vector<std::vector<int16_t>> &Y,
                           const std::vector<std::vector<int16_t>> &Cb,
                           const std::vector<std::vector<int16_t>> &Cr) {
    
    for (size_t i = 0; i < Y.size(); ++i) {
        for (size_t j = 0; j < Y.size(); ++j) {
            if (topmost + i >= ht || leftmost + j >= wd) continue;
            add_pixel(topmost + i, leftmost + j, pixel(Y[i][j], Cb[i][j], Cr[i][j]));
        }
    }
}
