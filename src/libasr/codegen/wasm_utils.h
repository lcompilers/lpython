#ifndef LFORTRAN_WASM_UTILS_H
#define LFORTRAN_WASM_UTILS_H

#include <iostream>
#include <unordered_map>

#include <libasr/alloc.h>
#include <libasr/containers.h>

namespace LCompilers {

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

struct Import {
    std::string mod_name;
    std::string name;
    uint8_t kind;
    union {
        uint32_t type_idx;
        std::pair<uint32_t, uint32_t> mem_page_size_limits;
    };
};

struct Data {
    uint32_t insts_start_index;
    std::string text;
};

uint32_t decode_leb128_u32(Vec<uint8_t> &code, uint32_t &offset);
int32_t decode_leb128_i32(Vec<uint8_t> &code, uint32_t &offset);
int64_t decode_leb128_i64(Vec<uint8_t> &code, uint32_t &offset);

uint8_t read_b8(Vec<uint8_t> &code, uint32_t &offset);

float read_f32(Vec<uint8_t> &code, uint32_t &offset);

double read_f64(Vec<uint8_t> &code, uint32_t &offset);

uint32_t read_u32(Vec<uint8_t> &code, uint32_t &offset);

int32_t read_i32(Vec<uint8_t> &code, uint32_t &offset);

int64_t read_i64(Vec<uint8_t> &code, uint32_t &offset);

void hexdump(void *ptr, int buflen);

}  // namespace wasm

}  // namespace LCompilers

#endif  // LFORTRAN_WASM_UTILS_H
