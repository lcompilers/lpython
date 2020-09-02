#include <iostream>
#include <memory>

#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/codegen/asr_to_x86.h>
#include <lfortran/codegen/x86_assembler.h>
#include <lfortran/pass/do_loops.h>
#include <lfortran/pass/global_stmts.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>


namespace LFortran {

// Platform dependent fast unique hash:
uint64_t static get_hash(ASR::asr_t *node)
{
    return (uint64_t)node;
}

class ASRToX86Visitor : public ASR::BaseVisitor<ASRToX86Visitor>
{
public:
    Allocator &m_al;
    X86Assembler m_a;
    std::map<std::string,std::string> m_global_strings;
public:

    ASRToX86Visitor(Allocator &al) : m_al{al}, m_a{al} {}

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LFORTRAN_ASSERT(x.n_items == 0);

        for (auto &item : x.m_global_scope->scope) {
            if (item.second->type != ASR::asrType::var) {
                visit_asr(*item.second);
            }
        }
    }

    void visit_Program(const ASR::Program_t &x) {

        emit_elf32_header(m_a);

        emit_exit(m_a, "exit");

        m_a.add_label("_start");

        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }

        m_a.asm_call_label("exit");

        for (auto &s : m_global_strings) {
            emit_data_string(m_a, s.first, s.second);
        }

        emit_elf32_footer(m_a);
    }

    void visit_Print(const ASR::Print_t &x) {
        LFORTRAN_ASSERT(x.n_values == 1);
        ASR::expr_t *e = x.m_values[0];
        ASR::Str_t *s = EXPR_STR((ASR::asr_t*)e);
        std::string msg = s->m_s;
        msg += "\n";
        std::string id = "string" + std::to_string(get_hash((ASR::asr_t*)e));
        emit_print(m_a, id, msg.size());
        m_global_strings[id] = msg;
    }


};


void asr_to_x86(ASR::TranslationUnit_t &asr, Allocator &al,
        const std::string &filename)
{
    ASRToX86Visitor v(al);
    pass_wrap_global_stmts_into_function(al, asr, "f");
    pass_replace_do_loops(al, asr);
    v.visit_asr((ASR::asr_t&)asr);

    v.m_a.verify();

    v.m_a.save_binary(filename);
}

} // namespace LFortran
