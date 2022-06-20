#ifndef LFORTRAN_WASM_TO_WAT_H
#define LFORTRAN_WASM_TO_WAT_H

#include <libasr/wasm_visitor.h>

namespace LFortran {

namespace WASM_INSTS_VISITOR {

class WATVisitor : public BaseWASMVisitor<WATVisitor> {
   public:
    std::string src, indent;

    WATVisitor() : src(""), indent("") {}

    void visit_Return() { src += indent + "return"; }

    void visit_Call(uint32_t func_index) { src += indent + "call " + std::to_string(func_index); }

    void visit_LocalGet(uint32_t localidx) { src += indent + "local.get " + std::to_string(localidx); }

    void visit_LocalSet(uint32_t localidx) { src += indent + "local.set " + std::to_string(localidx); }

    void visit_I32Const(int value) { src += indent + "i32.const " + std::to_string(value); }

    void visit_I32Add() { src += indent + "i32.add"; }
    void visit_I64Add() { src += indent + "i64.add"; }

    void visit_I32Sub() { src += indent + "i32.sub"; }
    void visit_I64Sub() { src += indent + "i64.sub"; }

    void visit_I32Mul() { src += indent + "i32.mul"; }
    void visit_I64Mul() { src += indent + "i64.mul"; }

    void visit_I32DivS() { src += indent + "i32.div_s"; }
    void visit_I64DivS() { src += indent + "i64.div_s"; }
};

}  // namespace WASM_INSTS_VISITOR

namespace wasm {

class WASMDecoder {
   public:
    std::unordered_map<uint8_t, std::string> var_type_to_string;
    std::unordered_map<uint8_t, std::string> kind_to_string;

    Allocator &al;
    Vec<uint8_t> wasm_bytes;

    Vec<wasm::FuncType> func_types;
    Vec<uint32_t> type_indices;
    Vec<wasm::Export> exports;
    Vec<wasm::Code> codes;

    WASMDecoder(Allocator &al) : al(al) {
        var_type_to_string = {{0x7F, "i32"}, {0x7E, "i64"}, {0x7D, "f32"}, {0x7C, "f64"}};
        kind_to_string = {{0x00, "func"}, {0x01, "table"}, {0x02, "mem"}, {0x03, "global"}};
        
        // wasm_bytes.reserve(al, 1024 * 128);
        // func_types.reserve(al, 1024 * 128);
        // type_indices.reserve(al, 1024 * 128);
        // exports.reserve(al, 1024 * 128);
        // codes.reserve(al, 1024 * 128);
    }

    void load_file(std::string filename);
    void decode_type_section(uint32_t offset);
    void decode_function_section(uint32_t offset);
    void decode_export_section(uint32_t offset);
    void decode_code_section(uint32_t offset);
    void decode_wasm();
    std::string get_wat();
};

}  // namespace wasm

}  // namespace LFortran

#endif  // LFORTRAN_WASM_TO_WAT_H
