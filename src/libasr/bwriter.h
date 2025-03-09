#ifndef LFORTRAN_BWRITER_H
#define LFORTRAN_BWRITER_H

#include <sstream>
#include <iomanip>

#include <libasr/asr_utils.h>
#include <libasr/exception.h>

namespace LCompilers {

std::string static inline uint16_to_string(uint16_t i) {
    char bytes[2];
    bytes[0] = (i >>  8) & 0xFF;
    bytes[1] =  i        & 0xFF;
    return std::string(bytes, 2);
}

std::string static inline uint32_to_string(uint32_t i) {
    char bytes[4];
    bytes[0] = (i >> 24) & 0xFF;
    bytes[1] = (i >> 16) & 0xFF;
    bytes[2] = (i >>  8) & 0xFF;
    bytes[3] =  i        & 0xFF;
    return std::string(bytes, 4);
}

std::string static inline uint64_to_string(uint64_t i) {
    char bytes[8];
    bytes[0] = (i >> 56) & 0xFF;
    bytes[1] = (i >> 48) & 0xFF;
    bytes[2] = (i >> 40) & 0xFF;
    bytes[3] = (i >> 32) & 0xFF;
    bytes[4] = (i >> 24) & 0xFF;
    bytes[5] = (i >> 16) & 0xFF;
    bytes[6] = (i >>  8) & 0xFF;
    bytes[7] =  i        & 0xFF;
    return std::string(bytes, 8);
}

std::string static inline uintptr_to_string(uintptr_t i) {
    char bytes[8];
    bytes[0] = (i >> 56) & 0xFF;
    bytes[1] = (i >> 48) & 0xFF;
    bytes[2] = (i >> 40) & 0xFF;
    bytes[3] = (i >> 32) & 0xFF;
    bytes[4] = (i >> 24) & 0xFF;
    bytes[5] = (i >> 16) & 0xFF;
    bytes[6] = (i >>  8) & 0xFF;
    bytes[7] =  i        & 0xFF;
    return std::string(bytes, 8);
}

uint16_t static inline string_to_uint16(const char *s) {
    // The cast from signed char to unsigned char is important,
    // otherwise the signed char shifts return wrong value for negative numbers
    const uint8_t *p = (const unsigned char*)s;
    return (((uint16_t)p[0]) <<  8) |
                       p[1];
}

uint32_t static inline string_to_uint32(const char *s) {
    // The cast from signed char to unsigned char is important,
    // otherwise the signed char shifts return wrong value for negative numbers
    const uint8_t *p = (const unsigned char*)s;
    return (((uint32_t)p[0]) << 24) |
           (((uint32_t)p[1]) << 16) |
           (((uint32_t)p[2]) <<  8) |
                       p[3];
}

uint64_t static inline string_to_uint64(const char *s) {
    // The cast from signed char to unsigned char is important,
    // otherwise the signed char shifts return wrong value for negative numbers
    const uint8_t *p = (const unsigned char*)s;
    return (((uint64_t)p[0]) << 56) |
           (((uint64_t)p[1]) << 48) |
           (((uint64_t)p[2]) << 40) |
           (((uint64_t)p[3]) << 32) |
           (((uint64_t)p[4]) << 24) |
           (((uint64_t)p[5]) << 16) |
           (((uint64_t)p[6]) <<  8) |
                       p[7];
}


uintptr_t static inline string_to_uintptr(const char *s) {
    // The cast from signed char to unsigned char is important,
    // otherwise the signed char shifts return wrong value for negative numbers
    const uint8_t *p = (const unsigned char*)s;
    return (((uintptr_t)p[0]) << 56) |
           (((uintptr_t)p[1]) << 48) |
           (((uintptr_t)p[2]) << 40) |
           (((uintptr_t)p[3]) << 32) |
           (((uintptr_t)p[4]) << 24) |
           (((uintptr_t)p[5]) << 16) |
           (((uintptr_t)p[6]) <<  8) |
                       p[7];
}

uint16_t static inline string_to_uint16(const std::string &s) {
    return string_to_uint16(&s[0]);
}

uint32_t static inline string_to_uint32(const std::string &s) {
    return string_to_uint32(&s[0]);
}

uint64_t static inline string_to_uint64(const std::string &s) {
    return string_to_uint64(&s[0]);
}

uintptr_t static inline string_to_uintptr(const std::string &s) {
	return string_to_uintptr(&s[0]);
}

static inline void* string_to_void(const char *s) {
    return (void*)string_to_uintptr(s);
}

// BinaryReader / BinaryWriter encapsulate access to the file by providing
// primitives that other classes just use.
class BinaryWriter
{
private:
    std::string s;
public:
    std::string get_str() {
        return s;
    }

    void write_int8(uint8_t i) {
        char c=i;
        s.append(std::string(&c, 1));
    }

    void write_int16(uint16_t i) {
        s.append(uint16_to_string(i));
    }

    void write_int32(uint32_t i) {
        s.append(uint32_to_string(i));
    }

    void write_int64(uint64_t i) {
        s.append(uint64_to_string(i));
    }

    void write_string(const std::string &t) {
        write_int64(t.size());
        s.append(t);
    }

    void write_float64(double d) {
        void *p = &d;
        uint64_t *ip = (uint64_t*)p;
        write_int64(*ip);
    }

    void write_uintptr(uintptr_t i) {
        s.append(uintptr_to_string(i));
    }

    void write_void(void *p, int64_t n_data) {
        for (int64_t i = 0; i < n_data; i++) {
            uint8_t* p_i8 = (uint8_t*)p;
            write_int8(p_i8[i]);
        }
    }

};

class BinaryReader
{
private:
    std::string s;
    size_t pos;
public:
    BinaryReader(const std::string &s) : s{s}, pos{0} {}

