#include <fstream>

#include <libasr/assert.h>
#include <libasr/codegen/wasm_decoder.h>
#include <libasr/codegen/wasm_to_wat.h>

namespace LCompilers {

namespace wasm {

class WATVisitor : public WASMDecoder<WATVisitor>,
                     public WASM_INSTS_VISITOR::BaseWASMVisitor<WATVisitor> {

    std::string src, indent;

   public:
    WATVisitor(Allocator &al, diag::Diagnostics &diagonostics, Vec<uint8_t> &code)
        : WASMDecoder(al, diagonostics), BaseWASMVisitor(code, 0U /* temporary offset */),
         src(""), indent("") {
            // We are currently maintaining the two copies of code
            // one is wasm_bytes and the other is code
            // TODO: Use only single copy throughout
            wasm_bytes.from_pointer_n(code.data(), code.size());
         }

    void visit_Unreachable() { src += indent + "unreachable"; }
    void visit_Return() { src += indent + "return"; }
    void visit_Call(uint32_t func_index) {
        src += indent + "call " + std::to_string(func_index);
    }
    void visit_Br(uint32_t label_index) {
        src += indent + "br " + std::to_string(label_index);
    }
    void visit_BrIf(uint32_t label_index) {
        src += indent + "br_if " + std::to_string(label_index);
    }
    void visit_Drop() { src += indent + "drop"; }
    void visit_LocalGet(uint32_t localidx) {
        src += indent + "local.get " + std::to_string(localidx);
    }
    void visit_LocalSet(uint32_t localidx) {
        src += indent + "local.set " + std::to_string(localidx);
    }
    void visit_GlobalGet(uint32_t globalidx) {
        src += indent + "global.get " + std::to_string(globalidx);
    }
    void visit_GlobalSet(uint32_t globalidx) {
        src += indent + "global.set " + std::to_string(globalidx);
    }
    void visit_EmtpyBlockType() {}
    void visit_If() {
        src += indent + "if";
        {
            indent += "    ";
            decode_instructions();
            indent.resize(indent.length() - 4U);
        }
        src += indent + "end";
    }
    void visit_Else() {
        src += indent.substr(0, indent.length() - 4U) + "else";
    }
    void visit_Loop() {
        src += indent + "loop";
        {
            indent += "    ";
            decode_instructions();
            indent.resize(indent.length() - 4U);
        }
        src += indent + "end";
    }

    void visit_I32Const(int32_t value) {
        src += indent + "i32.const " + std::to_string(value);
    }
    void visit_I32Clz() { src += indent + "i32.clz"; }
    void visit_I32Ctz() { src += indent + "i32.ctz"; }
    void visit_I32Popcnt() { src += indent + "i32.popcnt"; }
    void visit_I32Add() { src += indent + "i32.add"; }
    void visit_I32Sub() { src += indent + "i32.sub"; }
    void visit_I32Mul() { src += indent + "i32.mul"; }
    void visit_I32DivS() { src += indent + "i32.div_s"; }
    void visit_I32DivU() { src += indent + "i32.div_u"; }
    void visit_I32RemS() { src += indent + "i32.rem_s"; }
    void visit_I32RemU() { src += indent + "i32.rem_u"; }
    void visit_I32And() { src += indent + "i32.and"; }
    void visit_I32Or() { src += indent + "i32.or"; }
    void visit_I32Xor() { src += indent + "i32.xor"; }
    void visit_I32Shl() { src += indent + "i32.shl"; }
    void visit_I32ShrS() { src += indent + "i32.shr_s"; }
    void visit_I32ShrU() { src += indent + "i32.shr_u"; }
    void visit_I32Rotl() { src += indent + "i32.rotl"; }
    void visit_I32Rotr() { src += indent + "i32.rotr"; }
    void visit_I32Eqz() { src += indent + "i32.eqz"; }
    void visit_I32Eq() { src += indent + "i32.eq"; }
    void visit_I32Ne() { src += indent + "i32.ne"; }
    void visit_I32LtS() { src += indent + "i32.lt_s"; }
    void visit_I32LtU() { src += indent + "i32.lt_u"; }
    void visit_I32GtS() { src += indent + "i32.gt_s"; }
    void visit_I32GtU() { src += indent + "i32.gt_u"; }
    void visit_I32LeS() { src += indent + "i32.le_s"; }
    void visit_I32LeU() { src += indent + "i32.le_u"; }
    void visit_I32GeS() { src += indent + "i32.ge_s"; }
    void visit_I32GeU() { src += indent + "i32.ge_u"; }

    void visit_I64Const(int64_t value) {
        src += indent + "i64.const " + std::to_string(value);
    }
    void visit_I64Clz() { src += indent + "i64.clz"; }
    void visit_I64Ctz() { src += indent + "i64.ctz"; }
    void visit_I64Popcnt() { src += indent + "i64.popcnt"; }
    void visit_I64Add() { src += indent + "i64.add"; }
    void visit_I64Sub() { src += indent + "i64.sub"; }
    void visit_I64Mul() { src += indent + "i64.mul"; }
    void visit_I64DivS() { src += indent + "i64.div_s"; }
    void visit_I64DivU() { src += indent + "i64.div_u"; }
    void visit_I64RemS() { src += indent + "i64.rem_s"; }
    void visit_I64RemU() { src += indent + "i64.rem_u"; }
    void visit_I64And() { src += indent + "i64.and"; }
    void visit_I64Or() { src += indent + "i64.or"; }
    void visit_I64Xor() { src += indent + "i64.xor"; }
    void visit_I64Shl() { src += indent + "i64.shl"; }
    void visit_I64ShrS() { src += indent + "i64.shr_s"; }
    void visit_I64ShrU() { src += indent + "i64.shr_u"; }
    void visit_I64Rotl() { src += indent + "i64.rotl"; }
    void visit_I64Rotr() { src += indent + "i64.rotr"; }
    void visit_I64Eqz() { src += indent + "i64.eqz"; }
    void visit_I64Eq() { src += indent + "i64.eq"; }
    void visit_I64Ne() { src += indent + "i64.ne"; }
    void visit_I64LtS() { src += indent + "i64.lt_s"; }
    void visit_I64LtU() { src += indent + "i64.lt_u"; }
    void visit_I64GtS() { src += indent + "i64.gt_s"; }
    void visit_I64GtU() { src += indent + "i64.gt_u"; }
    void visit_I64LeS() { src += indent + "i64.le_s"; }
    void visit_I64LeU() { src += indent + "i64.le_u"; }
    void visit_I64GeS() { src += indent + "i64.ge_s"; }
    void visit_I64GeU() { src += indent + "i64.ge_u"; }

    void visit_F32Const(float value) {
        src += indent + "f32.const " + std::to_string(value);
    }
    void visit_F32Add() { src += indent + "f32.add"; }
    void visit_F32Sub() { src += indent + "f32.sub"; }
    void visit_F32Mul() { src += indent + "f32.mul"; }
    void visit_F32Div() { src += indent + "f32.div"; }
    void visit_F32DivS() { src += indent + "f32.div_s"; }
    void visit_F32Eq() { src += indent + "f32.eq"; }
    void visit_F32Ne() { src += indent + "f32.ne"; }
    void visit_F32Lt() { src += indent + "f32.lt"; }
    void visit_F32Gt() { src += indent + "f32.gt"; }
    void visit_F32Le() { src += indent + "f32.le"; }
    void visit_F32Ge() { src += indent + "f32.ge"; }
    void visit_F32Abs() { src += indent + "f32.abs"; }
    void visit_F32Neg() { src += indent + "f32.neg"; }
    void visit_F32Ceil() { src += indent + "f32.ceil"; }
    void visit_F32Floor() { src += indent + "f32.floor"; }
    void visit_F32Trunc() { src += indent + "f32.trunc"; }
    void visit_F32Nearest() { src += indent + "f32.nearest"; }
    void visit_F32Sqrt() { src += indent + "f32.sqrt"; }
    void visit_F32Min() { src += indent + "f32.min"; }
    void visit_F32Max() { src += indent + "f32.max"; }
    void visit_F32Copysign() { src += indent + "f32.copysign"; }

    void visit_F64Const(double value) {
        src += indent + "f64.const " + std::to_string(value);
    }
    void visit_F64Add() { src += indent + "f64.add"; }
    void visit_F64Sub() { src += indent + "f64.sub"; }
    void visit_F64Mul() { src += indent + "f64.mul"; }
    void visit_F64Div() { src += indent + "f64.div"; }
    void visit_F64Eq() { src += indent + "f64.eq"; }
    void visit_F64Ne() { src += indent + "f64.ne"; }
    void visit_F64Lt() { src += indent + "f64.lt"; }
    void visit_F64Gt() { src += indent + "f64.gt"; }
    void visit_F64Le() { src += indent + "f64.le"; }
    void visit_F64Ge() { src += indent + "f64.ge"; }
    void visit_F64Abs() { src += indent + "f64.abs"; }
    void visit_F64Neg() { src += indent + "f64.neg"; }
    void visit_F64Ceil() { src += indent + "f64.ceil"; }
    void visit_F64Floor() { src += indent + "f64.floor"; }
    void visit_F64Trunc() { src += indent + "f64.trunc"; }
    void visit_F64Nearest() { src += indent + "f64.nearest"; }
    void visit_F64Sqrt() { src += indent + "f64.sqrt"; }
    void visit_F64Min() { src += indent + "f64.min"; }
    void visit_F64Max() { src += indent + "f64.max"; }
    void visit_F64Copysign() { src += indent + "f64.copysign"; }

    void visit_I32WrapI64() { src += indent + "i32.wrap_i64"; }
    void visit_I32TruncF32S() { src += indent + "i32.trunc_f32_s"; }
    void visit_I32TruncF64S() { src += indent + "i32.trunc_f64_s"; }
    void visit_I64ExtendI32S() { src += indent + "i64.extend_i32_s"; }
    void visit_I64TruncF32S() { src += indent + "i64.trunc_f32_s"; }
    void visit_I64TruncF64S() { src += indent + "i64.trunc_f64_s"; }
    void visit_F32ConvertI32S() { src += indent + "f32.convert_i32_s"; }
    void visit_F32ConvertI64S() { src += indent + "f32.convert_i64_s"; }
    void visit_F32DemoteF64() { src += indent + "f32.demote_f64"; }
    void visit_F64ConvertI32S() { src += indent + "f64.convert_i32_s"; }
    void visit_F64ConvertI64S() { src += indent + "f64.convert_i64_s"; }
    void visit_F64PromoteF32() { src += indent + "f64.promote_f32"; }
    void visit_F64DivS() { src += indent + "f64.div_s"; }

    void visit_I32Load(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i32.load offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I64Load(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i64.load offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_F32Load(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "f32.load offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_F64Load(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "f64.load offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I32Load8S(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i32.load8_s offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I32Load8U(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i32.load8_u offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I32Load16S(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i32.load16_s offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I32Load16U(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i32.load16_u offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I64Load8S(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i64.load8_s offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I64Load8U(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i64.load8_u offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I64Load16S(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i64.load16_s offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I64Load16U(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i64.load16_u offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I64Load32S(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i64.load32_s offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I64Load32U(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i64.load32_u offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I32Store(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i32.store offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I64Store(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i64.store offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_F32Store(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "f32.store offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_F64Store(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "f64.store offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I32Store8(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i32.store8 offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I32Store16(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i32.store16 offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I64Store8(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i64.store8 offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I64Store16(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i64.store16 offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }
    void visit_I64Store32(uint32_t mem_align, uint32_t mem_offset) {
        src += indent + "i64.store32 offset=" + std::to_string(mem_offset) +
               " align=" + std::to_string(1U << mem_align);
    }

    std::string get_escaped_str(const std::string &s, bool is_iov) {
        std::string escaped_str = "";
        for (auto ch:s) {
            if (!is_iov && ch >= 32) {
                escaped_str += ch;
            } else {
                std::string byte(2, ' ');
                snprintf(byte.data(), 3, "%02x", uint8_t(ch));
                escaped_str += "\\" + byte;
            }
        }
        return escaped_str;
    }

    std::string gen_wat() {
        std::string result = "(module";
        std::string indent = "\n    ";
        for (uint32_t i = 0U; i < func_types.size(); i++) {
            result +=
                indent + "(type (;" + std::to_string(i) + ";) (func (param";
            for (uint32_t j = 0; j < func_types[i].param_types.size(); j++) {
                result +=
                    " " + var_type_to_string[func_types[i].param_types.p[j]];
            }
            result += ") (result";
            for (uint32_t j = 0; j < func_types[i].result_types.size(); j++) {
                result +=
                    " " + var_type_to_string[func_types[i].result_types.p[j]];
            }
            result += ")))";
        }

        for (uint32_t i = 0; i < imports.size(); i++) {
            result += indent + "(import \"" + imports[i].mod_name + "\" \"" +
                      imports[i].name + "\" ";
            if (imports[i].kind == 0x00) {
                result += "(func (;" + std::to_string(imports[i].type_idx) +
                          ";) (type " + std::to_string(imports[i].type_idx) +
                          ")))";
            } else if (imports[i].kind == 0x02) {
                result +=
                    "(memory (;0;) " +
                    std::to_string(imports[i].mem_page_size_limits.first) +
                    " " +
                    std::to_string(imports[i].mem_page_size_limits.second) +
                    "))";
            }
        }

        for (uint32_t i = 0; i < globals.size(); i++) {
            std::string global_initialization_insts = "";
            {
                this->offset = globals.p[i].insts_start_idx;
                this->indent = "";
                this->src = "";
                decode_instructions();
                global_initialization_insts = this->src;
            }
            std::string global_type = ((globals[i].mut == 0x00) ? var_type_to_string[globals[i].type]:
            "(mut " + var_type_to_string[globals[i].type] + ")" );
            result += indent + "(global $" + std::to_string(i);
            result += " " + global_type;
            result += " (" + global_initialization_insts + "))";
        }

        for (uint32_t i = 0; i < type_indices.size(); i++) {
            uint32_t func_index = type_indices.p[i];
            result += indent + "(func $" + std::to_string(func_index);
            result += " (type " + std::to_string(func_index) + ") (param";
            for (uint32_t j = 0; j < func_types[func_index].param_types.size();
                 j++) {
                result +=
                    " " +
                    var_type_to_string[func_types[func_index].param_types.p[j]];
            }
            result += ") (result";
            for (uint32_t j = 0; j < func_types[func_index].result_types.size();
                 j++) {
                result += " " + var_type_to_string[func_types[func_index]
                                                       .result_types.p[j]];
            }
            result += ")";
            result += indent + "    (local";
            for (uint32_t j = 0; j < codes.p[i].locals.size(); j++) {
                for (uint32_t k = 0; k < codes.p[i].locals.p[j].count; k++) {
                    result +=
                        " " + var_type_to_string[codes.p[i].locals.p[j].type];
                }
            }
            result += ")";

            {
                this->offset = codes.p[i].insts_start_index;
                this->indent = indent + "    ";
                this->src = "";
                decode_instructions();
                result += this->src;
            }

            result += indent + ")";
        }

        for (uint32_t i = 0; i < memories.size(); i++) {
            result += indent + "(memory (;" + std::to_string(i) + ";) " +
                      std::to_string(memories[i].first) + " " +
                      ((memories[i].second > 0) ?
                      std::to_string(memories[i].second) : "") + ")";
        }

        for (uint32_t i = 0; i < exports.size(); i++) {
            result += indent + "(export \"" + exports.p[i].name + "\" (" +
                      kind_to_string[exports.p[i].kind] + " " +
                      std::to_string(exports.p[i].index) + "))";
        }

        for (uint32_t i = 0; i < data_segments.size(); i++) {
            std::string date_segment_insts;
            {
                this->offset = data_segments.p[i].insts_start_index;
                this->indent = "";
                this->src = "";
                decode_instructions();
                date_segment_insts = this->src;
            }
            result += indent + "(data (;" + std::to_string(i) + ";) (" +
                      date_segment_insts + ") \"" +
                      get_escaped_str(data_segments[i].text, (i % 2 == 0)) + "\")";
        }

        result += "\n)\n";

        return result;
    }
};

}  // namespace wasm

Result<std::string> wasm_to_wat(Vec<uint8_t> &wasm_bytes, Allocator &al,
                                diag::Diagnostics &diagnostics) {
    wasm::WATVisitor wasm_generator(al, diagnostics, wasm_bytes);
    std::string wat;

    try {
        wasm_generator.decode_wasm();
    } catch (const CodeGenError &e) {
        diagnostics.diagnostics.push_back(e.d);
        return Error();
    }

    wat = wasm_generator.gen_wat();

    return wat;
}

}  // namespace LCompilers
