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
    uint32_t block_id;
    uint32_t NO_OF_IMPORTS;
    std::vector<std::pair<uint32_t, Block>> blocks;
    std::map<std::string, std::string> label_to_str;
    std::map<std::string, double> double_consts;

    X64Visitor(X86Assembler &m_a, Allocator &al,
               diag::Diagnostics &diagonostics, Vec<uint8_t> &code)
        : WASMDecoder(al, diagonostics),
          BaseWASMVisitor(code, 0U /* temporary offset */),
          m_a(m_a) {
        wasm_bytes.from_pointer_n(code.data(), code.size());
        block_id = 1;
        NO_OF_IMPORTS = 0;
    }

    void visit_Return() {
        // Restore stack
        m_a.asm_mov_r64_r64(X64Reg::rsp, X64Reg::rbp);
        m_a.asm_pop_r64(X64Reg::rbp);
        m_a.asm_ret();
    }

    void visit_Unreachable() {}

    void visit_EmtpyBlockType() {}

    void visit_Drop() { m_a.asm_pop_r64(X64Reg::rax); }

    void call_imported_function(uint32_t func_idx) {

        switch (func_idx) {
            case 0: {  // proc_exit
            /*
                TODO: This way increases the number of intructions.
                There is a possibility that we can wrap these statements
                with some add label and then just jump/call to that label
            */
                m_a.asm_pop_r64(X64Reg::rdi); // get exit code from stack top
                m_a.asm_mov_r64_imm64(X64Reg::rax, 60); // sys_exit
                m_a.asm_syscall(); // syscall
                break;
            }
            case 1: {  // fd_write
            /*
                TODO: This way increases the number of intructions.
                There is a possibility that we can wrap these statements
                with some add label and then just jump/call to that label
            */

                m_a.asm_pop_r64(X64Reg::r11); // mem_loc to write return value (not usefull for us currently)
                m_a.asm_pop_r64(X64Reg::r12); // no of iov vectors (always emitted 1 by wasm, not usefull for us currently)
                m_a.asm_pop_r64(X64Reg::r13); // mem_loc to string iov vector
                m_a.asm_pop_r64(X64Reg::r14); // filetypes (1 for stdout, not usefull for us currently)

                m_a.asm_mov_r64_label(X64Reg::rbx, "base_memory");
                m_a.asm_add_r64_r64(X64Reg::rbx, X64Reg::r13);

                m_a.asm_mov_r64_imm64(X64Reg::rax, 0);
                m_a.asm_mov_r64_imm64(X64Reg::rdx, 0);
                // TODO: Currently this uses a combination of i32 and i64 registers. Fix it in upcoming PRs.
                X86Reg base = X86Reg::ebx;
                m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, 0); // location
                m_a.asm_mov_r32_m32(X86Reg::edx, &base, nullptr, 1, 4); // length

                {
                    // write system call
                    m_a.asm_mov_r64_label(X64Reg::rsi, "base_memory");
                    m_a.asm_add_r64_r64(X64Reg::rsi, X64Reg::rax); // base_memory + location
                    m_a.asm_mov_r64_imm64(X64Reg::rax, 1); // system call no (1 for write)
                    m_a.asm_mov_r64_imm64(X64Reg::rdi, 1); // stdout_file no
                    // rsi stores location, length is already stored in rdx
                    m_a.asm_syscall();

                    m_a.asm_push_r64(X64Reg::rax); // push return value onto stack
                }
                break;
            }
            default: {
                std::cerr << "Unsupported func_idx\n";
            }
        }
    }

    void visit_Call(uint32_t func_idx) {
        if (func_idx < NO_OF_IMPORTS) {
            call_imported_function(func_idx);
            return;
        }

        func_idx -= NO_OF_IMPORTS; // adjust function index as per imports
        m_a.asm_call_label(exports[func_idx + 1 /* offset by 1 becaz of mem export */].name);

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
                In case of block or if, it is a forward jump, resuming execution after the matching end.
                In case of loop, it is a backward jump to the beginning of the loop.
            */
            case Block::LOOP: m_a.asm_jmp_label(".loop.head_" + label); break;
            case Block::IF: m_a.asm_jmp_label(".endif_" + label); break;
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
            } else if (var_type == "i64") {
                m_a.asm_mov_r64_m64(X64Reg::rax, &base, nullptr, 1, 8 * (2 + no_of_params - (int)localidx - 1));
                m_a.asm_push_r64(X64Reg::rax);
            } else if (var_type == "f64") {
                m_a.asm_sub_r64_imm32(X64Reg::rsp,  8); // create space for value to be fetched
                m_a.asm_movsd_r64_m64(X64FReg::xmm0, &base, nullptr, 1, 8 * (2 + no_of_params - (int)localidx - 1));
                X64Reg stack_top = X64Reg::rsp;
                m_a.asm_movsd_m64_r64(&stack_top, nullptr, 1, 0, X64FReg::xmm0);
            } else {
                throw AssemblerError("WASM_X64: Var type not supported");
            }
        } else {
            localidx -= no_of_params;
            std::string var_type = var_type_to_string[codes[cur_func_idx].locals[localidx].type];
            if (var_type == "i32") {
                m_a.asm_mov_r64_m64(X64Reg::rax, &base, nullptr, 1, -8 * (1 + (int)localidx));
                m_a.asm_push_r64(X64Reg::rax);
            } else if (var_type == "i64") {
                m_a.asm_mov_r64_m64(X64Reg::rax, &base, nullptr, 1, -8 * (1 + (int)localidx));
                m_a.asm_push_r64(X64Reg::rax);
            } else if (var_type == "f64") {
                m_a.asm_sub_r64_imm32(X64Reg::rsp,  8); // create space for value to be fetched
                m_a.asm_movsd_r64_m64(X64FReg::xmm0, &base, nullptr, 1, -8 * (1 + (int)localidx));
                X64Reg stack_top = X64Reg::rsp;
                m_a.asm_movsd_m64_r64(&stack_top, nullptr, 1, 0, X64FReg::xmm0);
            } else {
                throw AssemblerError("WASM_X64: Var type not supported");
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
            } else if (var_type == "i64") {
                m_a.asm_pop_r64(X64Reg::rax);
                m_a.asm_mov_m64_r64(&base, nullptr, 1, 8 * (2 + no_of_params - (int)localidx - 1), X64Reg::rax);
            } else if (var_type == "f64") {
                X64Reg stack_top = X64Reg::rsp;
                m_a.asm_movsd_r64_m64(X64FReg::xmm0, &stack_top, nullptr, 1, 0);
                m_a.asm_movsd_m64_r64(&base, nullptr, 1, 8 * (2 + no_of_params - (int)localidx - 1), X64FReg::xmm0);
                m_a.asm_add_r64_imm32(X64Reg::rsp, 8); // remove from stack top
            } else {
                throw AssemblerError("WASM_X64: Var type not supported");
            }
        } else {
            localidx -= no_of_params;
            std::string var_type = var_type_to_string[codes[cur_func_idx].locals[localidx].type];
            if (var_type == "i32") {
                m_a.asm_pop_r64(X64Reg::rax);
                m_a.asm_mov_m64_r64(&base, nullptr, 1, -8 * (1 + (int)localidx), X64Reg::rax);
            } else if (var_type == "i64") {
                m_a.asm_pop_r64(X64Reg::rax);
                m_a.asm_mov_m64_r64(&base, nullptr, 1, -8 * (1 + (int)localidx), X64Reg::rax);
            } else if (var_type == "f64") {
                X64Reg stack_top = X64Reg::rsp;
                m_a.asm_movsd_r64_m64(X64FReg::xmm0, &stack_top, nullptr, 1, 0);
                m_a.asm_movsd_m64_r64(&base, nullptr, 1, -8 * (1 + (int)localidx), X64FReg::xmm0);
                m_a.asm_add_r64_imm32(X64Reg::rsp, 8); // remove from stack top
            } else {
                throw AssemblerError("WASM_X64: Var type not supported");
            }
        }
    }

    void visit_I32Const(int32_t value) {
        m_a.asm_mov_r64_imm64(X64Reg::rax, labs((int64_t)value));
        if (value < 0) m_a.asm_neg_r64(X64Reg::rax);
        m_a.asm_push_r64(X64Reg::rax);
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
        handleI32Opt([&](){
            m_a.asm_mov_r64_imm64(X64Reg::rdx, 0);
            m_a.asm_div_r64(X64Reg::rbx);
        });
    }

    void visit_I32And() {
        handleI32Opt([&](){ m_a.asm_and_r64_r64(X64Reg::rax, X64Reg::rbx);});
    }

    void visit_I32Or() {
        handleI32Opt([&](){ m_a.asm_or_r64_r64(X64Reg::rax, X64Reg::rbx);});
    }

    void visit_I32Xor() {
        handleI32Opt([&](){ m_a.asm_xor_r64_r64(X64Reg::rax, X64Reg::rbx);});
    }

    void visit_I32Shl() {
        m_a.asm_pop_r64(X64Reg::rcx);
        m_a.asm_pop_r64(X64Reg::rax);
        m_a.asm_shl_r64_cl(X64Reg::rax);
        m_a.asm_push_r64(X64Reg::rax);
    }
    void visit_I32ShrS() {
        m_a.asm_pop_r64(X64Reg::rcx);
        m_a.asm_pop_r64(X64Reg::rax);
        m_a.asm_sar_r64_cl(X64Reg::rax);
        m_a.asm_push_r64(X64Reg::rax);
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

    void visit_I64Const(int32_t value) {
        m_a.asm_mov_r64_imm64(X64Reg::rax, labs((int64_t)value));
        if (value < 0) m_a.asm_neg_r64(X64Reg::rax);
        m_a.asm_push_r64(X64Reg::rax);
    }

    template<typename F>
    void handleI64Opt(F && f) {
        m_a.asm_pop_r64(X64Reg::rbx);
        m_a.asm_pop_r64(X64Reg::rax);
        f();
        m_a.asm_push_r64(X64Reg::rax);
    }

    void visit_I64Add() {
        handleI64Opt([&](){ m_a.asm_add_r64_r64(X64Reg::rax, X64Reg::rbx);});
    }
    void visit_I64Sub() {
        handleI64Opt([&](){ m_a.asm_sub_r64_r64(X64Reg::rax, X64Reg::rbx);});
    }
    void visit_I64Mul() {
        handleI64Opt([&](){ m_a.asm_mul_r64(X64Reg::rbx);});
    }
    void visit_I64DivS() {
        handleI64Opt([&](){
            m_a.asm_mov_r64_imm64(X64Reg::rdx, 0);
            m_a.asm_div_r64(X64Reg::rbx);});
    }

    void visit_I64And() {
        handleI64Opt([&](){ m_a.asm_and_r64_r64(X64Reg::rax, X64Reg::rbx);});
    }

    void visit_I64Or() {
        handleI64Opt([&](){ m_a.asm_or_r64_r64(X64Reg::rax, X64Reg::rbx);});
    }

    void visit_I64Xor() {
        handleI64Opt([&](){ m_a.asm_xor_r64_r64(X64Reg::rax, X64Reg::rbx);});
    }

    void visit_I64RemS() {
        m_a.asm_pop_r64(X64Reg::rbx);
        m_a.asm_pop_r64(X64Reg::rax);
        m_a.asm_mov_r64_imm64(X64Reg::rdx, 0);
        m_a.asm_div_r64(X64Reg::rbx);
        m_a.asm_push_r64(X64Reg::rdx);
    }

    void visit_I32WrapI64() {
        // empty, since i32's and i64's are considered similar currently.
    }

    void visit_I64Store(uint32_t /*mem_align*/, uint32_t /*mem_offset*/) {
        m_a.asm_pop_r64(X64Reg::rbx);
        m_a.asm_pop_r64(X64Reg::rax);
        // Store value rbx at location rax
        X64Reg base = X64Reg::rax;
        m_a.asm_mov_m64_r64(&base, nullptr, 1, 0, X64Reg::rbx);
    }

    void visit_I64Shl() {
        m_a.asm_pop_r64(X64Reg::rcx);
        m_a.asm_pop_r64(X64Reg::rax);
        m_a.asm_shl_r64_cl(X64Reg::rax);
        m_a.asm_push_r64(X64Reg::rax);
    }
    void visit_I64ShrS() {
        m_a.asm_pop_r64(X64Reg::rcx);
        m_a.asm_pop_r64(X64Reg::rax);
        m_a.asm_sar_r64_cl(X64Reg::rax);
        m_a.asm_push_r64(X64Reg::rax);
     }

    void visit_I64Eqz() {
        m_a.asm_mov_r64_imm64(X64Reg::rax, 0);
        m_a.asm_push_r64(X64Reg::rax);
        handle_I64Compare<&X86Assembler::asm_je_label>();
    }

    template<JumpFn T>
    void handle_I64Compare() {
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

    void visit_I64Eq() { handle_I64Compare<&X86Assembler::asm_je_label>(); }
    void visit_I64GtS() { handle_I64Compare<&X86Assembler::asm_jg_label>(); }
    void visit_I64GeS() { handle_I64Compare<&X86Assembler::asm_jge_label>(); }
    void visit_I64LtS() { handle_I64Compare<&X86Assembler::asm_jl_label>(); }
    void visit_I64LeS() { handle_I64Compare<&X86Assembler::asm_jle_label>(); }
    void visit_I64Ne() { handle_I64Compare<&X86Assembler::asm_jne_label>(); }

    void visit_I64TruncF64S() {
        X64Reg stack_top = X64Reg::rsp;
        m_a.asm_movsd_r64_m64(X64FReg::xmm0, &stack_top, nullptr, 1, 0); // load into floating-point register
        m_a.asm_add_r64_imm32(X64Reg::rsp, 8); // increment stack and deallocate space
        m_a.asm_cvttsd2si_r64_r64(X64Reg::rax, X64FReg::xmm0); // rax now contains value int(xmm0)
        m_a.asm_push_r64(X64Reg::rax);
    }

    void visit_I64ExtendI32S() {
        // empty, since all i32's are already considered as i64's currently.
    }

    std::string float_to_str(double z) {
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
        m_a.asm_add_r64_imm32(X64Reg::rsp, 8); // pop the argument

        (m_a.*T)(X64FReg::xmm0, X64FReg::xmm1);

        m_a.asm_sub_r64_imm32(X64Reg::rsp, 8); // decrement stack and create space
        // store float result back on stack top;
        m_a.asm_movsd_m64_r64(&stack_top, nullptr, 1, 0, X64FReg::xmm0);
    }

    void visit_F64Add() { handleF64Operations<&X86Assembler::asm_addsd_r64_r64>(); }
    void visit_F64Sub() { handleF64Operations<&X86Assembler::asm_subsd_r64_r64>(); }
    void visit_F64Mul() { handleF64Operations<&X86Assembler::asm_mulsd_r64_r64>(); }
    void visit_F64Div() { handleF64Operations<&X86Assembler::asm_divsd_r64_r64>(); }

    void handleF64Compare(Fcmp cmp) {
        X64Reg stack_top = X64Reg::rsp;
        // load second operand into floating-point register
        m_a.asm_movsd_r64_m64(X64FReg::xmm1, &stack_top, nullptr, 1, 0);
        m_a.asm_add_r64_imm32(X64Reg::rsp, 8); // pop the argument
        // load first operand into floating-point register
        m_a.asm_movsd_r64_m64(X64FReg::xmm0, &stack_top, nullptr, 1, 0);
        m_a.asm_add_r64_imm32(X64Reg::rsp, 8); // pop the argument

        m_a.asm_cmpsd_r64_r64(X64FReg::xmm0, X64FReg::xmm1, cmp);
        /* From Assembly Docs:
            The result of the compare is a 64-bit value of all 1s (TRUE) or all 0s (FALSE).
        */
        m_a.asm_pmovmskb_r32_r64(X86Reg::eax, X64FReg::xmm0);
        m_a.asm_and_r64_imm8(X64Reg::rax, 1);
        m_a.asm_push_r64(X64Reg::rax);
    }

    void visit_F64Eq() { handleF64Compare(Fcmp::eq); }
    void visit_F64Gt() { handleF64Compare(Fcmp::gt); }
    void visit_F64Ge() { handleF64Compare(Fcmp::ge); }
    void visit_F64Lt() { handleF64Compare(Fcmp::lt); }
    void visit_F64Le() { handleF64Compare(Fcmp::le); }
    void visit_F64Ne() { handleF64Compare(Fcmp::ne); }

    void visit_F64ConvertI64S() {
        m_a.asm_pop_r64(X64Reg::rax);
        m_a.asm_cvtsi2sd_r64_r64(X64FReg::xmm0, X64Reg::rax);
        m_a.asm_sub_r64_imm32(X64Reg::rsp, 8); // decrement stack and create space
        X64Reg stack_top = X64Reg::rsp;
        m_a.asm_movsd_m64_r64(&stack_top, nullptr, 1, 0, X64FReg::xmm0); // store float on integer stack top;
    }

    void gen_x64_bytes() {
        emit_elf64_header(m_a);

        // declare compile-time strings
        std::string base_memory = "    "; /* in wasm backend, memory starts after 4 bytes*/
        for (uint32_t i = 0; i < data_segments.size(); i++) {
            base_memory += data_segments[i].text;
        }
        label_to_str["base_memory"] = base_memory;

        NO_OF_IMPORTS = imports.size();
        for (uint32_t idx = 0; idx < type_indices.size(); idx++) {
            m_a.add_label(exports[idx + 1].name);
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
