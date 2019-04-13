#include <cstdio>

#include "image.hpp"
#include "codec.hpp"
#include "huffman.hpp"

int main(int argc, const char **argv) {
    if (argc < 2) {
        fprintf(stderr, "[Usage] ./encode source [destination]\n");
        exit(1);
    }

}
