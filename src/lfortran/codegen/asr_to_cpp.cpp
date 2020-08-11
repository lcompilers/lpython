#include <iostream>
#include <memory>

#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/codegen/asr_to_cpp.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>


namespace LFortran {

// Platform dependent fast unique hash:
uint64_t get_hash(ASR::asr_t *node)
{
    return (uint64_t)node;
}

struct SymbolInfo
{
    bool needs_declaration;
};

class ASRToCPPVisitor : public ASR::BaseVisitor<ASRToCPPVisitor>
{
public:
    std::map<uint64_t, SymbolInfo> sym_info;
    std::string src;
    int indentation_level;
    int indentation_spaces;

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LFORTRAN_ASSERT(x.n_items == 0);
        std::string unit_src = "";
        indentation_level = 0;
        indentation_spaces = 4;
        for (auto &item : x.m_global_scope->scope) {
            visit_asr(*item.second);
            unit_src += src;
        }
        src = unit_src;
    }

    void visit_Program(const ASR::Program_t &x) {
        indentation_level += 1;
        std::string decl;
        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::var) {
                ASR::var_t *v2 = (ASR::var_t*)(item.second);
                ASR::Variable_t *v = (ASR::Variable_t *)v2;
                std::string indent(indentation_level*indentation_spaces, ' ');
                decl += indent;

                if (v->m_type->type == ASR::ttypeType::Integer) {
                    decl += "int " + std::string(v->m_name) + ";\n";
                } else if (v->m_type->type == ASR::ttypeType::Logical) {
                    decl += "bool " + std::string(v->m_name) + ";\n";
                } else {
                    throw CodeGenError("Variable type not supported");
                }
            }
        }

        std::string body;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            body += src;
        }

        std::string headers = "#include <iostream>\n\n";

        src = headers + "int main()\n{\n" + decl + body + "    return 0;\n}\n";
        indentation_level -= 1;
    }

    void visit_Subroutine(const ASR::Subroutine_t &x) {
        indentation_level += 1;
        std::string sub = "void " + std::string(x.m_name) + "(";
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = VARIABLE((ASR::asr_t*)EXPR_VAR((ASR::asr_t*)x.m_args[i])->m_v);
            LFORTRAN_ASSERT(is_arg_dummy(arg->m_intent));
            if (arg->m_type->type == ASR::ttypeType::Integer) {
                if (arg->m_intent == intent_in) {
                    sub += "int " + std::string(arg->m_name);
                } else if (arg->m_intent == intent_out || arg->m_intent == intent_inout) {
                    sub += "int &" + std::string(arg->m_name);
                } else {
                    LFORTRAN_ASSERT(false);
                }
            } else if (arg->m_type->type == ASR::ttypeType::Real) {
                if (arg->m_intent == intent_in) {
                    sub += "float " + std::string(arg->m_name);
                } else if (arg->m_intent == intent_out || arg->m_intent == intent_inout) {
                    sub += "float &" + std::string(arg->m_name);
                } else {
                    LFORTRAN_ASSERT(false);
                }
            } else {
                throw CodeGenError("Type not supported yet.");
            }
            if (i < x.n_args-1) sub += ", ";
        }
        sub += ")\n";

        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::var) {
                ASR::var_t *v2 = (ASR::var_t*)(item.second);
                ASR::Variable_t *v = (ASR::Variable_t *)v2;
                if (v->m_intent == intent_local) {
                    SymbolInfo s;
                    s.needs_declaration = true;
                    sym_info[get_hash((ASR::asr_t*)v)] = s;
                }
            }
        }

        std::string body;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            body += src;
        }

        std::string decl;
        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::var) {
                ASR::var_t *v2 = (ASR::var_t*)(item.second);
                ASR::Variable_t *v = (ASR::Variable_t *)v2;
                if (v->m_intent == intent_local) {
                    if (sym_info[get_hash((ASR::asr_t*) v)].needs_declaration) {
                        std::string indent(indentation_level*indentation_spaces, ' ');
                        decl += indent;
                        if (v->m_type->type == ASR::ttypeType::Integer) {
                            decl += "int " + std::string(v->m_name) + ";\n";
                        } else if (v->m_type->type == ASR::ttypeType::Logical) {
                            decl += "bool " + std::string(v->m_name) + ";\n";
                        } else {
                            throw CodeGenError("Variable type not supported");
                        }
                    }
                }
            }
        }

        sub += "{\n" + decl + body + "}\n";
        src = sub;
        indentation_level -= 1;
    }

    void visit_Function(const ASR::Function_t &x) {
        std::string sub = "int " + std::string(x.m_name) + "(";
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = VARIABLE((ASR::asr_t*)EXPR_VAR((ASR::asr_t*)x.m_args[i])->m_v);
            LFORTRAN_ASSERT(is_arg_dummy(arg->m_intent));
            if (arg->m_type->type == ASR::ttypeType::Integer) {
                if (arg->m_intent == intent_in) {
                    sub += "int " + std::string(arg->m_name);
                } else if (arg->m_intent == intent_out || arg->m_intent == intent_inout) {
                    sub += "int &" + std::string(arg->m_name);
                } else {
                    LFORTRAN_ASSERT(false);
                }
            } else if (arg->m_type->type == ASR::ttypeType::Real) {
                if (arg->m_intent == intent_in) {
                    sub += "float " + std::string(arg->m_name);
                } else if (arg->m_intent == intent_out || arg->m_intent == intent_inout) {
                    sub += "float &" + std::string(arg->m_name);
                } else {
                    LFORTRAN_ASSERT(false);
                }
            } else {
                throw CodeGenError("Type not supported yet.");
            }
            if (i < x.n_args-1) sub += ", ";
        }
        sub += ")\n";

        std::string decl;
        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::var) {
                ASR::var_t *v2 = (ASR::var_t*)(item.second);
                ASR::Variable_t *v = (ASR::Variable_t *)v2;
                if (v->m_intent == intent_local) {
                    if (v->m_type->type == ASR::ttypeType::Integer) {
                        decl += "    int " + std::string(v->m_name) + ";\n";
                    } else if (v->m_type->type == ASR::ttypeType::Logical) {
                        decl += "    bool " + std::string(v->m_name) + ";\n";
                    } else {
                        throw CodeGenError("Variable type not supported");
                    }
                }
            }
        }

        std::string body;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            body += "    " + src;
        }

        if (decl.size() > 0 || body.size() > 0) {
            sub += "{\n" + decl + body + "}\n";
        } else {
            sub[sub.size()-1] = ';';
            sub += "\n";
        }
        src = sub;
    }

    void visit_FuncCall(const ASR::FuncCall_t &x) {
        src = std::string(FUNCTION((ASR::asr_t*)x.m_func)->m_name) + "()";
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        std::string target;
        if (x.m_target->type == ASR::exprType::Var) {
            ASR::var_t *t1 = EXPR_VAR((ASR::asr_t*)(x.m_target))->m_v;
            target = VARIABLE((ASR::asr_t*)t1)->m_name;
        } else if (x.m_target->type == ASR::exprType::ArrayRef) {
            visit_ArrayRef(*(ASR::ArrayRef_t*)x.m_target);
            target = src;
        } else {
            LFORTRAN_ASSERT(false)
        }
        this->visit_expr(*x.m_value);
        std::string value = src;
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + target + " = " + value + ";\n";
    }

    void visit_Num(const ASR::Num_t &x) {
        src = std::to_string(x.m_n);
    }

    void visit_Var(const ASR::Var_t &x) {
        src = VARIABLE((ASR::asr_t*)(x.m_v))->m_name;
    }

    void visit_ArrayRef(const ASR::ArrayRef_t &x) {
        std::string out = VARIABLE((ASR::asr_t*)(x.m_v))->m_name;
        out += "[";
        for (size_t i=0; i<x.n_args; i++) {
            visit_expr(*x.m_args[i].m_right);
            out += src;
            if (i < x.n_args-1) out += ",";
        }
        out += "]";
        src = out;
    }

    void visit_BinOp(const ASR::BinOp_t &x) {
        this->visit_expr(*x.m_left);
        std::string left_val = src;
        this->visit_expr(*x.m_right);
        std::string right_val = src;
        switch (x.m_op) {
            case ASR::operatorType::Add: {
                src = left_val + " + " + right_val;
                break;
            }
            case ASR::operatorType::Sub: {
                src = left_val + " - " + right_val;
                break;
            }
            case ASR::operatorType::Mul: {
                src = left_val + "*" + right_val;
                break;
            }
            case ASR::operatorType::Div: {
                src = left_val + "/" + right_val;
                break;
            }
            case ASR::operatorType::Pow: {
                src = "std::pow(" + left_val + ", " + right_val + ")";
                break;
            }
            default : throw CodeGenError("Unhandled switch case");
        }
    }


    void visit_Print(const ASR::Print_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "std::cout ";
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
            out += "<< " + src + " ";
        }
        out += "<< std::endl;\n";
        src = out;
    }

    void visit_DoConcurrentLoop(const ASR::DoConcurrentLoop_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "Kokkos::parallel_for(";
        visit_expr(*x.m_head.m_end);
        out += src;
        ASR::Variable_t *loop_var = VARIABLE((ASR::asr_t*)EXPR_VAR((ASR::asr_t*)x.m_head.m_v)->m_v);
        sym_info[get_hash((ASR::asr_t*) loop_var)].needs_declaration = false;
        out += ", KOKKOS_LAMBDA(const long " + std::string(loop_var->m_name)
                + ") {\n";
        indentation_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            out += src;
        }
        out += indent + "});\n";
        indentation_level -= 1;
        src = out;
    }

};

std::string asr_to_cpp(ASR::asr_t &asr)
{
    ASRToCPPVisitor v;
    LFORTRAN_ASSERT(asr.type == ASR::asrType::unit);
    v.visit_asr(asr);
    return v.src;
}

} // namespace LFortran
