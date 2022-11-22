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
#include <fstream>
#include <map>

#include <libasr/alloc.h>
#include <libasr/containers.h>

// Define to allow the Assembler print the asm instructions
#define LFORTRAN_ASM_PRINT

#ifdef LFORTRAN_ASM_PRINT
#    define EMIT(s) emit("    ", s)
#    define EMIT_LABEL(s) emit("", s)
#    define EMIT_VAR(a, b) emit("\n", a + " equ " + i2s(b) + "\n")
#else
#    define EMIT(s)
#    define EMIT_LABEL(s)
#    define EMIT_VAR(a, b)
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

static std::string r2s(X86Reg r32) {
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

static std::string m2s(X86Reg *base, X86Reg *index, uint8_t scale, int32_t disp) {
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
static std::string hexify(T i)
{
    std::stringbuf buf;
    std::ostream os(&buf);
    os << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
    return buf.str();
}

static std::string i2s(uint32_t imm32) {
    return "0x" + hexify(imm32);
}

static std::string i2s(uint16_t imm16) {
    return "0x" + hexify(imm16);
}

static std::string i2s(uint8_t imm8) {
    // hexify() for some reason does not work with uint8_t, only with longer
    // integers
    std::string s = hexify((uint16_t)imm8);
    // Strip the two leading zeros
    return "0x" + s.substr(2,4);
}

static void push_back_uint32(Vec<uint8_t> &code, Allocator &al, uint32_t i32) {
    code.push_back(al, (i32      ) & 0xFF);
    code.push_back(al, (i32 >>  8) & 0xFF);
    code.push_back(al, (i32 >> 16) & 0xFF);
    code.push_back(al, (i32 >> 24) & 0xFF);
}

static void insert_uint32(Vec<uint8_t> &code, size_t pos, uint32_t i32) {
    code.p[pos  ] = (i32      ) & 0xFF;
    code.p[pos+1] = (i32 >>  8) & 0xFF;
    code.p[pos+2] = (i32 >> 16) & 0xFF;
    code.p[pos+3] = (i32 >> 24) & 0xFF;
}

static void push_back_uint16(Vec<uint8_t> &code, Allocator &al, uint16_t i16) {
    code.push_back(al, (i16      ) & 0xFF);
    code.push_back(al, (i16 >>  8) & 0xFF);
}

static void insert_uint16(Vec<uint8_t> &code, size_t pos, uint16_t i16) {
    code.p[pos  ] = (i16      ) & 0xFF;
    code.p[pos+1] = (i16 >>  8) & 0xFF;
}

// Implements table 2-2 in [1].
static uint8_t ModRM_byte(uint8_t mode, uint8_t reg, uint8_t rm) {
    LFORTRAN_ASSERT(mode <= 3);
    LFORTRAN_ASSERT(reg <= 7);
    LFORTRAN_ASSERT(rm <= 7);
    return (mode << 6) | (reg << 3) | rm;
}

// Implements table 2-3 in [1].
static uint8_t SIB_byte(uint8_t base, uint8_t index, uint8_t scale_index) {
    LFORTRAN_ASSERT(base <= 7);
    LFORTRAN_ASSERT(index <= 7);
    LFORTRAN_ASSERT(scale_index <= 3);
    return (scale_index << 6) | (index << 3) | base;
}

// Implements the logic of tables 2-2 and 2-3 in [1] and correctly appends the
// SIB and displacement bytes as appropriate.
static void ModRM_SIB_disp_bytes(Vec<uint8_t> &code, Allocator &al,
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

static void modrm_sib_disp(Vec<uint8_t> &code, Allocator &al,
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
            // index will not be used, but silence a compiler warning:
            index = 0;
        }
    } else {
        mod = 0b11;
        if (base_opt) {
            base = *base_opt;
        } else {
            throw AssemblerError("base_opt must be supplied if mem=false");
        }
        rm = base;
        // index will not be used, but silence a compiler warning:
        index = 0;
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

struct Symbol {
    std::string name;
    uint32_t value;
    bool defined;
    Vec<uint32_t> undefined_positions;
    Vec<uint32_t> undefined_positions_imm16;
    Vec<uint32_t> undefined_positions_rel;
};

class X86Assembler {
    Allocator &m_al;
    Vec<uint8_t> m_code;
    std::map<std::string,Symbol> m_symbols;
    uint32_t m_origin;
#ifdef LFORTRAN_ASM_PRINT
    std::string m_asm_code;
    void emit(const std::string &indent, const std::string &s) {
        m_asm_code += indent + s + "\n";
    }
#endif
public:
    X86Assembler(Allocator &al) : m_al{al} {
        m_code.reserve(m_al, 1024*128);
        m_origin = 0x08048000;
#ifdef LFORTRAN_ASM_PRINT
        m_asm_code = "BITS 32\n\n";
#endif
    }

#ifdef LFORTRAN_ASM_PRINT
    std::string get_asm() {
        return m_asm_code;
    }

    // Saves the generated assembly into a file
    // Can be compiled with:
    // nasm -f bin filename.asm
    void save_asm(const std::string &filename) {
        std::ofstream out;
        out.open(filename);
        out << get_asm();
    }
#endif

    Vec<uint8_t>& get_machine_code() {
        return m_code;
    }

    void define_symbol(const std::string &name, uint32_t value) {
        if (m_symbols.find(name) == m_symbols.end()) {
            Symbol s;
            s.defined = true;
            s.value = value;
            s.name = name;
            m_symbols[name] = s;
        } else {
            Symbol &s = m_symbols[name];
            s.defined = true;
            s.value = value;
            // Fix previous undefined positions
            for (size_t i=0; i < s.undefined_positions.size(); i++) {
                uint32_t pos = s.undefined_positions[i];
                insert_uint32(m_code, pos, s.value);
            }
            for (size_t i=0; i < s.undefined_positions_rel.size(); i++) {
                uint32_t pos = s.undefined_positions_rel[i];
                insert_uint32(m_code, pos, s.value-pos-m_origin-4);
            }
            for (size_t i=0; i < s.undefined_positions_imm16.size(); i++) {
                uint32_t pos = s.undefined_positions_imm16[i];
                insert_uint16(m_code, pos, s.value);
            }
        }
    }

    // Adds to undefined_positions, creates a symbol if needed
    // type = 0 imm32
    // type = 1 imm16
    // type = 2 relative
    Symbol &reference_symbol(const std::string &name, int type=0) {
        if (m_symbols.find(name) == m_symbols.end()) {
            Symbol s;
            s.defined = false;
            s.value = 0;
            s.name = name;
            s.undefined_positions.reserve(m_al, 8);
            s.undefined_positions_imm16.reserve(m_al, 8);
            s.undefined_positions_rel.reserve(m_al, 8);
            m_symbols[name] = s;
        }
        Symbol &s = m_symbols[name];
        if (!s.defined) {
            switch (type) {
                case (0) :
                    s.undefined_positions.push_back(m_al, pos()-m_origin);
                    break;
                case (1) :
                    s.undefined_positions_imm16.push_back(m_al, pos()-m_origin);
                    break;
                case (2) :
                    s.undefined_positions_rel.push_back(m_al, pos()-m_origin);
                    break;
                default : throw AssemblerError("Unknown label type");
            }
        }
        return s;
    }

    uint32_t relative_symbol(const std::string &name) {
        return reference_symbol(name, 2).value-pos()-4;
    }

    // Does not touch undefined_positions, symbol must be defined
    Symbol &get_defined_symbol(const std::string &name) {
        LFORTRAN_ASSERT(m_symbols.find(name) != m_symbols.end());
        return m_symbols[name];
    }

    void add_label(const std::string &label) {
        define_symbol(label, pos());
        EMIT_LABEL(label + ":");
    }

    void add_var(const std::string &var, uint32_t val) {
        define_symbol(var, val);
        EMIT_VAR(var, val);
    }

    uint32_t pos() {
        return m_origin + m_code.size();
    }

    uint32_t origin() {
        return m_origin;
    }

    // Verifies that all symbols are defined (and thus resolved).
    void verify() {
        for (auto &s : m_symbols) {
            if (!s.second.defined) {
                throw AssemblerError("The symbol '" + s.first + "' is undefined.");
            }
        }
    }

    // Saves the generated machine code into a binary file
    void save_binary(const std::string &filename);

    void asm_pop_r32(X86Reg r32) {
        m_code.push_back(m_al, 0x58 + r32);
        EMIT("pop " + r2s(r32));
    }

    void asm_pop_r16(X86Reg r16) {
        m_code.push_back(m_al, 0x66);
        m_code.push_back(m_al, 0x58 + r16);
        EMIT("popl " + r2s(r16));
    }

    void asm_push_r32(X86Reg r32) {
        m_code.push_back(m_al, 0x50 + r32);
        EMIT("push " + r2s(r32));
    }

    void asm_push_r16(X86Reg r16) {
        m_code.push_back(m_al, 0x66);
        m_code.push_back(m_al, 0x50 + r16);
        EMIT("pushl " + r2s(r16));
    }

    void asm_push_imm8(uint8_t imm8) {
        m_code.push_back(m_al, 0x6a);
        m_code.push_back(m_al, imm8);
        EMIT("push " + i2s(imm8));
    }

    void asm_push_imm32(uint32_t imm32) {
        m_code.push_back(m_al, 0x68);
        push_back_uint32(m_code, m_al, imm32);
        EMIT("push " + i2s(imm32));
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

    void asm_jge_imm32(uint32_t imm32) {
        m_code.push_back(m_al, 0x0F);
        m_code.push_back(m_al, 0x8D);
        push_back_uint32(m_code, m_al, imm32);
        EMIT("jge " + i2s(imm32));
    }

    // Jump if ==
    void asm_je_label(const std::string &label) {
        m_code.push_back(m_al, 0x0F);
        m_code.push_back(m_al, 0x84);
        uint32_t imm32 = relative_symbol(label);
        push_back_uint32(m_code, m_al, imm32);
        EMIT("je " + label);
    }

    // Jump if !=
    void asm_jne_label(const std::string &label) {
        m_code.push_back(m_al, 0x0F);
        m_code.push_back(m_al, 0x85);
        uint32_t imm32 = relative_symbol(label);
        push_back_uint32(m_code, m_al, imm32);
        EMIT("jne " + label);
    }

    // Jump if <
    void asm_jl_label(const std::string &label) {
        m_code.push_back(m_al, 0x0F);
        m_code.push_back(m_al, 0x8C);
        uint32_t imm32 = relative_symbol(label);
        push_back_uint32(m_code, m_al, imm32);
        EMIT("jl " + label);
    }

    // Jump if <=
    void asm_jle_label(const std::string &label) {
        m_code.push_back(m_al, 0x0F);
        m_code.push_back(m_al, 0x8E);
        uint32_t imm32 = relative_symbol(label);
        push_back_uint32(m_code, m_al, imm32);
        EMIT("jle " + label);
    }

    // Jump if >
    void asm_jg_label(const std::string &label) {
        m_code.push_back(m_al, 0x0F);
        m_code.push_back(m_al, 0x8F);
        uint32_t imm32 = relative_symbol(label);
        push_back_uint32(m_code, m_al, imm32);
        EMIT("jg " + label);
    }

    // Jump if >=
    void asm_jge_label(const std::string &label) {
        m_code.push_back(m_al, 0x0F);
        m_code.push_back(m_al, 0x8D);
        uint32_t imm32 = relative_symbol(label);
        push_back_uint32(m_code, m_al, imm32);
        EMIT("jge " + label);
    }

    void asm_inc_r32(X86Reg r32) {
        m_code.push_back(m_al, 0x40+r32);
        EMIT("inc " + r2s(r32));
    }

    void asm_dec_r32(X86Reg r32) {
        m_code.push_back(m_al, 0x48+r32);
        EMIT("inc " + r2s(r32));
    }

    void asm_inc_m32(X86Reg *base, X86Reg *index, uint8_t scale, int32_t disp) {
        m_code.push_back(m_al, 0xff);
        modrm_sib_disp(m_code, m_al,
                X86Reg::eax, base, index, scale, disp, true);
        EMIT("inc " + m2s(base, index, scale, disp));
    }

    void asm_int_imm8(uint8_t imm8) {
        m_code.push_back(m_al, 0xcd);
        m_code.push_back(m_al, imm8);
        EMIT("int " + i2s(imm8));
    }

    void asm_ret() {
        m_code.push_back(m_al, 0xc3);
        EMIT("ret");
    }

    void asm_xor_r32_r32(X86Reg r32, X86Reg s32) {
        m_code.push_back(m_al, 0x31);
        modrm_sib_disp(m_code, m_al,
                s32, &r32, nullptr, 1, 0, false);
        EMIT("xor " + r2s(r32) + ", " + r2s(s32));
    }

    void asm_mov_r32_imm32(X86Reg r32, uint32_t imm32) {
        m_code.push_back(m_al, 0xb8 + r32);
        push_back_uint32(m_code, m_al, imm32);
        EMIT("mov " + r2s(r32) + ", " + i2s(imm32));
    }

    void asm_mov_r32_label(X86Reg r32, const std::string &label) {
        m_code.push_back(m_al, 0xb8 + r32);
        uint32_t imm32 = reference_symbol(label).value;
        push_back_uint32(m_code, m_al, imm32);
        EMIT("mov " + r2s(r32) + ", " + label);
    }

    void asm_mov_r32_r32(X86Reg r32, X86Reg s32) {
        m_code.push_back(m_al, 0x89);
        modrm_sib_disp(m_code, m_al,
                s32, &r32, nullptr, 1, 0, false);
        EMIT("mov " + r2s(r32) + ", " + r2s(s32));
    }

    void asm_mov_r32_m32(X86Reg r32, X86Reg *base, X86Reg *index,
                uint8_t scale, int32_t disp) {
        if (r32 == X86Reg::eax && !base && !index) {
            m_code.push_back(m_al, 0xa1);
            uint32_t disp32 = disp;
            push_back_uint32(m_code, m_al, disp32);
        } else {
            m_code.push_back(m_al, 0x8b);
            modrm_sib_disp(m_code, m_al,
                    r32, base, index, scale, disp, true);
        }
        EMIT("mov " + r2s(r32) + ", " + m2s(base, index, scale, disp));
    }

    void asm_mov_m32_r32(X86Reg *base, X86Reg *index,
                uint8_t scale, int32_t disp, X86Reg r32) {
        if (r32 == X86Reg::eax && !base && !index) {
            m_code.push_back(m_al, 0xa3);
            uint32_t disp32 = disp;
            push_back_uint32(m_code, m_al, disp32);
        } else {
            m_code.push_back(m_al, 0x89);
            modrm_sib_disp(m_code, m_al,
                    r32, base, index, scale, disp, true);
        }
        EMIT("mov " + m2s(base, index, scale, disp) + ", " + r2s(r32));
    }

    void asm_test_r32_r32(X86Reg r32, X86Reg s32) {
        m_code.push_back(m_al, 0x85);
        modrm_sib_disp(m_code, m_al,
                s32, &r32, nullptr, 1, 0, false);
        EMIT("test " + r2s(r32) + ", " + r2s(s32));
    }

    void asm_sub_r32_imm8(X86Reg r32, uint8_t imm8) {
        m_code.push_back(m_al, 0x83);
        modrm_sib_disp(m_code, m_al,
                X86Reg::ebp, &r32, nullptr, 1, 0, false);
        m_code.push_back(m_al, imm8);
        EMIT("sub " + r2s(r32) + ", " + i2s(imm8));
    }

    void asm_sub_r32_r32(X86Reg r32, X86Reg s32) {
        m_code.push_back(m_al, 0x29);
        modrm_sib_disp(m_code, m_al,
                s32, &r32, nullptr, 1, 0, false);
        EMIT("sub " + r2s(r32) + ", " + r2s(s32));
    }

    void asm_sar_r32_imm8(X86Reg r32, uint8_t imm8) {
        if (r32 == X86Reg::eax) {
            m_code.push_back(m_al, 0xc1);
            m_code.push_back(m_al, 0xf8);
            m_code.push_back(m_al, imm8);
        } else {
            throw AssemblerError("Not implemented.");
        }
        EMIT("sar " + r2s(r32) + ", " + i2s(imm8));
    }

    void asm_cmp_r32_imm8(X86Reg r32, uint8_t imm8) {
        m_code.push_back(m_al, 0x83);
        modrm_sib_disp(m_code, m_al,
                X86Reg::edi, &r32, nullptr, 1, 0, false);
        m_code.push_back(m_al, imm8);
        EMIT("cmp " + r2s(r32) + ", " + i2s(imm8));
    }

    void asm_cmp_r32_r32(X86Reg r32, X86Reg s32) {
        m_code.push_back(m_al, 0x39);
        modrm_sib_disp(m_code, m_al,
                s32, &r32, nullptr, 1, 0, false);
        EMIT("cmp " + r2s(r32) + ", " + r2s(s32));
    }

    void asm_jmp_imm8(uint8_t imm8) {
        m_code.push_back(m_al, 0xeb);
        m_code.push_back(m_al, imm8);
        EMIT("jmp " + i2s(imm8));
    }

    void asm_jmp_imm32(uint32_t imm32) {
        m_code.push_back(m_al, 0xe9);
        push_back_uint32(m_code, m_al, imm32);
        EMIT("jmp " + i2s(imm32));
    }

    void asm_jmp_label(const std::string &label) {
        m_code.push_back(m_al, 0xe9);
        uint32_t imm32 = relative_symbol(label);
        push_back_uint32(m_code, m_al, imm32);
        EMIT("jmp " + label);
    }

    void asm_call_imm32(uint32_t imm32) {
        m_code.push_back(m_al, 0xe8);
        push_back_uint32(m_code, m_al, imm32);
        EMIT("call " + i2s(imm32));
    }

    void asm_call_label(const std::string &label) {
        m_code.push_back(m_al, 0xe8);
        uint32_t imm32 = relative_symbol(label);
        push_back_uint32(m_code, m_al, imm32);
        EMIT("call " + label);
    }

    void asm_shl_r32_imm8(X86Reg r32, uint8_t imm8) {
        if (r32 == X86Reg::eax) {
            m_code.push_back(m_al, 0xc1);
            m_code.push_back(m_al, 0xe0);
            m_code.push_back(m_al, imm8);
        } else {
            throw AssemblerError("Not implemented.");
        }
        EMIT("shl " + r2s(r32) + ", " + i2s(imm8));
    }

    void asm_db_imm8(uint8_t imm8) {
        m_code.push_back(m_al, imm8);
        EMIT("db " + i2s(imm8));
    }

    void asm_db_imm8(const void *data, size_t size) {
        const uint8_t *data_char=(const uint8_t*)data;
        for (size_t i=0; i < size; i++) {
            asm_db_imm8(data_char[i]);
        }
    }

    void asm_dw_imm16(uint16_t imm16) {
        push_back_uint16(m_code, m_al, imm16);
        EMIT("dw " + i2s(imm16));
    }

    void asm_dd_imm32(uint32_t imm32) {
        push_back_uint32(m_code, m_al, imm32);
        EMIT("dd " + i2s(imm32));
    }

    void asm_dw_label(const std::string &label) {
        uint32_t imm16 = reference_symbol(label, 1).value;
        push_back_uint16(m_code, m_al, imm16);
        EMIT("dw " + label);
    }

    void asm_dd_label(const std::string &label) {
        uint32_t imm32 = reference_symbol(label).value;
        push_back_uint32(m_code, m_al, imm32);
        EMIT("dd " + label);
    }

    void asm_add_m32_r32(X86Reg *base, X86Reg *index,
                uint8_t scale, int32_t disp, X86Reg r32) {
        m_code.push_back(m_al, 0x01);
        modrm_sib_disp(m_code, m_al,
                r32, base, index, scale, disp, true);
        EMIT("add " + m2s(base, index, scale, disp) + ", " + r2s(r32));
    }

    void asm_add_r32_r32(X86Reg s32, X86Reg r32) {
        m_code.push_back(m_al, 0x01);
        modrm_sib_disp(m_code, m_al,
                r32, &s32, nullptr, 1, 0, false);
        EMIT("add " + r2s(s32) + ", " + r2s(r32));
    }

    void asm_add_r32_imm8(X86Reg r32, uint8_t imm8) {
        m_code.push_back(m_al, 0x83);
        modrm_sib_disp(m_code, m_al,
                X86Reg::eax, &r32, nullptr, 1, 0, false);
        m_code.push_back(m_al, imm8);
        EMIT("add " + r2s(r32) + ", " + i2s(imm8));
    }

    void asm_add_r32_imm32(X86Reg r32, uint32_t imm32) {
        m_code.push_back(m_al, 0x81);
        modrm_sib_disp(m_code, m_al,
                X86Reg::eax, &r32, nullptr, 1, 0, false);
        push_back_uint32(m_code, m_al, imm32);
        EMIT("add " + r2s(r32) + ", " + i2s(imm32));
    }

    void asm_mul_r32(X86Reg r32) {
        m_code.push_back(m_al, 0xF7);
        modrm_sib_disp(m_code, m_al,
                X86Reg::esp, &r32, nullptr, 1, 0, false);
        EMIT("mul " + r2s(r32));
    }

    void asm_div_r32(X86Reg r32) {
        m_code.push_back(m_al, 0xF7);
        modrm_sib_disp(m_code, m_al,
                X86Reg::esi, &r32, nullptr, 1, 0, false);
        EMIT("div " + r2s(r32));
    }

    void asm_neg_r32(X86Reg r32) {
        m_code.push_back(m_al, 0xF7);
        modrm_sib_disp(m_code, m_al,
                X86Reg::ebx, &r32, nullptr, 1, 0, false);
        EMIT("neg " + r2s(r32));
    }

    void asm_lea_r32_m32(X86Reg r32, X86Reg *base, X86Reg *index,
                uint8_t scale, int32_t disp) {
        m_code.push_back(m_al, 0x8d);
        modrm_sib_disp(m_code, m_al,
                r32, base, index, scale, disp, true);
        EMIT("lea " + r2s(r32) + ", " + m2s(base, index, scale, disp));
    }

    void asm_and_r32_imm32(X86Reg r32, uint32_t imm32) {
        if (r32 == X86Reg::eax) {
            m_code.push_back(m_al, 0x25);
            push_back_uint32(m_code, m_al, imm32);
        } else {
            throw AssemblerError("Not implemented.");
        }
        EMIT("and " + r2s(r32) + ", " + i2s(imm32));
    }

};


// Generate an ELF 32 bit header and footer
// With these two functions, one only has to generate a `_start` assembly
// function to have a working binary on Linux.
void emit_elf32_header(X86Assembler &a, uint32_t p_flags=5);
void emit_elf32_footer(X86Assembler &a);

void emit_exit(X86Assembler &a, const std::string &name,
    uint32_t exit_code);

// this is similar to emit_exit() but takes the argument (i.e. exit code)
// from top of stack. To call this exit2, one needs to jump to it
// instead of call it. (Because calling pushes the instruction address and
// base pointer value (ebp) of previous function and thus makes the
// exit code parameter less reachable)
void emit_exit2(X86Assembler &a, const std::string &name);

void emit_data_string(X86Assembler &a, const std::string &label,
    const std::string &s);
void emit_print(X86Assembler &a, const std::string &msg_label,
    uint32_t size);
void emit_print_int(X86Assembler &a, const std::string &name);

} // namespace LFortran

#endif // LFORTRAN_CODEGEN_X86_ASSEMBER_H
