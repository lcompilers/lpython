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

        src = "int main()\n{\n" + decl + "    return 0;\n}";
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
