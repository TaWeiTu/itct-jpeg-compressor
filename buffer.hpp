#ifndef BUFFER_HPP_INCLUDED
#define BUFFER_HPP_INCLUDED


#include <cassert>
#include <cstdio>
#include <cstdint>


struct buffer {
    FILE *fp;
    int8_t bpos;
    uint8_t cbyte;
    bool start_mcu;

    inline buffer();
    inline buffer(FILE *);
    inline uint8_t read_byte();

    template <typename dtype>
    inline dtype read_bits(uint8_t);

    template <typename dtype>
    inline dtype read_bytes(uint8_t);

    inline void skip_byte();
    inline void skip_bits(uint8_t);
    inline void skip_bytes(uint8_t);

    inline void start_processing_mcu();
    inline void end_processing_mcu();
    inline void flush();

    inline void write_byte(uint8_t);

    template <typename dtype>
    inline void write_bits(dtype, uint8_t);

    template <typename dtype>
    inline void write_bytes(dtype, uint8_t);

    inline void finish();
};

inline buffer::buffer(): fp(nullptr), start_mcu(false) {}

inline buffer::buffer(FILE *fp): fp(fp), bpos(7) {
    cbyte = (uint8_t)fgetc(fp);
    start_mcu = false;
}

template <typename dtype>
inline dtype buffer::read_bits(uint8_t s) {
    dtype res = 0;
    for (int i = 0; i < (int)s; ++i) {
        res = (dtype)(res << 1 | (cbyte >> bpos & 1));

        if (--bpos < 0) {
            bpos = 7;
            cbyte = (uint8_t)fgetc(fp);
            if (cbyte == 0xFF) {
                uint8_t nxt = (uint8_t)fgetc(fp);
                if (!start_mcu || nxt != 0x00) {
                    ungetc(nxt, fp);
                    if (i + 1 == s) return res;
                }
            } 
        }
    }
    return res;
}

inline uint8_t buffer::read_byte() {
    return read_bits<uint8_t>(8);
}

template <typename dtype>
inline dtype buffer::read_bytes(uint8_t s) {
    return read_bits<dtype>((uint8_t)(s * 8));
}

inline void buffer::skip_byte() {
    read_byte();
}

inline void buffer::skip_bits(uint8_t s) {
    read_bits<uint32_t>(s);
}

inline void buffer::start_processing_mcu() {
    start_mcu = true;
}

inline void buffer::end_processing_mcu() {
    start_mcu = false;
}

inline void buffer::skip_bytes(uint8_t s) {
    read_bytes<uint32_t>(s);
}

inline void buffer::flush() {
    while (bpos != 7) read_bits<uint8_t>(1);
}

inline void buffer::finish() {
    while (bpos != 7) write_bits(0, 1);
}

template <typename dtype>
inline void buffer::write_bits(dtype data, uint8_t s) {
    for (int i = (int)s - 1; i >= 0; --i) {
        cbyte = (uint8_t)(cbyte << 1 | (uint8_t)(data >> i & 1ll));
        if (--bpos < 0) {
            fwrite(&cbyte, 1, 1, fp);
            if (start_mcu && cbyte == 0xFF) {
                cbyte = 0x00;
                fwrite(&cbyte, 1, 1, fp);
            }
            bpos = 7;
            cbyte = 0;
        }
    }
}

template <typename dtype>
inline void buffer::write_bytes(dtype data, uint8_t s) {
    write_bits(data, (uint8_t)(s * 8));
}

inline void buffer::write_byte(uint8_t data) {
    write_bits(data, 8);
}


#endif
