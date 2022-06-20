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
    ASR::expr_t* result_var;

public:

    bool is_constructor_present, is_init_constructor;

    ClassConstructorVisitor(Allocator &al) : PassVisitor(al, nullptr),
    result_var(nullptr), is_constructor_present(false),
    is_init_constructor(false) {
        pass_result.reserve(al, 0);
    }

    void visit_Subroutine(const ASR::Subroutine_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Subroutine_t &xx = const_cast<ASR::Subroutine_t&>(x);
        current_scope = xx.m_symtab;
        for( auto item: current_scope->get_scope() ) {
            if( ASR::is_a<ASR::Variable_t>(*item.second) ) {
                ASR::Variable_t* variable = ASR::down_cast<ASR::Variable_t>(item.second);
                if( variable->m_symbolic_value ) {
                    result_var = ASRUtils::EXPR(ASR::make_Var_t(al, variable->base.base.loc,
                                                                item.second));
                    is_init_constructor = false;
                    this->visit_expr(*variable->m_symbolic_value);
                    if( is_init_constructor ) {
                        variable->m_symbolic_value = nullptr;
                    }
                }
            }
        }
        transform_stmts(xx.m_body, xx.n_body);
    }

    void visit_Assignment(const ASR::Assignment_t& x) {
        if( x.m_value->type == ASR::exprType::DerivedTypeConstructor ) {
            is_constructor_present = true;
            result_var = x.m_target;
            visit_expr(*x.m_value);
        }
    }

    void visit_DerivedTypeConstructor(const ASR::DerivedTypeConstructor_t &x) {
        is_init_constructor = true;
        if( x.n_args == 0 ) {
            remove_original_stmt = true;
        }
        ASR::Derived_t* dt_der = down_cast<ASR::Derived_t>(x.m_type);
        ASR::DerivedType_t* dt_dertype = ASR::down_cast<ASR::DerivedType_t>(
                                         LFortran::ASRUtils::symbol_get_past_external(dt_der->m_derived_type));
        for( size_t i = 0; i < std::min(dt_dertype->n_members, x.n_args); i++ ) {
            ASR::symbol_t* member = dt_dertype->m_symtab->resolve_symbol(std::string(dt_dertype->m_members[i], strlen(dt_dertype->m_members[i])));
            ASR::expr_t* derived_ref = LFortran::ASRUtils::EXPR(ASRUtils::getDerivedRef_t(al, x.base.base.loc, (ASR::asr_t*)result_var, member, current_scope));
            ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, derived_ref, x.m_args[i], nullptr));
            pass_result.push_back(al, assign);
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
