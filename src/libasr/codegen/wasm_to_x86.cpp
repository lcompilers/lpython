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
As registers are limited in number, if we use them to pass function arguments,
the number of arguments we could pass to a function would get limited by
the number of registers available with the CPU.

*/

enum Block {
    LOOP = 0,
    IF = 1
};

class X86Visitor : public WASMDecoder<X86Visitor>,
                   public WASM_INSTS_VISITOR::BaseWASMVisitor<X86Visitor> {
   public:
    X86Assembler &m_a;
    uint32_t cur_func_idx;
    uint32_t block_id;
    uint32_t NO_OF_IMPORTS;
    std::vector<std::pair<uint32_t, Block>> blocks;
    std::map<std::string, std::string> label_to_str;
    std::map<std::string, float> float_consts;

    X86Visitor(X86Assembler &m_a, Allocator &al,
               diag::Diagnostics &diagonostics, Vec<uint8_t> &code)
        : WASMDecoder(al, diagonostics),
          BaseWASMVisitor(code, 0U /* temporary offset */),
          m_a(m_a) {
        wasm_bytes.from_pointer_n(code.data(), code.size());
        block_id = 1;
        NO_OF_IMPORTS = 0;
    }

    void visit_Unreachable() {}

    void visit_EmtpyBlockType() {}

    void visit_Drop() { m_a.asm_pop_r32(X86Reg::eax); }

    void visit_Return() {
        // Restore stack
        m_a.asm_mov_r32_r32(X86Reg::esp, X86Reg::ebp);
        m_a.asm_pop_r32(X86Reg::ebp);
        m_a.asm_ret();
    }

    void call_imported_function(uint32_t func_index) {
        switch (func_index) {
            case 0: {  // proc_exit
                m_a.asm_jmp_label("my_exit");
                break;
            }
            case 1: {  // fd_write
            /*
                TODO: This way increases the number of instructions.
                There is a possibility that we can wrap these statements
                with some add label and then just jump/call to that label
            */

                m_a.asm_pop_r32(X86Reg::eax); // mem_loc to write return value (not usefull for us currently)
                m_a.asm_pop_r32(X86Reg::eax); // no of iov vectors (always emitted 1 by wasm, not usefull for us currently)
                m_a.asm_pop_r32(X86Reg::eax); // mem_loc to string iov vector
                m_a.asm_pop_r32(X86Reg::ebx); // filetypes (1 for stdout)

                m_a.asm_mov_r32_label(X86Reg::esi, "base_memory");
                m_a.asm_add_r32_r32(X86Reg::esi, X86Reg::eax);

                X86Reg base = X86Reg::esi;
                m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, 0); // location
                m_a.asm_mov_r32_m32(X86Reg::edx, &base, nullptr, 1, 4); // length

                {
                    // ssize_t write(int fd, const void *buf, size_t count);
                    m_a.asm_mov_r32_imm32(X86Reg::ebx, 1); // fd (stdout)
                    m_a.asm_mov_r32_label(X86Reg::ecx, "base_memory");
                    m_a.asm_add_r32_r32(X86Reg::ecx, X86Reg::eax);
                    m_a.asm_mov_r32_imm32(X86Reg::eax, 4); // sys_write
                    // ecx stores location, length is already stored in edx
                    m_a.asm_int_imm8(0x80);

                    m_a.asm_push_r32(X86Reg::eax); // push return value onto stack
                }


                break;
            }
            default: {
                std::cerr << "Unsupported func_index: " << func_index << std::endl;
            }
        }
    }

    void visit_Call(uint32_t func_index) {
        if (func_index < NO_OF_IMPORTS) {
            call_imported_function(func_index);
            return;
        }

        func_index -= NO_OF_IMPORTS;
        m_a.asm_call_label(exports[func_index + 1 /* offset by 1 becaz of mem export */].name);

        // Pop the passed function arguments
        wasm::FuncType func_type =
            func_types[type_indices[func_index]];
        m_a.asm_add_r32_imm32(X86Reg::esp, 4 * func_type.param_types.size()); // pop the passed arguments

        // Adjust the return values of the called function
        X86Reg base = X86Reg::esp;
        for (uint32_t i = 0; i < func_type.result_types.size(); i++) {
            // take value into eax
            m_a.asm_mov_r32_m32(
                X86Reg::eax, &base, nullptr, 1,
                -(4 * (func_type.param_types.size() + 2 +
                       codes[func_index].locals.size() + 1)));

            // push eax value onto stack
            m_a.asm_push_r32(X86Reg::eax);
        }
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
                In case of block or if, it is a forward jump, resuming execution after the matching end.
                In case of loop, it is a backward jump to the beginning of the loop.
            */
            case Block::LOOP: m_a.asm_jmp_label(".loop.head_" + label); break;
            case Block::IF: m_a.asm_jmp_label(".else_" + label); break;
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
                .Br
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

    void visit_If() {
        std::string label = std::to_string(block_id);
        blocks.push_back({block_id++, Block::IF});
        m_a.asm_pop_r32(X86Reg::eax); // now `eax` contains the logical value (true = 1, false = 0) of the if condition
        m_a.asm_cmp_r32_imm8(X86Reg::eax, 1);
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
        X86Reg base = X86Reg::ebp;
        auto cur_func_param_type = func_types[type_indices[cur_func_idx]];
        int no_of_params = (int)cur_func_param_type.param_types.size();
        if ((int)localidx < no_of_params) {
            std::string var_type = vt2s(cur_func_param_type.param_types[localidx]);
            if (var_type == "i32") {
                m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, 4 * (2 + no_of_params - (int)localidx - 1));
                m_a.asm_push_r32(X86Reg::eax);
            } else if (var_type == "i64") {
                m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, 4 * (2 + no_of_params - (int)localidx - 1));
                m_a.asm_push_r32(X86Reg::eax);
            } else if (var_type == "f64") {
                m_a.asm_sub_r32_imm32(X86Reg::esp,  4); // create space for value to be fetched
                X86Reg stack_top = X86Reg::esp;
                m_a.asm_fld_m32(&base, nullptr, 1, 4 * (2 + no_of_params - (int)localidx - 1));
                m_a.asm_fstp_m32(&stack_top, nullptr, 1, 0);
            } else {
                throw AssemblerError("WASM_X86: Var type not supported");
            }

        } else {
            localidx -= no_of_params;
            std::string var_type = vt2s(codes[cur_func_idx].locals[localidx].type);
            if (var_type == "i32") {
                m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, -4 * (1 + localidx));
                m_a.asm_push_r32(X86Reg::eax);
            } else if (var_type == "i64") {
                m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, -4 * (1 + localidx));
                m_a.asm_push_r32(X86Reg::eax);
            } else if (var_type == "f64") {
                m_a.asm_sub_r32_imm32(X86Reg::esp,  4); // create space for value to be fetched
                X86Reg stack_top = X86Reg::esp;
                m_a.asm_fld_m32(&base, nullptr, 1, -4 * (1 + localidx));
                m_a.asm_fstp_m32(&stack_top, nullptr, 1, 0);
            } else {
                throw AssemblerError("WASM_X86: Var type not supported");
            }
        }
    }
    void visit_LocalSet(uint32_t localidx) {
        X86Reg base = X86Reg::ebp;
        auto cur_func_param_type = func_types[type_indices[cur_func_idx]];
        int no_of_params = (int)cur_func_param_type.param_types.size();
        if ((int)localidx < no_of_params) {
            std::string var_type = vt2s(cur_func_param_type.param_types[localidx]);
            if (var_type == "i32") {
                m_a.asm_pop_r32(X86Reg::eax);
                m_a.asm_mov_m32_r32(&base, nullptr, 1, 4 * (2 + no_of_params - (int)localidx - 1), X86Reg::eax);
            } else if (var_type == "i64") {
                m_a.asm_pop_r32(X86Reg::eax);
                m_a.asm_mov_m32_r32(&base, nullptr, 1, 4 * (2 + no_of_params - (int)localidx - 1), X86Reg::eax);
            } else if (var_type == "f64") {
                X86Reg stack_top = X86Reg::esp;
                m_a.asm_fld_m32(&stack_top, nullptr, 1, 0); // load stack top into floating register stack
                m_a.asm_fstp_m32(&base, nullptr, 1, 4 * (2 + no_of_params - (int)localidx - 1)); // store float at variable location
                m_a.asm_add_r32_imm32(X86Reg::esp, 4); // increment stack top and thus pop the value to be set
            } else {
                throw AssemblerError("WASM_X86: Var type not supported");
            }

        } else {
            localidx -= no_of_params;
            std::string var_type = vt2s(codes[cur_func_idx].locals[localidx].type);
            if (var_type == "i32") {
                m_a.asm_pop_r32(X86Reg::eax);
                m_a.asm_mov_m32_r32(&base, nullptr, 1, -4 * (1 + (int)localidx), X86Reg::eax);
            } else if (var_type == "i64") {
                m_a.asm_pop_r32(X86Reg::eax);
                m_a.asm_mov_m32_r32(&base, nullptr, 1, -4 * (1 + (int)localidx), X86Reg::eax);
            } else if (var_type == "f64") {
                X86Reg stack_top = X86Reg::esp;
                m_a.asm_fld_m32(&stack_top, nullptr, 1, 0); // load stack top into floating register stack
                m_a.asm_fstp_m32(&base, nullptr, 1, -4 * (1 + (int)localidx)); // store float at variable location
                m_a.asm_add_r32_imm32(X86Reg::esp, 4); // increment stack top and thus pop the value to be set
            } else {
                throw AssemblerError("WASM_X86: Var type not supported");
            }
        }
    }

    void visit_I32Eqz() {
        m_a.asm_push_imm32(0U);
        handle_I32Compare<&X86Assembler::asm_je_label>();
    }

    void visit_I32Const(int32_t value) {
        m_a.asm_push_imm32(value);
    }

    void visit_I32WrapI64() {
        // empty, since i32's and i64's are considered similar currently.
    }

    template<typename F>
    void handleI32Opt(F && f) {
        m_a.asm_pop_r32(X86Reg::ebx);
        m_a.asm_pop_r32(X86Reg::eax);
        f();
        m_a.asm_push_r32(X86Reg::eax);
    }

    void visit_I32Add() {
        handleI32Opt([&](){ m_a.asm_add_r32_r32(X86Reg::eax, X86Reg::ebx);});
    }
    void visit_I32Sub() {
        handleI32Opt([&](){ m_a.asm_sub_r32_r32(X86Reg::eax, X86Reg::ebx);});
    }
    void visit_I32Mul() {
        handleI32Opt([&](){ m_a.asm_mul_r32(X86Reg::ebx);});
    }
    void visit_I32DivS() {
        handleI32Opt([&](){
            m_a.asm_mov_r32_imm32(X86Reg::edx, 0);
            m_a.asm_div_r32(X86Reg::ebx);
        });
    }

    using JumpFn = void(X86Assembler::*)(const std::string&);
    template<JumpFn T>
    void handle_I32Compare() {
        std::string label = std::to_string(offset);
        m_a.asm_pop_r32(X86Reg::ebx);
        m_a.asm_pop_r32(X86Reg::eax);
        // `eax` and `ebx` contain the left and right operands, respectively
        m_a.asm_cmp_r32_r32(X86Reg::eax, X86Reg::ebx);

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

    void visit_I64Const(int64_t value) {
        m_a.asm_push_imm32(value);
    }

    void visit_I64ExtendI32S() {
        // empty, since all i32's are already considered as i64's currently.
    }

    template<typename F>
    void handleI64Opt(F && f) {
        m_a.asm_pop_r32(X86Reg::ebx);
        m_a.asm_pop_r32(X86Reg::eax);
        f();
        m_a.asm_push_r32(X86Reg::eax);
    }

    void visit_I64Add() {
        handleI64Opt([&](){ m_a.asm_add_r32_r32(X86Reg::eax, X86Reg::ebx);});
    }
    void visit_I64Sub() {
        handleI64Opt([&](){ m_a.asm_sub_r32_r32(X86Reg::eax, X86Reg::ebx);});
    }
    void visit_I64Mul() {
        handleI64Opt([&](){ m_a.asm_mul_r32(X86Reg::ebx);});
    }
    void visit_I64DivS() {
        handleI64Opt([&](){
            m_a.asm_mov_r32_imm32(X86Reg::edx, 0);
            m_a.asm_div_r32(X86Reg::ebx);
        });
    }

    void visit_I64RemS() {
        m_a.asm_pop_r32(X86Reg::ebx);
        m_a.asm_pop_r32(X86Reg::eax);
        m_a.asm_mov_r32_imm32(X86Reg::edx, 0);
        m_a.asm_div_r32(X86Reg::ebx);
        m_a.asm_push_r32(X86Reg::edx);
    }

    template<JumpFn T>
    void handle_I64Compare() {
        std::string label = std::to_string(offset);
        m_a.asm_pop_r32(X86Reg::ebx);
        m_a.asm_pop_r32(X86Reg::eax);
        // `eax` and `ebx` contain the left and right operands, respectively
        m_a.asm_cmp_r32_r32(X86Reg::eax, X86Reg::ebx);

        (m_a.*T)(".compare_1" + label);
        // if the `compare` condition in `true`, jump to compare_1
        // and assign `1` else assign `0`
        m_a.asm_push_imm8(0);
        m_a.asm_jmp_label(".compare.end_" + label);
        m_a.add_label(".compare_1" + label);
        m_a.asm_push_imm8(1);
        m_a.add_label(".compare.end_" + label);
    }

    void visit_I64Eq() { handle_I64Compare<&X86Assembler::asm_je_label>(); }
    void visit_I64GtS() { handle_I64Compare<&X86Assembler::asm_jg_label>(); }
    void visit_I64GeS() { handle_I64Compare<&X86Assembler::asm_jge_label>(); }
    void visit_I64LtS() { handle_I64Compare<&X86Assembler::asm_jl_label>(); }
    void visit_I64LeS() { handle_I64Compare<&X86Assembler::asm_jle_label>(); }
    void visit_I64Ne() { handle_I64Compare<&X86Assembler::asm_jne_label>(); }

     void visit_I64Eqz() {
        m_a.asm_push_imm32(0U);
        handle_I64Compare<&X86Assembler::asm_je_label>();
    }

    std::string float_to_str(float z) {
        std::string float_str = "";
        for (auto ch:std::to_string(z)) {
            if (ch == '-') {
                float_str += "neg_";
            } else if (ch == '.') {
                float_str += "_dot_";
            } else {
                float_str += ch;
            }
        }
        return float_str;
    }

    void visit_F64Const(double z) {
        std::string label = "float_" + float_to_str(z);
        float_consts[label] = z;
        m_a.asm_mov_r32_label(X86Reg::eax, label);
        X86Reg label_reg = X86Reg::eax;
        m_a.asm_fld_m32(&label_reg, nullptr, 1, 0); // loads into floating register stack
        m_a.asm_sub_r32_imm32(X86Reg::esp,  4); // decrement stack and create space
        X86Reg stack_top = X86Reg::esp;
        m_a.asm_fstp_m32(&stack_top, nullptr, 1, 0); // store float on integer stack top;
    }

    void gen_x86_bytes() {
        emit_elf32_header(m_a);

        // Add runtime library functions
        emit_exit2(m_a, "my_exit");

        // declare compile-time strings
         std::string base_memory = "    "; /* in wasm backend, memory starts after 4 bytes*/
        for (uint32_t i = 0; i < data_segments.size(); i++) {
            base_memory += data_segments[i].text;
        }
        label_to_str["base_memory"] = base_memory;

        NO_OF_IMPORTS = imports.size();
        for (uint32_t i = 0; i < type_indices.size(); i++) {
            std::string func = exports[i + 1 /* offset by 1 becaz of mem export */].name;
            if (func == "print_f64") {
                // "print_f64" needs floating-point comparison support, which is
                // not yet supported in the wasm_x86 backend, hence skipping it.
                continue;
            }
            m_a.add_label(func);

            {
                // Initialize the stack
                m_a.asm_push_r32(X86Reg::ebp);
                m_a.asm_mov_r32_r32(X86Reg::ebp, X86Reg::esp);

                 // Allocate space for local variables
                // TODO: locals is an array where every element has a count (currently wasm emits count = 1 always)
                m_a.asm_sub_r32_imm32(X86Reg::esp, 4 * codes[i].locals.size());

                offset = codes.p[i].insts_start_index;
                cur_func_idx = i;
                decode_instructions();
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
