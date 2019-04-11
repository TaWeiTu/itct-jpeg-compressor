#ifndef BUFFER_HPP_INCLUDED
#define BUFFER_HPP_INCLUDED

struct buffer {
    FILE *fp;
    int8_t bpos;

    inline buffer();
    inline buffer(FILE *);
    inline uint8_t fpeek();
    inline uint8_t read_byte();

    template <typename dtype = uint8_t>
    inline dtype read_bits(uint8_t);

    template <typename dtype = uint8_t>
    inline dtype read_bytes(uint8_t);

    inline void skip_byte();
    inline void skip_bits(uint8_t);
    inline void skip_bytes(uint8_t);
    inline void unread(uint8_t);
};

inline buffer::buffer() {}

inline buffer::buffer(FILE *fp): fp(fp), bpos(7) {}

inline uint8_t buffer::fpeek() {
    uint8_t res = (uint8_t)fgetc(fp);
    ungetc(res, fp);
    return res;
}

template <typename dtype = uint8_t>
inline dtype buffer::read_bits(uint8_t s) {
    dtype res = 0;
    for (int i = 0; i < (int)s; ++i) {
        res = (dtype)(res << 1 | (fpeek() >> bpos & 1));
        if (--bpos < 0) {
            bpos = 7;
            fgetc(fp);
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

inline void buffer::skip_bytes(uint8_t s) {
    read_bytes<uint32_t>(s);
}

inline void buffer::unread(uint8_t byte) {
    ungetc(byte, fp);
}

#endif
