#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/replace_implied_do_loops.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/intrinsic_function_registry.h>

#include <vector>
#include <utility>

namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

class ReplaceArrayConstant: public ASR::BaseExprReplacer<ReplaceArrayConstant> {

    public:

    Allocator& al;
    Vec<ASR::stmt_t*>& pass_result;
    bool& remove_original_statement;

    SymbolTable* current_scope;
    ASR::expr_t* result_var;
    int result_counter;
    std::map<ASR::expr_t*, ASR::expr_t*>& resultvar2value;

    ReplaceArrayConstant(Allocator& al_, Vec<ASR::stmt_t*>& pass_result_,
        bool& remove_original_statement_,
        std::map<ASR::expr_t*, ASR::expr_t*>& resultvar2value_) :
    al(al_), pass_result(pass_result_),
    remove_original_statement(remove_original_statement_),
    current_scope(nullptr), result_var(nullptr), result_counter(0),
    resultvar2value(resultvar2value_) {}

    ASR::expr_t* get_ImpliedDoLoop_size(ASR::ImpliedDoLoop_t* implied_doloop) {
        const Location& loc = implied_doloop->base.base.loc;
        ASRUtils::ASRBuilder builder(al, loc);
        ASR::expr_t* start = implied_doloop->m_start;
        ASR::expr_t* end = implied_doloop->m_end;
        ASR::expr_t* d = implied_doloop->m_increment;
        ASR::expr_t* implied_doloop_size = nullptr;
        if( d == nullptr ) {
            implied_doloop_size = builder.ElementalAdd(
                builder.ElementalSub(end, start, loc),
                make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, 1, 4, loc), loc);
        } else {
            implied_doloop_size = builder.ElementalAdd(builder.ElementalDiv(
                builder.ElementalSub(end, start, loc), d, loc),
                make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, 1, 4, loc), loc);
        }
        int const_elements = 0;
        for( size_t i = 0; i < implied_doloop->n_values; i++ ) {
            if( ASR::is_a<ASR::ImpliedDoLoop_t>(*implied_doloop->m_values[i]) ) {
                ASR::expr_t* implied_doloop_size_ = get_ImpliedDoLoop_size(
                    ASR::down_cast<ASR::ImpliedDoLoop_t>(implied_doloop->m_values[i]));
                implied_doloop_size = builder.ElementalMul(implied_doloop_size_, implied_doloop_size, loc);
            } else {
                const_elements += 1;
            }
        }
        if(  const_elements > 0 ) {
            implied_doloop_size = builder.ElementalAdd(
                make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, const_elements, 4, loc),
                implied_doloop_size, loc);
        }
        return implied_doloop_size;
    }

    size_t get_constant_ArrayConstant_size(ASR::ArrayConstant_t* x) {
        size_t size = 0;
        for( size_t i = 0; i < x->n_args; i++ ) {
            if( ASR::is_a<ASR::ArrayConstant_t>(*x->m_args[i]) ) {
                size += get_constant_ArrayConstant_size(
                    ASR::down_cast<ASR::ArrayConstant_t>(x->m_args[i]));
            } else {
                size += 1;
            }
        }
        return size;
    }

    ASR::expr_t* get_ArrayConstant_size(ASR::ArrayConstant_t* x, bool& is_allocatable) {
        ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x->base.base.loc, 4));
        ASR::expr_t* array_size = nullptr;
        size_t constant_size = 0;
        const Location& loc = x->base.base.loc;
        ASRUtils::ASRBuilder builder(al, loc);
        for( size_t i = 0; i < x->n_args; i++ ) {
            ASR::expr_t* element = x->m_args[i];
            if( ASR::is_a<ASR::ArrayConstant_t>(*element) ) {
                if( ASRUtils::is_value_constant(element) ) {
                    constant_size += get_constant_ArrayConstant_size(
                        ASR::down_cast<ASR::ArrayConstant_t>(element));
                } else {
                    ASR::expr_t* element_array_size = get_ArrayConstant_size(
                                    ASR::down_cast<ASR::ArrayConstant_t>(element), is_allocatable);
                    if( array_size == nullptr ) {
                        array_size = element_array_size;
                    } else {
                        array_size = builder.ElementalAdd(array_size,
                                        element_array_size, x->base.base.loc);
                    }
                }
            } else if( ASR::is_a<ASR::Var_t>(*element) ) {
                ASR::ttype_t* element_type = ASRUtils::type_get_past_allocatable(
                    ASRUtils::expr_type(element));
                if( ASRUtils::is_array(element_type) ) {
                    if( ASRUtils::is_fixed_size_array(element_type) ) {
                        ASR::dimension_t* m_dims = nullptr;
                        size_t n_dims = ASRUtils::extract_dimensions_from_ttype(element_type, m_dims);
                        constant_size += ASRUtils::get_fixed_size_of_array(m_dims, n_dims);
                    } else {
                        ASR::expr_t* element_array_size = ASRUtils::get_size(element, al);
                        if( array_size == nullptr ) {
                            array_size = element_array_size;
                        } else {
                            array_size = builder.ElementalAdd(array_size,
                                            element_array_size, x->base.base.loc);
                        }
                    }
                } else {
                    constant_size += 1;
                }
            } else if( ASR::is_a<ASR::ImpliedDoLoop_t>(*element) ) {
                ASR::expr_t* implied_doloop_size = get_ImpliedDoLoop_size(
                    ASR::down_cast<ASR::ImpliedDoLoop_t>(element));
                if( array_size ) {
                    array_size = builder.ElementalAdd(implied_doloop_size, array_size, loc);
                } else {
                    array_size = implied_doloop_size;
                }
            } else if( ASR::is_a<ASR::ArraySection_t>(*element) ) {
                ASR::ArraySection_t* array_section_t = ASR::down_cast<ASR::ArraySection_t>(element);
                ASR::expr_t* array_section_size = nullptr;
                for( size_t j = 0; j < array_section_t->n_args; j++ ) {
                    ASR::expr_t* start = array_section_t->m_args[j].m_left;
                    ASR::expr_t* end = array_section_t->m_args[j].m_right;
                    ASR::expr_t* d = array_section_t->m_args[j].m_step;
                    if( d == nullptr ) {
                        continue;
                    }
                    ASR::expr_t* dim_size = builder.ElementalAdd(builder.ElementalDiv(
                        builder.ElementalSub(end, start, loc), d, loc),
                        make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, 1, 4, loc), loc);
                    if( array_section_size == nullptr ) {
                        array_section_size = dim_size;
                    } else {
                        array_section_size = builder.ElementalMul(array_section_size, dim_size, loc);
                    }
                }
                if( array_size == nullptr ) {
                    array_size = array_section_size;
                } else {
                    builder.ElementalAdd(array_section_size, array_size, loc);
                }
            } else {
                constant_size += 1;
            }
        }
        ASR::expr_t* constant_size_asr = nullptr;
        if( constant_size != 0 ) {
            constant_size_asr = make_ConstantWithType(make_IntegerConstant_t,
                                    constant_size, int_type, x->base.base.loc);
            if( array_size == nullptr ) {
                return constant_size_asr;
            }
        }
        if( constant_size_asr ) {
            array_size = builder.ElementalAdd(array_size, constant_size_asr, x->base.base.loc);
        }
        is_allocatable = true;
        return array_size;
    }

    void replace_ArrayConstant(ASR::ArrayConstant_t* x) {
        const Location& loc = x->base.base.loc;
        ASR::expr_t* result_var_copy = result_var;
        if (result_var == nullptr ||
            !(resultvar2value.find(result_var) != resultvar2value.end() &&
              resultvar2value[result_var] == &(x->base))) {
            remove_original_statement = false;
            ASR::ttype_t* result_type_ = nullptr;
            bool is_allocatable = false;
            ASR::expr_t* array_constant_size = get_ArrayConstant_size(x, is_allocatable);
            Vec<ASR::dimension_t> dims;
            dims.reserve(al, 1);
            ASR::dimension_t dim;
            dim.loc = loc;
            dim.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc,
                            1, ASRUtils::type_get_past_allocatable(
                                ASRUtils::expr_type(array_constant_size))));
            dim.m_length = array_constant_size;
            dims.push_back(al, dim);
            if( is_allocatable ) {
                result_type_ = ASRUtils::TYPE(ASR::make_Allocatable_t(al, x->m_type->base.loc,
                    ASRUtils::type_get_past_allocatable(
                        ASRUtils::duplicate_type_with_empty_dims(al, x->m_type))));
            } else {
                result_type_ = ASRUtils::duplicate_type(al,
                    ASRUtils::type_get_past_allocatable(x->m_type), &dims);
            }
            result_var = PassUtils::create_var(result_counter, "_array_constant_",
                            loc, result_type_, al, current_scope);
            result_counter += 1;
            if( is_allocatable ) {
                Vec<ASR::alloc_arg_t> alloc_args;
                alloc_args.reserve(al, 1);
                ASR::alloc_arg_t arg;
                arg.m_len_expr = nullptr;
                arg.m_type = nullptr;
                arg.loc = result_var->base.loc;
                arg.m_a = result_var;
                arg.m_dims = dims.p;
                arg.n_dims = dims.size();
                alloc_args.push_back(al, arg);
                ASR::stmt_t* allocate_stmt = ASRUtils::STMT(ASR::make_Allocate_t(al, loc,
                                                alloc_args.p, alloc_args.size(),
                                                nullptr, nullptr, nullptr));
                pass_result.push_back(al, allocate_stmt);
            }
            *current_expr = result_var;
        } else {
            remove_original_statement = true;
        }
        LCOMPILERS_ASSERT(result_var != nullptr);
        Vec<ASR::stmt_t*>* result_vec = &pass_result;
        PassUtils::ReplacerUtils::replace_ArrayConstant(x, this,
            remove_original_statement, result_vec);
        result_var = result_var_copy;
    }

    void replace_ArrayPhysicalCast(ASR::ArrayPhysicalCast_t* x) {
        ASR::BaseExprReplacer<ReplaceArrayConstant>::replace_ArrayPhysicalCast(x);
        x->m_old = ASRUtils::extract_physical_type(ASRUtils::expr_type(x->m_arg));
        if( x->m_old == x->m_new ) {
            *current_expr = x->m_arg;
        }
    }

};

