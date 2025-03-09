#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/replace_implied_do_loops.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/intrinsic_function_registry.h>


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
    bool realloc_lhs, allocate_target;

    ReplaceArrayConstant(Allocator& al_, Vec<ASR::stmt_t*>& pass_result_,
        bool& remove_original_statement_,
        std::map<ASR::expr_t*, ASR::expr_t*>& resultvar2value_,
        bool realloc_lhs_, bool allocate_target_) :
    al(al_), pass_result(pass_result_),
    remove_original_statement(remove_original_statement_),
    current_scope(nullptr),
    result_var(nullptr), result_counter(0),
    resultvar2value(resultvar2value_),
    realloc_lhs(realloc_lhs_), allocate_target(allocate_target_) {}

    ASR::expr_t* get_ImpliedDoLoop_size(ASR::ImpliedDoLoop_t* implied_doloop) {
        const Location& loc = implied_doloop->base.base.loc;
        ASRUtils::ASRBuilder builder(al, loc);
        ASR::expr_t* start = implied_doloop->m_start;
        ASR::expr_t* end = implied_doloop->m_end;
        ASR::expr_t* d = implied_doloop->m_increment;
        ASR::expr_t* implied_doloop_size = nullptr;
        int kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(end));
        start = builder.i2i_t(start, ASRUtils::expr_type(end));
        if( d == nullptr ) {
            implied_doloop_size = ASRUtils::compute_length_from_start_end(al, start, end);
        } else {
            implied_doloop_size = builder.Add(builder.Div(
                builder.Sub(end, start), d),
                make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, 1, kind, loc));
        }
        int const_elements = 0;
        ASR::expr_t* implied_doloop_size_ = nullptr;
        for( size_t i = 0; i < implied_doloop->n_values; i++ ) {
            if( ASR::is_a<ASR::ImpliedDoLoop_t>(*implied_doloop->m_values[i]) ) {
                if( implied_doloop_size_ == nullptr ) {
                    implied_doloop_size_ = get_ImpliedDoLoop_size(
                        ASR::down_cast<ASR::ImpliedDoLoop_t>(implied_doloop->m_values[i]));
                } else {
                    implied_doloop_size_ = builder.Add(get_ImpliedDoLoop_size(
                        ASR::down_cast<ASR::ImpliedDoLoop_t>(implied_doloop->m_values[i])),
                        implied_doloop_size_);
                }
            } else {
                const_elements += 1;
            }
        }
        if( const_elements > 1 ) {
            if( implied_doloop_size_ == nullptr ) {
                implied_doloop_size_ = make_ConstantWithKind(make_IntegerConstant_t,
                    make_Integer_t, const_elements, kind, loc);
            } else {
                implied_doloop_size_ = builder.Add(
                    make_ConstantWithKind(make_IntegerConstant_t,
                        make_Integer_t, const_elements, kind, loc),
                    implied_doloop_size_);
            }
        }
        if( implied_doloop_size_ ) {
            implied_doloop_size = builder.Mul(implied_doloop_size_, implied_doloop_size);
        }
        return implied_doloop_size;
    }

    size_t get_constant_ArrayConstant_size(ASR::ArrayConstant_t* x) {
        return ASRUtils::get_fixed_size_of_array(x->m_type);
    }

    ASR::expr_t* get_ArrayConstructor_size(ASR::ArrayConstructor_t* x, bool& is_allocatable) {
        ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x->base.base.loc, 4));
        ASR::expr_t* array_size = nullptr;
        int64_t constant_size = 0;
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
                        ASR::down_cast<ASR::ArrayConstant_t>(element));
                    if( array_size == nullptr ) {
                        array_size = element_array_size;
                    } else {
                        array_size = builder.Add(array_size,
                                        element_array_size);
                    }
                }
            } else if( ASR::is_a<ASR::ArrayConstructor_t>(*element) ) {
                ASR::expr_t* element_array_size = get_ArrayConstructor_size(
                    ASR::down_cast<ASR::ArrayConstructor_t>(element), is_allocatable);
                if( array_size == nullptr ) {
                    array_size = element_array_size;
                } else {
                    array_size = builder.Add(array_size,
                                    element_array_size);
                }
            } else if ( ASR::is_a<ASR::ArrayItem_t>(*element) ) {
                ASR::ArrayItem_t* array_item = ASR::down_cast<ASR::ArrayItem_t>(element);
                if ( ASR::is_a<ASR::Array_t>(*array_item->m_type) ) {
                    if ( ASRUtils::is_fixed_size_array(array_item->m_type) ) {
                        ASR::Array_t* array_type = ASR::down_cast<ASR::Array_t>(array_item->m_type);
                        constant_size += ASRUtils::get_fixed_size_of_array(array_type->m_dims, array_type->n_dims);
                    } else {
                        ASR::expr_t* element_array_size = ASRUtils::get_size(element, al, false);
                        if( array_size == nullptr ) {
                            array_size = element_array_size;
                        } else {
                            array_size = builder.Add(array_size,
                                            element_array_size);
                        }
                    }
                } else {
                    constant_size += 1;
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
                            array_size = builder.Add(array_size,
                                            element_array_size);
                        }
                    }
                } else {
                    constant_size += 1;
                }
            } else if( ASR::is_a<ASR::ImpliedDoLoop_t>(*element) ) {
                ASR::expr_t* implied_doloop_size = get_ImpliedDoLoop_size(
                    ASR::down_cast<ASR::ImpliedDoLoop_t>(element));
                if( array_size ) {
                    array_size = builder.Add(implied_doloop_size, array_size);
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
                    ASR::expr_t* dim_size = builder.Add(builder.Div(
                        builder.Sub(end, start), d),
                        make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, 1, 4, loc));
                    if( array_section_size == nullptr ) {
                        array_section_size = dim_size;
                    } else {
                        array_section_size = builder.Mul(array_section_size, dim_size);
                    }
                }
                if( array_size == nullptr ) {
                    array_size = array_section_size;
                } else {
                    array_size = builder.Add(array_section_size, array_size);
                }
            } else if( ASR::is_a<ASR::IntrinsicElementalFunction_t>(*element) &&
                       ASRUtils::is_array(ASRUtils::expr_type(element)) ) {
                ASR::IntrinsicElementalFunction_t* intrinsic_element_t = ASR::down_cast<ASR::IntrinsicElementalFunction_t>(element);
                if( ASRUtils::is_fixed_size_array(intrinsic_element_t->m_type) ) {
                    constant_size += ASRUtils::get_fixed_size_of_array(intrinsic_element_t->m_type);
                } else {
                    ASR::expr_t* element_array_size = ASRUtils::get_size(element, al, false);
                    if( array_size == nullptr ) {
                        array_size = element_array_size;
                    } else {
                        array_size = builder.Add(array_size,
                                        element_array_size);
                    }
                }
            } else if( ASR::is_a<ASR::FunctionCall_t>(*element)  &&
                       ASRUtils::is_array(ASRUtils::expr_type(element)) ) {
                ASR::FunctionCall_t* fc_element_t = ASR::down_cast<ASR::FunctionCall_t>(element);
                if( ASRUtils::is_fixed_size_array(fc_element_t->m_type) ) {
                    constant_size += ASRUtils::get_fixed_size_of_array(fc_element_t->m_type);
                } else {
                    ASR::expr_t* element_array_size = ASRUtils::get_size(element, al, false);
                    if( array_size == nullptr ) {
                        array_size = element_array_size;
                    } else {
                        array_size = builder.Add(array_size,
                                        element_array_size);
                    }
                }
            } else if( ASR::is_a<ASR::IntrinsicArrayFunction_t>(*element)  &&
                       ASRUtils::is_array(ASRUtils::expr_type(element)) ) {
                ASR::IntrinsicArrayFunction_t* intrinsic_element_t = ASR::down_cast<ASR::IntrinsicArrayFunction_t>(element);
                if( ASRUtils::is_fixed_size_array(intrinsic_element_t->m_type) ) {
                    constant_size += ASRUtils::get_fixed_size_of_array(intrinsic_element_t->m_type);
                } else {
                    ASR::expr_t* element_array_size = ASRUtils::get_size(element, al, false);
                    if( array_size == nullptr ) {
                        array_size = element_array_size;
                    } else {
                        array_size = builder.Add(array_size,
                                        element_array_size);
                    }
                }
            } else if( (ASR::is_a<ASR::RealBinOp_t>(*element) || ASR::is_a<ASR::IntegerBinOp_t>(*element) ||
                       ASR::is_a<ASR::RealUnaryMinus_t>(*element) || ASR::is_a<ASR::IntegerUnaryMinus_t>(*element)) &&
                       ASRUtils::is_array(ASRUtils::expr_type(element)) ) {
                ASR::ttype_t* arr_type = ASRUtils::expr_type(element);
                if( ASRUtils::is_fixed_size_array(arr_type) ) {
                    constant_size += ASRUtils::get_fixed_size_of_array(arr_type);
                } else {
                    ASR::expr_t* element_array_size = ASRUtils::get_size(element, al, false);
                    if( array_size == nullptr ) {
                        array_size = element_array_size;
                    } else {
                        array_size = builder.Add(array_size,
                                        element_array_size);
                    }
                }
            } else {
                constant_size += 1;
            }
        }
        ASR::expr_t* constant_size_asr = nullptr;
        if (constant_size == 0 && array_size == nullptr) {
            constant_size = ASRUtils::get_fixed_size_of_array(x->m_type);
        }
        if( constant_size > 0 ) {
            constant_size_asr = make_ConstantWithType(make_IntegerConstant_t,
                                    constant_size, int_type, x->base.base.loc);
            if( array_size == nullptr ) {
                return constant_size_asr;
            }
        }
        if( constant_size_asr ) {
            array_size = builder.Add(array_size, constant_size_asr);
        }
        is_allocatable = true;
        if( array_size == nullptr ) {
            array_size = make_ConstantWithKind(make_IntegerConstant_t,
                make_Integer_t, 0, 4, x->base.base.loc);
        }
        return array_size;
    }

    ASR::expr_t* get_ArrayConstant_size(ASR::ArrayConstant_t* x) {
        ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x->base.base.loc, 4));
        return make_ConstantWithType(make_IntegerConstant_t,
                ASRUtils::get_fixed_size_of_array(x->m_type), int_type, x->base.base.loc);
    }

   void replace_ArrayConstructor(ASR::ArrayConstructor_t* x) {
        const Location& loc = x->base.base.loc;
        ASR::expr_t* result_var_copy = result_var;
        bool is_result_var_fixed_size = false;
        if (result_var != nullptr &&
            resultvar2value.find(result_var) != resultvar2value.end() &&
            resultvar2value[result_var] == &(x->base)) {
            is_result_var_fixed_size = ASRUtils::is_fixed_size_array(ASRUtils::expr_type(result_var));
        }
        ASR::ttype_t* result_type_ = nullptr;
        bool is_allocatable = false;
        ASR::expr_t* array_constructor = get_ArrayConstructor_size(x, is_allocatable);
        Vec<ASR::dimension_t> dims;
        dims.reserve(al, 1);
        ASR::dimension_t dim;
        dim.loc = loc;
        dim.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 1,
                        ASRUtils::type_get_past_pointer(
                            ASRUtils::type_get_past_allocatable(
                                ASRUtils::expr_type(array_constructor)))));
        dim.m_length = array_constructor;
        dims.push_back(al, dim);
        remove_original_statement = false;
        if( is_result_var_fixed_size ) {
            result_type_ = ASRUtils::expr_type(result_var);
            is_allocatable = false;
        } else {
            if( is_allocatable ) {
                result_type_ = ASRUtils::TYPE(ASR::make_Allocatable_t(al, x->m_type->base.loc,
                    ASRUtils::type_get_past_allocatable(
                        ASRUtils::duplicate_type_with_empty_dims(al, x->m_type))));
            } else {
                result_type_ = ASRUtils::duplicate_type(al,
                    ASRUtils::type_get_past_allocatable(x->m_type), &dims);
            }
        }
        result_var = PassUtils::create_var(result_counter, "_array_constructor_",
                        loc, result_type_, al, current_scope);
        result_counter += 1;
        *current_expr = result_var;

        Vec<ASR::alloc_arg_t> alloc_args;
        alloc_args.reserve(al, 1);
        ASR::alloc_arg_t arg;
        arg.m_len_expr = nullptr;
        arg.m_type = nullptr;
        arg.m_dims = dims.p;
        arg.n_dims = dims.size();
        if( is_allocatable ) {
            arg.loc = result_var->base.loc;
            arg.m_a = result_var;
            alloc_args.push_back(al, arg);
            Vec<ASR::expr_t*> to_be_deallocated;
            to_be_deallocated.reserve(al, alloc_args.size());
            for( size_t i = 0; i < alloc_args.size(); i++ ) {
                to_be_deallocated.push_back(al, alloc_args.p[i].m_a);
            }
            pass_result.push_back(al, ASRUtils::STMT(ASR::make_ExplicitDeallocate_t(
                al, loc, to_be_deallocated.p, to_be_deallocated.size())));
            ASR::stmt_t* allocate_stmt = ASRUtils::STMT(ASR::make_Allocate_t(
                al, loc, alloc_args.p, alloc_args.size(), nullptr, nullptr, nullptr));
            pass_result.push_back(al, allocate_stmt);
        }
        if ( allocate_target && realloc_lhs ) {
            allocate_target = false;
            arg.loc = result_var_copy->base.loc;
            arg.m_a = result_var_copy;
            alloc_args.push_back(al, arg);
            Vec<ASR::expr_t*> to_be_deallocated;
            to_be_deallocated.reserve(al, alloc_args.size());
            for( size_t i = 0; i < alloc_args.size(); i++ ) {
                to_be_deallocated.push_back(al, alloc_args.p[i].m_a);
            }
            pass_result.push_back(al, ASRUtils::STMT(ASR::make_ExplicitDeallocate_t(
                al, loc, to_be_deallocated.p, to_be_deallocated.size())));
            ASR::stmt_t* allocate_stmt = ASRUtils::STMT(ASR::make_Allocate_t(
                al, loc, alloc_args.p, alloc_args.size(), nullptr, nullptr, nullptr));
            pass_result.push_back(al, allocate_stmt);
        }
        for (size_t i = 0; i < x->n_args; i++) {
            if(ASR::is_a<ASR::ArrayItem_t>(*x->m_args[i]) || ASR::is_a<ASR::ArraySection_t>(*x->m_args[i])){
                ASR::expr_t** temp = current_expr;
                current_expr = &(x->m_args[i]);
                self().replace_expr(x->m_args[i]);
                current_expr = temp;
            }
        }
        LCOMPILERS_ASSERT(result_var != nullptr);
        Vec<ASR::stmt_t*>* result_vec = &pass_result;
        PassUtils::ReplacerUtils::replace_ArrayConstructor_(al, x, result_var,
            result_vec, current_scope);
        result_var = result_var_copy;
    }

    void replace_ArrayConstant(ASR::ArrayConstant_t* x) {
        const Location& loc = x->base.base.loc;
        ASR::expr_t* result_var_copy = result_var;
        bool is_result_var_fixed_size = false;
        if (result_var != nullptr &&
            resultvar2value.find(result_var) != resultvar2value.end() &&
            resultvar2value[result_var] == &(x->base)) {
            is_result_var_fixed_size = ASRUtils::is_fixed_size_array(ASRUtils::expr_type(result_var));
        }
        ASR::ttype_t* result_type_ = nullptr;
        bool is_allocatable = false;
        ASR::expr_t* array_constant_size = get_ArrayConstant_size(x);
        Vec<ASR::dimension_t> dims;
        dims.reserve(al, 1);
        ASR::dimension_t dim;
        dim.loc = loc;
        dim.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 1,
                        ASRUtils::type_get_past_pointer(
                            ASRUtils::type_get_past_allocatable(
                                ASRUtils::expr_type(array_constant_size)))));
        dim.m_length = array_constant_size;
        dims.push_back(al, dim);
        remove_original_statement = false;
        if( is_result_var_fixed_size ) {
            result_type_ = ASRUtils::expr_type(result_var);
            is_allocatable = false;
        } else {
            if( is_allocatable ) {
                result_type_ = ASRUtils::TYPE(ASRUtils::make_Allocatable_t_util(al, x->m_type->base.loc,
                    ASRUtils::type_get_past_allocatable(
                        ASRUtils::duplicate_type_with_empty_dims(al, x->m_type))));
            } else {
                result_type_ = ASRUtils::duplicate_type(al,
                    ASRUtils::type_get_past_allocatable(x->m_type), &dims);
            }
        }
        result_var = PassUtils::create_var(result_counter, "_array_constant_",
                        loc, result_type_, al, current_scope);
        result_counter += 1;
        *current_expr = result_var;

        Vec<ASR::alloc_arg_t> alloc_args;
        alloc_args.reserve(al, 1);
        ASR::alloc_arg_t arg;
        arg.m_len_expr = nullptr;
        arg.m_type = nullptr;
        arg.m_dims = dims.p;
        arg.n_dims = dims.size();
        if( is_allocatable ) {
            arg.loc = result_var->base.loc;
            arg.m_a = result_var;
            alloc_args.push_back(al, arg);
            Vec<ASR::expr_t*> to_be_deallocated;
            to_be_deallocated.reserve(al, alloc_args.size());
            for( size_t i = 0; i < alloc_args.size(); i++ ) {
                to_be_deallocated.push_back(al, alloc_args.p[i].m_a);
            }
            pass_result.push_back(al, ASRUtils::STMT(ASR::make_ExplicitDeallocate_t(
                al, loc, to_be_deallocated.p, to_be_deallocated.size())));
            ASR::stmt_t* allocate_stmt = ASRUtils::STMT(ASR::make_Allocate_t(
                al, loc, alloc_args.p, alloc_args.size(), nullptr, nullptr, nullptr));
            pass_result.push_back(al, allocate_stmt);
        }
        if ( allocate_target && realloc_lhs ) {
            allocate_target = false;
            arg.loc = result_var_copy->base.loc;
            arg.m_a = result_var_copy;
            alloc_args.push_back(al, arg);
            Vec<ASR::expr_t*> to_be_deallocated;
            to_be_deallocated.reserve(al, alloc_args.size());
            for( size_t i = 0; i < alloc_args.size(); i++ ) {
                to_be_deallocated.push_back(al, alloc_args.p[i].m_a);
            }
            pass_result.push_back(al, ASRUtils::STMT(ASR::make_ExplicitDeallocate_t(
                al, loc, to_be_deallocated.p, to_be_deallocated.size())));
            ASR::stmt_t* allocate_stmt = ASRUtils::STMT(ASR::make_Allocate_t(
                al, loc, alloc_args.p, alloc_args.size(), nullptr, nullptr, nullptr));
            pass_result.push_back(al, allocate_stmt);
        }
        LCOMPILERS_ASSERT(result_var != nullptr);
        Vec<ASR::stmt_t*>* result_vec = &pass_result;
        PassUtils::ReplacerUtils::replace_ArrayConstant(x, this,
            remove_original_statement, result_vec);
        result_var = result_var_copy;
    }

    void replace_ArrayPhysicalCast(ASR::ArrayPhysicalCast_t* x) {
        [[maybe_unused]] bool is_arr_construct_arg = ASR::is_a<ASR::ArrayConstructor_t>(*x->m_arg);
        ASR::BaseExprReplacer<ReplaceArrayConstant>::replace_ArrayPhysicalCast(x);
        if( x->m_old != ASRUtils::extract_physical_type(ASRUtils::expr_type(x->m_arg)) ) {
            x->m_old = ASRUtils::extract_physical_type(ASRUtils::expr_type(x->m_arg));
        }
        if( (is_arr_construct_arg && ASRUtils::is_fixed_size_array(ASRUtils::expr_type(x->m_arg))) ){
            *current_expr = x->m_arg;
            return;
        }

        if( (x->m_old == x->m_new &&
             x->m_old != ASR::array_physical_typeType::DescriptorArray) ||
            (x->m_old == x->m_new && x->m_old == ASR::array_physical_typeType::DescriptorArray &&
            (ASR::is_a<ASR::Allocatable_t>(*ASRUtils::expr_type(x->m_arg)) ||
            ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x->m_arg)))) ||
            x->m_old != ASRUtils::extract_physical_type(ASRUtils::expr_type(x->m_arg)) ) {
            *current_expr = x->m_arg;
        } else {
            x->m_old = ASRUtils::extract_physical_type(ASRUtils::expr_type(x->m_arg));
        }
    }

    void replace_ArrayBroadcast(ASR::ArrayBroadcast_t* x) {
        ASR::expr_t** current_expr_copy_161 = current_expr;
        current_expr = &(x->m_array);
        replace_expr(x->m_array);
        current_expr = current_expr_copy_161;
    }

};

