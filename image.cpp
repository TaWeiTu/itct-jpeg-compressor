#include "image.hpp"


pixel::pixel() {}

pixel::pixel(uint8_t r, uint8_t g, uint8_t b): r(r), g(g), b(b) {
    y  = (int16_t)(  0 + 0.299    * r + 0.587    * g + 0.114    * b);
    cb = (int16_t)(128 - 0.168736 * r - 0.331264 * g + 0.5      * b);
    cr = (int16_t)(128 + 0.5      * r - 0.418688 * g - 0.081312 * b);
}

pixel::pixel(int16_t y, int16_t cb, int16_t cr): y(y), cb(cb), cr(cr) {
    fprintf(stderr, "[Pixel] r = %d\n", (int)(y + 1.402 * (cr - 128)));
    fprintf(stderr, "[Pixel] g = %d\n", (int)(y - 0.344136 * (cb - 128) - 0.714136 * (cr - 128)));
    fprintf(stderr, "[Pixel] b = %d\n", (int)(y + 1.772 * (cb - 128)));

    r = (uint8_t)(y + 1.402 * (cr - 128));
    g = (uint8_t)(y - 0.344136 * (cb - 128) - 0.714136 * (cr - 128));
    b = (uint8_t)(y + 1.772 * (cb - 128));
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
    
    fprintf(stderr, "topmost = %d leftmost = %d\n", topmost, leftmost);
    for (size_t i = 0; i < Y.size(); ++i) {
        for (size_t j = 0; j < Y.size(); ++j) {
            if (topmost + i >= ht || leftmost + j >= wd) continue;
            add_pixel(topmost + i, leftmost + j, pixel(Y[i][j], Cb[i][j], Cr[i][j]));
        }
    }
}

// image::image() {}

// image::image(const std::string &file) {
    // std::ifstream fl(file.c_str());

    // if (!fl.is_open()) {
        // fprintf(stderr, "[Error] Image not found.\n");
        // exit(1);
    // }

    // std::string version, _;
    // fl >> version >> wd >> ht >> _;

    // if (version != "P6") {
        // fprintf(stderr, "[Error] Wrong version of image.\n");
        // exit(1);
    // }

    // const size_t bufsize = wd * ht * 3;
    // unsigned char *buf = new unsigned char[bufsize];
    
    // fl.get();
    // fl.read((char*)buf, bufsize);

    // pix = new pixel *[ht];
    // for (int i = 0; i < (int)ht; ++i)
        // pix[i] = new pixel[wd];

    // for (int i = 0; i < (int)ht; ++i) {
        // for (int j = 0; j < (int)wd; ++j) {
            // uint8_t r = buf[i * j * 3 + 0];
            // uint8_t g = buf[i * j * 3 + 1];
            // uint8_t b = buf[i * j * 3 + 2];
            // pix[i][j] = pixel(r, g, b);
        // }
    // }

    // delete buf;
// }
