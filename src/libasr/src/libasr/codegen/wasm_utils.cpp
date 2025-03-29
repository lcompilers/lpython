#include <libasr/codegen/wasm_utils.h>

namespace LCompilers {

namespace wasm {

void encode_leb128_u32(Vec<uint8_t> &code, Allocator &al, uint32_t n) {
    do {
        uint8_t byte = n & 0x7f;
        n >>= 7;
        if (n != 0) {
            byte |= 0x80;
        }
        code.push_back(al, byte);
    } while (n != 0);
}

uint32_t decode_leb128_u32(Vec<uint8_t> &code, uint32_t &offset) {
    uint32_t result = 0U;
    uint32_t shift = 0U;
    while (true) {
        uint8_t byte = read_b8(code, offset);
        uint32_t slice = byte & 0x7f;
        result |= slice << shift;
        if ((byte & 0x80) == 0) {
            return result;
        }
        shift += 7;
    }
}

void encode_leb128_i32(Vec<uint8_t> &code, Allocator &al, int32_t n) {
    bool more = true;
    do {
        uint8_t byte = n & 0x7f;
        n >>= 7;
        more = !((((n == 0) && ((byte & 0x40) == 0)) ||
                  ((n == -1) && ((byte & 0x40) != 0))));
        if (more) {
            byte |= 0x80;
        }
        code.push_back(al, byte);
    } while (more);
}

int32_t decode_leb128_i32(Vec<uint8_t> &code, uint32_t &offset) {
    int32_t result = 0;
    uint32_t shift = 0U;
    uint8_t byte;

    do {
        byte = read_b8(code, offset);
        uint32_t slice = byte & 0x7f;
        result |= slice << shift;
        shift += 7;
    } while (byte & 0x80);

    // Sign extend negative numbers if needed.
    if ((shift < 32U) && (byte & 0x40)) {
        result |= (-1U << shift);
    }

    return result;
}

void encode_leb128_i64(Vec<uint8_t> &code, Allocator &al, int64_t n) {
    bool more = true;
    do {
        uint8_t byte = n & 0x7f;
        n >>= 7;
        more = !((((n == 0) && ((byte & 0x40) == 0)) ||
                  ((n == -1) && ((byte & 0x40) != 0))));
        if (more) {
            byte |= 0x80;
        }
        code.push_back(al, byte);
    } while (more);
}

int64_t decode_leb128_i64(Vec<uint8_t> &code, uint32_t &offset) {
    int64_t result = 0;
    uint32_t shift = 0U;
    uint8_t byte;

    do {
        byte = read_b8(code, offset);
        uint64_t slice = byte & 0x7f;
        result |= slice << shift;
        shift += 7;
    } while (byte & 0x80);

    // Sign extend negative numbers if needed.
    if ((shift < 64U) && (byte & 0x40)) {
        result |= (-1ULL << shift);
    }

    return result;
}

void encode_ieee754_f32(Vec<uint8_t> &code, Allocator &al, float z) {
    uint8_t encoded_float[sizeof(z)];
    std::memcpy(&encoded_float, &z, sizeof(z));
    for (auto &byte : encoded_float) {
        code.push_back(al, byte);
    }
}

float decode_ieee754_f32(Vec<uint8_t> &code, uint32_t &offset) {
    float value = 0.0;
    std::memcpy(&value, &code.p[offset], sizeof(value));
    offset += sizeof(value);
    return value;
}

void encode_ieee754_f64(Vec<uint8_t> &code, Allocator &al, double z) {
    uint8_t encoded_float[sizeof(z)];
    std::memcpy(&encoded_float, &z, sizeof(z));
    for (auto &byte : encoded_float) {
        code.push_back(al, byte);
    }
}

double decode_ieee754_f64(Vec<uint8_t> &code, uint32_t &offset) {
    double value = 0.0;
    std::memcpy(&value, &code.p[offset], sizeof(value));
    offset += sizeof(value);
    return value;
}

// function to append a given bytecode to the end of the code
void emit_b8(Vec<uint8_t> &code, Allocator &al, uint8_t x) {
    code.push_back(al, x);
}

// function to emit unsigned 32 bit integer
void emit_u32(Vec<uint8_t> &code, Allocator &al, uint32_t x) {
    encode_leb128_u32(code, al, x);
}

// function to emit signed 32 bit integer
void emit_i32(Vec<uint8_t> &code, Allocator &al, int32_t x) {
    encode_leb128_i32(code, al, x);
}

// function to emit signed 64 bit integer
void emit_i64(Vec<uint8_t> &code, Allocator &al, int64_t x) {
    encode_leb128_i64(code, al, x);
}

// function to emit 32 bit float
void emit_f32(Vec<uint8_t> &code, Allocator &al, float x) {
    encode_ieee754_f32(code, al, x);
}

// function to emit 64 bit float
void emit_f64(Vec<uint8_t> &code, Allocator &al, double x) {
    encode_ieee754_f64(code, al, x);
}

uint8_t read_b8(Vec<uint8_t> &code, uint32_t &offset) {
    LCOMPILERS_ASSERT(offset < code.size());
    return code.p[offset++];
}

float read_f32(Vec<uint8_t> &code, uint32_t &offset) {
    LCOMPILERS_ASSERT(offset + sizeof(float) <= code.size());
    return decode_ieee754_f32(code, offset);
}

double read_f64(Vec<uint8_t> &code, uint32_t &offset) {
    LCOMPILERS_ASSERT(offset + sizeof(double) <= code.size());
    return decode_ieee754_f64(code, offset);
}

uint32_t read_u32(Vec<uint8_t> &code, uint32_t &offset) {
    return decode_leb128_u32(code, offset);
}

int32_t read_i32(Vec<uint8_t> &code, uint32_t &offset) {
    return decode_leb128_i32(code, offset);
}

int64_t read_i64(Vec<uint8_t> &code, uint32_t &offset) {
    return decode_leb128_i64(code, offset);
}

void hexdump(void *ptr, int buflen) {
    unsigned char *buf = (unsigned char *)ptr;
    int i, j;
    for (i = 0; i < buflen; i += 16) {
        printf("%06x: ", i);
        for (j = 0; j < 16; j++) {
            if (i + j < buflen)
                printf("%02x ", buf[i + j]);
            else
                printf("   ");
        }
        printf(" ");
        for (j = 0; j < 16; j++) {
            if (i + j < buflen)
                printf("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
        }
        printf("\n");
    }
}

}  // namespace wasm

}  // namespace LCompilers
