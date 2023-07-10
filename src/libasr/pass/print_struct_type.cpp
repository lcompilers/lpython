#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/replace_print_arr.h>


namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

class PrintStructTypeVisitor : public PassUtils::PassVisitor<PrintStructTypeVisitor>
{

private:

    std::string rl_path;

public:

    PrintStructTypeVisitor(Allocator &al_) : PassVisitor(al_, nullptr) {
        pass_result.reserve(al, 1);
    }

    void print_struct_type(ASR::expr_t* obj,
        ASR::StructType_t* struct_type_t,
        Vec<ASR::expr_t*>& new_values) {
        if( struct_type_t->m_parent ) {
            ASR::symbol_t* parent = ASRUtils::symbol_get_past_external(struct_type_t->m_parent);
            if( ASR::is_a<ASR::StructType_t>(*parent) ) {
                ASR::StructType_t* parent_struct_type_t = ASR::down_cast<ASR::StructType_t>(parent);
                print_struct_type(obj, parent_struct_type_t, new_values);
            } else {
                LCOMPILERS_ASSERT(false);
            }
        }

        ASR::symbol_t* obj_v = nullptr;
        if( ASR::is_a<ASR::Var_t>(*obj) ) {
            obj_v = ASR::down_cast<ASR::Var_t>(obj)->m_v;
        }
        for( size_t i = 0; i < struct_type_t->n_members; i++ ) {
            ASR::symbol_t* member = struct_type_t->m_symtab->resolve_symbol(struct_type_t->m_members[i]);
            new_values.push_back(al, ASRUtils::EXPR(ASRUtils::getStructInstanceMember_t(
                    al, struct_type_t->base.base.loc,
                (ASR::asr_t*) obj, obj_v, member, current_scope)));
        }
    }

    void visit_Print(const ASR::Print_t& x) {
        #define is_struct_type(value) if( ASR::is_a<ASR::Struct_t>(    \
            *ASRUtils::expr_type(value)) )    \

        bool is_struct_type_present = false;
        for( size_t i = 0; i < x.n_values; i++ ) {
            is_struct_type(x.m_values[i])
            {

            is_struct_type_present = true;
            break;

            }
        }
        if( !is_struct_type_present ) {
            return ;
        }

        /*
            TODO: Respect fmt.
        */
        Vec<ASR::expr_t*> new_values;
        new_values.reserve(al, 1);
        for( size_t i = 0; i < x.n_values; i++ ) {
            ASR::expr_t* x_m_value = x.m_values[i];
            if( ASR::is_a<ASR::OverloadedUnaryMinus_t>(*x_m_value) ) {
                x_m_value = ASR::down_cast<ASR::OverloadedUnaryMinus_t>(x_m_value)->m_overloaded;
            }
            is_struct_type(x_m_value)
            {

            ASR::Struct_t* struct_t = ASR::down_cast<ASR::Struct_t>(ASRUtils::expr_type(x_m_value));
            ASR::symbol_t* struct_t_sym = ASRUtils::symbol_get_past_external(struct_t->m_derived_type);
            if( ASR::is_a<ASR::StructType_t>(*struct_t_sym) ) {
                ASR::StructType_t* struct_type_t = ASR::down_cast<ASR::StructType_t>(struct_t_sym);
                print_struct_type(x_m_value, struct_type_t, new_values);
            } else {
                LCOMPILERS_ASSERT(false);
            }

            } else {

            new_values.push_back(al, x.m_values[i]);

            }
        }

        ASR::Print_t& xx = const_cast<ASR::Print_t&>(x);
        xx.m_values = new_values.p;
        xx.n_values = new_values.size();
    }

};

void pass_replace_print_struct_type(
    Allocator &al, ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions& /*pass_options*/) {
    PrintStructTypeVisitor v(al);
    v.visit_TranslationUnit(unit);
}


} // namespace LCompilers
