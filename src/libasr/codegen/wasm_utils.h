#ifndef LFORTRAN_WASM_UTILS_H
#define LFORTRAN_WASM_UTILS_H

#include <iostream>
#include <unordered_map>

#include <libasr/alloc.h>
#include <libasr/containers.h>

namespace LCompilers {

namespace wasm {

enum var_type: uint8_t {
    i32 = 0x7F,
    i64 = 0x7E,
    f32 = 0x7D,
    f64 = 0x7C
};

enum mem_align : uint8_t {
    b8 = 0,
    b16 = 1,
    b32 = 2,
    b64 = 3
};

enum wasm_kind: uint8_t {
    func = 0x00,
    table = 0x01,
    memory = 0x02,
    global = 0x03
};

template <typename T>
std::string vt2s(T vt) {
    switch(vt) {
        case var_type::i32: return "i32";
        case var_type::i64: return "i64";
        case var_type::f32: return "f32";
        case var_type::f64: return "f64";
        default:
            std::cerr << "Unsupported wasm var_type" << std::endl;
            LCOMPILERS_ASSERT(false);
            return "";

    }
}

template <typename T>
std::string k2s(T k) {
    switch(k) {
        case wasm_kind::func: return "func";
        case wasm_kind::table: return "table";
        case wasm_kind::memory: return "memory";
        case wasm_kind::global: return "global";
        default:
            std::cerr << "Unsupported wasm kind" << std::endl;
            LCOMPILERS_ASSERT(false);
            return "";
    }
}

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
