#ifndef BUFFER_HPP_INCLUDED
#define BUFFER_HPP_INCLUDED


#include <cassert>
#include <cstdio>
#include <cstdint>


struct buffer {
    FILE *fp;
    int8_t bpos;
    uint8_t cbyte;
    long fpos, flen;
    bool start_mcu;

    inline buffer();
    inline buffer(FILE *);
    inline uint8_t read_byte();

    template <typename dtype = uint8_t>
    inline dtype read_bits(uint8_t);

    template <typename dtype = uint8_t>
    inline dtype read_bytes(uint8_t);

    inline void skip_byte();
    inline void skip_bits(uint8_t);
    inline void skip_bytes(uint8_t);

    inline void start_read_mcu();
    inline bool read_mcu() const;
    inline void flush();
};

inline buffer::buffer(): fp(nullptr), start_mcu(false) {}

inline buffer::buffer(FILE *fp): fp(fp), bpos(7), fpos(0) {
    fseek(fp, 0, SEEK_END);
    flen = ftell(fp);
    rewind(fp);
    cbyte = (uint8_t)fgetc(fp);
    start_mcu = false;
}

template <typename dtype = uint8_t>
inline dtype buffer::read_bits(uint8_t s) {
    dtype res = 0;
    for (int i = 0; i < (int)s; ++i) {
        res = (dtype)(res << 1 | (cbyte >> bpos & 1));

        if (--bpos < 0) {
            bpos = 7;
            cbyte = (uint8_t)fgetc(fp);
            if (start_mcu && cbyte == 0xFF) {
                uint8_t nxt = (uint8_t)fgetc(fp);
                if (nxt != 0x00) {
                    ungetc(nxt, fp);
                    start_mcu = false;
                } else {
                    fpos++;
                }
            } 
            fpos++;
        }
    }
    return res;
}

inline uint8_t buffer::read_byte() {
    return read_bits<uint8_t>(8);
}

template <typename dtype = uint8_t>
inline dtype buffer::read_bytes(uint8_t s) {
    return read_bits<dtype>((uint8_t)(s * 8));
}

inline void buffer::skip_byte() {
    read_byte();
}

inline void buffer::skip_bits(uint8_t s) {
    read_bits<uint32_t>(s);
}

inline void buffer::start_read_mcu() {
    start_mcu = true;
}

inline void buffer::skip_bytes(uint8_t s) {
    read_bytes<uint32_t>(s);
}

inline bool buffer::read_mcu() const {
    return fpos < flen - 2;
}

inline void buffer::flush() {
    while (bpos != 7) read_bits(1);
}

// void _ugly_define_template_() {
    // buffer _;

    // _.read_bits<int8_t>(0);
    // _.read_bits<uint8_t>(0);
    // _.read_bits<int16_t>(0);
    // _.read_bits<uint16_t>(0);
    // _.read_bits<int32_t>(0);
    // _.read_bits<uint32_t>(0);
    // _.read_bits<size_t>(0);
    // _.read_bits<int>(0);
    // _.read_bits<int64_t>(0);
    // _.read_bits<uint64_t>(0);
    // _.read_bytes<int8_t>(0);
    // _.read_bytes<uint8_t>(0);
    // _.read_bytes<int16_t>(0);
    // _.read_bytes<uint16_t>(0);
    // _.read_bytes<int32_t>(0);
    // _.read_bytes<uint32_t>(0);
    // _.read_bytes<int64_t>(0);
    // _.read_bytes<uint64_t>(0);
    // _.read_bytes<int>(0);
    // _.read_bytes<size_t>(0);
// }

#endif
