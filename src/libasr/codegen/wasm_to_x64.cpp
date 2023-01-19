#include <fstream>
#include <chrono>
#include <iomanip>

#include <libasr/assert.h>
#include <libasr/codegen/wasm_decoder.h>
#include <libasr/codegen/wasm_to_x64.h>
#include <libasr/codegen/x86_assembler.h>

namespace LCompilers {

namespace wasm {

/*

This X64Visitor uses stack to pass arguments and return values from functions.
Since in X64, instructions operate on registers (and not on stack),
for every instruction we pop elements from top of stack and store them into
registers. After operating on the registers, the result value is then
pushed back onto the stack.

One of the reasons to use stack to pass function arguments is that,
it allows us to define and call functions with any number of parameters.
As registers are limited in number, if we use them to pass function arguments,
the number of arguments we could pass to a function would get limited by
the number of registers available with the CPU.

*/
enum Block {
    LOOP = 0,
    IF = 1
};

class X64Visitor : public WASMDecoder<X64Visitor>,
                   public WASM_INSTS_VISITOR::BaseWASMVisitor<X64Visitor> {
   public:
    X86Assembler &m_a;
    uint32_t cur_func_idx;
    int32_t last_vis_i32_const, last_last_vis_i32_const;
    std::map<std::string, std::string> label_to_str;
    uint32_t block_id;
    std::vector<std::pair<uint32_t, Block>> blocks;
    std::map<std::string, double> double_consts;

    /*
     A data segment in wasm uses a set of instructions/expressions to specify
     the offset in memory where the string/data is to be stored.
     To obtain the string offset we need to decode these set of instructions.
     Since, we currently support compile-time strings and the string offset is
     stored in last_vis_i32_const, the instructions are not needed for us at the moment.
     The following variables helps us control the emitting of instructions when
     deconding a data segment in wasm.
    */
    bool decoding_data_segment;

    X64Visitor(X86Assembler &m_a, Allocator &al,
               diag::Diagnostics &diagonostics, Vec<uint8_t> &code)
        : WASMDecoder(al, diagonostics),
          BaseWASMVisitor(code, 0U /* temporary offset */),
          m_a(m_a) {
        wasm_bytes.from_pointer_n(code.data(), code.size());
        block_id = 1;
        decoding_data_segment = false;
    }

    void visit_Return() {
        // Restore stack
        m_a.asm_mov_r64_r64(X64Reg::rsp, X64Reg::rbp);
        m_a.asm_pop_r64(X64Reg::rbp);
        m_a.asm_ret();
    }

    void visit_Unreachable() {}

    void visit_EmtpyBlockType() {}

    void call_imported_function(uint32_t func_idx) {
        switch (func_idx) {
            case 0: {  // print_i32
                m_a.asm_call_label("print_i64");
                m_a.asm_pop_r64(X64Reg::r15); // pop the passed argument
                break;
            }
            case 1: {  // print_i64
                std::cerr << "Call to print_i64() is not yet supported\n";
                break;
            }
            case 2: {  // print_f32
                std::cerr << "Call to print_f32() is not yet supported\n";
                break;
            }
            case 3: {  // print_f64
                m_a.asm_call_label("print_f64");
                m_a.asm_add_r64_imm32(X64Reg::rsp, 8); // pop the passed argument
                break;
            }
            case 4: {  // print_str

                // pop the string length and string location
                // we do not need them at the moment
                m_a.asm_pop_r64(X64Reg::rax);
                m_a.asm_pop_r64(X64Reg::rax);

                // we need compile-time string length and location
                std::string label = "string" + std::to_string(last_last_vis_i32_const);
                emit_print_64(m_a, label, label_to_str[label].size());
                break;
            }
            case 5: {  // flush_buf
                emit_print_64(m_a, "string_newline", 1);
                break;
            }
            case 6: {  // set_exit_code
                m_a.asm_pop_r64(X64Reg::rdi); // get exit code from stack top
                m_a.asm_mov_r64_imm64(X64Reg::rax, 60); // sys_exit
                m_a.asm_syscall(); // syscall
                break;
            }
            default: {
                std::cerr << "Unsupported func_idx";
            }
        }
    }

    void visit_Call(uint32_t func_idx) {
        if (func_idx <= 6U) {
            call_imported_function(func_idx);
            return;
        }

        func_idx -= 7u; // adjust function index as per imports
        m_a.asm_call_label(exports[func_idx].name);

        // Pop the passed function arguments
        wasm::FuncType func_type = func_types[type_indices[func_idx]];
        m_a.asm_add_r64_imm32(X64Reg::rsp, 8 * func_type.param_types.size()); // pop the passed argument

        // Adjust the return values of the called function
        X64Reg base = X64Reg::rsp;
        for (uint32_t i = 0; i < func_type.result_types.size(); i++) {
            // take value into eax
            m_a.asm_mov_r64_m64(X64Reg::rax, &base, nullptr, 1,
                -8 * (func_type.param_types.size() + 2 +
                       codes[func_idx].locals.size() + 1));

            // push eax value onto stack
            m_a.asm_push_r64(X64Reg::rax);
        }
    }

    void visit_Loop() {
        std::string label = std::to_string(block_id);
        blocks.push_back({block_id++, Block::LOOP});
        /*
        The loop statement starts with `loop.head`. The `loop.body` and
        `loop.branch` are enclosed within the `if.block`. If the condition
        fails, the loop is exited through `else.block`.
        .head
            .If
                # Statements
                .Br to loop head
            .Else
            .endIf
        .end
        */
        m_a.add_label(".loop.head_" + label);
        {
            decode_instructions();
        }
        // end
        m_a.add_label(".loop.end_" + label);
        blocks.pop_back();
    }

    void visit_Br(uint32_t labelidx) {
        // Branch is used to jump to the `loop.head` or `loop.end`.

        uint32_t b_id;
        Block block_type;
        std::tie(b_id, block_type) = blocks[blocks.size() - 1 - labelidx];
        std::string label = std::to_string(b_id);
        switch (block_type) {
            /*
            From WebAssembly Docs:
                The exact effect of branch depends on that control construct.
                In case of block or if it is a forward jump, resuming execution after the matching end.
                In case of loop it is a backward jump to the beginning of the loop.
            */
            case Block::LOOP: m_a.asm_jmp_label(".loop.head_" + label); break;
            case Block::IF: m_a.asm_jmp_label(".else_" + label); break;
        }
    }

    void visit_If() {
        std::string label = std::to_string(block_id);
        blocks.push_back({block_id++, Block::IF});
        m_a.asm_pop_r64(X64Reg::rax); // now `rax` contains the logical value (true = 1, false = 0) of the if condition
        m_a.asm_cmp_r64_imm8(X64Reg::rax, 1);
        m_a.asm_je_label(".then_" + label);
        m_a.asm_jmp_label(".else_" + label);
        m_a.add_label(".then_" + label);
        {
            decode_instructions();
        }
        m_a.add_label(".endif_" + label);
        blocks.pop_back();
    }

    void visit_Else() {
        std::string label = std::to_string(blocks.back().first);
        m_a.asm_jmp_label(".endif_" + label);
        m_a.add_label(".else_" + label);
    }

    void visit_LocalGet(uint32_t localidx) {
        X64Reg base = X64Reg::rbp;
        auto cur_func_param_type = func_types[type_indices[cur_func_idx]];
        int no_of_params = (int)cur_func_param_type.param_types.size();
        if ((int)localidx < no_of_params) {
            std::string var_type = var_type_to_string[cur_func_param_type.param_types[localidx]];
            if (var_type == "i32") {
                m_a.asm_mov_r64_m64(X64Reg::rax, &base, nullptr, 1, 8 * (2 + no_of_params - (int)localidx - 1));
                m_a.asm_push_r64(X64Reg::rax);
            } else if (var_type == "f64") {
                m_a.asm_sub_r64_imm32(X64Reg::rsp,  8); // create space for value to be fetched
                m_a.asm_movsd_r64_m64(X64FReg::xmm0, &base, nullptr, 1, 8 * (2 + no_of_params - (int)localidx - 1));
                X64Reg stack_top = X64Reg::rsp;
                m_a.asm_movsd_m64_r64(&stack_top, nullptr, 1, 0, X64FReg::xmm0);
            } else {
                throw CodeGenError("WASM_X64: Var type not supported");
            }
        } else {
            localidx -= no_of_params;
            std::string var_type = var_type_to_string[codes[cur_func_idx].locals[localidx].type];
            if (var_type == "i32") {
                m_a.asm_mov_r64_m64(X64Reg::rax, &base, nullptr, 1, -8 * (1 + (int)localidx));
                m_a.asm_push_r64(X64Reg::rax);
            } else if (var_type == "f64") {
                m_a.asm_sub_r64_imm32(X64Reg::rsp,  8); // create space for value to be fetched
                m_a.asm_movsd_r64_m64(X64FReg::xmm0, &base, nullptr, 1, -8 * (1 + (int)localidx));
                X64Reg stack_top = X64Reg::rsp;
                m_a.asm_movsd_m64_r64(&stack_top, nullptr, 1, 0, X64FReg::xmm0);
            } else {
                throw CodeGenError("WASM_X64: Var type not supported");
            }
        }
    }

    void visit_LocalSet(uint32_t localidx) {
        X64Reg base = X64Reg::rbp;
        auto cur_func_param_type = func_types[type_indices[cur_func_idx]];
        int no_of_params = (int)cur_func_param_type.param_types.size();
        if ((int)localidx < no_of_params) {
            std::string var_type = var_type_to_string[cur_func_param_type.param_types[localidx]];
            if (var_type == "i32") {
                m_a.asm_pop_r64(X64Reg::rax);
                m_a.asm_mov_m64_r64(&base, nullptr, 1, 8 * (2 + no_of_params - (int)localidx - 1), X64Reg::rax);
            } else if (var_type == "f64") {
                X64Reg stack_top = X64Reg::rsp;
                m_a.asm_movsd_r64_m64(X64FReg::xmm0, &stack_top, nullptr, 1, 0);
                m_a.asm_movsd_m64_r64(&base, nullptr, 1, 8 * (2 + no_of_params - (int)localidx - 1), X64FReg::xmm0);
                m_a.asm_add_r64_imm32(X64Reg::rsp, 8); // remove from stack top
            } else {
                throw CodeGenError("WASM_X64: Var type not supported");
            }
        } else {
            localidx -= no_of_params;
            std::string var_type = var_type_to_string[codes[cur_func_idx].locals[localidx].type];
            if (var_type == "i32") {
                m_a.asm_pop_r64(X64Reg::rax);
                m_a.asm_mov_m64_r64(&base, nullptr, 1, -8 * (1 + (int)localidx), X64Reg::rax);
            } else if (var_type == "f64") {
                X64Reg stack_top = X64Reg::rsp;
                m_a.asm_movsd_r64_m64(X64FReg::xmm0, &stack_top, nullptr, 1, 0);
                m_a.asm_movsd_m64_r64(&base, nullptr, 1, -8 * (1 + (int)localidx), X64FReg::xmm0);
                m_a.asm_add_r64_imm32(X64Reg::rsp, 8); // remove from stack top
            } else {
                throw CodeGenError("WASM_X64: Var type not supported");
            }
        }
    }

    void visit_I32Const(int32_t value) {
        if (!decoding_data_segment) {
            m_a.asm_mov_r64_imm64(X64Reg::rax, labs((int64_t)value));
            if (value < 0) m_a.asm_neg_r64(X64Reg::rax);
            m_a.asm_push_r64(X64Reg::rax);
        }

        // TODO: Following seems/is hackish. Fix/Improve it.
        last_last_vis_i32_const = last_vis_i32_const;
        last_vis_i32_const = value;
    }

    template<typename F>
    void handleI32Opt(F && f) {
        m_a.asm_pop_r64(X64Reg::rbx);
        m_a.asm_pop_r64(X64Reg::rax);
        f();
        m_a.asm_push_r64(X64Reg::rax);
    }

    void visit_I32Add() {
        handleI32Opt([&](){ m_a.asm_add_r64_r64(X64Reg::rax, X64Reg::rbx);});
    }
    void visit_I32Sub() {
        handleI32Opt([&](){ m_a.asm_sub_r64_r64(X64Reg::rax, X64Reg::rbx);});
    }
    void visit_I32Mul() {
        handleI32Opt([&](){ m_a.asm_mul_r64(X64Reg::rbx);});
    }
    void visit_I32DivS() {
        handleI32Opt([&](){ m_a.asm_div_r64(X64Reg::rbx);});
    }

    void visit_I32Eqz() {
        m_a.asm_mov_r64_imm64(X64Reg::rax, 0);
        m_a.asm_push_r64(X64Reg::rax);
        handle_I32Compare<&X86Assembler::asm_je_label>();
    }

    using JumpFn = void(X86Assembler::*)(const std::string&);
    template<JumpFn T>
    void handle_I32Compare() {
        std::string label = std::to_string(offset);
        m_a.asm_pop_r64(X64Reg::rbx);
        m_a.asm_pop_r64(X64Reg::rax);
        // `rax` and `rbx` contain the left and right operands, respectively
        m_a.asm_cmp_r64_r64(X64Reg::rax, X64Reg::rbx);

        (m_a.*T)(".compare_1" + label);

        // if the `compare` condition in `true`, jump to compare_1
        // and assign `1` else assign `0`
        m_a.asm_push_imm8(0);
        m_a.asm_jmp_label(".compare.end_" + label);
        m_a.add_label(".compare_1" + label);
        m_a.asm_push_imm8(1);
        m_a.add_label(".compare.end_" + label);
    }

    void visit_I32Eq() { handle_I32Compare<&X86Assembler::asm_je_label>(); }
    void visit_I32GtS() { handle_I32Compare<&X86Assembler::asm_jg_label>(); }
    void visit_I32GeS() { handle_I32Compare<&X86Assembler::asm_jge_label>(); }
    void visit_I32LtS() { handle_I32Compare<&X86Assembler::asm_jl_label>(); }
    void visit_I32LeS() { handle_I32Compare<&X86Assembler::asm_jle_label>(); }
    void visit_I32Ne() { handle_I32Compare<&X86Assembler::asm_jne_label>(); }

    void visit_F64Const(double z) {
        std::string label = "float_" + std::to_string(z);
        // std::cerr << label << " " << z << std::endl;
        double_consts[label] = z;
        m_a.asm_mov_r64_label(X64Reg::rax, label);
        X64Reg label_reg = X64Reg::rax;
        m_a.asm_movsd_r64_m64(X64FReg::xmm0, &label_reg, nullptr, 1, 0); // load into floating-point register
        m_a.asm_sub_r64_imm32(X64Reg::rsp, 8); // decrement stack and create space
        X64Reg stack_top = X64Reg::rsp;
        m_a.asm_movsd_m64_r64(&stack_top, nullptr, 1, 0, X64FReg::xmm0); // store float on integer stack top;
    }

    using F64OptFn = void(X86Assembler::*)(X64FReg, X64FReg);
    template<F64OptFn T>
    void handleF64Operations() {
        X64Reg stack_top = X64Reg::rsp;
        // load second operand into floating-point register
        m_a.asm_movsd_r64_m64(X64FReg::xmm1, &stack_top, nullptr, 1, 0);
        m_a.asm_add_r64_imm32(X64Reg::rsp, 8); // pop the argument
        // load first operand into floating-point register
        m_a.asm_movsd_r64_m64(X64FReg::xmm0, &stack_top, nullptr, 1, 0);
        // no need to pop this operand since we need space to output back result

        (m_a.*T)(X64FReg::xmm0, X64FReg::xmm1);

        // store float result back on stack top;
        m_a.asm_movsd_m64_r64(&stack_top, nullptr, 1, 0, X64FReg::xmm0);
    }

    void visit_F64Add() { handleF64Operations<&X86Assembler::asm_addsd_r64_r64>(); }
    void visit_F64Sub() { handleF64Operations<&X86Assembler::asm_subsd_r64_r64>(); }
    void visit_F64Mul() { handleF64Operations<&X86Assembler::asm_mulsd_r64_r64>(); }
    void visit_F64Div() { handleF64Operations<&X86Assembler::asm_divsd_r64_r64>(); }

    template<JumpFn T>
    void handleF64Compare() {
        std::string label = std::to_string(offset);

        X64Reg stack_top = X64Reg::rsp;
        // load second operand into floating-point register
        m_a.asm_movsd_r64_m64(X64FReg::xmm1, &stack_top, nullptr, 1, 0);
        m_a.asm_add_r64_imm32(X64Reg::rsp, 8); // pop the argument
        // load first operand into floating-point register
        m_a.asm_movsd_r64_m64(X64FReg::xmm0, &stack_top, nullptr, 1, 0);
        m_a.asm_add_r64_imm32(X64Reg::rsp, 8); // pop the argument

        // m_a.asm_uomisd_r64_r64(X64FReg::xmm0, X64FReg::xmm1);
        m_a.asm_comisd_r64_r64(X64FReg::xmm0, X64FReg::xmm1);

        (m_a.*T)(".compare_1" + label);

        // if the `compare` condition in `true`, jump to compare_1
        // and assign `1` else assign `0`
        m_a.asm_push_imm8(0);
        m_a.asm_jmp_label(".compare.end_" + label);
        m_a.add_label(".compare_1" + label);
        m_a.asm_push_imm8(1);
        m_a.add_label(".compare.end_" + label);
    }

    void visit_F64Eq() { handleF64Compare<&X86Assembler::asm_je_label>(); }
    void visit_F64Gt() { handleF64Compare<&X86Assembler::asm_jg_label>(); }
    void visit_F64Ge() { handleF64Compare<&X86Assembler::asm_jge_label>(); }
    void visit_F64Lt() { handleF64Compare<&X86Assembler::asm_jl_label>(); }
    void visit_F64Le() { handleF64Compare<&X86Assembler::asm_jle_label>(); }
    void visit_F64Ne() { handleF64Compare<&X86Assembler::asm_jne_label>(); }

    void gen_x64_bytes() {
        {   // Initialize/Modify values of entities
            exports.back().name = "_start"; // Update _lcompilers_main() to _start
            label_to_str["string_newline"] = "\n";
            label_to_str["string_neg"] = "-"; // - symbol for printing negative ints
            label_to_str["string_dot"] = "."; // . symbol for printing floats
        }

        emit_elf64_header(m_a);
        emit_print_int_64(m_a, "print_i64");
        emit_print_double(m_a, "print_f64");

        decoding_data_segment = true;
        // declare compile-time strings
        for (uint32_t i = 0; i < data_segments.size(); i++) {
            offset = data_segments[i].insts_start_index;
            decode_instructions();
            std::string label = "string" + std::to_string(last_vis_i32_const);
            label_to_str[label] = data_segments[i].text;
        }
        decoding_data_segment = false;

        for (uint32_t idx = 0; idx < type_indices.size(); idx++) {
            m_a.add_label(exports[idx].name);
            {
                // Initialize the stack
                m_a.asm_push_r64(X64Reg::rbp);
                m_a.asm_mov_r64_r64(X64Reg::rbp, X64Reg::rsp);

                // Allocate space for local variables
                // TODO: locals is an array where every element has a count (currently wasm emits count = 1 always)
                m_a.asm_sub_r64_imm32(X64Reg::rsp, 8 * codes[idx].locals.size());

                offset = codes[idx].insts_start_index;
                cur_func_idx = idx;
                decode_instructions();
            }

        }

        for (auto &s : label_to_str) {
            emit_data_string(m_a, s.first, s.second);
        }

        for (auto &d : double_consts) {
            emit_double_const(m_a, d.first, d.second);
        }

        emit_elf64_footer(m_a);
    }
};

}  // namespace wasm

Result<int> wasm_to_x64(Vec<uint8_t> &wasm_bytes, Allocator &al,
                        const std::string &filename, bool time_report,
                        diag::Diagnostics &diagnostics) {
    int time_decode_wasm = 0;
    int time_gen_x64_bytes = 0;
    int time_save = 0;
    int time_verify = 0;

    X86Assembler m_a(al, true /* bits 64 */);

    wasm::X64Visitor x64_visitor(m_a, al, diagnostics, wasm_bytes);

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        try {
            x64_visitor.decode_wasm();
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
        x64_visitor.gen_x64_bytes();
        auto t2 = std::chrono::high_resolution_clock::now();
        time_gen_x64_bytes =
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
    // std::cout << x64_visitor.m_a.get_asm() << std::endl;

    if (time_report) {
        std::cout << "Codegen Time report:" << std::endl;
        std::cout << "Decode wasm: " << std::setw(5) << time_decode_wasm
                  << std::endl;
        std::cout << "Generate asm: " << std::setw(5) << time_gen_x64_bytes
                  << std::endl;
        std::cout << "Verify:       " << std::setw(5) << time_verify
                  << std::endl;
        std::cout << "Save:       " << std::setw(5) << time_save << std::endl;
        int total =
            time_decode_wasm + time_gen_x64_bytes + time_verify + time_save;
        std::cout << "Total:      " << std::setw(5) << total << std::endl;
    }
    return 0;
}

}  // namespace LCompilers
