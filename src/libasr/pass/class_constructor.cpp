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

    void visit_Assignment(const ASR::Assignment_t& x) {
        if( x.m_value->type == ASR::exprType::StructTypeConstructor ) {
            is_constructor_present = true;
            result_var = x.m_target;
            visit_expr(*x.m_value);
        }
    }

    void visit_StructTypeConstructor(const ASR::StructTypeConstructor_t &x) {
        is_init_constructor = true;
        if( x.n_args == 0 ) {
            remove_original_stmt = true;
        }
        ASR::Struct_t* dt_der = down_cast<ASR::Struct_t>(x.m_type);
        ASR::StructType_t* dt_dertype = ASR::down_cast<ASR::StructType_t>(
                                         LFortran::ASRUtils::symbol_get_past_external(dt_der->m_derived_type));
        for( size_t i = 0; i < std::min(dt_dertype->n_members, x.n_args); i++ ) {
            ASR::symbol_t* member = dt_dertype->m_symtab->resolve_symbol(std::string(dt_dertype->m_members[i], strlen(dt_dertype->m_members[i])));
            ASR::expr_t* derived_ref = LFortran::ASRUtils::EXPR(ASRUtils::getStructInstanceMember_t(al, x.base.base.loc, (ASR::asr_t*)result_var, member, current_scope));
            ASR::stmt_t* assign = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, derived_ref, x.m_args[i], nullptr));
            pass_result.push_back(al, assign);
        }
    }
};

void pass_replace_class_constructor(Allocator &al, ASR::TranslationUnit_t &unit,
                                    const LCompilers::PassOptions& /*pass_options*/) {
    ClassConstructorVisitor v(al);
    do {
        v.is_constructor_present = false;
        v.visit_TranslationUnit(unit);
    } while( v.is_constructor_present );
}


} // namespace LFortran
