#ifdef __unix__
#define LFORTRAN_LINUX
#endif

#ifdef LFORTRAN_LINUX
#include <sys/stat.h>
#endif

#include <libasr/codegen/x86_assembler.h>

namespace LCompilers {

void X86Assembler::save_binary64(const std::string &filename) {
    Vec<uint8_t> header = create_elf64_x86_header(
        m_al, origin(), get_defined_symbol("_start").value,
        compute_seg_size("text_segment_start", "text_segment_end"),
        compute_seg_size("data_segment_start", "data_segment_end"));
    {
        std::ofstream out;
        out.open(filename);
        out.write((const char*) header.p, header.size());
        out.write((const char*) m_code.p, m_code.size());
    }
#ifdef LFORTRAN_LINUX
    int mod = 0755;
    if (chmod(filename.c_str(),mod) < 0) {
        throw AssemblerError("chmod failed");
    }
#endif
}

void X86Assembler::save_binary(const std::string &filename) {
    {
        std::ofstream out;
        out.open(filename);
        out.write((const char*) m_code.p, m_code.size());
    }
#ifdef LFORTRAN_LINUX
    std::string mode = "0755";
    int mod = strtol(mode.c_str(), 0, 8);
    if (chmod(filename.c_str(),mod) < 0) {
        throw AssemblerError("chmod failed");
    }
#endif
}

// ELF header structure for 32-bit
struct Elf32_Ehdr {
    uint8_t ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
};

// Program header structure for 32-bit
struct Elf32_Phdr {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
};

void emit_elf32_header(X86Assembler &a, uint32_t p_flags) {
    /* Elf32_Ehdr */
    a.add_label("ehdr");
    // e_ident
    a.asm_db_imm8(0x7F);
    a.asm_db_imm8('E');
    a.asm_db_imm8('L');
    a.asm_db_imm8('F');
    a.asm_db_imm8(1);
    a.asm_db_imm8(1);
    a.asm_db_imm8(1);
    a.asm_db_imm8(0);

    a.asm_db_imm8(0);
    a.asm_db_imm8(0);
    a.asm_db_imm8(0);
    a.asm_db_imm8(0);

    a.asm_db_imm8(0);
    a.asm_db_imm8(0);
    a.asm_db_imm8(0);
    a.asm_db_imm8(0);

    a.asm_dw_imm16(2);  // e_type
    a.asm_dw_imm16(3);  // e_machine
    a.asm_dd_imm32(1);  // e_version
    a.asm_dd_label("_start");  // e_entry
    a.asm_dd_label("e_phoff");  // e_phoff
    a.asm_dd_imm32(0);  // e_shoff
    a.asm_dd_imm32(0);  // e_flags
    a.asm_dw_label("ehdrsize");  // e_ehsize
    a.asm_dw_label("phdrsize");  // e_phentsize
    a.asm_dw_imm16(1);  // e_phnum
    a.asm_dw_imm16(0);  // e_shentsize
    a.asm_dw_imm16(0);  // e_shnum
    a.asm_dw_imm16(0);  // e_shstrndx


    /* Elf32_Phdr */
    a.add_label("phdr");
    a.asm_dd_imm32(1);        // p_type
    a.asm_dd_imm32(0);        // p_offset
    a.asm_dd_imm32(a.origin());   // p_vaddr
    a.asm_dd_imm32(a.origin());   // p_paddr
    a.asm_dd_label("filesize"); // p_filesz
    a.asm_dd_label("filesize"); // p_memsz
    a.asm_dd_imm32(p_flags);        // p_flags
    a.asm_dd_imm32(0x1000);   // p_align
    a.add_label("phdr_end");

    a.add_var("ehdrsize", "ehdr", "phdr");
    a.add_var("phdrsize", "phdr", "phdr_end");
    a.add_var("e_phoff", "ehdr", "phdr");
}

void emit_elf32_footer(X86Assembler &a) {
    a.add_label("footer");
    a.add_var("filesize", "ehdr", "footer");
}

void emit_exit(X86Assembler &a, const std::string &name,
    uint32_t exit_code)
{
    a.add_label(name);
    // void exit(int status);
    a.asm_mov_r32_imm32(X86Reg::eax, 1); // sys_exit
    a.asm_mov_r32_imm32(X86Reg::ebx, exit_code); // exit code
    a.asm_int_imm8(0x80); // syscall
}

void emit_exit2(X86Assembler &a, const std::string &name)
{
    a.add_label(name);
    // void exit();
    a.asm_mov_r32_imm32(X86Reg::eax, 1); // sys_exit
    a.asm_pop_r32(X86Reg::ebx); // exit code on stack, move to register
    a.asm_int_imm8(0x80); // syscall
}

void emit_data_string(X86Assembler &a, const std::string &label,
    const std::string &s)
{
    a.add_label(label);
    a.asm_db_imm8(s.c_str(), s.size());
}

void emit_i32_const(X86Assembler &a, const std::string &label,
    const int32_t z) {
    uint8_t encoded_i32[sizeof(z)];
    std::memcpy(&encoded_i32, &z, sizeof(z));
    a.add_label(label);
    a.asm_db_imm8(encoded_i32, sizeof(z));
}

void emit_i64_const(X86Assembler &a, const std::string &label,
    const int64_t z) {
    uint8_t encoded_i64[sizeof(z)];
    std::memcpy(&encoded_i64, &z, sizeof(z));
    a.add_label(label);
    a.asm_db_imm8(encoded_i64, sizeof(z));
}

void emit_float_const(X86Assembler &a, const std::string &label,
    const float z) {
    uint8_t encoded_float[sizeof(z)];
    std::memcpy(&encoded_float, &z, sizeof(z));
    a.add_label(label);
    a.asm_db_imm8(encoded_float, sizeof(z));
}

void emit_double_const(X86Assembler &a, const std::string &label,
    const double z) {
    uint8_t encoded_double[sizeof(z)];
    std::memcpy(&encoded_double, &z, sizeof(z));
    a.add_label(label);
    a.asm_db_imm8(encoded_double, sizeof(z));
}

void emit_print(X86Assembler &a, const std::string &msg_label,
    uint32_t size)
{
    // ssize_t write(int fd, const void *buf, size_t count);
    a.asm_mov_r32_imm32(X86Reg::eax, 4); // sys_write
    a.asm_mov_r32_imm32(X86Reg::ebx, 1); // fd (stdout)
    a.asm_mov_r32_label(X86Reg::ecx, msg_label); // buf
    a.asm_mov_r32_imm32(X86Reg::edx, size); // count
    a.asm_int_imm8(0x80);
}

void emit_print_int(X86Assembler &a, const std::string &name)
{
    // void print_int(uint32_t i);
    a.add_label(name);

    // Initialize stack
    a.asm_push_r32(X86Reg::ebp);
    a.asm_mov_r32_r32(X86Reg::ebp, X86Reg::esp);

    X86Reg base = X86Reg::ebp;
    // mov eax, [ebp+8]  // argument "i"
    a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, 8);

