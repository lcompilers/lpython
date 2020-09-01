#include <tests/doctest.h>

#include <iostream>
#include <sstream>

#include <lfortran/codegen/x86_assembler.h>

// Print any vector like iterable to a string
template <class T>
inline std::ostream &print_vec(std::ostream &out, T &d)
{
    out << "[";
    for (auto p = d.begin(); p != d.end(); p++) {
        if (p != d.begin())
            out << ", ";
        out << *p;
    }
    out << "]";
    return out;
}

// Specialization for `const std::vector<uint8_t>`
template <>
inline std::ostream &print_vec(std::ostream &out, const std::vector<uint8_t> &d)
{
    out << "[";
    for (auto p = d.begin(); p != d.end(); p++) {
        if (p != d.begin())
            out << ", ";
        out << LFortran::i2s(*p);
    }
    out << "]";
    return out;
}


namespace doctest {
    // Convert std::vector<T> to string for doctest
    template<typename T> struct StringMaker<std::vector<T>> {
        static String convert(const std::vector<T> &value) {
            std::ostringstream oss;
            print_vec(oss, value);
            return oss.str().c_str();
        }
    };
}

// Strips the first character (\n)
std::string S(const std::string &s) {
    return s.substr(1, s.size());
}

TEST_CASE("Store and get instructions") {
    Allocator al(1024);
    LFortran::X86Assembler a(al);
    a.asm_pop_r32(LFortran::X86Reg::eax);
    a.asm_jz_imm8(13);

    LFortran::Vec<uint8_t> &code = a.get_machine_code();
    CHECK(code.size() == 3);

#ifdef LFORTRAN_ASM_PRINT
    std::string asm_code = a.get_asm();
    std::string ref = S(R"""(
    pop eax
    jz 0x0d
)""");
    CHECK(asm_code == ref);
#endif
}

TEST_CASE("modrm_sib_disp") {
    Allocator al(1024);
    LFortran::Vec<uint8_t> code;
    LFortran::X86Reg base;
    LFortran::X86Reg index;
    std::vector<uint8_t> ref;

    base = LFortran::X86Reg::ecx;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, nullptr, 1, 0, false);
    ref = {0xd9};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ecx;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, nullptr, 1, 0, true);
    ref = {0x19};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::esi;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, nullptr, 1, 0, true);
    ref = {0x1e};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebp;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, nullptr, 1, 0, true);
    ref = {0x5d, 0x00};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebp;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, nullptr, 1, 3, true);
    ref = {0x5d, 0x03};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebp;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, nullptr, 1, -3, true);
    ref = {0x5d, 0xfd};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebp;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, nullptr, 1, 127, true);
    ref = {0x5d, 0x7f};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebp;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, nullptr, 1, 128, true);
    ref = {0x9d, 0x80, 0x00, 0x00, 0x00};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebp;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, nullptr, 1, -128, true);
    ref = {0x5d, 0x80};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebp;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, nullptr, 1, -129, true);
    ref = {0x9d, 0x7f, 0xff, 0xff, 0xff};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::esp;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, nullptr, 1, 0, true);
    ref = {0x1c, 0x24};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::eax;
    index = LFortran::X86Reg::ebx;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, &index, 1, 0, true);
    ref = {0x1c, 0x18};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebx;
    index = LFortran::X86Reg::eax;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, &index, 1, 0, true);
    ref = {0x1c, 0x03};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebx;
    index = LFortran::X86Reg::eax;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, &index, 2, 0, true);
    ref = {0x1c, 0x43};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebx;
    index = LFortran::X86Reg::eax;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, &index, 4, 0, true);
    ref = {0x1c, 0x83};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebx;
    index = LFortran::X86Reg::eax;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, &index, 8, 0, true);
    ref = {0x1c, 0xc3};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebx;
    index = LFortran::X86Reg::eax;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, &index, 8, 127, true);
    ref = {0x5c, 0xc3, 0x7f};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebx;
    index = LFortran::X86Reg::eax;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, &index, 8, 128, true);
    ref = {0x9c, 0xc3, 0x80, 0x00, 0x00, 0x00};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebx;
    index = LFortran::X86Reg::eax;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, &index, 2, 1, true);
    ref = {0x5c, 0x43, 0x01};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebx;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ebx, &base, nullptr, 1, 1, true);
    ref = {0x5b, 0x01};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebp;
    index = LFortran::X86Reg::ebx;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ecx, &base, &index, 4, 1, true);
    ref = {0x4c, 0x9d, 0x01};
    CHECK( code.as_vector() == ref );

    base = LFortran::X86Reg::ebp;
    index = LFortran::X86Reg::ebx;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ecx, &base, &index, 4, 128, true);
    ref = {0x8c, 0x9d, 0x80, 0x00, 0x00, 0x00};
    CHECK( code.as_vector() == ref );

    index = LFortran::X86Reg::ebx;
    code.reserve(al, 32);
    LFortran::modrm_sib_disp(code, al,
        LFortran::X86Reg::ecx, nullptr, &index, 4, 1, true);
    ref = {0x0c, 0x9d, 0x01, 0x00, 0x00, 0x00};
    CHECK( code.as_vector() == ref );

    index = LFortran::X86Reg::ebx;
    code.reserve(al, 32);
    CHECK_THROWS_AS(LFortran::modrm_sib_disp(code, al,
            LFortran::X86Reg::ecx, nullptr, &index, 3, 1, true),
        LFortran::AssemblerError);

    index = LFortran::X86Reg::ebx;
    code.reserve(al, 32);
    CHECK_THROWS_AS(LFortran::modrm_sib_disp(code, al,
            LFortran::X86Reg::ecx, nullptr, nullptr, 1, 0, false),
        LFortran::AssemblerError);

    index = LFortran::X86Reg::ebx;
    code.reserve(al, 32);
    CHECK_THROWS_AS(LFortran::modrm_sib_disp(code, al,
            LFortran::X86Reg::ecx, nullptr, nullptr, 1, 0, true),
        LFortran::AssemblerError);
}

