#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/class_constructor.h>

#include <cstring>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

class ClassConstructorVisitor : public PassUtils::PassVisitor<ClassConstructorVisitor>
{
private:
    Allocator &al;
    Vec<ASR::stmt_t*> class_constructor_result;
    ASR::expr_t* result_var;
    SymbolTable* current_scope;

public:

    bool is_constructor_present;

    ClassConstructorVisitor(Allocator &al) : al{al},
    result_var{nullptr}, is_constructor_present{false} {
        class_constructor_result.reserve(al, 0);
    }

    // TODO: Only Program and While is processed, we need to process all calls
    // to visit_stmt().

    void visit_Program(const ASR::Program_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Program_t &xx = const_cast<ASR::Program_t&>(x);
        current_scope = xx.m_symtab;
        transform_stmts(xx.m_body, xx.n_body, al, class_constructor_result);

        // Transform nested functions and subroutines
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Subroutine_t>(*item.second)) {
                ASR::Subroutine_t *s = down_cast<ASR::Subroutine_t>(item.second);
                visit_Subroutine(*s);
            }
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
            }
        }
    }

    void visit_Subroutine(const ASR::Subroutine_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Subroutine_t &xx = const_cast<ASR::Subroutine_t&>(x);
        current_scope = xx.m_symtab;
        transform_stmts(xx.m_body, xx.n_body, al, class_constructor_result);
    }

    void visit_Function(const ASR::Function_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
        current_scope = xx.m_symtab;
        transform_stmts(xx.m_body, xx.n_body, al, class_constructor_result);
    }

    void visit_Assignment(const ASR::Assignment_t& x) {
        if( x.m_value->type == ASR::exprType::DerivedTypeConstructor ) {
            is_constructor_present = true;
            result_var = x.m_target;
            visit_expr(*x.m_value);
        }
    }

    void visit_DerivedTypeConstructor(const ASR::DerivedTypeConstructor_t &x) {
        ASR::Derived_t* dt_der = down_cast<ASR::Derived_t>(x.m_type);
        ASR::DerivedType_t* dt_dertype = (ASR::DerivedType_t*)(&(
                                         LFortran::ASRUtils::symbol_get_past_external(dt_der->m_derived_type)->base));
        for( size_t i = 0; i < dt_dertype->n_members; i++ ) {
            ASR::symbol_t* member = dt_dertype->m_symtab->resolve_symbol(std::string(dt_dertype->m_members[i], strlen(dt_dertype->m_members[i])));
            ASR::expr_t* derived_ref = LFortran::ASRUtils::EXPR(ASRUtils::getDerivedRef_t(al, x.base.base.loc, (ASR::asr_t*)result_var, member, current_scope));
            ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, derived_ref, x.m_args[i], nullptr, false));
            class_constructor_result.push_back(al, assign);
        }
    }
};

void pass_replace_class_constructor(Allocator &al, ASR::TranslationUnit_t &unit) {
    ClassConstructorVisitor v(al);
    do {
        v.is_constructor_present = false;
        v.visit_TranslationUnit(unit);
    } while( v.is_constructor_present );
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