    a.asm_mov_r32_r32(X86Reg::ecx, X86Reg::eax); // make a copy in ecx
    a.asm_mov_r32_imm32(X86Reg::ebx, 0);
    a.asm_cmp_r32_r32(X86Reg::eax, X86Reg::ebx);
    a.asm_jge_label(".print_int_"); // if num >= 0 then print it

    // print "-" and then negate the integer
    emit_print(a, "string_neg", 1U);
    // ecx value changed during print so fetch back
    a.asm_mov_r32_m32(X86Reg::ecx, &base, nullptr, 1, 8);
    a.asm_neg_r32(X86Reg::ecx);

    a.add_label(".print_int_");

    a.asm_mov_r32_r32(X86Reg::eax, X86Reg::ecx); // fetch the val in ecx back to eax
    a.asm_xor_r32_r32(X86Reg::esi, X86Reg::esi);

    a.add_label(".loop");
//    mov edx, 0
    a.asm_mov_r32_imm32(X86Reg::edx, 0);
//    mov ebx, 10
    a.asm_mov_r32_imm32(X86Reg::ebx, 10);
//    div ebx
    a.asm_div_r32(X86Reg::ebx);
//    add edx, 48
    a.asm_add_r32_imm32(X86Reg::edx, 48);
//    push edx
    a.asm_push_r32(X86Reg::edx);
//    inc esi
    a.asm_inc_r32(X86Reg::esi);
//    cmp eax, 0
    a.asm_cmp_r32_imm8(X86Reg::eax, 0);
//    jz .print
    a.asm_je_label(".print");
//    jmp .loop
    a.asm_jmp_label(".loop");

