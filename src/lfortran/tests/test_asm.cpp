#include <tests/doctest.h>

#include <iostream>
#include <sstream>

#include <libasr/codegen/x86_assembler.h>

using LFortran::X86Reg;

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
BITS 32

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
BITS 32

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

    std::string msg = "Hello World!\n";
    a.add_label("msg");
    a.asm_db_imm8(msg.c_str(), msg.size());

    a.add_label("_start");
    // ssize_t write(int fd, const void *buf, size_t count);
    a.asm_mov_r32_imm32(LFortran::X86Reg::eax, 4); // sys_write
    a.asm_mov_r32_imm32(LFortran::X86Reg::ebx, 1); // fd (stdout)
    a.asm_mov_r32_imm32(LFortran::X86Reg::ecx, a.get_defined_symbol("msg").value); // buf
    a.asm_mov_r32_imm32(LFortran::X86Reg::edx, msg.size()); // count
    a.asm_int_imm8(0x80);
    a.asm_call_label("exit");

    a.add_label("exit");
    // void exit(int status);
    a.asm_mov_r32_imm32(LFortran::X86Reg::eax, 1); // sys_exit
    a.asm_mov_r32_imm32(LFortran::X86Reg::ebx, 0); // exit code
    a.asm_int_imm8(0x80); // syscall

    LFortran::emit_elf32_footer(a);

    a.verify();

    a.save_binary("write32");

    // Note: both the machine code as well as the assembly below is checked
    // for consistency using
    //
    //     nasm -f bin write32.asm
    //
    // and the nasm generated `write32` binary exactly agrees with our binary.
    //
    // If any of the two tests below are modified, they have to be checked that
    // they are still consistent with each other and that they work.

#ifdef LFORTRAN_ASM_PRINT
    a.save_asm("write32.asm");
    std::string asm_code = a.get_asm();
    std::string ref = S(R"""(
BITS 32

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
    dd e_entry
    dd e_phoff
    dd 0x00000000
    dd 0x00000000
    dw ehdrsize
    dw phdrsize
    dw 0x0001
    dw 0x0000
    dw 0x0000
    dw 0x0000

ehdrsize equ 0x00000034

phdr:
    dd 0x00000001
    dd 0x00000000
    dd 0x08048000
    dd 0x08048000
    dd filesize
    dd filesize
    dd 0x00000005
    dd 0x00001000

phdrsize equ 0x00000020


e_phoff equ 0x00000034

msg:
    db 0x48
    db 0x65
    db 0x6c
    db 0x6c
    db 0x6f
    db 0x20
    db 0x57
    db 0x6f
    db 0x72
    db 0x6c
    db 0x64
    db 0x21
    db 0x0a
_start:
    mov eax, 0x00000004
    mov ebx, 0x00000001
    mov ecx, 0x08048054
    mov edx, 0x0000000d
    int 0x80
    call exit
exit:
    mov eax, 0x00000001
    mov ebx, 0x00000000
    int 0x80

e_entry equ 0x08048061


filesize equ 0x00000088

)""");
    CHECK(asm_code == ref);
#endif

    std::vector<uint8_t> cref;

    cref = {0x7f, 0x45, 0x4c, 0x46, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x00, 0x01, 0x00,
        0x00, 0x00, 0x61, 0x80, 0x04, 0x08, 0x34, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00, 0x20, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x04, 0x08, 0x00, 0x80,
        0x04, 0x08, 0x88, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x05,
        0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x48, 0x65, 0x6c, 0x6c,
        0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0x0a, 0xb8, 0x04,
        0x00, 0x00, 0x00, 0xbb, 0x01, 0x00, 0x00, 0x00, 0xb9, 0x54, 0x80,
        0x04, 0x08, 0xba, 0x0d, 0x00, 0x00, 0x00, 0xcd, 0x80, 0xe8, 0x00,
        0x00, 0x00, 0x00, 0xb8, 0x01, 0x00, 0x00, 0x00, 0xbb, 0x00, 0x00,
        0x00, 0x00, 0xcd, 0x80};
    CHECK( a.get_machine_code().as_vector() == cref );
}

TEST_CASE("print") {
    Allocator al(1024);
    LFortran::X86Assembler a(al);
    std::string msg = "Hello World!\n";

    LFortran::emit_elf32_header(a);
    a.add_label("_start");
    LFortran::emit_print(a, "msg", msg.size());
    a.asm_call_label("exit");
    LFortran::emit_exit(a, "exit", 0);
    LFortran::emit_data_string(a, "msg", msg);
    LFortran::emit_elf32_footer(a);

    a.verify();

    a.save_binary("print32");
}

TEST_CASE("cmp") {
    Allocator al(1024);
    LFortran::X86Assembler a(al);
    std::string msg1 = "Branch 1\n";
    std::string msg2 = "Branch 2\n";

    LFortran::emit_elf32_header(a);
    a.add_label("_start");
    // if (3 >= 5) then
    a.asm_mov_r32_imm32(LFortran::X86Reg::eax, 3);
    a.asm_cmp_r32_imm8(LFortran::X86Reg::eax, 5);
    a.asm_jge_label(".then");
    a.asm_jmp_label(".else");
    a.add_label(".then");
    LFortran::emit_print(a, "msg1", msg1.size());
    a.asm_jmp_label(".endif");
    a.add_label(".else");
    LFortran::emit_print(a, "msg2", msg2.size());
    a.add_label(".endif");
    a.asm_call_label("exit");
    LFortran::emit_exit(a, "exit", 0);
    LFortran::emit_data_string(a, "msg1", msg1);
    LFortran::emit_data_string(a, "msg2", msg2);
    LFortran::emit_elf32_footer(a);

    a.verify();

    a.save_binary("cmp32");
}

