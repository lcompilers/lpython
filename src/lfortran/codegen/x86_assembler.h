#ifndef LFORTRAN_CODEGEN_X86_ASSEMBER_H
#define LFORTRAN_CODEGEN_X86_ASSEMBER_H

/*

X86 Assembler implementation in the X86Assembler class.

The goal of the X86Assembler class is to emit machine code as quickly as
possible. For that reason the assembler is implemented as a two pass assembler:
in the first pass it emits all the instructions as fixed size byte code, and in
the second pass it fixes all references to labels (jumps). As a result, the
final machine code is not the shortest possible, because jumps could possibly be
encoded shorter if the final relative address is shorter, but it would require
more passes and thus slower compilation.

For debugging purposes, one can enable the macro LFORTRAN_ASM_PRINT and one can
then obtain a human readable assembly printout of all instructions. Disable the
macro for best performance.

References:

[1] Intel 64 and IA-32 Architectures Software Developer's Manual
https://www.systutorials.com/go/intel-x86-64-reference-manual/

*/

#include <iomanip>
#include <sstream>

#include <lfortran/parser/alloc.h>
#include <lfortran/containers.h>

// Define to allow the Assembler print the asm instructions
#define LFORTRAN_ASM_PRINT

#ifdef LFORTRAN_ASM_PRINT
#    define EMIT(s) emit("    ", s)
#else
#    define EMIT(s)
#endif

namespace LFortran {

enum X86Reg : uint8_t {
    eax = 0,
    ecx = 1,
    edx = 2,
    ebx = 3,
    esp = 4,
    ebp = 5,
    esi = 6,
    edi = 7,
};

std::string r2s(X86Reg r32) {
    switch (r32) {
        case (X86Reg::eax) : return "eax";
        case (X86Reg::ecx) : return "ecx";
        case (X86Reg::edx) : return "edx";
        case (X86Reg::ebx) : return "ebx";
        case (X86Reg::esp) : return "esp";
        case (X86Reg::ebp) : return "ebp";
        case (X86Reg::esi) : return "esi";
        case (X86Reg::edi) : return "edi";
        default : throw AssemblerError("Unknown instruction");
    }
}

std::string m2s(X86Reg *base, X86Reg *index, uint8_t scale, int32_t disp) {
    std::string r;
    r = "[";
    if (base) r += r2s(*base);
    if (index) {
        if (base) r += "+";
        if (scale == 1) {
            r += r2s(*index);
        } else {
            r += std::to_string(scale) + "*" + r2s(*index);
        }
    }
    if (disp) {
        if ((base || index) && (disp > 0)) r += "+";
        r += std::to_string(disp);
    }
    r += "]";
    return r;
}

template< typename T >
std::string hexify(T i)
{
    std::stringbuf buf;
    std::ostream os(&buf);
    os << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
    return buf.str();
}

std::string i2s(uint8_t imm8) {
    // hexify() for some reason does not work with uint8_t, only with longer
    // integers
    std::string s = hexify((uint16_t)imm8);
    // Strip the two leading zeros
    return "0x" + s.substr(2,4);
}

void push_back_uint32(Vec<uint8_t> &code, Allocator &al, uint32_t i32) {
    code.push_back(al, (i32      ) & 0xFF);
    code.push_back(al, (i32 >>  8) & 0xFF);
    code.push_back(al, (i32 >> 16) & 0xFF);
    code.push_back(al, (i32 >> 24) & 0xFF);
}

// Implements table 2-2 in [1].
uint8_t ModRM_byte(uint8_t mode, uint8_t reg, uint8_t rm) {
    LFORTRAN_ASSERT(mode <= 3);
    LFORTRAN_ASSERT(reg <= 7);
    LFORTRAN_ASSERT(rm <= 7);
    return (mode << 6) | (reg << 3) | rm;
}

// Implements table 2-3 in [1].
uint8_t SIB_byte(uint8_t base, uint8_t index, uint8_t scale_index) {
    LFORTRAN_ASSERT(base <= 7);
    LFORTRAN_ASSERT(index <= 7);
    LFORTRAN_ASSERT(scale_index <= 3);
    return (scale_index << 6) | (index << 3) | base;
}

// Implements the logic of tables 2-2 and 2-3 in [1] and correctly appends the
// SIB and displacement bytes as appropriate.
void ModRM_SIB_disp_bytes(Vec<uint8_t> &code, Allocator &al,
        uint8_t mod, uint8_t reg, uint8_t rm,
        uint8_t base, uint8_t index, uint8_t scale_index, int32_t disp) {
    code.push_back(al, ModRM_byte(mod, reg, rm));
    if (rm == 0b100 && (mod == 0b00 || mod == 0b01 || mod == 0b10)) {
        // SIB byte is present
        code.push_back(al, SIB_byte(base, index, scale_index));
    }
    if (mod == 0b01) {
        // disp8 is present
        LFORTRAN_ASSERT(-128 <= disp && disp < 128);
        uint8_t disp8 = disp;
        code.push_back(al, disp8);
    } else if ((mod == 0b00 && (rm==0b101 || base==0b101)) || (mod == 0b10)) {
        // disp32 is present
        uint32_t disp32 = disp;
        push_back_uint32(code, al, disp32);
    }
}

void modrm_sib_disp(Vec<uint8_t> &code, Allocator &al,
        X86Reg reg,
        X86Reg *base_opt, // nullptr if None
        X86Reg *index_opt, // nullptr if None
        uint8_t scale, // 1 if None
        int32_t disp, // 0 if None
        bool mem) {
    uint8_t mod, rm, base, index, scale_index;

    if (mem) {
        // Determine mod
        if (!base_opt || (disp == 0 && *base_opt != 0b101)) {
            mod = 0b00;
        } else if (-128 <= disp && disp < 128) {
            mod = 0b01;
        } else {
            mod = 0b10;
        }

        // Determine rm
        if (index_opt) {
            rm = 0b100;
        } else if (!base_opt) {
            rm = 0b101;
        } else {
            rm = *base_opt;
        }

        // Determine base
        if (base_opt) {
            base = *base_opt;
        } else if (index_opt) {
            base = 0b101;
        } else {
            throw AssemblerError("base_opt or index_opt must be supplied if mem=true");
        }

        // Determine index
        if (index_opt) {
            index = *index_opt;
        } else if (base == 0b100) {
            index = 0b100;
        } else {
            // index will not be used
        }
    } else {
        mod = 0b11;
        if (base_opt) {
            base = *base_opt;
        } else {
            throw AssemblerError("base_opt must be supplied if mem=false");
        }
        rm = base;
    }

    switch (scale) {
        case (1) : scale_index = 0b00; break;
        case (2) : scale_index = 0b01; break;
        case (4) : scale_index = 0b10; break;
        case (8) : scale_index = 0b11; break;
        default : throw AssemblerError("Scale must be one of [1, 2, 4, 8]");
    }

    ModRM_SIB_disp_bytes(code, al, mod, reg, rm,
            base, index, scale_index, disp);
}

class X86Assembler {
    Allocator &m_al;
    Vec<uint8_t> m_code;
#ifdef LFORTRAN_ASM_PRINT
    std::string m_asm_code;
    void emit(const std::string &indent, const std::string &s) {
        m_asm_code += indent + s + "\n";
    }
#endif
public:
    X86Assembler(Allocator &al) : m_al{al} {
        m_code.reserve(m_al, 1024*128);
    }

#ifdef LFORTRAN_ASM_PRINT
    std::string get_asm() {
        return m_asm_code;
    }
#endif

