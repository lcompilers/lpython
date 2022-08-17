#include <iostream>
#include <memory>
#include <chrono>

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/codegen/asr_to_x86.h>
#include <libasr/codegen/x86_assembler.h>
#include <libasr/pass/do_loops.h>
#include <libasr/pass/global_stmts.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>


namespace LFortran {

namespace {

    // Local exception that is only used in this file to exit the visitor
    // pattern and caught later (not propagated outside)
    class CodeGenError
    {
    public:
        diag::Diagnostic d;
    public:
        CodeGenError(const std::string &msg)
            : d{diag::Diagnostic(msg, diag::Level::Error, diag::Stage::CodeGen)}
        { }
    };

}

using ASR::down_cast;
using ASR::is_a;

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

        for (auto &item : x.m_global_scope->get_scope()) {
            if (!is_a<ASR::Variable_t>(*item.second)) {
                visit_symbol(*item.second);
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
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
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
        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = down_cast<ASR::Variable_t>(item.second);

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

    void visit_Function(const ASR::Function_t &x) {
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
            ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(x.m_args[i]);
            LFORTRAN_ASSERT(LFortran::ASRUtils::is_arg_dummy(arg->m_intent));
            // TODO: we are assuming integer here:
            LFORTRAN_ASSERT(arg->m_type->type == ASR::ttypeType::Integer);
            Sym s;
            s.stack_offset = -(i*4+8); // TODO: reverse the sign of offset
            // We pass intent(in) as value, otherwise as pointer
            s.pointer = (arg->m_intent != ASR::intentType::In);
            uint32_t h = get_hash((ASR::asr_t*)arg);
            x86_symtab[h] = s;
        }

        // Initialize the stack
        m_a.asm_push_r32(X86Reg::ebp);
        m_a.asm_mov_r32_r32(X86Reg::ebp, X86Reg::esp);

        // Allocate stack space for local variables
        uint32_t total_offset = 0;
        for (auto &item : x.m_symtab->get_scope()) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = down_cast<ASR::Variable_t>(item.second);

                if (v->m_intent == LFortran::ASRUtils::intent_local || v->m_intent == LFortran::ASRUtils::intent_return_var) {
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

        // Leave return value in eax
        {
            ASR::Variable_t *retv = LFortran::ASRUtils::EXPR2VAR(x.m_return_var);

            uint32_t h = get_hash((ASR::asr_t*)retv);
            LFORTRAN_ASSERT(x86_symtab.find(h) != x86_symtab.end());
            Sym s = x86_symtab[h];
            X86Reg base = X86Reg::ebp;
            // mov eax, [ebp-s.stack_offset]
            m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, -s.stack_offset);
            LFORTRAN_ASSERT(!s.pointer);
        }

        // Restore stack
        m_a.asm_mov_r32_r32(X86Reg::esp, X86Reg::ebp);
        m_a.asm_pop_r32(X86Reg::ebp);
        m_a.asm_ret();
    }

    // Expressions leave integer values in eax

    void visit_IntegerConstant(const ASR::IntegerConstant_t &x) {
        m_a.asm_mov_r32_imm32(X86Reg::eax, x.m_n);
    }

    void visit_LogicalConstant(const ASR::LogicalConstant_t &x) {
        int val;
        if (x.m_value == true) {
            val = 1;
        } else {
            val = 0;
        }
        m_a.asm_mov_r32_imm32(X86Reg::eax, val);
    }

    void visit_Var(const ASR::Var_t &x) {
        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(x.m_v);
        uint32_t h = get_hash((ASR::asr_t*)v);
        LFORTRAN_ASSERT(x86_symtab.find(h) != x86_symtab.end());
        Sym s = x86_symtab[h];
        X86Reg base = X86Reg::ebp;
        // mov eax, [ebp-s.stack_offset]
        m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, -s.stack_offset);
        if (s.pointer) {
            base = X86Reg::eax;
            // Dereference a pointer
            // mov eax, [eax]
            m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, 0);
        }
    }

    void visit_IntegerBinOp(const ASR::IntegerBinOp_t &x) {
        this->visit_expr(*x.m_right);
        m_a.asm_push_r32(X86Reg::eax);
        this->visit_expr(*x.m_left);
        m_a.asm_pop_r32(X86Reg::ecx);
        // The left operand is in eax, the right operand is in ecx
        // Leave the result in eax.
        switch (x.m_op) {
            case ASR::binopType::Add: {
                m_a.asm_add_r32_r32(X86Reg::eax, X86Reg::ecx);
                break;
            };
            case ASR::binopType::Sub: {
                m_a.asm_sub_r32_r32(X86Reg::eax, X86Reg::ecx);
                break;
            };
            case ASR::binopType::Mul: {
                m_a.asm_mov_r32_imm32(X86Reg::edx, 0);
                m_a.asm_mul_r32(X86Reg::ecx);
                break;
            };
            case ASR::binopType::Div: {
                m_a.asm_mov_r32_imm32(X86Reg::edx, 0);
                m_a.asm_div_r32(X86Reg::ecx);
                break;
            };
            default: {
                throw CodeGenError("Binary operator '" + ASRUtils::binop_to_str_python(x.m_op) + "' not supported yet");
            }
        }
    }

    void visit_IntegerUnaryMinus(const ASR::IntegerUnaryMinus_t &x) {
        this->visit_expr(*x.m_arg);
        m_a.asm_neg_r32(X86Reg::eax);
    }

    void visit_IntegerCompare(const ASR::IntegerCompare_t &x) {
        std::string id = std::to_string(get_hash((ASR::asr_t*)&x));
        this->visit_expr(*x.m_right);
        m_a.asm_push_r32(X86Reg::eax);
        this->visit_expr(*x.m_left);
        m_a.asm_pop_r32(X86Reg::ecx);
        // The left operand is in eax, the right operand is in ecx
        // Leave the result in eax.
        m_a.asm_cmp_r32_r32(X86Reg::eax, X86Reg::ecx);
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
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        this->visit_expr(*x.m_value);
        // RHS is in eax

        ASR::Variable_t *v = LFortran::ASRUtils::EXPR2VAR(x.m_target);
        uint32_t h = get_hash((ASR::asr_t*)v);
        LFORTRAN_ASSERT(x86_symtab.find(h) != x86_symtab.end());
        Sym s = x86_symtab[h];
        X86Reg base = X86Reg::ebp;
        if (s.pointer) {
            // mov ecx, [ebp-s.stack_offset]
            m_a.asm_mov_r32_m32(X86Reg::ecx, &base, nullptr, 1, -s.stack_offset);
            // mov [ecx], eax
            base = X86Reg::ecx;
            m_a.asm_mov_m32_r32(&base, nullptr, 1, 0, X86Reg::eax);
        } else {
            // mov [ebp-s.stack_offset], eax
            m_a.asm_mov_m32_r32(&base, nullptr, 1, -s.stack_offset, X86Reg::eax);
        }
    }

    void visit_Print(const ASR::Print_t &x) {
        LFORTRAN_ASSERT(x.n_values == 1);
        ASR::expr_t *e = x.m_values[0];
        if (e->type == ASR::exprType::StringConstant) {
            ASR::StringConstant_t *s = down_cast<ASR::StringConstant_t>(e);
            std::string msg = s->m_s;
            msg += "\n";
            std::string id = "string" + std::to_string(get_hash((ASR::asr_t*)e));
            emit_print(m_a, id, msg.size());
            m_global_strings[id] = msg;
        } else {
            this->visit_expr(*e);
            ASR::ttype_t *t = LFortran::ASRUtils::expr_type(e);
            if (t->type == ASR::ttypeType::Integer) {
                m_a.asm_push_r32(X86Reg::eax);
                m_a.asm_call_label("print_int");
                m_a.asm_add_r32_imm8(LFortran::X86Reg::esp, 4);
            } else if (t->type == ASR::ttypeType::Real) {
                throw LCompilersException("Type not implemented");
            } else if (t->type == ASR::ttypeType::Character) {
                throw LCompilersException("Type not implemented");
            } else {
                throw LCompilersException("Type not implemented");
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
    template <typename T, typename T2>
    uint8_t push_call_args(const T &x, const T2 &sub) {
        LFORTRAN_ASSERT(sub.n_args == x.n_args);
        // Note: when counting down in a loop, we have to use signed ints
        // for `i`, so that it can become negative and fail the i>=0 condition.
        for (int i=x.n_args-1; i>=0; i--) {
            bool pass_as_pointer;
            {
                ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(sub.m_args[i]);
                LFORTRAN_ASSERT(LFortran::ASRUtils::is_arg_dummy(arg->m_intent));
                // TODO: we are assuming integer here:
                LFORTRAN_ASSERT(arg->m_type->type == ASR::ttypeType::Integer);
                uint32_t h = get_hash((ASR::asr_t*)arg);
                Sym &s = x86_symtab[h];
                pass_as_pointer = s.pointer;
            }
            if (x.m_args[i].m_value->type == ASR::exprType::Var) {
                ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(x.m_args[i].m_value);
                uint32_t h = get_hash((ASR::asr_t*)arg);
                LFORTRAN_ASSERT(x86_symtab.find(h) != x86_symtab.end());
                Sym s = x86_symtab[h];
                X86Reg base = X86Reg::ebp;
                if (s.pointer) {
                    if (pass_as_pointer) {
                        // Copy over the stack variable (already a pointer)
                        // mov eax, [ebp-s.stack_offset]
                        m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, -s.stack_offset);
                    } else {
                        // Copy and dereference the stack variable

                        // Copy
                        // mov eax, [ebp-s.stack_offset]
                        m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, -s.stack_offset);

                        // Dereference a pointer
                        // mov eax, [eax]
                        m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, 0);
                    }
                    m_a.asm_push_r32(X86Reg::eax);
                } else {
                    if (pass_as_pointer) {
                        // Get a pointer to the stack variable
                        // lea eax, [ebp-s.stack_offset]
                        m_a.asm_lea_r32_m32(X86Reg::eax, &base, nullptr, 1, -s.stack_offset);
                    } else {
                        // Copy over the stack variable
                        // mov eax, [ebp-s.stack_offset]
                        m_a.asm_mov_r32_m32(X86Reg::eax, &base, nullptr, 1, -s.stack_offset);
                    }
                    m_a.asm_push_r32(X86Reg::eax);
                }
            } else {
                LFORTRAN_ASSERT(!pass_as_pointer);
                this->visit_expr(*(x.m_args[i].m_value));
                // The value of the argument is in eax, push it onto the stack
                m_a.asm_push_r32(X86Reg::eax);
            }
        }
        return x.n_args*4;
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(x.m_name);

        uint32_t h = get_hash((ASR::asr_t*)s);
        if (x86_symtab.find(h) == x86_symtab.end()) {
            throw CodeGenError("Subroutine code not generated for '"
                + std::string(s->m_name) + "'");
        }
        Sym &sym = x86_symtab[h];
        // Push arguments to stack (last argument first)
        uint8_t arg_offset = push_call_args(x, *s);
        // Call the subroutine
        m_a.asm_call_label(sym.fn_label);
        // Remove arguments from stack
        m_a.asm_add_r32_imm8(LFortran::X86Reg::esp, arg_offset);
    }

    void visit_FunctionCall(const ASR::FunctionCall_t &x) {
        ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(x.m_name);

        uint32_t h = get_hash((ASR::asr_t*)s);
        if (x86_symtab.find(h) == x86_symtab.end()) {
            throw CodeGenError("Function code not generated for '"
                + std::string(s->m_name) + "'");
        }
        Sym &sym = x86_symtab[h];
        // Push arguments to stack (last argument first)
        uint8_t arg_offset = push_call_args(x, *s);
        // Call the function (the result is in eax, we leave it there)
        m_a.asm_call_label(sym.fn_label);
        // Remove arguments from stack
        m_a.asm_add_r32_imm8(LFortran::X86Reg::esp, arg_offset);
    }

};


