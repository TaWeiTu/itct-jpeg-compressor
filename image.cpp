#include "image.hpp"


uint8_t byte_;
#define WRITE(b) do { byte_ = b; fwrite(&byte_, 1, 1, fp); } while (false)

pixel::pixel() {} pixel::pixel(uint8_t r, uint8_t g, uint8_t b): r(r), g(g), b(b) {
    y  = (int16_t)(+0.299    * r + 0.587    * g + 0.114    * b - 128 + 0.5);
    cb = (int16_t)(-0.168736 * r - 0.331264 * g + 0.5      * b + 0.5);
    cr = (int16_t)(+0.5      * r - 0.418688 * g - 0.081312 * b + 0.5);
}

pixel::pixel(int16_t y, int16_t cb, int16_t cr): y(y), cb(cb), cr(cr) {
    int16_t r_ = (int16_t)(y + 1.402 * cr + 128 + 0.5);
    int16_t g_ = (int16_t)(y - 0.344136 * cb - 0.714136 * cr + 128 + 0.5);
    int16_t b_ = (int16_t)(y + 1.772 * cb + 128 + 0.5);
    r_ = std::clamp(r_, (int16_t)0, (int16_t)255);
    g_ = std::clamp(g_, (int16_t)0, (int16_t)255);
    b_ = std::clamp(b_, (int16_t)0, (int16_t)255);
    r = (uint8_t)r_;
    g = (uint8_t)g_;
    b = (uint8_t)b_;
}

image::image(size_t ht, size_t wd): ht(ht), wd(wd) {
    pix.resize(ht, std::vector<pixel>(wd));
}

void image::add_pixel(size_t r, size_t c, pixel px) {
    pix[r][c] = px; 
}

void image::add_block(size_t topmost, size_t leftmost, 
                      const std::vector<std::vector<int16_t>> &Y,
                      const std::vector<std::vector<int16_t>> &Cb,
                      const std::vector<std::vector<int16_t>> &Cr) {
    
    for (size_t i = 0; i < Y.size(); ++i) {
        for (size_t j = 0; j < Y[0].size(); ++j) {
            if (topmost + i >= ht || leftmost + j >= wd) continue;
            pixel px(Y[i][j], Cb[i][j], Cr[i][j]);
            add_pixel(topmost + i, leftmost + j, px);
        }
    }
}

std::array<std::array<int16_t, 8>, 8> image::Y_block(size_t topmost, size_t leftmost) const {
    std::array<std::array<int16_t, 8>, 8> block;

    for (size_t i = topmost, ci = 0; ci < 8; ++i, ++ci) {
        for (size_t j = leftmost, cj = 0; cj < 8; ++j, ++cj) {
            if (i < ht && j < wd)
                block[ci][cj] = pix[i][j].y;
        }
    }
    return block;
}

std::array<std::array<int16_t, 8>, 8> image::Cb_block(size_t topmost, size_t leftmost) const {
    std::array<std::array<int16_t, 8>, 8> block;

    for (size_t i = topmost, ci = 0; ci < 8; ++i, ++ci) {
        for (size_t j = leftmost, cj = 0; cj < 8; ++j, ++cj) {
            if (i < ht && j < wd)
                block[ci][cj] = pix[i][j].cb;
        }
    }
    return block;
}

std::array<std::array<int16_t, 8>, 8> image::Cr_block(size_t topmost, size_t leftmost) const {
    std::array<std::array<int16_t, 8>, 8> block;

    for (size_t i = topmost, ci = 0; ci < 8; ++i, ++ci) {
        for (size_t j = leftmost, cj = 0; cj < 8; ++j, ++cj) {
            if (i < ht && j < wd)
                block[ci][cj] = pix[i][j].cr;
        }
    }
    return block;
}

void PPM::write(const char *filename) const {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "[Error] Unable to open file %s\n", filename);
        exit(1);
    }

    fprintf(fp, "P6\n");
    fprintf(fp, "%d %d\n", (int)wd, (int)ht);
    fprintf(fp, "255\n");

    for (int i = 0; i < (int)ht; ++i) {
        for (int j = 0; j < (int)wd; ++j) {
            WRITE(pix[i][j].r);
            WRITE(pix[i][j].g);
            WRITE(pix[i][j].b);
        }
    }
    fclose(fp);
}

void BMP::write(const char *filename) const {
    FILE *fp = fopen(filename, "wb"); 
    if (!fp) { 
        fprintf(stderr, "[Error] Can't write BMP\n");
        exit(1);
    }

#ifdef DEBUG
    fprintf(stdout, "height = %d width = %d\n", (int)ht, (int)wd);
#endif

    size_t padded = (3 * wd + 3) / 4 * 4;
    size_t size = 14 + 12 + ht * padded;
    size_t offset = 14 + 12;

    // Bitmap file header
    WRITE(0x42); WRITE(0x4D);
    WRITE(size & 255); WRITE(size >> 8 & 255); WRITE(size >> 16 & 255); WRITE(size >> 24 & 255);
    WRITE(0x00); WRITE(0x00); WRITE(0x00); WRITE(0x00);
    WRITE(offset & 255); WRITE(offset >> 8 & 255); WRITE(offset >> 16 & 255); WRITE(offset >> 24 & 255);

    // DIB header
    WRITE(0x0C); WRITE(0x00); WRITE(0x00); WRITE(0x00);
    WRITE(wd & 255); WRITE(wd >> 8 & 255);
    WRITE(ht & 255); WRITE(ht >> 8 & 255);
    WRITE(0x01); WRITE(0x00);
    WRITE(0x18); WRITE(0x00);

#ifdef DEBUG
    fprintf(stdout, "padded = %d\n", (int)padded);
#endif

    for (int i = (int)ht - 1; i >= 0; --i) {
        size_t bytes = 0;
        for (int j = 0; j < (int)wd; ++j) {
            WRITE(pix[i][j].b);
            WRITE(pix[i][j].g);
            WRITE(pix[i][j].r);
            bytes += 3;
        }

        while (bytes < padded) {
            WRITE(0x00);
            bytes++;
        }
    }

    fclose(fp);
}

void PPM::read(const char *filename) {
    FILE *fp = fopen(filename, "rb"); 
    if (!fp) {
        fprintf(stderr, "[Error] Unable to read file %s\n", filename);
        exit(1);
    }

    char header[20];
    int maxval, ht_, wd_;
    fscanf(fp, "%s\n%d %d\n%d\n", header, &wd_, &ht_, &maxval);

    ht = ht_, wd = wd_;
    pix.resize(ht, std::vector<pixel>(wd));

    for (int i = 0; i < (int)ht; ++i) {
        for (int j = 0; j < (int)wd; ++j) {
            uint8_t r = (uint8_t)fgetc(fp);
            uint8_t g = (uint8_t)fgetc(fp);
            uint8_t b = (uint8_t)fgetc(fp);
            pix[i][j] = pixel(r, g, b);
        }
    }
    fclose(fp);
} 

void BMP::read(const char *filename) {

}
