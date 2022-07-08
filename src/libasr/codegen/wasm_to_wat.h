#ifndef LFORTRAN_WASM_TO_WAT_H
#define LFORTRAN_WASM_TO_WAT_H

#include <libasr/wasm_visitor.h>

namespace LFortran {

namespace WASM_INSTS_VISITOR {

class WATVisitor : public BaseWASMVisitor<WATVisitor> {
   public:
    std::string src, indent;

    WATVisitor(Vec<uint8_t> &code, uint32_t offset, std::string src, std::string indent) : BaseWASMVisitor(code, offset), src(src), indent(indent) {}

    void visit_Unreachable() { src += indent + "unreachable"; }
    void visit_Return() { src += indent + "return"; }
    void visit_Call(uint32_t func_index) { src += indent + "call " + std::to_string(func_index); }
    void visit_LocalGet(uint32_t localidx) { src += indent + "local.get " + std::to_string(localidx); }
    void visit_LocalSet(uint32_t localidx) { src += indent + "local.set " + std::to_string(localidx); }
    void visit_EmtpyBlockType() {}
    void visit_If() {
        src += indent + "if";
        {
            WATVisitor v = WATVisitor(code, offset, "", indent + "    ");
            v.decode_instructions();
            src += v.src;
            offset = v.offset;
        }
        src += indent + "end";
    }
    void visit_Else() { src += indent + "\b\b\b\b" + "else"; }

    void visit_I32Const(int32_t value) { src += indent + "i32.const " + std::to_string(value); }
    void visit_I32Add() { src += indent + "i32.add"; }
    void visit_I32Sub() { src += indent + "i32.sub"; }
    void visit_I32Mul() { src += indent + "i32.mul"; }
    void visit_I32DivS() { src += indent + "i32.div_s"; }

    void visit_I64Const(int64_t value) { src += indent + "i64.const " + std::to_string(value); }
    void visit_I64Add() { src += indent + "i64.add"; }
    void visit_I64Sub() { src += indent + "i64.sub"; }
    void visit_I64Mul() { src += indent + "i64.mul"; }
    void visit_I64DivS() { src += indent + "i64.div_s"; }

    void visit_F32Const(float value) { src += indent + "f32.const " + std::to_string(value); }
    void visit_F32Add() { src += indent + "f32.add"; }
    void visit_F32Sub() { src += indent + "f32.sub"; }
    void visit_F32Mul() { src += indent + "f32.mul"; }
    void visit_F32DivS() { src += indent + "f32.div_s"; }
    
    void visit_F64Const(double value) { src += indent + "f64.const " + std::to_string(value); }
    void visit_F64Add() { src += indent + "f64.add"; }
    void visit_F64Sub() { src += indent + "f64.sub"; }
    void visit_F64Mul() { src += indent + "f64.mul"; }
    void visit_F64DivS() { src += indent + "f64.div_s"; }
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
    Vec<wasm::Import> imports;
    Vec<uint32_t> type_indices;
    Vec<wasm::Export> exports;
    Vec<wasm::Code> codes;
    Vec<wasm::Data> data_segments;

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
    void decode_imports_section(uint32_t offset);
    void decode_function_section(uint32_t offset);
    void decode_export_section(uint32_t offset);
    void decode_code_section(uint32_t offset);
    void decode_data_section(uint32_t offset);
    void decode_wasm();
    std::string get_wat();
};

}  // namespace wasm

}  // namespace LFortran

#endif  // LFORTRAN_WASM_TO_WAT_H