TEST_CASE("subroutines") {
    Allocator al(1024);
    LFortran::X86Assembler a(al);
    std::string msg1 = "Subroutine 1, calling 2\n";
    std::string msg1b = "Subroutine 1, done\n";
    std::string msg2 = "Subroutine 2, modifying a message\n";

    LFortran::emit_elf32_header(a, 7);

    a.add_label("sub1");
    LFortran::emit_print(a, "msg1", msg1.size());
    a.asm_call_label("sub2");
    LFortran::emit_print(a, "msg1b", msg1b.size());
    a.asm_ret();

    a.add_label("sub2");
    LFortran::emit_print(a, "msg2", msg2.size());
    a.asm_mov_r32_imm32(LFortran::X86Reg::eax, 0x42414443);
    a.asm_mov_r32_label(LFortran::X86Reg::ebx, "msg1b");
    LFortran::X86Reg base = LFortran::X86Reg::ebx;
    a.asm_mov_m32_r32(&base, nullptr, 1, 5, LFortran::X86Reg::eax);
    a.asm_ret();

    LFortran::emit_exit(a, "exit", 0);

    a.add_label("_start");
    a.asm_call_label("sub1");
    a.asm_call_label("exit");

    LFortran::emit_data_string(a, "msg1", msg1);
    LFortran::emit_data_string(a, "msg1b", msg1b);
    LFortran::emit_data_string(a, "msg2", msg2);
    LFortran::emit_elf32_footer(a);

    a.verify();

    a.save_binary("subroutines32");
}

TEST_CASE("subroutine args") {
    Allocator al(1024);
    LFortran::X86Assembler a(al);
    std::string msg1 = "Subroutine 1\n";
    std::string msg2 = "Sum equal to 9\n";
    std::string msg3 = "Sum not equal to 9\n";

    LFortran::emit_elf32_header(a, 7);

    a.add_label("sub1");
    // Initialize stack
    a.asm_push_r32(X86Reg::ebp);
    a.asm_mov_r32_r32(X86Reg::ebp, X86Reg::esp);
    a.asm_sub_r32_imm8(X86Reg::esp, 4); // one local variable

    LFortran::emit_print(a, "msg1", msg1.size());

    X86Reg base = X86Reg::ebp;
    // mov eax, [ebp+8] // first argument
    a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, 8);
    // mov ecx, [ebp+12] // second argument
    a.asm_mov_r32_m32(X86Reg::ecx, &base, nullptr, 1, 12);
    // mov [ebp-4], eax // move eax to a local variable
    a.asm_mov_m32_r32(&base, nullptr, 1, -4, X86Reg::eax);
    // add [ebp-4], ecx // add ecx
    a.asm_add_m32_r32(&base, nullptr, 1, -4, X86Reg::ecx);
    // mov eax, [ebp-4] // move the sum into the return value (eax)
    a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, -4);

    // Restore stack
    a.asm_mov_r32_r32(X86Reg::esp, X86Reg::ebp);
    a.asm_pop_r32(X86Reg::ebp);
    a.asm_ret();

    LFortran::emit_exit(a, "exit", 0);

    a.add_label("_start");
    // Push arguments to stack (last argument first)
    a.asm_push_imm8(5); // second argument
    a.asm_push_imm8(4); // first argument
    a.asm_call_label("sub1");
    // Remove arguments from stack: 2 arguments, 4 bytes each
    a.asm_add_r32_imm8(LFortran::X86Reg::esp, 2*4);

    a.asm_cmp_r32_imm8(LFortran::X86Reg::eax, 9);
    a.asm_je_label(".then");
    a.asm_jmp_label(".else");
    a.add_label(".then");
    LFortran::emit_print(a, "msg2", msg2.size());
    a.asm_jmp_label(".endif");
    a.add_label(".else");
    LFortran::emit_print(a, "msg3", msg3.size());
    a.add_label(".endif");

    a.asm_call_label("exit");

    LFortran::emit_data_string(a, "msg1", msg1);
    LFortran::emit_data_string(a, "msg2", msg2);
    LFortran::emit_data_string(a, "msg3", msg3);
    LFortran::emit_elf32_footer(a);

    a.verify();

    a.save_binary("subroutines_args32");
}

TEST_CASE("print integer") {
    Allocator al(1024);
    LFortran::X86Assembler a(al);

    LFortran::emit_elf32_header(a);
    LFortran::emit_print_int(a, "print_int");
    LFortran::emit_exit(a, "exit", 0);
    a.add_label("_start");

    a.asm_push_imm32(1234);
    a.asm_call_label("print_int");
    a.asm_add_r32_imm8(LFortran::X86Reg::esp, 4);

    a.asm_push_imm32(5678);
    a.asm_call_label("print_int");
    a.asm_add_r32_imm8(LFortran::X86Reg::esp, 4);

    a.asm_call_label("exit");

    LFortran::emit_elf32_footer(a);

    a.verify();

    a.save_binary("print_integer");
}