    a.add_label(".print");
//    cmp esi, 0
    a.asm_cmp_r32_imm8(X86Reg::esi, 0);
//    jz end
    a.asm_je_label(".end");
//    dec esi
    a.asm_dec_r32(X86Reg::esi);
//    mov eax, 4
    a.asm_mov_r32_imm32(X86Reg::eax, 4);
//    mov ecx, esp
    a.asm_mov_r32_r32(X86Reg::ecx, X86Reg::esp);
//    mov ebx, 1
    a.asm_mov_r32_imm32(X86Reg::ebx, 1);
//    mov edx, 1
    a.asm_mov_r32_imm32(X86Reg::edx, 1);
//    int 0x80
    a.asm_int_imm8(0x80);
//    add esp, 4
    a.asm_add_r32_imm32(X86Reg::esp, 4);
//    jmp .print
    a.asm_jmp_label(".print");

    a.add_label(".end");

    // Restore stack
    a.asm_mov_r32_r32(X86Reg::esp, X86Reg::ebp);
    a.asm_pop_r32(X86Reg::ebp);
    a.asm_ret();
}

void emit_print_float(X86Assembler &a, const std::string &name) {
    // void print_float(float z);
    a.add_label(name);

    // Initialize stack
    a.asm_push_r32(X86Reg::ebp);
    a.asm_mov_r32_r32(X86Reg::ebp, X86Reg::esp);

    X86Reg base = X86Reg::ebp;
    a.asm_fld_m32(&base, nullptr, 1, 8); // load argument into floating register stack
    a.asm_push_imm32(0); // decrement stack pointer and create space
    X86Reg stack_top = X86Reg::esp;
    a.asm_fistp_m32(&stack_top, nullptr, 1, 0);

    // print the integral part
    {
        a.asm_call_label("print_i32");
        a.asm_add_r32_imm32(X86Reg::esp, 4); // increment stack top and thus pop the value to be set
    }

    // print dot
    emit_print(a, "string_dot", 1U);

    // print fractional part
    {
        a.asm_fld_m32(&base, nullptr, 1, 8); // load argument into floating register stack
        a.asm_fld_m32(&base, nullptr, 1, 8); // load another copy of argument into floating register stack
        a.asm_frndint(); // round st(0) to integral part
        a.asm_fsubp();

        // st(0) now contains only the fractional part

        a.asm_push_imm32(100000000);
        a.asm_fimul_m32int(&stack_top, nullptr, 1, 0);
        a.asm_fistp_m32(&stack_top, nullptr, 1, 0);
        // print the fractional part
        {
            a.asm_call_label("print_i32");
            a.asm_add_r32_imm32(X86Reg::esp, 4); // increment stack top and thus pop the value to be set
        }
    }

    // Restore stack
    a.asm_mov_r32_r32(X86Reg::esp, X86Reg::ebp);
    a.asm_pop_r32(X86Reg::ebp);
    a.asm_ret();
}

/************************* 64-bit functions **************************/

// ELF header structure for 64-bit
struct Elf64_Ehdr {
    uint8_t ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
};

// Program header structure for 64-bit
struct Elf64_Phdr {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
};

