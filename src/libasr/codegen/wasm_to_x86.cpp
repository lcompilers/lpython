#include <fstream>
#include <chrono>
#include <iomanip>

#include <libasr/assert.h>
#include <libasr/codegen/wasm_decoder.h>
#include <libasr/codegen/wasm_to_x86.h>
#include <libasr/codegen/x86_assembler.h>

namespace LFortran {

namespace wasm {

class X86Visitor : public WASMDecoder<X86Visitor>,
                   public WASM_INSTS_VISITOR::BaseWASMVisitor<X86Visitor> {
   public:
    X86Assembler &m_a;
    uint32_t cur_func_idx;
    std::vector<uint32_t> unique_id;

    X86Visitor(X86Assembler &m_a, Allocator &al,
               diag::Diagnostics &diagonostics, Vec<uint8_t> &code)
        : WASMDecoder(al, diagonostics),
          BaseWASMVisitor(code, 0U /* temporary offset */),
          m_a(m_a) {
        wasm_bytes.from_pointer_n(code.data(), code.size());
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
        wasm::FuncType func_type =
            func_types[type_indices[imports_adjusted_func_index]];
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
                       codes[imports_adjusted_func_index].locals.size() + 1)));

            // push eax value onto stack
            m_a.asm_push_r32(X86Reg::eax);
        }
    }

    void visit_EmtpyBlockType() {}

    void visit_If() {
        unique_id.push_back(offset);
        m_a.asm_pop_r32(X86Reg::eax);
        m_a.asm_cmp_r32_imm8(LFortran::X86Reg::eax, 1);
        m_a.asm_je_label(".then_" + std::to_string(unique_id.back()));
        m_a.asm_jmp_label(".else_" + std::to_string(unique_id.back()));
        m_a.add_label(".then_" + std::to_string(unique_id.back()));
        {
            decode_instructions();
        }
        m_a.add_label(".endif_" + std::to_string(unique_id.back()));
        unique_id.pop_back();
    }

    void visit_Else() {
        m_a.asm_jmp_label(".endif_" + std::to_string(unique_id.back()));
        m_a.add_label(".else_" + std::to_string(unique_id.back()));
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

    void gen_x86_bytes() {
        emit_elf32_header(m_a);

        // Add runtime library functions
        emit_print_int(m_a, "print_i32");
        emit_exit(m_a, "exit", 0);

        for (uint32_t i = 0; i < type_indices.size(); i++) {
            if (i < type_indices.size() - 1U) {
                m_a.add_label(exports[i].name);
            } else {
                m_a.add_label("_start");
            }

            {
                // Initialize the stack
                m_a.asm_push_r32(X86Reg::ebp);
                m_a.asm_mov_r32_r32(X86Reg::ebp, X86Reg::esp);

                // Initialze local variables to zero and thus allocate space
                m_a.asm_mov_r32_imm32(X86Reg::eax, 0U);
                for (uint32_t j = 0; j < codes.p[i].locals.size(); j++) {
                    for (uint32_t k = 0; k < codes.p[i].locals.p[j].count;
                         k++) {
                        m_a.asm_push_r32(X86Reg::eax);
                    }
                }

                offset = codes.p[i].insts_start_index;
                cur_func_idx = i;
                decode_instructions();

                // Restore stack
                m_a.asm_mov_r32_r32(X86Reg::esp, X86Reg::ebp);
                m_a.asm_pop_r32(X86Reg::ebp);
                m_a.asm_ret();
            }
        }

        emit_elf32_footer(m_a);
    }
};

}  // namespace wasm

Result<int> wasm_to_x86(Vec<uint8_t> &wasm_bytes, Allocator &al,
                        const std::string &filename, bool time_report,
                        diag::Diagnostics &diagnostics) {
    int time_decode_wasm = 0;
    int time_gen_x86_bytes = 0;
    int time_save = 0;
    int time_verify = 0;

    X86Assembler m_a(al);

    wasm::X86Visitor x86_visitor(m_a, al, diagnostics, wasm_bytes);

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        try {
            x86_visitor.decode_wasm();
        } catch (const CodeGenError &e) {
            diagnostics.diagnostics.push_back(e.d);
            return Error();
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        time_decode_wasm =
            std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
                .count();
    }

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        x86_visitor.gen_x86_bytes();
        auto t2 = std::chrono::high_resolution_clock::now();
        time_gen_x86_bytes =
            std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
                .count();
    }

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        m_a.verify();
        auto t2 = std::chrono::high_resolution_clock::now();
        time_verify =
            std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
                .count();
    }

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        m_a.save_binary(filename);
        auto t2 = std::chrono::high_resolution_clock::now();
        time_save =
            std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
                .count();
    }

    //! Helpful for debugging
    // std::cout << x86_visitor.m_a.get_asm() << std::endl;

    if (time_report) {
        std::cout << "Codegen Time report:" << std::endl;
        std::cout << "Decode wasm: " << std::setw(5) << time_decode_wasm
                  << std::endl;
        std::cout << "Generate asm: " << std::setw(5) << time_gen_x86_bytes
                  << std::endl;
        std::cout << "Verify:       " << std::setw(5) << time_verify
                  << std::endl;
        std::cout << "Save:       " << std::setw(5) << time_save << std::endl;
        int total =
            time_decode_wasm + time_gen_x86_bytes + time_verify + time_save;
        std::cout << "Total:      " << std::setw(5) << total << std::endl;
    }
    return 0;
}

}  // namespace LFortran
