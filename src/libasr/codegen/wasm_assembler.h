#include <cassert>

#include <libasr/alloc.h>
#include <libasr/containers.h>

namespace LFortran {
namespace wasm {

std::vector<uint8_t> encode_signed_leb128(int32_t n) {
    std::vector<uint8_t> out;
    auto more = true;
    do {
        uint8_t byte = n & 0x7f;
        n >>= 7;
        more = !((((n == 0) && ((byte & 0x40) == 0)) ||
                  ((n == -1) && ((byte & 0x40) != 0))));
        if (more) {
            byte |= 0x80;
        }
        out.emplace_back(byte);
    } while (more);
    return out;
}

std::vector<uint8_t> encode_unsigned_leb128(uint32_t n) {
    std::vector<uint8_t> out;
    do {
        uint8_t byte = n & 0x7f;
        n >>= 7;
        if (n != 0) {
            byte |= 0x80;
        }
        out.emplace_back(byte);
    } while (n != 0);
    return out;
}

void emit_signed_leb128(Vec<uint8_t> &code, Allocator &al, int32_t n) {
    auto more = true;
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

void emit_unsigned_leb128(Vec<uint8_t> &code, Allocator &al, uint32_t n) {
    do {
        uint8_t byte = n & 0x7f;
        n >>= 7;
        if (n != 0) {
            byte |= 0x80;
        }
        code.push_back(al, byte);
    } while (n != 0);
}

// function to emit header of Wasm Binary Format
void emit_header(Vec<uint8_t> &code, Allocator &al) {
    code.push_back(al, 0x00);
    code.push_back(al, 0x61);
    code.push_back(al, 0x73);
    code.push_back(al, 0x6D);
    code.push_back(al, 0x01);
    code.push_back(al, 0x00);
    code.push_back(al, 0x00);
    code.push_back(al, 0x00);
}

// function to emit unsigned 32 bit integer
void emit_u32(Vec<uint8_t> &code, Allocator &al, uint32_t x) {
    emit_unsigned_leb128(code, al, x);
}

// function to emit signed 32 bit integer
void emit_i32(Vec<uint8_t> &code, Allocator &al, int32_t x) {
    emit_signed_leb128(code, al, x);
}

// function to append a given bytecode to the end of the code
void emit_b8(Vec<uint8_t> &code, Allocator &al, uint8_t x) {
    code.push_back(al, x);
}

void emit_u32_b32_idx(Vec<uint8_t> &code, uint32_t idx,
                      uint32_t section_size) {
    /*
    Encodes the integer `i` using LEB128 and adds trailing zeros to always
    occupy 4 bytes. Stores the int `i` at the index `idx` in `code`.
    */
    std::vector<uint8_t> num = encode_unsigned_leb128(section_size);
    std::vector<uint8_t> num_4b = {0x80, 0x80, 0x80, 0x00};
    assert(num.size() <= 4);
    for (uint32_t i = 0; i < num.size(); i++) {
        num_4b[i] |= num[i];
    }
    for (uint32_t i = 0; i < 4u; i++) {
        code.p[idx + i] = num_4b[i];
    }
}

// function to fixup length at the given length index
void fixup_len(Vec<uint8_t> &code, uint32_t len_idx) {
    uint32_t section_len = code.size() - len_idx - 4u;
    emit_u32_b32_idx(code, len_idx, section_len);
}

// function to emit length placeholder
uint32_t emit_len_placeholder(Vec<uint8_t> &code, Allocator &al) {
    uint32_t len_idx = code.size();
    code.push_back(al, 0x00);
    code.push_back(al, 0x00);
    code.push_back(al, 0x00);
    code.push_back(al, 0x00);
    return len_idx;
}

// function to emit a i32.const instruction
void emit_i32_const(Vec<uint8_t> &code, Allocator &al, int32_t x) {
    code.push_back(al, 0x41);
    emit_i32(code, al, x);
}

// function to emit end of wasm expression
void emit_expr_end(Vec<uint8_t> &code, Allocator &al) {
    code.push_back(al, 0x0B);
}

// function to emit get local variable at given index
void emit_get_local(Vec<uint8_t> &code, Allocator &al, uint32_t idx) {
    code.push_back(al, 0x20);
    emit_u32(code, al, idx);
}

// function to emit set local variable at given index
void emit_set_local(Vec<uint8_t> &code, Allocator &al, uint32_t idx) {
    code.push_back(al, 0x21);
    emit_u32(code, al, idx);
}

// function to emit i32.add instruction
void emit_i32_add(Vec<uint8_t> &code, Allocator &al) {
    code.push_back(al, 0x6A);
}

// function to emit i32.sub instruction
void emit_i32_sub(Vec<uint8_t> &code, Allocator &al) {
    code.push_back(al, 0x6B);
}

// function to emit i32.mul instruction
void emit_i32_mul(Vec<uint8_t> &code, Allocator &al) {
    code.push_back(al, 0x6C);
}

// function to emit i32.div instruction
void emit_i32_div(Vec<uint8_t> &code, Allocator &al) {
    code.push_back(al, 0x6D);
}

void emit_i64_add(Vec<uint8_t> &code, Allocator &al) {
    code.push_back(al, 0x7C);
}

void emit_i64_sub(Vec<uint8_t> &code, Allocator &al) {
    code.push_back(al, 0x7D);
}

void emit_i64_mul(Vec<uint8_t> &code, Allocator &al) {
    code.push_back(al, 0x7E);
}

void emit_i64_div(Vec<uint8_t> &code, Allocator &al) {
    code.push_back(al, 0x7F);
}

// function to emit call instruction
void emit_call(Vec<uint8_t> &code, Allocator &al, uint32_t idx) {
    code.push_back(al, 0x10);
    emit_u32(code, al, idx);
}

void emit_export_fn(Vec<uint8_t> &code, Allocator &al, const std::string& name,
                    uint32_t idx) {
    std::vector<uint8_t> name_bytes(name.size());
    std::memcpy(name_bytes.data(), name.data(), name.size());
    emit_u32(code, al, name_bytes.size());
    for(auto &byte:name_bytes)
        emit_b8(code, al, byte);
    emit_b8(code, al, 0x00);
    emit_u32(code, al, idx);
}

}  // namespace wasm

}  // namespace LFortran