Elf64_Ehdr get_elf_header(uint64_t asm_entry) {
    Elf64_Ehdr e;
    e.ident[0] = 0x7f; // magic number
    e.ident[1] = 'E';
    e.ident[2] = 'L';
    e.ident[3] = 'F';
    e.ident[4] = 2; // file class (64-bit)
    e.ident[5] = 1; // data encoding (little endian)
    e.ident[6] = 1; // ELF version
    e.ident[7] = 0; // padding
    e.ident[8] = 0;
    e.ident[9] = 0;
    e.ident[10] = 0;
    e.ident[11] = 0;
    e.ident[12] = 0;
    e.ident[13] = 0;
    e.ident[14] = 0;
    e.ident[15] = 0;
    e.type = 2;
    e.machine = 0x3e;
    e.version = 1;
    e.entry = asm_entry;
    e.phoff = sizeof(Elf64_Ehdr);
    e.shoff = 0;
    e.flags = 0;
    e.ehsize = sizeof(Elf64_Ehdr);
    e.phentsize = sizeof(Elf64_Phdr);
    e.phnum = 3;
    e.shentsize = 0;
    e.shnum = 0;
    e.shstrndx = 0;
    return e;
}

Elf64_Phdr get_seg_header(uint32_t flags, uint64_t origin_addr,
    uint64_t seg_size, uint64_t prev_seg_offset, uint64_t prev_seg_size) {
    Elf64_Phdr p;
    p.type = 1;
    p.flags = flags;
    p.offset = prev_seg_offset + prev_seg_size;
    p.vaddr = origin_addr + p.offset;
    p.paddr = p.vaddr;
    p.filesz = seg_size;
    p.memsz = p.filesz;
    p.align = 0x1000;
    return p;
}

template <typename T>
void append_header_bytes(Allocator &al, T src, Vec<uint8_t> &des) {
    char *byteArray = (char *)&src;
    for (size_t i = 0; i < sizeof(src); i++) {
        des.push_back(al, byteArray[i]);
    }
 }


void align_by_byte(Allocator &al, Vec<uint8_t> &code, uint64_t alignment) {
    uint64_t code_size = code.size() ;
    uint64_t padding_size = (alignment * ceil(code_size / (double)alignment)) - code_size;
    for (size_t i = 0; i < padding_size; i++) {
        code.push_back(al, 0);
    }
 }

Vec<uint8_t> create_elf64_x86_header(Allocator &al, uint64_t origin, uint64_t entry,
    uint64_t text_seg_size, uint64_t data_seg_size) {

    /*
        The header segment is a segment which holds the elf and program headers.
        Its size currently is
            sizeof(Elf64_Ehdr) + 3 * sizeof(Elf64_Phdr)
            that is, 64 + 3 * 56 = 232
        Since, it is a segment, it needs to be aligned by boundary 0x1000
        (we add temporary zero bytes as padding to accomplish this alignment)

        Thus, the header segment size for us currently is 0x1000.

        For now, we are hardcoding this size here.

        TODO: Later compute this header segment size dynamically depending
        on the different segments present
    */
    const int HEADER_SEGMENT_SIZE = 0x1000;

    // adjust/offset the origin address as per the extra bytes of HEADER_SEGMENT_SIZE
    uint64_t origin_addr = origin - HEADER_SEGMENT_SIZE;

    Elf64_Ehdr e = get_elf_header(entry);
    Elf64_Phdr p_program = get_seg_header(4, origin_addr, HEADER_SEGMENT_SIZE, 0, 0);
    Elf64_Phdr p_text_seg = get_seg_header(5, origin_addr, text_seg_size, p_program.offset, p_program.filesz);
    Elf64_Phdr p_data_seg = get_seg_header(6, origin_addr, data_seg_size, p_text_seg.offset, p_text_seg.filesz);

    Vec<uint8_t> header;
    header.reserve(al, HEADER_SEGMENT_SIZE);

    {
        append_header_bytes(al, e, header);
        append_header_bytes(al, p_program, header);
        append_header_bytes(al, p_text_seg, header);
        append_header_bytes(al, p_data_seg, header);

        LCompilers::align_by_byte(al, header, 0x1000);
    }

    return header;
}

void emit_print_64(X86Assembler &a, const std::string &msg_label, uint64_t size)
{
    // mov rax, 1        ; write(
    // mov rdi, 1        ;   STDOUT_FILENO,
    // mov rsi, msg      ;   "Hello, world!\n",
    // mov rdx, msglen   ;   sizeof("Hello, world!\n")
    // syscall           ; );

    a.asm_mov_r64_imm64(X64Reg::rax, 1);
    a.asm_mov_r64_imm64(X64Reg::rdi, 1);
    a.asm_mov_r64_label(X64Reg::rsi, msg_label); // buf
    a.asm_mov_r64_imm64(X64Reg::rdx, size);
    a.asm_syscall();
}

