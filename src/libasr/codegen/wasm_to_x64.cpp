#include <fstream>
#include <chrono>
#include <iomanip>

#include <libasr/assert.h>
#include <libasr/codegen/wasm_decoder.h>
#include <libasr/codegen/wasm_to_x64.h>
#include <libasr/codegen/x86_assembler.h>

namespace LFortran {

namespace wasm {

/*

This X64Visitor uses stack to pass arguments and return values from functions.
Since in X64, instructions operate on registers (and not on stack),
for every instruction we pop elements from top of stack and store them into
registers. After operating on the registers, the result value is then
pushed back onto the stack.

One of the reasons to use stack to pass function arguments is that,
it allows us to define and call functions with any number of parameters.
As registers are limited in number, if we use them to pass function arugments,
the number of arguments we could pass to a function would get limited by
the number of registers available with the CPU.

*/

class X64Visitor : public WASMDecoder<X64Visitor>,
                   public WASM_INSTS_VISITOR::BaseWASMVisitor<X64Visitor> {
   public:
    X86Assembler &m_a;

    X64Visitor(X86Assembler &m_a, Allocator &al,
               diag::Diagnostics &diagonostics, Vec<uint8_t> &code)
        : WASMDecoder(al, diagonostics),
          BaseWASMVisitor(code, 0U /* temporary offset */),
          m_a(m_a) {
        wasm_bytes.from_pointer_n(code.data(), code.size());
    }

    void gen_x64_bytes() {
        // update the initial value of asm text as per X64 text format
        m_a.update_asm("BITS 64\n\n");

        emit_elf64_header(m_a);

        {
            m_a.add_label("_start");

            // exit with a fixed non-zero exit code
            m_a.asm_mov_r64_imm64(LFortran::X64Reg::rax, 60); // sys_exit
            m_a.asm_mov_r64_imm64(LFortran::X64Reg::rdi, 24 /* exit_code */); // exit code
            m_a.asm_syscall(); // syscall
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

    X86Assembler m_a(al);

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

}  // namespace LFortran
