#ifndef PARSE_HPP_INCLUDED
#define PARSE_HPP_INCLUDED


#include <array>
#include <map>
#include <string>

void usage() {
    fprintf(stderr, "[Usage] ./decode [-f format=bmp] -s source [-d destination=output.bmp]\n");
    fprintf(stderr, "[Usage] ./encode [-f format=bmp] -s source [-d destination=output.bmp] [-p subsampling=0]\n");
    exit(1);
}

std::array<uint8_t, 3> fh = {1, 1, 1};
std::array<uint8_t, 3> fv = {1, 1, 1};

std::map<std::string, std::string> parse(int argc, const char **argv) {
    std::map<std::string, std::string> res = {
        {"dest", "output.bmp"},
        {"format", "bmp"},
    };

    static std::map<std::string, int> cmd = {
        {"-f", 1},
        {"-s", 2},
        {"-d", 3},
        {"-p", 4}
    };

    for (int i = 1; i < argc; ++i) {
        std::string s(argv[i]);

        switch (cmd[s]) {
            case 1: {
                if (i + 1 == argc)
                    usage();

                std::string fmt(argv[i + 1]); 
                if (fmt != "bmp" && fmt != "ppm") {
                    fprintf(stderr, "[Usage] format must be either bmp or ppm\n");
                    exit(1);
                }
                res["format"] = fmt;
                if (fmt == "ppm" && res["dest"] == "output.bmp")
                    res["dest"] = "output.ppm";

                ++i;
                break;
            }
            case 2: {
                if (i + 1 == argc)
                    usage();

                std::string src(argv[i + 1]);
                res["src"] = src;
                ++i;
                break;
            }
            case 3: {
                if (i + 1 == argc)
                    usage();

                std::string dest(argv[i + 1]);
                res["dest"] = dest;
                ++i;
                break;
            }

            case 4: {
                if (i + 1 == argc) 
                    usage();

                std::string mode(argv[i + 1]);
                if (mode == "0") {
                    fh = {1, 1, 1};
                    fv = {1, 1, 1};
                } else if (mode == "1") {
                    fh = {2, 1, 1};
                    fv = {2, 1, 1};
                } else {
                    usage();
                }
                ++i;
                break;
            }

            default: 
                fprintf(stderr, "[Error] Unknown argument\n");
                exit(1);
        }
    }

    if (res.find("src") == res.end())
        usage();

    return res;
}


#endif
