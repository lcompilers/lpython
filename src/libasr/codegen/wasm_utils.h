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

struct Global {
    uint8_t type;
    uint8_t mut;
    uint32_t insts_start_idx;
    union {
        int32_t n32;
        int64_t n64;
        float r32;
        double r64;
    };
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

void encode_leb128_u32(Vec<uint8_t> &code, Allocator &al, uint32_t n);
uint32_t decode_leb128_u32(Vec<uint8_t> &code, uint32_t &offset);

void encode_leb128_i32(Vec<uint8_t> &code, Allocator &al, int32_t n);
int32_t decode_leb128_i32(Vec<uint8_t> &code, uint32_t &offset);

void encode_leb128_i64(Vec<uint8_t> &code, Allocator &al, int64_t n);
int64_t decode_leb128_i64(Vec<uint8_t> &code, uint32_t &offset);

void encode_ieee754_f32(Vec<uint8_t> &code, Allocator &al, float z);
float decode_ieee754_f32(Vec<uint8_t> &code, uint32_t &offset);

void encode_ieee754_f64(Vec<uint8_t> &code, Allocator &al, double z);
double decode_ieee754_f64(Vec<uint8_t> &code, uint32_t &offset);

void emit_b8(Vec<uint8_t> &code, Allocator &al, uint8_t x);
void emit_u32(Vec<uint8_t> &code, Allocator &al, uint32_t x);
void emit_i32(Vec<uint8_t> &code, Allocator &al, int32_t x);
void emit_i64(Vec<uint8_t> &code, Allocator &al, int64_t x);
void emit_f32(Vec<uint8_t> &code, Allocator &al, float x);
void emit_f64(Vec<uint8_t> &code, Allocator &al, double x);

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
