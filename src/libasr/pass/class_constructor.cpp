#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/class_constructor.h>

#include <cstring>


namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

class ClassConstructorVisitor : public PassUtils::PassVisitor<ClassConstructorVisitor>
{
private:
    ASR::expr_t* result_var;

public:

    bool is_constructor_present;

    ClassConstructorVisitor(Allocator &al) : PassVisitor(al, nullptr),
    result_var(nullptr), is_constructor_present(false) {
        pass_result.reserve(al, 0);
    }

    void visit_Assignment(const ASR::Assignment_t& x) {
        ASR::Assignment_t& xx = const_cast<ASR::Assignment_t&>(x);
        if( x.m_value->type == ASR::exprType::StructTypeConstructor ) {
            is_constructor_present = true;
            if( x.m_overloaded == nullptr ) {
                result_var = x.m_target;
                visit_expr(*x.m_value);
            } else {
                std::string result_var_name = current_scope->get_unique_name("temp_struct_var__");
                result_var = PassUtils::create_auxiliary_variable(x.m_value->base.loc, result_var_name,
                                al, current_scope, ASRUtils::expr_type(x.m_target));
                visit_expr(*x.m_value);
                ASR::stmt_t* x_m_overloaded = x.m_overloaded;
                if( ASR::is_a<ASR::SubroutineCall_t>(*x.m_overloaded) ) {
                    ASR::SubroutineCall_t* assign_call = ASR::down_cast<ASR::SubroutineCall_t>(xx.m_overloaded);
                    Vec<ASR::call_arg_t> assign_call_args;
                    assign_call_args.reserve(al, 2);
                    assign_call_args.push_back(al, assign_call->m_args[0]);
                    ASR::call_arg_t arg_1;
                    arg_1.loc = assign_call->m_args[1].loc;
                    arg_1.m_value = result_var;
                    assign_call_args.push_back(al, arg_1);
                    x_m_overloaded = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, x_m_overloaded->base.loc,
                                        assign_call->m_name, assign_call->m_original_name, assign_call_args.p,
                                        assign_call_args.size(), assign_call->m_dt));
                }
                pass_result.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, x.m_target,
                                                result_var, x_m_overloaded)));
            }
        }

    }

    void visit_StructTypeConstructor(const ASR::StructTypeConstructor_t &x) {
        if( x.n_args == 0 ) {
            remove_original_stmt = true;
        }
        ASR::Struct_t* dt_der = down_cast<ASR::Struct_t>(x.m_type);
        ASR::StructType_t* dt_dertype = ASR::down_cast<ASR::StructType_t>(
                                         ASRUtils::symbol_get_past_external(dt_der->m_derived_type));
        for( size_t i = 0; i < std::min(dt_dertype->n_members, x.n_args); i++ ) {
            ASR::symbol_t* member = dt_dertype->m_symtab->resolve_symbol(std::string(dt_dertype->m_members[i], strlen(dt_dertype->m_members[i])));
            ASR::expr_t* derived_ref = ASRUtils::EXPR(ASRUtils::getStructInstanceMember_t(al, x.base.base.loc, (ASR::asr_t*)result_var, member, current_scope));
            ASR::stmt_t* assign = ASRUtils::STMT(ASR::make_Assignment_t(al, x.base.base.loc, derived_ref, x.m_args[i], nullptr));
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


} // namespace LCompilers
