#include <cstdio>
#include <map>
#include <string>

#include "image.hpp"
#include "codec.hpp"
#include "huffman.hpp"
#include "parse.hpp"
#include "marker.hpp"


int main(int argc, const char **argv) {
    std::map<std::string, std::string> args = parse(argc, argv);

    image *img = nullptr;
    if (args["format"] == "bmp")
        img = new BMP();
    else
        img = new PPM();

    img->read(args["src"].c_str());


    delete img;
    return 0;
}
