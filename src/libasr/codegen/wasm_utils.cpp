#include <libasr/codegen/wasm_utils.h>

namespace LFortran {

namespace wasm {

uint32_t decode_unsigned_leb128(Vec<uint8_t> &code, uint32_t &offset) {
    uint32_t result = 0U;
    uint32_t shift = 0U;
    while (true) {
        uint8_t byte = code.p[offset++];
        result |= (byte & 0x7f) << shift;
        if ((byte & 0x80) == 0) {
            return result;
        }
        shift += 7;
    }
}

int32_t decode_signed_leb128(Vec<uint8_t> &code, uint32_t &offset) {
    int32_t result = 0;
    uint32_t shift = 0U;
    uint32_t size = 32U;
    uint8_t byte;

    do {
        byte = code.p[offset++];
        result |= (byte & 0x7f) << shift;
        shift += 7;
    } while ((byte & 0x80) != 0);

    if ((shift < size) && (byte & 0x40)) {
        // without U it gives warning that negative number is being shifted, unsure if this works currently
        // this needs to be tested/fixed once the ASR->WASM Backend supports negative integers
        result |= (~0U << shift);
    }

    return result;
}

uint8_t read_byte(Vec<uint8_t> &code, uint32_t &offset) {
    if (offset >= code.size()) {
        throw LFortran::LFortranException("read_byte: offset out of bounds");
    }
    return code.p[offset++];
}

float read_float(Vec<uint8_t> & /*code*/, uint32_t & /*offset*/) {
    // to implement
    return 0.00;
}

double read_double(Vec<uint8_t> & /*code*/, uint32_t & /*offset*/) {
    // to implement
    return 0.00;
}

int32_t read_signed_num(Vec<uint8_t> &code, uint32_t &offset) { return decode_signed_leb128(code, offset); }

uint32_t read_unsigned_num(Vec<uint8_t> &code, uint32_t &offset) { return decode_unsigned_leb128(code, offset); }

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
            if (i + j < buflen) printf("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
        }
        printf("\n");
    }
}

}  // namespace wasm

}  // namespace LFortran