Result<int> asr_to_x86(ASR::TranslationUnit_t &asr, Allocator &al,
        const std::string &filename, bool time_report)
{
    int time_pass_global=0;
    int time_pass_do_loops=0;
    int time_visit_asr=0;
    int time_verify=0;
    int time_save=0;

    ASRToX86Visitor v(al);

    LCompilers::PassOptions pass_options;
    pass_options.run_fun = "f";

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        pass_wrap_global_stmts_into_function(al, asr, pass_options);
        auto t2 = std::chrono::high_resolution_clock::now();
        time_pass_global = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    }

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        pass_replace_do_loops(al, asr, pass_options);
        auto t2 = std::chrono::high_resolution_clock::now();
        time_pass_do_loops = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    }

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        try {
            v.visit_asr((ASR::asr_t &)asr);
        } catch (const CodeGenError &e) {
            Error error;
            return error;
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        time_visit_asr = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    }

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        v.m_a.verify();
        auto t2 = std::chrono::high_resolution_clock::now();
        time_verify = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    }

    {
        auto t1 = std::chrono::high_resolution_clock::now();
        v.m_a.save_binary(filename);
        auto t2 = std::chrono::high_resolution_clock::now();
        time_save = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    }

    if (time_report) {
        std::cout << "Codegen Time report:" << std::endl;
        std::cout << "Global:     " << std::setw(5) << time_pass_global << std::endl;
        std::cout << "Do loops:   " << std::setw(5) << time_pass_do_loops << std::endl;
        std::cout << "ASR -> x86: " << std::setw(5) << time_visit_asr << std::endl;
        std::cout << "Verify:     " << std::setw(5) << time_verify << std::endl;
        std::cout << "Save:       " << std::setw(5) << time_save << std::endl;
        int total = time_pass_global + time_pass_do_loops + time_visit_asr + time_verify + time_verify + time_save;
        std::cout << "Total:      " << std::setw(5) << total << std::endl;
    }
    return 0;
}

} // namespace LFortran