void emit_print_int_64(X86Assembler &a, const std::string &name)
{
    // void print_int_64(uint64_t i);
    a.add_label(name);
        // Initialize stack
        a.asm_push_r64(X64Reg::rbp);
        a.asm_mov_r64_r64(X64Reg::rbp, X64Reg::rsp);

        X64Reg base = X64Reg::rbp;
        a.asm_mov_r64_m64(X64Reg::r8, &base, nullptr, 1, 16); // mov r8, [rbp+16]  // argument "i"
        a.asm_mov_r64_imm64(X64Reg::r9, 0); // r9 holds count of digits

        // if num >= 0 then print it
        a.asm_cmp_r64_imm8(X64Reg::r8, 0);
        a.asm_jge_label("_print_i64_loop_initialize");

        // print "-" and then negate the integer
        emit_print_64(a, "string_neg", 1);
        a.asm_neg_r64(X64Reg::r8);

    a.add_label("_print_i64_loop_initialize");
        a.asm_mov_r64_r64(X64Reg::rax, X64Reg::r8); // rax as quotient
        a.asm_mov_r64_imm64(X64Reg::r10, 10); // 10 as divisor

    a.add_label("_print_i64_loop");
        a.asm_mov_r64_imm64(X64Reg::rdx, 0);
        a.asm_div_r64(X64Reg::r10);
        a.asm_add_r64_imm32(X64Reg::rdx, 48);
        a.asm_push_r64(X64Reg::rdx);
        a.asm_inc_r64(X64Reg::r9);
        a.asm_cmp_r64_imm8(X64Reg::rax, 0);
        a.asm_je_label("_print_i64_digit");
        a.asm_jmp_label("_print_i64_loop");

    a.add_label("_print_i64_digit");
        a.asm_cmp_r64_imm8(X64Reg::r9, 0);
        a.asm_je_label("_print_i64_end");
        a.asm_dec_r64(X64Reg::r9);
        { // write() syscall
            a.asm_mov_r64_imm64(X64Reg::rax, 1);
            a.asm_mov_r64_imm64(X64Reg::rdi, 1);
            a.asm_mov_r64_r64(X64Reg::rsi, X64Reg::rsp);
            a.asm_mov_r64_imm64(X64Reg::rdx, 1);
            a.asm_syscall();
        }
        a.asm_add_r64_imm32(X64Reg::rsp, 8); // pop and increment stack pointer
        a.asm_jmp_label("_print_i64_digit");

    a.add_label("_print_i64_end");
        // Restore stack
        a.asm_mov_r64_r64(X64Reg::rsp, X64Reg::rbp);
        a.asm_pop_r64(X64Reg::rbp);
        a.asm_ret();
}

