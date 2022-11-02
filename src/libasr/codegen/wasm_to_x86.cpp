#include <fstream>
#include <chrono>
#include <iomanip>

#include <libasr/assert.h>
#include <libasr/codegen/wasm_decoder.h>
#include <libasr/codegen/wasm_to_x86.h>
#include <libasr/codegen/x86_assembler.h>

namespace LFortran {

namespace wasm {

class X86Generator : public WASMDecoder<X86Generator> {
   public:
    X86Generator(Allocator &al, diag::Diagnostics &diagonostics)
        : WASMDecoder(al, diagonostics) {}

    void gen_x86_bytes(X86Assembler &m_a) {
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

                // Initialze space for local variables to zero
                m_a.asm_mov_r32_imm32(X86Reg::eax, 0U);
                for (uint32_t j = 0; j < codes.p[i].locals.size(); j++) {
                    for (uint32_t k = 0; k < codes.p[i].locals.p[j].count;
                         k++) {
                        m_a.asm_push_r32(X86Reg::eax);
                    }
                }

                WASM_INSTS_VISITOR::X86Visitor v =
                    WASM_INSTS_VISITOR::X86Visitor(
                        wasm_bytes, codes.p[i].insts_start_index, m_a,
                        func_types, imports, type_indices, exports, codes,
                        data_segments, i);

                v.decode_instructions();

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

    wasm::X86Generator x86_generator(al, diagnostics);
    x86_generator.wasm_bytes.from_pointer_n(wasm_bytes.data(),
                                            wasm_bytes.size());

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        try {
            x86_generator.decode_wasm();
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
        x86_generator.gen_x86_bytes(m_a);
        auto t2 = std::chrono::high_resolution_clock::now();
        time_gen_x86_bytes =
            std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
                .count();
    }

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        m_a.verify();
        auto t2 = std::chrono::high_resolution_clock::now();
        time_verify = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    }

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        m_a.save_binary(filename);
        auto t2 = std::chrono::high_resolution_clock::now();
        time_save =
            std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
                .count();
    }

    if (time_report) {
        std::cout << "Codegen Time report:" << std::endl;
        std::cout << "Decode wasm: " << std::setw(5) << time_decode_wasm
                  << std::endl;
        std::cout << "Generate asm: " << std::setw(5) << time_gen_x86_bytes
                  << std::endl;
        std::cout << "Verify:       " << std::setw(5) << time_verify << std::endl;
        std::cout << "Save:       " << std::setw(5) << time_save << std::endl;
        int total = time_decode_wasm + time_gen_x86_bytes + time_verify + time_save;
        std::cout << "Total:      " << std::setw(5) << total << std::endl;
    }
    return 0;
}

}  // namespace LFortran
