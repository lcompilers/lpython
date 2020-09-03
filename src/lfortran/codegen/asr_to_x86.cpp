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
        std::string fn_label; // Subroutine / Function assembly label
        bool pointer; // Is variable represented as a pointer (or value)
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

        // Add runtime library functions
        emit_print_int(m_a, "print_int");
        emit_exit(m_a, "exit", 0);
        emit_exit(m_a, "exit_error_stop", 1);

        // Generate code for nested subroutines and functions first:
        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::sub) {
                ASR::Subroutine_t *s = SUBROUTINE(item.second);
                visit_Subroutine(*s);
            }
            if (item.second->type == ASR::asrType::fn) {
                ASR::Function_t *s = FUNCTION(item.second);
                visit_Function(*s);
            }
        }

        // Generate code for the main program
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
                    s.pointer = false;
                    uint32_t h = get_hash((ASR::asr_t*)v);
                    x86_symtab[h] = s;
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

    void visit_Subroutine(const ASR::Subroutine_t &x) {
        uint32_t h = get_hash((ASR::asr_t*)&x);
        std::string id = std::to_string(h);

        // Generate code for the subroutine
        Sym s;
        s.stack_offset = 0;
        s.pointer = false;
        s.fn_label = x.m_name + id;
        x86_symtab[h] = s;
        m_a.add_label(s.fn_label);

        // Add arguments to x86_symtab with their correct offset
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = VARIABLE((ASR::asr_t*)EXPR_VAR((ASR::asr_t*)x.m_args[i])->m_v);
            LFORTRAN_ASSERT(is_arg_dummy(arg->m_intent));
            // TODO: we are assuming integer here:
            LFORTRAN_ASSERT(arg->m_type->type == ASR::ttypeType::Integer);
            // We pass all arguments as pointers for now
            Sym s;
            s.stack_offset = -(i*4+8); // TODO: reverse the sign of offset
            s.pointer = true;
            uint32_t h = get_hash((ASR::asr_t*)arg);
            x86_symtab[h] = s;
        }

        // Initialize the stack
        m_a.asm_push_r32(X86Reg::ebp);
        m_a.asm_mov_r32_r32(X86Reg::ebp, X86Reg::esp);

        // Allocate stack space for local variables
        uint32_t total_offset = 0;
        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::var) {
                ASR::var_t *v2 = (ASR::var_t*)(item.second);
                ASR::Variable_t *v = (ASR::Variable_t *)v2;

                if (v->m_intent == intent_local || v->m_intent == intent_return_var) {
                    if (v->m_type->type == ASR::ttypeType::Integer) {
                        total_offset += 4;
                        Sym s;
                        s.stack_offset = total_offset;
                        s.pointer = false;
                        uint32_t h = get_hash((ASR::asr_t*)v);
                        x86_symtab[h] = s;
                    } else {
                        throw CodeGenError("Variable type not supported");
                    }
                }
            }
        }
        m_a.asm_sub_r32_imm8(X86Reg::esp, total_offset);

        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }

        // Restore stack
        m_a.asm_mov_r32_r32(X86Reg::esp, X86Reg::ebp);
        m_a.asm_pop_r32(X86Reg::ebp);
        //m_a.asm_ret();
    }

    // Expressions leave integer values in eax

    void visit_Num(const ASR::Num_t &x) {
        m_a.asm_mov_r32_imm32(X86Reg::eax, x.m_n);
    }

    // TODO: rename to LogicalConstant
    void visit_Constant(const ASR::Constant_t &x) {
        int val;
        if (x.m_value == true) {
            val = 1;
        } else {
            val = 0;
        }
        m_a.asm_mov_r32_imm32(X86Reg::eax, val);
    }

    void visit_Var(const ASR::Var_t &x) {
        ASR::Variable_t *v = VARIABLE((ASR::asr_t*)(x.m_v));
        uint32_t h = get_hash((ASR::asr_t*)v);
        LFORTRAN_ASSERT(x86_symtab.find(h) != x86_symtab.end());
        Sym s = x86_symtab[h];
        X86Reg base = X86Reg::ebp;
        // mov eax, [ebp-s.stack_offset]
        m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, -s.stack_offset);
        if (s.pointer) {
            base = X86Reg::eax;
            // mov eax, [eax]
            m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, 0);
        }
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

    void visit_UnaryOp(const ASR::UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        if (x.m_type->type == ASR::ttypeType::Integer) {
            if (x.m_op == ASR::unaryopType::UAdd) {
                // eax = eax
                return;
            } else if (x.m_op == ASR::unaryopType::USub) {
                m_a.asm_neg_r32(X86Reg::eax);
                return;
            } else {
                throw CodeGenError("Unary type not implemented yet");
            }
        } else {
            throw CodeGenError("UnaryOp: type not supported yet");
        }
    }

    void visit_Compare(const ASR::Compare_t &x) {
        std::string id = std::to_string(get_hash((ASR::asr_t*)&x));
        this->visit_expr(*x.m_right);
        m_a.asm_push_r32(X86Reg::eax);
        this->visit_expr(*x.m_left);
        m_a.asm_pop_r32(X86Reg::ecx);
        // The left operand is in eax, the right operand is in ecx
        // Leave the result in eax.
        m_a.asm_cmp_r32_r32(X86Reg::eax, X86Reg::ecx);
        LFORTRAN_ASSERT(expr_type(x.m_left)->type == expr_type(x.m_right)->type);
        ASR::ttypeType optype = expr_type(x.m_left)->type;
        if (optype == ASR::ttypeType::Integer) {
            switch (x.m_op) {
                case (ASR::cmpopType::Eq) : {
                    m_a.asm_je_label(".compare1" + id);
                    break;
                }
                case (ASR::cmpopType::Gt) : {
                    m_a.asm_jg_label(".compare1" + id);
                    break;
                }
                case (ASR::cmpopType::GtE) : {
                    m_a.asm_jge_label(".compare1" + id);
                    break;
                }
                case (ASR::cmpopType::Lt) : {
                    m_a.asm_jl_label(".compare1" + id);
                    break;
                }
                case (ASR::cmpopType::LtE) : {
                    m_a.asm_jle_label(".compare1" + id);
                    break;
                }
                case (ASR::cmpopType::NotEq) : {
                    m_a.asm_jne_label(".compare1" + id);
                    break;
                }
                default : {
                    throw CodeGenError("Comparison operator not implemented");
                }
            }
            m_a.asm_mov_r32_imm32(X86Reg::eax, 0);
            m_a.asm_jmp_label(".compareend" + id);
            m_a.add_label(".compare1" + id);
            m_a.asm_mov_r32_imm32(X86Reg::eax, 1);
            m_a.add_label(".compareend" + id);
        } else {
            throw CodeGenError("Only Integer implemented in Compare");
        }
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        this->visit_expr(*x.m_value);
        // RHS is in eax

        ASR::var_t *t1 = EXPR_VAR((ASR::asr_t*)(x.m_target))->m_v;
        ASR::Variable_t *v = VARIABLE((ASR::asr_t*)t1);
        uint32_t h = get_hash((ASR::asr_t*)v);
        LFORTRAN_ASSERT(x86_symtab.find(h) != x86_symtab.end());
        Sym s = x86_symtab[h];
        X86Reg base = X86Reg::ebp;
        if (s.pointer) {
            // mov ecx, [ebp-s.stack_offset]
            m_a.asm_mov_r32_m32(X86Reg::ecx, &base, nullptr, 1, -s.stack_offset);
            // mov [ecx], eax
            base = X86Reg::ecx;
            m_a.asm_mov_m32_r32(&base, nullptr, 1, -s.stack_offset, X86Reg::eax);
        } else {
            // mov [ebp-s.stack_offset], eax
            m_a.asm_mov_m32_r32(&base, nullptr, 1, -s.stack_offset, X86Reg::eax);
        }
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

    void visit_ErrorStop(const ASR::ErrorStop_t &x) {
        std::string id = "err" + std::to_string(get_hash((ASR::asr_t*)&x));
        std::string msg = "ERROR STOP\n";
        emit_print(m_a, id, msg.size());
        m_global_strings[id] = msg;

        m_a.asm_call_label("exit_error_stop");
    }

    void visit_If(const ASR::If_t &x) {
        std::string id = std::to_string(get_hash((ASR::asr_t*)&x));
        this->visit_expr(*x.m_test);
        // eax contains the logical value (true=1, false=0) of the if condition
        m_a.asm_cmp_r32_imm8(LFortran::X86Reg::eax, 1);
        m_a.asm_je_label(".then" + id);
        m_a.asm_jmp_label(".else" + id);
        m_a.add_label(".then" + id);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }
        m_a.asm_jmp_label(".endif" +id);
        m_a.add_label(".else" + id);
        for (size_t i=0; i<x.n_orelse; i++) {
            this->visit_stmt(*x.m_orelse[i]);
        }
        m_a.add_label(".endif" + id);
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        std::string id = std::to_string(get_hash((ASR::asr_t*)&x));

        // head
        m_a.add_label(".loop.head" + id);
        this->visit_expr(*x.m_test);
        // eax contains the logical value (true=1, false=0) of the while condition
        m_a.asm_cmp_r32_imm8(LFortran::X86Reg::eax, 1);
        m_a.asm_je_label(".loop.body" + id);
        m_a.asm_jmp_label(".loop.end" + id);

        // body
        m_a.add_label(".loop.body" + id);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
        }
        m_a.asm_jmp_label(".loop.head" + id);

        // end
        m_a.add_label(".loop.end" + id);
    }

    // Push arguments to stack (last argument first)
    template <typename T>
    uint8_t push_call_args(const T &x) {
        // Note: when counting down in a loop, we have to use signed ints
        // for `i`, so that it can become negative and fail the i>=0 condition.
        for (int i=x.n_args-1; i>=0; i--) {
            if (x.m_args[i]->type == ASR::exprType::Var) {
                ASR::Variable_t *arg = VARIABLE((ASR::asr_t*)EXPR_VAR((ASR::asr_t*)x.m_args[i])->m_v);
                uint32_t h = get_hash((ASR::asr_t*)arg);
                LFORTRAN_ASSERT(x86_symtab.find(h) != x86_symtab.end());
                Sym s = x86_symtab[h];
                X86Reg base = X86Reg::ebp;
                if (s.pointer) {
                    throw CodeGenError("Not implemented yet.");
                } else {
                    // lea eax, [ebp-s.stack_offset]
                    m_a.asm_lea_r32_m32(X86Reg::eax, &base, nullptr, 1, -s.stack_offset);
                    m_a.asm_push_r32(X86Reg::eax);
                }
            } else {
                throw CodeGenError("Values not implemented yet.");
            }
        }
        return x.n_args*4;
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        ASR::Subroutine_t *s = SUBROUTINE((ASR::asr_t*)x.m_name);

        uint32_t h = get_hash((ASR::asr_t*)s);
        if (x86_symtab.find(h) == x86_symtab.end()) {
            throw CodeGenError("Subroutine code not generated for '"
                + std::string(s->m_name) + "'");
        }
        Sym &sym = x86_symtab[h];
        // Push arguments to stack (last argument first)
        uint8_t arg_offset = push_call_args(x);
        // Call the subroutine
        m_a.asm_call_label(sym.fn_label);
        // Remove arguments from stack
        m_a.asm_add_r32_imm8(LFortran::X86Reg::esp, arg_offset);
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
