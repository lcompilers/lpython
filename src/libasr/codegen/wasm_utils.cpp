#include <libasr/codegen/wasm_utils.h>

namespace LFortran {

namespace wasm {

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

float decode_ieee754_f32(Vec<uint8_t> &code, uint32_t &offset) {
    float value = 0.0;
    std::memcpy(&value, &code.p[offset], sizeof(value));
    offset += sizeof(value);
    return value;
}

double decode_ieee754_f64(Vec<uint8_t> &code, uint32_t &offset) {
    double value = 0.0;
    std::memcpy(&value, &code.p[offset], sizeof(value));
    offset += sizeof(value);
    return value;
}

uint8_t read_b8(Vec<uint8_t> &code, uint32_t &offset) {
    LFORTRAN_ASSERT(offset < code.size());
    return code.p[offset++];
}

float read_f32(Vec<uint8_t> &code, uint32_t &offset) {
    LFORTRAN_ASSERT(offset + sizeof(float) <= code.size());
    return decode_ieee754_f32(code, offset);
}

double read_f64(Vec<uint8_t> &code, uint32_t &offset) {
    LFORTRAN_ASSERT(offset + sizeof(double) <= code.size());
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

}  // namespace LFortran
