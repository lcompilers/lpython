#include <iostream>
#include <memory>

#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/codegen/asr_to_cpp.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>


namespace LFortran {

class ASRToCPPVisitor : public ASR::BaseVisitor<ASRToCPPVisitor>
{
public:
    std::string src;

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LFORTRAN_ASSERT(x.n_items == 0);
        for (auto &item : x.m_global_scope->scope) {
            visit_asr(*item.second);
        }
    }

    void visit_Program(const ASR::Program_t &x) {
        std::string decl;
        for (auto &item : x.m_symtab->scope) {
            if (item.second->type == ASR::asrType::var) {
                ASR::var_t *v2 = (ASR::var_t*)(item.second);
                ASR::Variable_t *v = (ASR::Variable_t *)v2;

                if (v->m_type->type == ASR::ttypeType::Integer) {
                    decl += "    int " + std::string(v->m_name) + ";\n";
                } else if (v->m_type->type == ASR::ttypeType::Logical) {
                    decl += "    bool " + std::string(v->m_name) + ";\n";
                } else {
                    throw CodeGenError("Variable type not supported");
                }
            }
        }

        std::string body;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            body += "    " + src;
        }

        std::string headers = "#include <iostream>\n\n";

        src = headers + "int main()\n{\n" + decl + body + "    return 0;\n}";
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        ASR::var_t *t1 = EXPR_VAR((ASR::asr_t*)(x.m_target))->m_v;
        std::string target = VARIABLE((ASR::asr_t*)t1)->m_name;
        this->visit_expr(*x.m_value);
        std::string value = src;
        src = target + " = " + value + ";\n";
    }

    void visit_Num(const ASR::Num_t &x) {
        src = std::to_string(x.m_n);
    }

    void visit_Var(const ASR::Var_t &x) {
        src = VARIABLE((ASR::asr_t*)(x.m_v))->m_name;
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
        std::string out = "std::cout ";
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
            out += "<< " + src + " ";
        }
        out += "<< std::endl;\n";
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