class ArrayConstantVisitor : public ASR::CallReplacerOnExpressionsVisitor<ArrayConstantVisitor>
{
    private:

        Allocator& al;
        bool remove_original_statement;
        ReplaceArrayConstant replacer;
        Vec<ASR::stmt_t*> pass_result;
        std::map<ASR::expr_t*, ASR::expr_t*> resultvar2value;

    public:

        ArrayConstantVisitor(Allocator& al_) :
        al(al_), remove_original_statement(false),
        replacer(al_, pass_result,
            remove_original_statement, resultvar2value) {
            pass_result.n = 0;
            pass_result.reserve(al, 0);
        }

        void visit_Variable(const ASR::Variable_t& /*x*/) {
            // Do nothing, already handled in init_expr pass
        }

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.current_scope = current_scope;
            replacer.replace_expr(*current_expr);
        }

        void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
            Vec<ASR::stmt_t*> body;
            body.reserve(al, n_body);

            for (size_t i = 0; i < n_body; i++) {
                pass_result.n = 0;
                pass_result.reserve(al, 1);
                remove_original_statement = false;
                replacer.result_var = nullptr;
                visit_stmt(*m_body[i]);
                for (size_t j = 0; j < pass_result.size(); j++) {
                    body.push_back(al, pass_result[j]);
                }
                if( !remove_original_statement ) {
                    body.push_back(al, m_body[i]);
                }
                remove_original_statement = false;
            }
            m_body = body.p;
            n_body = body.size();
            replacer.result_var = nullptr;
            pass_result.n = 0;
            pass_result.reserve(al, 0);
        }

        void visit_Assignment(const ASR::Assignment_t &x) {
            if( (ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x.m_target)) &&
                ASR::is_a<ASR::GetPointer_t>(*x.m_value)) ||
                ASR::is_a<ASR::ArrayReshape_t>(*x.m_value) ) {
                return ;
            }

            if (x.m_overloaded) {
                this->visit_stmt(*x.m_overloaded);
                remove_original_statement = false;
                return ;
            }

            replacer.result_var = x.m_target;
            resultvar2value[replacer.result_var] = x.m_value;
            ASR::expr_t** current_expr_copy_9 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_value));
            this->call_replacer();
            current_expr = current_expr_copy_9;
            if( !remove_original_statement ) {
                this->visit_expr(*x.m_value);
            }
        }

        void visit_CPtrToPointer(const ASR::CPtrToPointer_t& /*x*/) {
            // Do nothing.
        }

};

void pass_replace_implied_do_loops(Allocator &al,
    ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions& /*pass_options*/) {
    ArrayConstantVisitor v(al);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LCompilers
