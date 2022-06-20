#ifndef LFORTRAN_WASM_UTILS_H
#define LFORTRAN_WASM_UTILS_H

#include <iostream>
#include <unordered_map>

#include <libasr/alloc.h>
#include <libasr/containers.h>

namespace LFortran {

namespace wasm {

struct FuncType {
    Vec<uint8_t> param_types;
    Vec<uint8_t> result_types;
};

struct Export {
    std::string name;
    uint8_t kind;
    uint32_t index;
};

struct Local {
    uint32_t count;
    uint8_t type;
};

struct Code {
    int size;
    Vec<Local> locals;
    uint32_t insts_start_index;
};

uint32_t decode_unsigned_leb128(Vec<uint8_t> &code, uint32_t &offset);

int32_t decode_signed_leb128(Vec<uint8_t> &code, uint32_t &offset);

uint8_t read_byte(Vec<uint8_t> &code, uint32_t &offset);

float read_float(Vec<uint8_t> & code, uint32_t & offset);

double read_double(Vec<uint8_t> & code, uint32_t & offset);

int32_t read_signed_num(Vec<uint8_t> &code, uint32_t &offset);

uint32_t read_unsigned_num(Vec<uint8_t> &code, uint32_t &offset);

void hexdump(void *ptr, int buflen);

}  // namespace wasm

}  // namespace LFortran

#endif  // LFORTRAN_WASM_UTILS_H