    Vec<uint8_t>& get_machine_code() {
        return m_code;
    }

    void asm_pop_r32(X86Reg r32) {
        if (r32 == X86Reg::eax) {
            m_code.push_back(m_al, 0x58);
        } else {
            throw AssemblerError("Register not supported yet");
        }
        EMIT("pop " + r2s(r32));
    }

    void asm_push_r32(X86Reg r32) {
        if (r32 == X86Reg::eax) {
            m_code.push_back(m_al, 0x50);
        } else {
            throw AssemblerError("Register not supported yet");
        }
        EMIT("push " + r2s(r32));
    }

    void asm_push_imm8(uint8_t imm8) {
        m_code.push_back(m_al, 0x6a);
        m_code.push_back(m_al, imm8);
        EMIT("push " + i2s(imm8));
    }

    void asm_jz_imm8(uint8_t imm8) {
        m_code.push_back(m_al, 0x74);
        m_code.push_back(m_al, imm8);
        EMIT("jz " + i2s(imm8));
    }

    void asm_jnz_imm8(uint8_t imm8) {
        m_code.push_back(m_al, 0x75);
        m_code.push_back(m_al, imm8);
        EMIT("jnz " + i2s(imm8));
    }

    void asm_jle_imm8(uint8_t imm8) {
        m_code.push_back(m_al, 0x7e);
        m_code.push_back(m_al, imm8);
        EMIT("jle " + i2s(imm8));
    }

    void asm_jl_imm8(uint8_t imm8) {
        m_code.push_back(m_al, 0x7c);
        m_code.push_back(m_al, imm8);
        EMIT("jl " + i2s(imm8));
    }

    void asm_jne_imm8(uint8_t imm8) {
        asm_jnz_imm8(imm8);
    }

    void asm_jge_imm8(uint8_t imm8) {
        m_code.push_back(m_al, 0x7d);
        m_code.push_back(m_al, imm8);
        EMIT("jge " + i2s(imm8));
    }

    void asm_inc_r32(X86Reg r32) {
        if (r32 == X86Reg::eax) {
            m_code.push_back(m_al, 0x40);
        } else if (r32 == X86Reg::ebx) {
            m_code.push_back(m_al, 0x43);
        } else if (r32 == X86Reg::edx) {
            m_code.push_back(m_al, 0x42);
        } else {
            throw AssemblerError("Register not supported yet");
        }
        EMIT("inc " + r2s(r32));
    }

    void asm_inc_m32(X86Reg *base, X86Reg *index, uint8_t scale, int32_t disp) {
        m_code.push_back(m_al, 0xff);
        modrm_sib_disp(m_code, m_al,
                X86Reg::eax, base, index, scale, disp, true);
        EMIT("inc " + m2s(base, index, scale, disp));
    }

};



} // namespace LFortran

#endif // LFORTRAN_CODEGEN_X86_ASSEMBER_H
