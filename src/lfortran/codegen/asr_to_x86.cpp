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
    struct Sym {
        uint32_t stack_offset; // The local variable is [ebp-stack_offset]
    };
public:
    Allocator &m_al;
    X86Assembler m_a;
    std::map<std::string,std::string> m_global_strings;
    std::map<uint64_t, Sym> x86_symtab;
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

        emit_print_int(m_a, "print_int");
        emit_exit(m_a, "exit");

        m_a.add_label("_start");

        // Initialize the stack
        m_a.asm_push_r32(X86Reg::ebp);
        m_a.asm_mov_r32_r32(X86Reg::ebp, X86Reg::esp);

        // Allocate stack space for local variables
        uint32_t total_offset = 0;
        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::var) {
                ASR::var_t *v2 = (ASR::var_t*)(item.second);
                ASR::Variable_t *v = (ASR::Variable_t *)v2;

                if (v->m_type->type == ASR::ttypeType::Integer) {
                    total_offset += 4;
                    Sym s;
                    s.stack_offset = total_offset;
                    x86_symtab[get_hash((ASR::asr_t*)v)] = s;
                } else {
                    throw CodeGenError("Variable type not supported");
                }
            }
        }
        m_a.asm_sub_r32_imm8(X86Reg::esp, total_offset);

        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }

        m_a.asm_call_label("exit");

        // Restore stack
        m_a.asm_mov_r32_r32(X86Reg::esp, X86Reg::ebp);
        m_a.asm_pop_r32(X86Reg::ebp);
        //m_a.asm_ret();

        for (auto &s : m_global_strings) {
            emit_data_string(m_a, s.first, s.second);
        }

        emit_elf32_footer(m_a);
    }

    // Expressions leave integer values in eax

    void visit_Num(const ASR::Num_t &x) {
        m_a.asm_mov_r32_imm32(X86Reg::eax, x.m_n);
    }

    void visit_Var(const ASR::Var_t &x) {
        ASR::Variable_t *v = VARIABLE((ASR::asr_t*)(x.m_v));
        uint32_t h = get_hash((ASR::asr_t*)v);
        LFORTRAN_ASSERT(x86_symtab.find(h) != x86_symtab.end());
        Sym s = x86_symtab[h];
        X86Reg base = X86Reg::ebp;
        // mov eax, [ebp-s.stack_offset]
        m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, -s.stack_offset);
    }

    void visit_BinOp(const ASR::BinOp_t &x) {
        this->visit_expr(*x.m_right);
        m_a.asm_push_r32(X86Reg::eax);
        this->visit_expr(*x.m_left);
        m_a.asm_pop_r32(X86Reg::ecx);
        // The left operand is in eax, the right operand is in ecx
        // Leave the result in eax.
        if (x.m_type->type == ASR::ttypeType::Integer) {
            switch (x.m_op) {
                case ASR::operatorType::Add: {
                    m_a.asm_add_r32_r32(X86Reg::eax, X86Reg::ecx);
                    break;
                };
                case ASR::operatorType::Sub: {
                    m_a.asm_sub_r32_r32(X86Reg::eax, X86Reg::ecx);
                    break;
                };
                case ASR::operatorType::Mul: {
                    m_a.asm_mov_r32_imm32(X86Reg::edx, 0);
                    m_a.asm_mul_r32(X86Reg::ecx);
                    break;
                };
                case ASR::operatorType::Div: {
                    m_a.asm_mov_r32_imm32(X86Reg::edx, 0);
                    m_a.asm_div_r32(X86Reg::ecx);
                    break;
                };
                case ASR::operatorType::Pow: {
                    throw CodeGenError("Pow not implemented yet.");
                    break;
                };
            }
        } else {
            throw CodeGenError("Binop: Only Integer types implemented so far");
        }
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        //ASR::var_t *t1 = EXPR_VAR((ASR::asr_t*)(x.m_target))->m_v;
        // Lookup where the variable lives
        //target= x86_symtab[std::string(VARIABLE((ASR::asr_t*)t1)->m_name)];
        // We assume [ebp-4] for now
        this->visit_expr(*x.m_value);
        // RHS is in eax
        X86Reg base = X86Reg::ebp;
        m_a.asm_mov_m32_r32(&base, nullptr, 1, -4, X86Reg::eax);
    }

    void visit_Print(const ASR::Print_t &x) {
        LFORTRAN_ASSERT(x.n_values == 1);
        ASR::expr_t *e = x.m_values[0];
        if (e->type == ASR::exprType::Str) {
            ASR::Str_t *s = EXPR_STR((ASR::asr_t*)e);
            std::string msg = s->m_s;
            msg += "\n";
            std::string id = "string" + std::to_string(get_hash((ASR::asr_t*)e));
            emit_print(m_a, id, msg.size());
            m_global_strings[id] = msg;
        } else {
            this->visit_expr(*e);
            ASR::ttype_t *t = expr_type(e);
            if (t->type == ASR::ttypeType::Integer) {
                m_a.asm_push_r32(X86Reg::eax);
                m_a.asm_call_label("print_int");
                m_a.asm_add_r32_imm8(LFortran::X86Reg::esp, 4);
            } else if (t->type == ASR::ttypeType::Real) {
                throw LFortranException("Type not implemented");
            } else if (t->type == ASR::ttypeType::Character) {
                throw LFortranException("Type not implemented");
            } else {
                throw LFortranException("Type not implemented");
            }


            std::string msg = "\n";
            std::string id = "string" + std::to_string(get_hash((ASR::asr_t*)e));
            emit_print(m_a, id, msg.size());
            m_global_strings[id] = msg;
        }
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
