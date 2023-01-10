#include <fstream>
#include <chrono>
#include <iomanip>

#include <libasr/assert.h>
#include <libasr/codegen/wasm_decoder.h>
#include <libasr/codegen/wasm_to_x86.h>
#include <libasr/codegen/x86_assembler.h>

namespace LCompilers {

namespace wasm {

/*

This X86Visitor uses stack to pass arguments and return values from functions.
Since in X86, instructions operate on registers (and not on stack),
for every instruction we pop elements from top of stack and store them into
registers. After operating on the registers, the result value is then
pushed back onto the stack.

One of the reasons to use stack to pass function arguments is that,
it allows us to define and call functions with any number of parameters.
As registers are limited in number, if we use them to pass function arugments,
the number of arguments we could pass to a function would get limited by
the number of registers available with the CPU.

*/

class X86Visitor : public WASMDecoder<X86Visitor>,
                   public WASM_INSTS_VISITOR::BaseWASMVisitor<X86Visitor> {
   public:
    X86Assembler &m_a;
    uint32_t cur_func_idx;
    std::vector<std::string> if_unique_id;
    std::vector<std::string> loop_unique_id;
    uint32_t cur_nesting_length;
    int32_t last_vis_i32_const, last_last_vis_i32_const;
    std::map<std::string, std::string> label_to_str;
    std::map<std::string, float> float_consts;
    bool decoding_data_segment;

    X86Visitor(X86Assembler &m_a, Allocator &al,
               diag::Diagnostics &diagonostics, Vec<uint8_t> &code)
        : WASMDecoder(al, diagonostics),
          BaseWASMVisitor(code, 0U /* temporary offset */),
          m_a(m_a) {
        wasm_bytes.from_pointer_n(code.data(), code.size());
        cur_nesting_length = 0;
        decoding_data_segment = false;
    }

    void visit_Unreachable() {}

    void visit_Return() {}

    void call_imported_function(uint32_t func_index) {
        switch (func_index) {
            case 0: {  // print_i32
                m_a.asm_call_label("print_i32");
                m_a.asm_pop_r32(X86Reg::eax);
                break;
            }
            case 1: {  // print_i64
                std::cerr << "Call to print_i64() is not yet supported";
                break;
            }
            case 2: {  // print_f32
                std::cerr << "Call to print_f32() is not yet supported";
                break;
            }
            case 3: {  // print_f64
                m_a.asm_call_label("print_f64");
                m_a.asm_add_r32_imm32(X86Reg::esp, 4);  // increment stack top and thus pop the value passed as argument
                break;
            }
            case 4: {  // print_str
                {
                    // pop the string length and string location
                    // we do not need them at the moment
                    m_a.asm_pop_r32(X86Reg::eax);
                    m_a.asm_pop_r32(X86Reg::eax);
                }
                std::string label =
                    "string" + std::to_string(last_last_vis_i32_const);
                emit_print(m_a, label,
                           label_to_str[label].size());
                break;
            }
            case 5: {  // flush_buf
                std::string label = "string_newline";
                std::string msg = "\n";
                emit_print(m_a, label, msg.size());
                label_to_str["string_newline"] = msg;
                break;
            }
            case 6: {  // set_exit_code
                m_a.asm_jmp_label("my_exit");
                break;
            }
            default: {
                std::cerr << "Unsupported func_index";
            }
        }
    }

    void visit_Call(uint32_t func_index) {
        if (func_index <= 6U) {
            call_imported_function(func_index);
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

    void visit_Br(uint32_t label_index) {
        // Branch is used to jump to the `loop.head` or `loop.end`.
        if (loop_unique_id.size() + if_unique_id.size() - cur_nesting_length
                == label_index  + 1) {
            // cycle/continue or loop.end
            m_a.asm_jmp_label(".loop.head_" + loop_unique_id.back());
        } else {
            // exit/break
            m_a.asm_jmp_label(".loop.end_" + loop_unique_id.back());
        }
    }

    void visit_Loop() {
        uint32_t prev_nesting_length = cur_nesting_length;
        cur_nesting_length = loop_unique_id.size() + if_unique_id.size();
        loop_unique_id.push_back(std::to_string(offset));
        /*
        The loop statement starts with `loop.head`. The `loop.body` and
        `loop.branch` are enclosed within the `if.block`. If the condition
        fails, the loop is exited through `else.block`.
        .head
            .If
                # Statements
                .Br
            .Else
            .endIf
        .end
        */
        m_a.add_label(".loop.head_" + loop_unique_id.back());
        {
            decode_instructions();
        }
        // end
        m_a.add_label(".loop.end_" + loop_unique_id.back());
        loop_unique_id.pop_back();
        cur_nesting_length = prev_nesting_length;
    }

    void visit_If() {
        if_unique_id.push_back(std::to_string(offset));
        // `eax` contains the logical value (true = 1, false = 0)
        // of the if condition
        m_a.asm_pop_r32(X86Reg::eax);
        m_a.asm_cmp_r32_imm8(X86Reg::eax, 1);
        m_a.asm_je_label(".then_" + if_unique_id.back());
        m_a.asm_jmp_label(".else_" + if_unique_id.back());
        m_a.add_label(".then_" + if_unique_id.back());
        {
            decode_instructions();
        }
        m_a.add_label(".endif_" + if_unique_id.back());
        if_unique_id.pop_back();
    }

    void visit_Else() {
        m_a.asm_jmp_label(".endif_" + if_unique_id.back());
        m_a.add_label(".else_" + if_unique_id.back());
    }

    void visit_LocalGet(uint32_t localidx) {
        X86Reg base = X86Reg::ebp;
        auto cur_func_param_type = func_types[type_indices[cur_func_idx]];
        int no_of_params = (int)cur_func_param_type.param_types.size();
        if ((int)localidx < no_of_params) {
            std::string var_type = var_type_to_string[cur_func_param_type.param_types[localidx]];
            if (var_type == "i32") {
                m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, 8 + 4 * localidx);
                m_a.asm_push_r32(X86Reg::eax);
            } else if (var_type == "f64") {
                m_a.asm_push_imm32(0); // decrement stack top and thus create space for value to get
                X86Reg stack_top = X86Reg::esp;
                m_a.asm_fld_m32(&base, nullptr, 1, 8 + 4 * localidx);
                m_a.asm_fstp_m32(&stack_top, nullptr, 1, 0);
            } else {
                throw CodeGenError("WASM_X86: Var type not supported");
            }

        } else {
            localidx -= no_of_params;
            std::string var_type = var_type_to_string[codes[cur_func_idx].locals[localidx].type];
            if (var_type == "i32") {
                m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, -4 - 4 * localidx);
                m_a.asm_push_r32(X86Reg::eax);
            } else if (var_type == "f64") {
                m_a.asm_push_imm32(0); // decrement stack top and thus create space for value to get
                X86Reg stack_top = X86Reg::esp;
                m_a.asm_fld_m32(&base, nullptr, 1, -4 - 4 * localidx);
                m_a.asm_fstp_m32(&stack_top, nullptr, 1, 0);
            } else {
                throw CodeGenError("WASM_X86: Var type not supported");
            }
        }
    }
    void visit_LocalSet(uint32_t localidx) {
        X86Reg base = X86Reg::ebp;
        auto cur_func_param_type = func_types[type_indices[cur_func_idx]];
        int no_of_params = (int)cur_func_param_type.param_types.size();
        if ((int)localidx < no_of_params) {
            std::string var_type = var_type_to_string[cur_func_param_type.param_types[localidx]];
            if (var_type == "i32") {
                m_a.asm_pop_r32(X86Reg::eax);
                m_a.asm_mov_m32_r32(&base, nullptr, 1, 8 + 4 * localidx, X86Reg::eax);
            } else if (var_type == "f64") {
                X86Reg stack_top = X86Reg::esp;
                m_a.asm_fld_m32(&stack_top, nullptr, 1, 0); // load stack top into floating register stack
                m_a.asm_fstp_m32(&base, nullptr, 1, 8 + 4 * localidx); // store float at variable location
                m_a.asm_add_r32_imm32(X86Reg::esp, 4); // increment stack top and thus pop the value to be set
            } else {
                throw CodeGenError("WASM_X86: Var type not supported");
            }

        } else {
            localidx -= no_of_params;
            std::string var_type = var_type_to_string[codes[cur_func_idx].locals[localidx].type];
            if (var_type == "i32") {
                m_a.asm_pop_r32(X86Reg::eax);
                m_a.asm_mov_m32_r32(&base, nullptr, 1, -4 - 4 * localidx, X86Reg::eax);
            } else if (var_type == "f64") {
                X86Reg stack_top = X86Reg::esp;
                m_a.asm_fld_m32(&stack_top, nullptr, 1, 0); // load stack top into floating register stack
                m_a.asm_fstp_m32(&base, nullptr, 1, -4 - 4 * localidx); // store float at variable location
                m_a.asm_add_r32_imm32(X86Reg::esp, 4); // increment stack top and thus pop the value to be set
            } else {
                throw CodeGenError("WASM_X86: Var type not supported");
            }
        }
    }

    void visit_I32Eqz() {
        m_a.asm_push_imm32(0U);
        handle_I32Compare("Eq");
    }

    void visit_I32Const(int32_t value) {
        if (!decoding_data_segment) {
            m_a.asm_push_imm32(value);
        }

        // TODO: Following seems/is hackish. Fix/Improve it.
        last_last_vis_i32_const = last_vis_i32_const;
        last_vis_i32_const = value;
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

    void handle_I32Compare(const std::string &compare_op) {
        std::string label = std::to_string(offset);
        m_a.asm_pop_r32(X86Reg::ebx);
        m_a.asm_pop_r32(X86Reg::eax);
        // `eax` and `ebx` contain the left and right operands, respectively
        m_a.asm_cmp_r32_r32(X86Reg::eax, X86Reg::ebx);
        if (compare_op == "Eq") {
            m_a.asm_je_label(".compare_1" + label);
        } else if (compare_op == "Gt") {
            m_a.asm_jg_label(".compare_1" + label);
        } else if (compare_op == "GtE") {
            m_a.asm_jge_label(".compare_1" + label);
        } else if (compare_op == "Lt") {
            m_a.asm_jl_label(".compare_1" + label);
        } else if (compare_op == "LtE") {
            m_a.asm_jle_label(".compare_1" + label);
        } else if (compare_op == "NotEq") {
            m_a.asm_jne_label(".compare_1" + label);
        } else {
            throw CodeGenError("Comparison operator not implemented");
        }
        // if the `compare` condition in `true`, jump to compare_1
        // and assign `1` else assign `0`
        m_a.asm_push_imm8(0);
        m_a.asm_jmp_label(".compare.end_" + label);
        m_a.add_label(".compare_1" + label);
        m_a.asm_push_imm8(1);
        m_a.add_label(".compare.end_" + label);
    }

    void visit_I32Eq() { handle_I32Compare("Eq"); }
    void visit_I32GtS() { handle_I32Compare("Gt"); }
    void visit_I32GeS() { handle_I32Compare("GtE"); }
    void visit_I32LtS() { handle_I32Compare("Lt"); }
    void visit_I32LeS() { handle_I32Compare("LtE"); }
    void visit_I32Ne() { handle_I32Compare("NotEq"); }

    void visit_F64Const(double Z) {
        float z = Z; // down cast 64-bit double to 32-bit float
        std::string label = "float_" + std::to_string(z);
        float_consts[label] = z;
        m_a.asm_mov_r32_label(X86Reg::eax, label);
        X86Reg label_reg = X86Reg::eax;
        m_a.asm_fld_m32(&label_reg, nullptr, 1, 0); // loads into floating register stack
        X86Reg stack_top = X86Reg::esp;
        m_a.asm_push_imm32(0); // decrement stack and create space
        m_a.asm_fstp_m32(&stack_top, nullptr, 1, 0); // store float on integer stack top;
    }

    void gen_x86_bytes() {
        emit_elf32_header(m_a);

        // Add runtime library functions
        emit_print_int(m_a, "print_i32");
        emit_print_float(m_a, "print_f64");
        emit_exit2(m_a, "my_exit");

        decoding_data_segment = true;
        // declare compile-time strings
        for (uint32_t i = 0; i < data_segments.size(); i++) {
            offset = data_segments[i].insts_start_index;
            decode_instructions();
            std::string label = "string" + std::to_string(last_vis_i32_const);
            label_to_str[label] = data_segments[i].text;
        }
        decoding_data_segment = false;

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

        for (auto &s : label_to_str) {
            emit_data_string(m_a, s.first, s.second);
        }

        for (auto &f : float_consts) {
            emit_float_const(m_a, f.first, f.second);
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

    X86Assembler m_a(al, false /* bits 64 */);

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

}  // namespace LCompilers
