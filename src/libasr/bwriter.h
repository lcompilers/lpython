#ifndef LFORTRAN_BWRITER_H
#define LFORTRAN_BWRITER_H

#include <sstream>
#include <iomanip>

#include <libasr/exception.h>

namespace LFortran {

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

uint32_t static inline string_to_uint32(const std::string &s) {
    return string_to_uint32(&s[0]);
}

uint64_t static inline string_to_uint64(const std::string &s) {
    return string_to_uint64(&s[0]);
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
};

} // namespace LFortran

#endif // LFORTRAN_BWRITER_H
