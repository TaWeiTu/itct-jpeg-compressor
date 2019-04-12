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

    buffer();
    buffer(FILE *);
    uint8_t read_byte();

    template <typename dtype = uint8_t>
    dtype read_bits(uint8_t);

    template <typename dtype = uint8_t>
    dtype read_bytes(uint8_t);

    void skip_byte();
    void skip_bits(uint8_t);
    void skip_bytes(uint8_t);

    void start_read_mcu();
    bool read_mcu() const;
    void clear();
};

#endif