TEST_CASE("Memory operand") {
    Allocator al(1024);
    LFortran::X86Assembler a(al);
    LFortran::X86Reg base = LFortran::X86Reg::ebx;
    LFortran::X86Reg index = LFortran::X86Reg::ecx;

    a.asm_inc_m32(&base, nullptr, 1, 0);
    a.asm_inc_m32(&base, nullptr, 1, 3);
    a.asm_inc_m32(&base, nullptr, 1, -2);

    a.asm_inc_m32(&base, &index, 1, 2);
    a.asm_inc_m32(&base, &index, 2, 2);
    a.asm_inc_m32(&base, &index, 4, 2);
    a.asm_inc_m32(&base, &index, 8, 2);
    a.asm_inc_m32(&base, &index, 1, -2);
    a.asm_inc_m32(&base, &index, 1, 1024);
    a.asm_inc_m32(&base, &index, 2, 0);

    a.asm_inc_m32(nullptr, &index, 1, 2);
    a.asm_inc_m32(nullptr, &index, 2, 2);
    a.asm_inc_m32(nullptr, &index, 4, 2);
    a.asm_inc_m32(nullptr, &index, 8, 2);
    a.asm_inc_m32(nullptr, &index, 1, -2);
    a.asm_inc_m32(nullptr, &index, 1, 1024);
    a.asm_inc_m32(nullptr, &index, 2, 0);

#ifdef LFORTRAN_ASM_PRINT
    std::string asm_code = a.get_asm();
    std::string ref = S(R"""(
    inc [ebx]
    inc [ebx+3]
    inc [ebx-2]
    inc [ebx+ecx+2]
    inc [ebx+2*ecx+2]
    inc [ebx+4*ecx+2]
    inc [ebx+8*ecx+2]
    inc [ebx+ecx-2]
    inc [ebx+ecx+1024]
    inc [ebx+2*ecx]
    inc [ecx+2]
    inc [2*ecx+2]
    inc [4*ecx+2]
    inc [8*ecx+2]
    inc [ecx-2]
    inc [ecx+1024]
    inc [2*ecx]
)""");
    CHECK(asm_code == ref);
#endif
}

TEST_CASE("elf32 binary") {
    Allocator al(1024);
    LFortran::X86Assembler a(al);

    LFortran::emit_elf32_header(a);
    a.add_label("msg");
    std::string msg = "Hello World!\n";
    a.asm_db_imm8(msg.c_str(), msg.size());

#ifdef LFORTRAN_ASM_PRINT
    std::string asm_code = a.get_asm();
    std::string ref = S(R"""(
ehdr:
    db 0x7f
    db 0x45
    db 0x4c
    db 0x46
    db 0x01
    db 0x01
    db 0x01
    db 0x00
    db 0x00
    db 0x00
    db 0x00
    db 0x00
    db 0x00
    db 0x00
    db 0x00
    db 0x00
    dw 0x0002
    dw 0x0003
    dd 0x00000001
    dd 0x00000000
    dd 0x00000000
    dw 0x0001
    dw 0x0000
    dw 0x0000
    dw 0x0000

ehdrsize equ 0x00000028

phdr:
    dd 0x00000001
    dd 0x00000000
    dd 0x08048000
    dd 0x08048000
    dd 0x00000005
    dd 0x00001000

phdrsize equ 0x00000040

)""");
    CHECK(asm_code == ref);
#endif
}