void emit_print_double(X86Assembler &a, const std::string &name) {
    // void print_double(double z);
    a.add_label(name);

    // Initialize stack
    a.asm_push_r64(X64Reg::rbp);
    a.asm_mov_r64_r64(X64Reg::rbp, X64Reg::rsp);

    X64Reg base = X64Reg::rbp;
    a.asm_movsd_r64_m64(X64FReg::xmm0, &base, nullptr, 1, 16); // load argument into floating-point register

    // if z >= 0 then print it
    a.asm_mov_r64_imm64(X64Reg::rax, 0);
    a.asm_cvtsi2sd_r64_r64(X64FReg::xmm1, X64Reg::rax);
    a.asm_cmpsd_r64_r64(X64FReg::xmm0, X64FReg::xmm1, Fcmp::ge);
    a.asm_pmovmskb_r32_r64(X86Reg::eax, X64FReg::xmm0);
    a.asm_and_r64_imm8(X64Reg::rax, 1);
    a.asm_movsd_r64_m64(X64FReg::xmm0, &base, nullptr, 1, 16); // load argument back into floating-point register
    a.asm_cmp_r64_imm8(X64Reg::rax, 1);
    a.asm_je_label("_print_float_int_part");

    {
        // the float to be printed is < 0, so print '-' symbol and
        // multiply the float with -1
        emit_print_64(a, "string_neg", 1);

        a.asm_mov_r64_imm64(X64Reg::rax, 1);
        a.asm_neg_r64(X64Reg::rax);
        a.asm_cvtsi2sd_r64_r64(X64FReg::xmm1, X64Reg::rax);
        a.asm_mulsd_r64_r64(X64FReg::xmm0, X64FReg::xmm1);
    }

    a.add_label("_print_float_int_part");
    a.asm_cvttsd2si_r64_r64(X64Reg::rax, X64FReg::xmm0);
    a.asm_push_r64(X64Reg::rax);

    // print the integral part
    {
        a.asm_call_label("print_i64");
        a.asm_add_r64_imm32(X64Reg::rsp, 8); // pop and increment stack pointer
    }

    // print dot
    emit_print_64(a, "string_dot", 1U);

    // print fractional part
    {
        a.asm_cvttsd2si_r64_r64(X64Reg::rax, X64FReg::xmm0); // rax now contains value int(xmm0)
        a.asm_cvtsi2sd_r64_r64(X64FReg::xmm1, X64Reg::rax);
        a.asm_subsd_r64_r64(X64FReg::xmm0, X64FReg::xmm1);
        a.asm_mov_r64_imm64(X64Reg::rax, 100000000); // to multiply by 10^8
        a.asm_cvtsi2sd_r64_r64(X64FReg::xmm1, X64Reg::rax);
        a.asm_mulsd_r64_r64(X64FReg::xmm0, X64FReg::xmm1);
        a.asm_cvttsd2si_r64_r64(X64Reg::rax, X64FReg::xmm0);

        a.asm_mov_r64_r64(X64Reg::r15, X64Reg::rax); // keep a safe copy in r15
        a.asm_mov_r64_imm64(X64Reg::r8, 8); // 8 digits after decimal point to be printed
        a.asm_mov_r64_imm64(X64Reg::r10, 10); // 10 as divisor

        // count the number of digits available in the fractional part
        a.add_label("_count_fract_part_digits_loop");
            a.asm_mov_r64_imm64(X64Reg::rdx, 0);
            a.asm_div_r64(X64Reg::r10);
            a.asm_dec_r64(X64Reg::r8);
            a.asm_cmp_r64_imm8(X64Reg::rax, 0);
            a.asm_je_label("_print_fract_part_initial_zeroes_loop_head");
            a.asm_jmp_label("_count_fract_part_digits_loop");

        a.add_label("_print_fract_part_initial_zeroes_loop_head");
            a.asm_mov_r64_imm64(X64Reg::rax, 48);
            a.asm_push_r64(X64Reg::rax); // push zero ascii value on stack top

        a.add_label("_print_fract_part_initial_zeroes_loop");
            a.asm_cmp_r64_imm8(X64Reg::r8, 0);
            a.asm_je_label("_print_fract_part");
            {
                // write() syscall
                a.asm_mov_r64_imm64(X64Reg::rax, 1);
                a.asm_mov_r64_imm64(X64Reg::rdi, 1);
                a.asm_mov_r64_r64(X64Reg::rsi, X64Reg::rsp);
                a.asm_mov_r64_imm64(X64Reg::rdx, 1);
                a.asm_syscall();
            }
            a.asm_dec_r64(X64Reg::r8);
            a.asm_jmp_label("_print_fract_part_initial_zeroes_loop");

        a.add_label("_print_fract_part");
            a.asm_pop_r64(X64Reg::rax); // pop the zero ascii value from stack top
            a.asm_push_r64(X64Reg::r15);
            // print the fractional part
            {
                a.asm_call_label("print_i64");
                a.asm_add_r64_imm32(X64Reg::rsp, 8); // pop and increment stack pointer
            }
    }

    // Restore stack
    a.asm_mov_r64_r64(X64Reg::rsp, X64Reg::rbp);
    a.asm_pop_r64(X64Reg::rbp);
    a.asm_ret();
}
} // namespace LFortran
