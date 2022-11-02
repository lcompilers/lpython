#ifndef LFORTRAN_WASM_TO_X86_H
#define LFORTRAN_WASM_TO_X86_H

#include <libasr/wasm_visitor.h>
#include <libasr/codegen/x86_assembler.h>

namespace LFortran {

namespace WASM_INSTS_VISITOR {

class X86Visitor : public BaseWASMVisitor<X86Visitor> {
   public:
    X86Assembler &m_a;
    Vec<wasm::FuncType> func_types;
    Vec<wasm::Import> imports;
    Vec<uint32_t> type_indices;
    Vec<wasm::Export> exports;
    Vec<wasm::Code> func_codes;
    Vec<wasm::Data> data_segments;
    uint32_t cur_func_idx;

    X86Visitor(Vec<uint8_t> &code, uint32_t offset, X86Assembler &m_a,
               Vec<wasm::FuncType> &func_types, Vec<wasm::Import> &imports,
               Vec<uint32_t> &type_indices, Vec<wasm::Export> &exports,
               Vec<wasm::Code> &func_codes, Vec<wasm::Data> &data_segments,
               uint32_t cur_func_idx)
        : BaseWASMVisitor(code, offset), m_a(m_a) {
        this->func_types.from_pointer_n(func_types.p, func_types.size());
        this->imports.from_pointer_n(imports.p, imports.size());
        this->type_indices.from_pointer_n(type_indices.p, type_indices.size());
        this->exports.from_pointer_n(exports.p, exports.size());
        this->func_codes.from_pointer_n(func_codes.p, func_codes.size());
        this->data_segments.from_pointer_n(data_segments.p,
                                           data_segments.size());
        this->cur_func_idx = cur_func_idx;
    }

    void visit_Return() {}

    void visit_Call(uint32_t func_index) {
        if (func_index <= 6U) {
            // call to imported functions
            if (func_index == 0) {
                m_a.asm_call_label("print_i32");
            } else if (func_index == 5) {
                // currently ignoring flush_buf
            } else if (func_index == 6) {
                m_a.asm_call_label("exit");
            } else {
                std::cerr << "Call to imported function with index " +
                                 std::to_string(func_index) +
                                 " not yet supported";
            }
            return;
        }

        uint32_t imports_adjusted_func_index = func_index - 7U;
        m_a.asm_call_label(exports[imports_adjusted_func_index].name);


        // Pop the passed function arguments
        wasm::FuncType func_type = func_types[type_indices[imports_adjusted_func_index]];
        for (uint32_t i = 0; i < func_type.param_types.size(); i++) {
            m_a.asm_pop_r32(X86Reg::eax);
        }

        // Adjust the return values of the called function
        X86Reg base = X86Reg::esp;
        for (uint32_t i = 0; i < func_type.result_types.size(); i++) {
            // take value into eax
            m_a.asm_mov_r32_m32(
                X86Reg::eax, &base, nullptr, 1,
                -(4 * (func_type.param_types.size() + 2 +
                       func_codes[imports_adjusted_func_index].locals.size() + 1)));

            // push eax value onto stack
            m_a.asm_push_r32(X86Reg::eax);
        }
    }

    void visit_LocalGet(uint32_t localidx) {
        X86Reg base = X86Reg::ebp;
        int no_of_params =
            (int)func_types[type_indices[cur_func_idx]].param_types.size();
        if ((int)localidx < no_of_params) {
            m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1,
                                (4 * localidx + 8));
            m_a.asm_push_r32(X86Reg::eax);
        } else {
            m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1,
                                -(4 * ((int)localidx - no_of_params + 1)));
            m_a.asm_push_r32(X86Reg::eax);
        }
    }
    void visit_LocalSet(uint32_t localidx) {
        X86Reg base = X86Reg::ebp;
        int no_of_params =
            (int)func_types[type_indices[cur_func_idx]].param_types.size();
        if ((int)localidx < no_of_params) {
            m_a.asm_pop_r32(X86Reg::eax);
            m_a.asm_mov_m32_r32(&base, nullptr, 1, (4 * localidx + 8),
                                X86Reg::eax);
        } else {
            m_a.asm_pop_r32(X86Reg::eax);
            m_a.asm_mov_m32_r32(&base, nullptr, 1,
                                -(4 * ((int)localidx - no_of_params + 1)),
                                X86Reg::eax);
        }
    }

    void visit_I32Const(int32_t value) {
        m_a.asm_push_imm32(value);
        // if (value < 0) {
        // 	m_a.asm_pop_r32(X86Reg::eax);
        // 	m_a.asm_neg_r32(X86Reg::eax);
        // 	m_a.asm_push_r32(X86Reg::eax);
        // }
    }

    void visit_I32Add() {
        m_a.asm_pop_r32(X86Reg::ebx);
        m_a.asm_pop_r32(X86Reg::eax);
        m_a.asm_add_r32_r32(X86Reg::eax, X86Reg::ebx);
        m_a.asm_push_r32(X86Reg::eax);
    }
    void visit_I32Sub() {
        m_a.asm_pop_r32(X86Reg::ebx);
        m_a.asm_pop_r32(X86Reg::eax);
        m_a.asm_sub_r32_r32(X86Reg::eax, X86Reg::ebx);
        m_a.asm_push_r32(X86Reg::eax);
    }
    void visit_I32Mul() {
        m_a.asm_pop_r32(X86Reg::ebx);
        m_a.asm_pop_r32(X86Reg::eax);
        m_a.asm_mul_r32(X86Reg::ebx);
        m_a.asm_push_r32(X86Reg::eax);
    }
    void visit_I32DivS() {
        m_a.asm_pop_r32(X86Reg::ebx);
        m_a.asm_pop_r32(X86Reg::eax);
        m_a.asm_div_r32(X86Reg::ebx);
        m_a.asm_push_r32(X86Reg::eax);
    }
};

}  // namespace WASM_INSTS_VISITOR

Result<int> wasm_to_x86(Vec<uint8_t> &wasm_bytes, Allocator &al,
                        const std::string &filename, bool time_report,
                        diag::Diagnostics &diagnostics);

}  // namespace LFortran

#endif  // LFORTRAN_WASM_TO_X86_H
