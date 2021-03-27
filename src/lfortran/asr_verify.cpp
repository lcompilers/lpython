#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>


namespace LFortran {
namespace ASR {

class VerifyVisitor : public BaseWalkVisitor<VerifyVisitor>
{
public:
    void check(bool cond, const std::string &error_msg) {
        if (!cond) {
            throw LFortranException("ASR verify failed: " + error_msg);
        }
    }

    void visit_TranslationUnit(const TranslationUnit_t &x) {
        for (auto &a : x.m_global_scope->scope) {
            this->visit_symbol(*a.second);
        }
    }

    void visit_Var(const Var_t &x) {
        check(is_a<Variable_t>(*x.m_v),
            "Var_t::m_v does not point to a Variable_t");
        visit_symbol(*x.m_v);
    }

    void visit_Variable(const Variable_t &x) {
        SymbolTable *symtab = x.m_parent_symtab;
        check(symtab->scope.find(std::string(x.m_name)) != symtab->scope.end(),
            "Variable not found in parent_symtab symbol table");
        symbol_t *symtab_sym = symtab->scope[std::string(x.m_name)];
        const symbol_t *current_sym = &x.base;
        check(symtab_sym == current_sym,
            "Variable's parent symbol table does not point to it");

        if (x.m_value)
            visit_expr(*x.m_value);
        visit_ttype(*x.m_type);
    }

};


} // namespace ASR

bool asr_verify(const ASR::TranslationUnit_t &unit) {
    ASR::VerifyVisitor v;
    v.visit_TranslationUnit(unit);
    return true;
}

} // namespace LFortran