    uint8_t read_int8() {
        if (pos+1 > s.size()) {
            throw LCompilersException("read_int8: String is too short for deserialization.");
        }
        uint8_t n = s[pos];
        pos += 1;
        return n;
    }

    uint16_t read_int16() {
        if (pos+2 > s.size()) {
            throw LCompilersException("read_int16: String is too short for deserialization.");
        }
        uint16_t n = string_to_uint16(&s[pos]);
        pos += 2;
        return n;
    }

    uint32_t read_int32() {
        if (pos+4 > s.size()) {
            throw LCompilersException("read_int32: String is too short for deserialization.");
        }
        uint32_t n = string_to_uint32(&s[pos]);
        pos += 4;
        return n;
    }

    uint64_t read_int64() {
        if (pos+8 > s.size()) {
            throw LCompilersException("read_int64: String is too short for deserialization.");
        }
        uint64_t n = string_to_uint64(&s[pos]);
        pos += 8;
        return n;
    }

    std::string read_string() {
        size_t n = read_int64();
        if (pos+n > s.size()) {
            throw LCompilersException("read_string: String is too short for deserialization.");
        }
        std::string r = std::string(&s[pos], n);
        pos += n;
        return r;
    }

    double read_float64() {
        uint64_t x = read_int64();
        uint64_t *ip = &x;
        void *p = ip;
        double *dp = (double*)p;
        return *dp;
    }

    void* read_void(int64_t n_data) {
        void *p = new char[n_data];

        for (int64_t i = 0; i < n_data; i++) {
            uint8_t x = read_int8();
            uint8_t *ip = &x;
            void *p_i = (void*)((uintptr_t)p + i);
            uint8_t *p_i_8 = (uint8_t*)p_i;
            *p_i_8 = *ip;
        }

        return p;
    }
};

// TextReader / TextWriter encapsulate access to the file by providing
// primitives that other classes just use. The file is a human readable
// text file. These classes are useful for debugging.
class TextWriter
{
private:
    std::string s;
public:
    std::string get_str() {
        return s;
    }

    void write_int8(uint8_t i) {
        s.append(std::to_string(i));
        s += " ";
    }

    void write_int16(uint16_t i) {
        s.append(std::to_string(i));
        s += " ";
    }

    void write_int32(uint32_t i) {
        s.append(std::to_string(i));
        s += " ";
    }

    void write_int64(uint64_t i) {
        s.append(std::to_string(i));
        s += " ";
    }

    void write_string(const std::string &t) {
        write_int64(t.size());
        s.append(t);
        s += " ";
    }

    void write_float64(double d) {
        std::stringstream str;
        str << std::fixed << std::setprecision(17) << d;
        s.append(str.str());
        s += " ";
    }

    void write_uintptr(uintptr_t i) {
        s.append(uintptr_to_string(i));
        s += " ";
    }

    void write_void(void *p, int64_t n_data) {
        for (int64_t i = 0; i < n_data; i++) {
            uint8_t* p_i8 = (uint8_t*)p;
            write_int8(p_i8[i]);
        }
    }

};

class TextReader
{
private:
    std::string s;
    size_t pos;
public:
    TextReader(const std::string &s) : s{s}, pos{0} {}

    uint8_t read_int8() {
        uint64_t n = read_int64();
        if (n < 255) {
            return n;
        } else {
            throw LCompilersException("read_int8: Integer too large to fit 8 bits.");
        }
    }

    uint16_t read_int16() {
        uint64_t n = read_int64();
        if (n < 65535) {
            return n;
        } else {
            throw LCompilersException("read_int16: Integer too large to fit 16 bits.");
        }
    }

    uint32_t read_int32() {
        uint64_t n = read_int64();
        if (n < 4294967295) {
            return n;
        } else {
            throw LCompilersException("read_int32: Integer too large to fit 32 bits.");
        }
    }

    uint64_t read_int64() {
        std::string tmp;
        while (s[pos] != ' ') {
            tmp += s[pos];
            if (! (s[pos] >= '0' && s[pos] <= '9')) {
                throw LCompilersException("read_int64: Expected integer, got `" + tmp + "`");
            }
            pos++;
            if (pos >= s.size()) {
                throw LCompilersException("read_int64: String is too short for deserialization.");
            }
        }
        pos++;
        uint64_t n = std::stoull(tmp);
        return n;
    }

    double read_float64() {
        std::string tmp;
        while (s[pos] != ' ') {
            tmp += s[pos];
            pos++;
            if (pos >= s.size()) {
                throw LCompilersException("read_float64: String is too short for deserialization.");
            }
        }
        pos++;
        double n = std::stod(tmp);
        return n;
    }

    std::string read_string() {
        size_t n = read_int64();
        if (pos+n > s.size()) {
            throw LCompilersException("read_string: String is too short for deserialization.");
        }
        std::string r = std::string(&s[pos], n);
        pos += n;
        if (s[pos] != ' ') {
            throw LCompilersException("read_string: Space expected.");
        }
        pos ++;
        return r;
    }

    void* read_void(int64_t n_data) {
        void *p = new char[n_data];

        for (int64_t i = 0; i < n_data; i++) {
            uint8_t x = read_int8();
            uint8_t *ip = &x;
            void *p_i = (void*)((uintptr_t)p + i);
            uint8_t *p_i_8 = (uint8_t*)p_i;
            *p_i_8 = *ip;
        }

        return p;
    }
};

} // namespace LCompilers

#endif // LFORTRAN_BWRITER_H