class ArrayConstantVisitor : public ASR::CallReplacerOnExpressionsVisitor<ArrayConstantVisitor>
{
    private:

        Allocator& al;
        bool remove_original_statement, allocate_target = false;
        bool print = false, file_write = false;
        ASR::expr_t* m_unit = nullptr;
        ReplaceArrayConstant replacer;
        Vec<ASR::stmt_t*> pass_result;
        Vec<ASR::stmt_t*>* parent_body;
        std::map<ASR::expr_t*, ASR::expr_t*> resultvar2value;

    public:

        ArrayConstantVisitor(Allocator& al_, bool realloc_lhs_) :
        al(al_), remove_original_statement(false),
        replacer(al_, pass_result, remove_original_statement,
            resultvar2value, realloc_lhs_, allocate_target),
        parent_body(nullptr) {
            call_replacer_on_value = false;
            replacer.call_replacer_on_value = false;
            pass_result.n = 0;
            pass_result.reserve(al, 0);
            print = false, file_write = false;
            m_unit = nullptr;
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
            if( parent_body ) {
                for (size_t j=0; j < pass_result.size(); j++) {
                    parent_body->push_back(al, pass_result[j]);
                }
            }

            for (size_t i = 0; i < n_body; i++) {
                pass_result.n = 0;
                pass_result.reserve(al, 1);
                remove_original_statement = false;
                replacer.result_var = nullptr;
                Vec<ASR::stmt_t*>* parent_body_copy = parent_body;
                parent_body = &body;
                visit_stmt(*m_body[i]);
                parent_body = parent_body_copy;
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

            if (ASRUtils::is_allocatable(x.m_target) &&
                    ASR::is_a<ASR::ArrayConstant_t>(*x.m_value)) {
                allocate_target = true;
            }
            replacer.result_var = x.m_target;
            resultvar2value[replacer.result_var] = x.m_value;
            ASR::expr_t** current_expr_copy_9 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_value));
            this->call_replacer();
            current_expr = const_cast<ASR::expr_t**>(&(x.m_target));
            this->call_replacer();
            current_expr = current_expr_copy_9;
            if( !remove_original_statement ) {
                this->visit_expr(*x.m_value);
            }
        }

        template <typename T>
        ASR::asr_t* create_array_constant(const T& x, ASR::expr_t* value) {
            // wrap the implied do loop in an array constant
            Vec<ASR::expr_t*> args;
            args.reserve(al, 1);
            args.push_back(al, value);

            Vec<ASR::dimension_t> dim;
            dim.reserve(al, 1);

            ASR::dimension_t d;
            d.loc = value->base.loc;

            ASR::ttype_t *int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4));
            ASR::expr_t* one = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, 1, int32_type));

            d.m_start = one;
            d.m_length = one;

            dim.push_back(al, d);

            ASR::ttype_t* array_type = ASRUtils::TYPE(ASR::make_Array_t(al, value->base.loc, ASRUtils::expr_type(value), dim.p, dim.size(), ASR::array_physical_typeType::FixedSizeArray));
            ASR::asr_t* array_constant = ASRUtils::make_ArrayConstructor_t_util(al, value->base.loc,
                                        args.p, args.n, array_type, ASR::arraystorageType::ColMajor);
            return array_constant;
        }

        ASR::stmt_t* create_do_loop_form_idl(ASR::ImpliedDoLoop_t* x) {
            ASR::stmt_t* do_loop = nullptr;

            ASR::do_loop_head_t do_loop_head;
            do_loop_head.loc = x->base.base.loc; do_loop_head.m_v = x->m_var;
            do_loop_head.m_start = x->m_start; do_loop_head.m_end = x->m_end;
            do_loop_head.m_increment = x->m_increment;

            Vec<ASR::stmt_t*> do_loop_body; do_loop_body.reserve(al, x->n_values);
            for (size_t i = 0; i < x->n_values; i++ ) {
                if ( ASR::is_a<ASR::ImpliedDoLoop_t>(*x->m_values[i]) ) {
                    do_loop_body.push_back(al, create_do_loop_form_idl(
                        ASR::down_cast<ASR::ImpliedDoLoop_t>(x->m_values[i])));
                } else {
                    Vec<ASR::expr_t*> print_values; print_values.reserve(al, 1);
                    ASR::stmt_t* stmt = nullptr;
                    Vec<ASR::expr_t*> args;
                    args.reserve(al, 1);
                    args.push_back(al, x->m_values[i]);
                    ASR::expr_t* fmt_val = ASRUtils::EXPR(ASR::make_StringFormat_t(al,x->base.base.loc, nullptr,
                        args.p, 1,ASR::string_format_kindType::FormatFortran,
                        ASRUtils::TYPE(ASR::make_String_t(al,x->base.base.loc,-1,-1,nullptr, ASR::string_physical_typeType::PointerString)), nullptr));
                    print_values.push_back(al, fmt_val);
                    if ( print ) {
                        stmt = ASRUtils::STMT(ASRUtils::make_print_t_util(al, x->m_values[i]->base.loc, print_values.p, print_values.size()));
                    } else {
                        // this will be file_write
                        LCOMPILERS_ASSERT(file_write);
                        stmt = ASRUtils::STMT(ASR::make_FileWrite_t(al, x->m_values[i]->base.loc, 0, m_unit, nullptr, nullptr, nullptr, print_values.p, print_values.size(), nullptr, nullptr, nullptr));
                    }
                    do_loop_body.push_back(al, stmt);
                }
            }
            do_loop = ASRUtils::STMT(ASR::make_DoLoop_t(al, x->base.base.loc, nullptr, do_loop_head, do_loop_body.p, do_loop_body.size(), nullptr, 0));
            return do_loop;
        }

        void visit_Print(const ASR::Print_t &x) {
            print = true;
            /*
                integer :: i
                print *, (i, i=1, 10)

                TO

                integer :: i
                print *, [(i, i=1, 10)]
            */
            if(ASR::is_a<ASR::String_t>(*ASRUtils::expr_type(x.m_text))){
                ASR::Print_t* print_stmt = const_cast<ASR::Print_t*>(&x);
                ASR::expr_t** current_expr_copy_9 = current_expr;
                current_expr = const_cast<ASR::expr_t**>(&(print_stmt->m_text));
                this->call_replacer();
                current_expr = current_expr_copy_9;
                if( !remove_original_statement ) {
                    this->visit_expr(*print_stmt->m_text);
                }
                print = false;
            } else {
                LCOMPILERS_ASSERT_MSG(false, "print should support stringFormat or single string");
            }
        }

        void visit_StringFormat(const ASR::StringFormat_t &x) {
            /*
                integer :: i
                write(*, '(i)') (i, i=1, 10)

                TO

                integer :: i
                write(*, '(i)') [(i, i=1, 10)]
            */
            ASR::StringFormat_t* string_format_stmt = const_cast<ASR::StringFormat_t*>(&x);
            for(size_t i = 0; i < x.n_args; i++) {
                ASR::expr_t* value = x.m_args[i];
                if (ASR::is_a<ASR::ImpliedDoLoop_t>(*value)) {
                    ASR::ImpliedDoLoop_t* implied_do_loop = ASR::down_cast<ASR::ImpliedDoLoop_t>(value);
                    if ( ASR::is_a<ASR::Tuple_t>(*implied_do_loop->m_type) ) {
                        remove_original_statement = true;
                        pass_result.push_back(al, create_do_loop_form_idl(implied_do_loop));
                        continue;
                    }
                    ASR::asr_t* array_constant = create_array_constant(x, value);
                    string_format_stmt->m_args[i] = ASRUtils::EXPR(array_constant);

                    replacer.result_var = value;
                    resultvar2value[replacer.result_var] = ASRUtils::EXPR(array_constant);
                    ASR::expr_t** current_expr_copy_9 = current_expr;
                    current_expr = const_cast<ASR::expr_t**>(&(string_format_stmt->m_args[i]));
                    this->call_replacer();
                    current_expr = current_expr_copy_9;
                    if( !remove_original_statement ) {
                        this->visit_expr(*string_format_stmt->m_args[i]);
                    }
                } else {
                    ASR::expr_t** current_expr_copy_9 = current_expr;
                    current_expr = const_cast<ASR::expr_t**>(&(string_format_stmt->m_args[i]));
                    this->call_replacer();
                    current_expr = current_expr_copy_9;
                    if( !remove_original_statement ) {
                        this->visit_expr(*string_format_stmt->m_args[i]);
                    }
                }
            }
        }

        void visit_FileRead(const ASR::FileRead_t &x) {
            if (x.m_overloaded) {
                this->visit_stmt(*x.m_overloaded);
                remove_original_statement = false;
                return;
            }
        }

        void visit_FileWrite(const ASR::FileWrite_t &x) {
            file_write = true;
            m_unit = x.m_unit;
            if (x.m_overloaded) {
                this->visit_stmt(*x.m_overloaded);
                remove_original_statement = false;
                file_write = false;
                return;
            }

            /*
                integer :: i
                write(*,*) (i, i=1, 10)

                TO

                integer :: i
                write(*,*) [(i, i=1, 10)]
            */
            ASR::FileWrite_t* write_stmt = const_cast<ASR::FileWrite_t*>(&x);
            for(size_t i = 0; i < x.n_values; i++) {
                ASR::expr_t* value = x.m_values[i];
                if (ASR::is_a<ASR::ImpliedDoLoop_t>(*value)) {
                    ASR::asr_t* array_constant = create_array_constant(x, value);

                    write_stmt->m_values[i] = ASRUtils::EXPR(array_constant);

                    replacer.result_var = value;
                    resultvar2value[replacer.result_var] = ASRUtils::EXPR(array_constant);
                    ASR::expr_t** current_expr_copy_9 = current_expr;
                    current_expr = const_cast<ASR::expr_t**>(&(write_stmt->m_values[i]));
                    this->call_replacer();
                    current_expr = current_expr_copy_9;
                    if( !remove_original_statement ) {
                        this->visit_expr(*write_stmt->m_values[i]);
                    }
                } else {
                    ASR::expr_t** current_expr_copy_9 = current_expr;
                    current_expr = const_cast<ASR::expr_t**>(&(write_stmt->m_values[i]));
                    this->call_replacer();
                    current_expr = current_expr_copy_9;
                    if( !remove_original_statement ) {
                        this->visit_expr(*write_stmt->m_values[i]);
                    }
                }
            }
            file_write = false;
        }

        void visit_CPtrToPointer(const ASR::CPtrToPointer_t& x) {
            if (x.m_shape) {
                ASR::expr_t** current_expr_copy = current_expr;
                current_expr = const_cast<ASR::expr_t**>(&(x.m_shape));
                this->call_replacer();
                current_expr = current_expr_copy;
                if( x.m_shape )
                this->visit_expr(*x.m_shape);
            }
        }

        void visit_ArrayBroadcast(const ASR::ArrayBroadcast_t& x) {
            ASR::expr_t** current_expr_copy_269 = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&(x.m_array));
            call_replacer();
            current_expr = current_expr_copy_269;
            if( x.m_array ) {
                visit_expr(*x.m_array);
            }
        }

};

void pass_replace_implied_do_loops(Allocator &al,
    ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions& pass_options) {
    ArrayConstantVisitor v(al, pass_options.realloc_lhs);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LCompilers
