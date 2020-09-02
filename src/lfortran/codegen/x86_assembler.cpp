#ifdef __unix__
#define LFORTRAN_LINUX
#endif

#ifdef LFORTRAN_LINUX
#include <sys/stat.h>
#endif

#include <lfortran/codegen/x86_assembler.h>

namespace LFortran {

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

void emit_elf32_header(X86Assembler &a) {
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
    a.asm_dd_label("e_entry");  // e_entry
    a.asm_dd_label("e_phoff");  // e_phoff
    a.asm_dd_imm32(0);  // e_shoff
    a.asm_dd_imm32(0);  // e_flags
    a.asm_dw_label("ehdrsize");  // e_ehsize
    a.asm_dw_label("phdrsize");  // e_phentsize
    a.asm_dw_imm16(1);  // e_phnum
    a.asm_dw_imm16(0);  // e_shentsize
    a.asm_dw_imm16(0);  // e_shnum
    a.asm_dw_imm16(0);  // e_shstrndx

    a.add_var("ehdrsize", a.pos()-a.get_defined_symbol("ehdr").value);

    /* Elf32_Phdr */
    a.add_label("phdr");
    a.asm_dd_imm32(1);        // p_type
    a.asm_dd_imm32(0);        // p_offset
    a.asm_dd_imm32(a.origin());   // p_vaddr
    a.asm_dd_imm32(a.origin());   // p_paddr
    a.asm_dd_label("filesize"); // p_filesz
    a.asm_dd_label("filesize"); // p_memsz
    a.asm_dd_imm32(5);        // p_flags
    a.asm_dd_imm32(0x1000);   // p_align

    a.add_var("phdrsize", a.pos()-a.get_defined_symbol("phdr").value);
    a.add_var("e_phoff", a.get_defined_symbol("phdr").value-a.origin());
}

void emit_elf32_footer(X86Assembler &a) {
    a.add_var("e_entry", a.get_defined_symbol("_start").value);
    a.add_var("filesize", a.pos()-a.origin());
}

void emit_exit(X86Assembler &a, const std::string &name)
{
    a.add_label(name);
    // void exit(int status);
    a.asm_mov_r32_imm32(LFortran::X86Reg::eax, 1); // sys_exit
    a.asm_mov_r32_imm32(LFortran::X86Reg::ebx, 0); // exit code
    a.asm_int_imm8(0x80); // syscall
}

void emit_data_string(X86Assembler &a, const std::string &label,
    const std::string &s)
{
    a.add_label(label);
    a.asm_db_imm8(s.c_str(), s.size());
}

void emit_print(X86Assembler &a, const std::string &msg_label,
    uint32_t size)
{
    // ssize_t write(int fd, const void *buf, size_t count);
    a.asm_mov_r32_imm32(LFortran::X86Reg::eax, 4); // sys_write
    a.asm_mov_r32_imm32(LFortran::X86Reg::ebx, 1); // fd (stdout)
    a.asm_mov_r32_label(LFortran::X86Reg::ecx, msg_label); // buf
    a.asm_mov_r32_imm32(LFortran::X86Reg::edx, size); // count
    a.asm_int_imm8(0x80);
}

} // namespace LFortran
