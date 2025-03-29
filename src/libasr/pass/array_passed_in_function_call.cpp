#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/asr_builder.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/replace_array_passed_in_function_call.h>
#include <libasr/pass/intrinsic_function_registry.h>
#include <libasr/pass/intrinsic_subroutine_registry.h>
#include <libasr/pass/intrinsic_array_function_registry.h>

namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

/*
This pass collector that the BinOp only Var nodes and nothing else.
*/
class ArrayVarCollector: public ASR::BaseWalkVisitor<ArrayVarCollector> {
    private:

    Allocator& al;
    Vec<ASR::expr_t*>& vars;

    public:

    ArrayVarCollector(Allocator& al_, Vec<ASR::expr_t*>& vars_): al(al_), vars(vars_) {}

    void visit_Var(const ASR::Var_t& x) {
        if( ASRUtils::is_array(ASRUtils::symbol_type(x.m_v)) ) {
            vars.push_back(al, const_cast<ASR::expr_t*>(&(x.base)));
        }
    }

    void visit_StructInstanceMember(const ASR::StructInstanceMember_t& x) {
        if( ASRUtils::is_array(ASRUtils::symbol_type(x.m_m)) ) {
            vars.push_back(al, const_cast<ASR::expr_t*>(&(x.base)));
        }
    }

    void visit_ArrayBroadcast(const ASR::ArrayBroadcast_t& /*x*/) {

    }

    void visit_ArraySize(const ASR::ArraySize_t& /*x*/) {

    }

};

void transform_stmts_impl(Allocator& al, ASR::stmt_t**& m_body, size_t& n_body,
    Vec<ASR::stmt_t*>*& current_body, Vec<ASR::stmt_t*>*& body_after_curr_stmt,
    std::function<void(const ASR::stmt_t&)> visit_stmt) {
    Vec<ASR::stmt_t*>* current_body_copy = current_body;
    Vec<ASR::stmt_t*> current_body_vec; current_body_vec.reserve(al, 1);
    current_body_vec.reserve(al, n_body);
    current_body = &current_body_vec;
    for (size_t i = 0; i < n_body; i++) {
        Vec<ASR::stmt_t*>* body_after_curr_stmt_copy = body_after_curr_stmt;
        Vec<ASR::stmt_t*> body_after_curr_stmt_vec; body_after_curr_stmt_vec.reserve(al, 1);
        body_after_curr_stmt = &body_after_curr_stmt_vec;
        visit_stmt(*m_body[i]);
        current_body->push_back(al, m_body[i]);
        for (size_t j = 0; j < body_after_curr_stmt_vec.size(); j++) { 
            current_body->push_back(al, body_after_curr_stmt_vec[j]);
        }
        body_after_curr_stmt = body_after_curr_stmt_copy;
    }
    m_body = current_body_vec.p; n_body = current_body_vec.size();
    current_body = current_body_copy;
}

/*
    This pass is responsible to convert non-contiguous ( DescriptorArray, arrays with stride != 1  )
    arrays passed to functions by casting to contiguous ( PointerToDataArray ) arrays.

    For example:

    subroutine matprod(y)
        real(8), intent(inout) :: y(:, :)
        call istril(y)
    end subroutine 

    gets converted to:

    subroutine matprod(y)
        real(8), intent(inout) :: y(:, :)
        real(8), pointer :: y_tmp(:, :)
        if (.not. is_contiguous(y))
            allocate(y_tmp(size(y, 1), size(y, 2)))
            y_tmp = y
        else
            y_tmp => y
        end if
        call istril(y_tmp)
        if (.not. is_contiguous(y)) ! only if intent is inout, out
            y = y_tmp
            deallocate(y_tmp)
        end if
    end subroutine
*/
class CallVisitor : public ASR::CallReplacerOnExpressionsVisitor<CallVisitor>
{
public:

    Allocator &al;
    Vec<ASR::stmt_t*>* current_body;
    Vec<ASR::stmt_t*>* body_after_curr_stmt;

    CallVisitor(Allocator &al_) : al(al_) {}


    bool is_descriptor_array_casted_to_pointer_to_data( ASR::expr_t* expr ) {
        if ( ASRUtils::is_array(ASRUtils::expr_type(expr) ) &&
             ASR::is_a<ASR::ArrayPhysicalCast_t>(*expr) ) {
            ASR::ArrayPhysicalCast_t* cast = ASR::down_cast<ASR::ArrayPhysicalCast_t>(expr);
            return cast->m_new == ASR::array_physical_typeType::PointerToDataArray &&
                   cast->m_old == ASR::array_physical_typeType::DescriptorArray;
        }
        return false;
    }

    void transform_stmts(ASR::stmt_t**& m_body, size_t& n_body) {
        transform_stmts_impl(al, m_body, n_body, current_body, body_after_curr_stmt,
            [this](const ASR::stmt_t& stmt) { visit_stmt(stmt); });
    }

    template <typename T>
    ASR::expr_t* get_first_array_function_args(T* func) {
        int64_t first_array_arg_idx = -1;
        ASR::expr_t* first_array_arg = nullptr;
        for (int64_t i = 0; i < (int64_t)func->n_args; i++) {
            ASR::ttype_t* func_arg_type;
            if constexpr (std::is_same_v<T, ASR::FunctionCall_t>) {
                func_arg_type = ASRUtils::expr_type(func->m_args[i].m_value);
            } else {
                func_arg_type = ASRUtils::expr_type(func->m_args[i]);
            }
            if (ASRUtils::is_array(func_arg_type)) {
                first_array_arg_idx = i;
                break;
            }
        }
        LCOMPILERS_ASSERT(first_array_arg_idx != -1)
        if constexpr (std::is_same_v<T, ASR::FunctionCall_t>) {
            first_array_arg = func->m_args[first_array_arg_idx].m_value;
        } else {
            first_array_arg = func->m_args[first_array_arg_idx];
        }
        return first_array_arg;
    }


    /*
        sets allocation size of an elemental function, which can be
        either an intrinsic elemental function or a user-defined
    */
    template <typename T>
    void set_allocation_size_elemental_function(
        Allocator& al, const Location& loc,
        T* elemental_function,
        Vec<ASR::dimension_t>& allocate_dims
    ) {
        ASR::expr_t* int32_one = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                    al, loc, 1, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4))));
        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(elemental_function->m_type);
        allocate_dims.reserve(al, n_dims);
        ASR::expr_t* first_array_arg = get_first_array_function_args(elemental_function);
        for( size_t i = 0; i < n_dims; i++ ) {
            ASR::dimension_t allocate_dim;
            allocate_dim.loc = loc;
            allocate_dim.m_start = int32_one;
            ASR::expr_t* size_i_1 = ASRUtils::EXPR(ASR::make_ArraySize_t(
                al, loc, ASRUtils::get_past_array_physical_cast(first_array_arg),
                ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                    al, loc, i + 1, ASRUtils::expr_type(int32_one))),
                ASRUtils::expr_type(int32_one), nullptr));
            allocate_dim.m_length = size_i_1;
            allocate_dims.push_back(al, allocate_dim);
        }
    }

    ASR::expr_t* create_temporary_variable_for_array(Allocator& al,
        ASR::expr_t* value, SymbolTable* scope, std::string name_hint,
        bool is_pointer_required=false) {
        ASR::ttype_t* value_type = ASRUtils::expr_type(value);
        LCOMPILERS_ASSERT(ASRUtils::is_array(value_type));
    
        /* Figure out the type of the temporary array variable */
        ASR::dimension_t* value_m_dims = nullptr;
        size_t value_n_dims = ASRUtils::extract_dimensions_from_ttype(value_type, value_m_dims);
    
        if (ASR::is_a<ASR::IntegerCompare_t>(*value)) {
            ASR::IntegerCompare_t* integer_compare = ASR::down_cast<ASR::IntegerCompare_t>(value);
            ASR::ttype_t* logical_type = ASRUtils::TYPE(ASR::make_Logical_t(al, value->base.loc, 4));
    
            ASR::ttype_t* left_type = ASRUtils::expr_type(integer_compare->m_left);
            ASR::ttype_t* right_type = ASRUtils::expr_type(integer_compare->m_right);
    
            if (ASR::is_a<ASR::Array_t>(*left_type)) {
                ASR::Array_t* left_array_type = ASR::down_cast<ASR::Array_t>(left_type);
                ASR::dimension_t* left_m_dims = nullptr;
                size_t left_n_dims = ASRUtils::extract_dimensions_from_ttype(left_type, left_m_dims);
                value_m_dims = left_m_dims;
                value_n_dims = left_n_dims;
    
                if (left_array_type->m_physical_type == ASR::array_physical_typeType::FixedSizeArray) {
                    ASR::ttype_t* logical_array_type = ASRUtils::TYPE(ASR::make_Array_t(al, value->base.loc, logical_type, left_m_dims, left_n_dims, ASR::array_physical_typeType::FixedSizeArray));
                    value_type = logical_array_type;
                } else {
                    ASR::ttype_t* logical_array_type = ASRUtils::TYPE(ASR::make_Array_t(al, value->base.loc, logical_type, left_m_dims, left_n_dims, ASR::array_physical_typeType::PointerToDataArray));
                    value_type = logical_array_type;
                }
            } else if (ASR::is_a<ASR::Array_t>(*right_type)) {
                ASR::Array_t* right_array_type = ASR::down_cast<ASR::Array_t>(right_type);
                ASR::dimension_t* right_m_dims = nullptr;
                size_t right_n_dims = ASRUtils::extract_dimensions_from_ttype(right_type, right_m_dims);
                value_m_dims = right_m_dims;
                value_n_dims = right_n_dims;
    
                if (right_array_type->m_physical_type == ASR::array_physical_typeType::FixedSizeArray) {
                    ASR::ttype_t* logical_array_type = ASRUtils::TYPE(ASR::make_Array_t(al, value->base.loc, logical_type, right_m_dims, right_n_dims, ASR::array_physical_typeType::FixedSizeArray));
                    value_type = logical_array_type;
                } else {
                    ASR::ttype_t* logical_array_type = ASRUtils::TYPE(ASR::make_Array_t(al, value->base.loc, logical_type, right_m_dims, right_n_dims, ASR::array_physical_typeType::PointerToDataArray));
                    value_type = logical_array_type;
                }
            }
        }
        // dimensions can be different for an ArrayConstructor e.g. [1, a], where `a` is an
        // ArrayConstructor like [5, 2, 1]
        if (ASR::is_a<ASR::ArrayConstructor_t>(*value) &&
               !PassUtils::is_args_contains_allocatable(value)) {
            ASR::ArrayConstructor_t* arr_constructor = ASR::down_cast<ASR::ArrayConstructor_t>(value);
            value_m_dims->m_length = ASRUtils::get_ArrayConstructor_size(al, arr_constructor);
        }
        bool is_fixed_sized_array = ASRUtils::is_fixed_size_array(value_type);
        bool is_size_only_dependent_on_arguments = ASRUtils::is_dimension_dependent_only_on_arguments(
            value_m_dims, value_n_dims);
        bool is_allocatable = ASRUtils::is_allocatable(value_type);
        ASR::ttype_t* var_type = nullptr;
        if( (is_fixed_sized_array || is_size_only_dependent_on_arguments || is_allocatable) &&
            !is_pointer_required ) {
            var_type = value_type;
        } else {
            var_type = ASRUtils::create_array_type_with_empty_dims(al, value_n_dims, value_type);
            var_type = ASRUtils::TYPE(ASR::make_Pointer_t(al, var_type->base.loc, var_type));
        }
    
        std::string var_name = scope->get_unique_name("__libasr_created_" + name_hint);
        ASR::symbol_t* temporary_variable = ASR::down_cast<ASR::symbol_t>(ASRUtils::make_Variable_t_util(
            al, value->base.loc, scope, s2c(al, var_name), nullptr, 0, ASR::intentType::Local,
            nullptr, nullptr, ASR::storage_typeType::Default, var_type, nullptr, ASR::abiType::Source,
            ASR::accessType::Public, ASR::presenceType::Required, false));
        scope->add_symbol(var_name, temporary_variable);
    
        return ASRUtils::EXPR(ASR::make_Var_t(al, temporary_variable->base.loc, temporary_variable));
    }

    bool set_allocation_size(
        Allocator& al, ASR::expr_t* value,
        Vec<ASR::dimension_t>& allocate_dims,
        size_t target_n_dims
    ) {
        if ( !ASRUtils::is_array(ASRUtils::expr_type(value)) ) {
            return false;
        }
        const Location& loc = value->base.loc;
        ASR::expr_t* int32_one = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                    al, loc, 1, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4))));
        if( ASRUtils::is_fixed_size_array(ASRUtils::expr_type(value)) ) {
            ASR::dimension_t* m_dims = nullptr;
            size_t n_dims = ASRUtils::extract_dimensions_from_ttype(
                ASRUtils::expr_type(value), m_dims);
            allocate_dims.reserve(al, n_dims);
            for( size_t i = 0; i < n_dims; i++ ) {
                ASR::dimension_t allocate_dim;
                allocate_dim.loc = value->base.loc;
                allocate_dim.m_start = int32_one;
                allocate_dim.m_length = m_dims[i].m_length;
                allocate_dims.push_back(al, allocate_dim);
            }
            return true;
        }
        switch( value->type ) {
            case ASR::exprType::FunctionCall: {
                ASR::FunctionCall_t* function_call = ASR::down_cast<ASR::FunctionCall_t>(value);
                ASR::ttype_t* type = function_call->m_type;
                if( ASRUtils::is_allocatable(type) ) {
                    return false;
                }
                if (PassUtils::is_elemental(function_call->m_name)) {
                    set_allocation_size_elemental_function(al, loc, function_call, allocate_dims);
                    break;
                }
                ASRUtils::ExprStmtDuplicator duplicator(al);
                ASR::dimension_t* dims = nullptr;
                size_t n_dims = ASRUtils::extract_dimensions_from_ttype(type, dims);
                allocate_dims.reserve(al, n_dims);
                for( size_t i = 0; i < n_dims; i++ ) {
                    ASR::dimension_t dim = dims[i];
                    ASR::dimension_t dim_copy;
                    dim_copy.loc = dim.loc;
                    dim_copy.m_start = !dim.m_start ? nullptr : duplicator.duplicate_expr(dim.m_start);
                    dim_copy.m_length = !dim.m_length ? nullptr : duplicator.duplicate_expr(dim.m_length);
                    LCOMPILERS_ASSERT(dim_copy.m_start);
                    LCOMPILERS_ASSERT(dim_copy.m_length);
                    allocate_dims.push_back(al, dim_copy);
                }
                break ;
            }
            case ASR::exprType::IntegerBinOp:
            case ASR::exprType::RealBinOp:
            case ASR::exprType::ComplexBinOp:
            case ASR::exprType::LogicalBinOp:
            case ASR::exprType::UnsignedIntegerBinOp:
            case ASR::exprType::IntegerCompare:
            case ASR::exprType::RealCompare:
            case ASR::exprType::ComplexCompare:
            case ASR::exprType::LogicalCompare:
            case ASR::exprType::UnsignedIntegerCompare:
            case ASR::exprType::StringCompare:
            case ASR::exprType::IntegerUnaryMinus:
            case ASR::exprType::RealUnaryMinus:
            case ASR::exprType::ComplexUnaryMinus: {
                /*
                    Collect all the variables from these expressions,
                    then take the size of one of the arrays having
                    maximum dimensions for now. For now LFortran will
                    assume that broadcasting is doable for arrays with lesser
                    dimensions and the array having maximum dimensions
                    has compatible size of each dimension with other arrays.
                */
    
                Vec<ASR::expr_t*> array_vars; array_vars.reserve(al, 1);
                ArrayVarCollector array_var_collector(al, array_vars);
                array_var_collector.visit_expr(*value);
                Vec<ASR::expr_t*> arrays_with_maximum_rank;
                arrays_with_maximum_rank.reserve(al, 1);
                LCOMPILERS_ASSERT(target_n_dims > 0);
                for( size_t i = 0; i < array_vars.size(); i++ ) {
                    if( (size_t) ASRUtils::extract_n_dims_from_ttype(
                            ASRUtils::expr_type(array_vars[i])) == target_n_dims ) {
                        arrays_with_maximum_rank.push_back(al, array_vars[i]);
                    }
                }
    
                LCOMPILERS_ASSERT(arrays_with_maximum_rank.size() > 0);
                ASR::expr_t* selected_array = arrays_with_maximum_rank[0];
                allocate_dims.reserve(al, target_n_dims);
                for( size_t i = 0; i < target_n_dims; i++ ) {
                    ASR::dimension_t allocate_dim;
                    Location loc; loc.first = 1, loc.last = 1;
                    allocate_dim.loc = loc;
                    // Assume 1 for Fortran.
                    allocate_dim.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                        al, loc, 1, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4))));
                    ASR::expr_t* dim = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                        al, loc, i + 1, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4))));
                    allocate_dim.m_length = ASRUtils::EXPR(ASR::make_ArraySize_t(
                        al, loc, ASRUtils::get_past_array_physical_cast(selected_array),
                        dim, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4)), nullptr));
                    allocate_dims.push_back(al, allocate_dim);
                }
                break;
            }
            case ASR::exprType::LogicalNot: {
                ASR::LogicalNot_t* logical_not = ASR::down_cast<ASR::LogicalNot_t>(value);
                if ( ASRUtils::is_array(ASRUtils::expr_type(logical_not->m_arg)) ) {
                    size_t rank = ASRUtils::extract_n_dims_from_ttype(
                        ASRUtils::expr_type(logical_not->m_arg));
                    ASR::expr_t* selected_array = logical_not->m_arg;
                    allocate_dims.reserve(al, rank);
                    for( size_t i = 0; i < rank; i++ ) {
                        ASR::dimension_t allocate_dim;
                        allocate_dim.loc = loc;
                        // Assume 1 for Fortran.
                        allocate_dim.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                            al, loc, 1, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4))));
                        ASR::expr_t* dim = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                            al, loc, i + 1, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4))));
                        allocate_dim.m_length = ASRUtils::EXPR(ASR::make_ArraySize_t(
                            al, loc, ASRUtils::get_past_array_physical_cast(selected_array),
                            dim, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4)), nullptr));
                        allocate_dims.push_back(al, allocate_dim);
                    }
                }
                break;
            }
            case ASR::exprType::Cast: {
                ASR::Cast_t* cast = ASR::down_cast<ASR::Cast_t>(value);
                if ( ASRUtils::is_array(ASRUtils::expr_type(cast->m_arg)) ) {
                    size_t rank = ASRUtils::extract_n_dims_from_ttype(
                        ASRUtils::expr_type(cast->m_arg));
                    ASR::expr_t* selected_array = cast->m_arg;
                    allocate_dims.reserve(al, rank);
                    for( size_t i = 0; i < rank; i++ ) {
                        ASR::dimension_t allocate_dim;
                        allocate_dim.loc = loc;
                        // Assume 1 for Fortran.
                        allocate_dim.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                            al, loc, 1, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4))));
                        ASR::expr_t* dim = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                            al, loc, i + 1, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4))));
                        allocate_dim.m_length = ASRUtils::EXPR(ASR::make_ArraySize_t(
                            al, loc, ASRUtils::get_past_array_physical_cast(selected_array),
                            dim, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4)), nullptr));
                        allocate_dims.push_back(al, allocate_dim);
                    }
                }
                break;
            }
            case ASR::exprType::ArraySection: {
                ASR::ArraySection_t* array_section_t = ASR::down_cast<ASR::ArraySection_t>(value);
                allocate_dims.reserve(al, array_section_t->n_args);
                for( size_t i = 0; i < array_section_t->n_args; i++ ) {
                    ASR::expr_t* start = array_section_t->m_args[i].m_left;
                    ASR::expr_t* end = array_section_t->m_args[i].m_right;
                    ASR::expr_t* step = array_section_t->m_args[i].m_step;
                    ASR::dimension_t allocate_dim;
                    allocate_dim.loc = loc;
                    allocate_dim.m_start = int32_one;
                    if( start == nullptr && step == nullptr && end != nullptr ) {
                        if( ASRUtils::is_array(ASRUtils::expr_type(end)) ) {
                            allocate_dim.m_length = ASRUtils::EXPR(ASRUtils::make_ArraySize_t_util(
                                al, loc, end, nullptr, ASRUtils::expr_type(int32_one), nullptr, false));
                            allocate_dims.push_back(al, allocate_dim);
                        }
                    } else {
                        bool is_any_kind_8 = false;
                        ASR::expr_t * int_one = int32_one; 
                        if( ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(end)) == 8 || 
                            ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(start)) == 8 || 
                            ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(step)) == 8 ) {
                                is_any_kind_8 = true;
                        }
                        if( is_any_kind_8 ) {
                            int_one = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                                al, loc, 1, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 8))));
                        }
                        ASR::expr_t* end_minus_start = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc,
                            end, ASR::binopType::Sub, start, ASRUtils::expr_type(end), nullptr));
                        ASR::expr_t* by_step = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc,
                            end_minus_start, ASR::binopType::Div, step, ASRUtils::expr_type(end_minus_start),
                            nullptr));
                        ASR::expr_t* length = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc,
                            by_step, ASR::binopType::Add, int_one, ASRUtils::expr_type(by_step), nullptr));
                        allocate_dim.m_length = length;
                        allocate_dims.push_back(al, allocate_dim);
                    }
                }
                break;
            }
            case ASR::exprType::ArrayItem: {
                ASR::ArrayItem_t* array_item_t = ASR::down_cast<ASR::ArrayItem_t>(value);
                allocate_dims.reserve(al, array_item_t->n_args);
                for( size_t i = 0; i < array_item_t->n_args; i++ ) {
                    ASR::expr_t* start = array_item_t->m_args[i].m_left;
                    ASR::expr_t* end = array_item_t->m_args[i].m_right;
                    ASR::expr_t* step = array_item_t->m_args[i].m_step;
                    if( !(start == nullptr && step == nullptr && end != nullptr) ) {
                        continue ;
                    }
                    if( !ASRUtils::is_array(ASRUtils::expr_type(end)) ) {
                        continue ;
                    }
                    ASR::dimension_t allocate_dim;
                    allocate_dim.loc = loc;
                    allocate_dim.m_start = int32_one;
                    allocate_dim.m_length = ASRUtils::EXPR(ASRUtils::make_ArraySize_t_util(
                        al, loc, end, nullptr, ASRUtils::expr_type(int32_one), nullptr, false));
                    allocate_dims.push_back(al, allocate_dim);
                }
                break;
            }
            case ASR::exprType::IntrinsicElementalFunction: {
                ASR::IntrinsicElementalFunction_t* intrinsic_elemental_function =
                    ASR::down_cast<ASR::IntrinsicElementalFunction_t>(value);
                set_allocation_size_elemental_function(al, loc, intrinsic_elemental_function,
                            allocate_dims);
                break;
            }
            case ASR::exprType::IntrinsicArrayFunction: {
                ASR::IntrinsicArrayFunction_t* intrinsic_array_function =
                    ASR::down_cast<ASR::IntrinsicArrayFunction_t>(value);
                switch (intrinsic_array_function->m_arr_intrinsic_id) {
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::All):
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Any):
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Count):
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Parity):
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Sum):
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::MaxVal):
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::MinVal): {
                        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(
                            intrinsic_array_function->m_type);
                        allocate_dims.reserve(al, n_dims);
                        for( size_t i = 0; i < n_dims; i++ ) {
                            ASR::dimension_t allocate_dim;
                            allocate_dim.loc = loc;
                            allocate_dim.m_start = int32_one;
                            ASR::expr_t* size_i_1 = ASRUtils::EXPR(ASR::make_ArraySize_t(
                                al, loc, ASRUtils::get_past_array_physical_cast(intrinsic_array_function->m_args[0]),
                                ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                                    al, loc, i + 1, ASRUtils::expr_type(int32_one))),
                                ASRUtils::expr_type(int32_one), nullptr));
                            ASR::expr_t* size_i_2 = ASRUtils::EXPR(ASR::make_ArraySize_t(
                                al, loc, ASRUtils::get_past_array_physical_cast(intrinsic_array_function->m_args[0]),
                                ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                                    al, loc, i + 2, ASRUtils::expr_type(int32_one))),
                                ASRUtils::expr_type(int32_one), nullptr));
                            Vec<ASR::expr_t*> merge_i_args; merge_i_args.reserve(al, 3);
                            merge_i_args.push_back(al, size_i_1); merge_i_args.push_back(al, size_i_2);
                            merge_i_args.push_back(al, ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc,
                                ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                                    al, loc, i + 1, ASRUtils::expr_type(int32_one))), ASR::cmpopType::Lt,
                                    intrinsic_array_function->m_args[1],
                                    ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)), nullptr)));
                            ASR::expr_t* merge_i = ASRUtils::EXPR(ASRUtils::make_IntrinsicElementalFunction_t_util(
                                al, loc, static_cast<int64_t>(ASRUtils::IntrinsicElementalFunctions::Merge),
                                merge_i_args.p, merge_i_args.size(), 0, ASRUtils::expr_type(int32_one), nullptr));
                            allocate_dim.m_length = merge_i;
                            allocate_dims.push_back(al, allocate_dim);
                        }
                        break;
                    }
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Pack): {
                        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(
                            intrinsic_array_function->m_type);
                        allocate_dims.reserve(al, n_dims);
                        for ( size_t i = 0; i < n_dims; i++ ) {
                            ASR::dimension_t allocate_dim;
                            allocate_dim.loc = loc;
                            allocate_dim.m_start = int32_one;
                            ASR::expr_t* size_i_1 = nullptr;
                            if (intrinsic_array_function->n_args == 3) {
                                size_i_1 = ASRUtils::EXPR(ASR::make_ArraySize_t(
                                    al, loc, ASRUtils::get_past_array_physical_cast(intrinsic_array_function->m_args[2]),
                                    ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                                        al, loc, i + 1, ASRUtils::expr_type(int32_one))),
                                    ASRUtils::expr_type(int32_one), nullptr));
                            } else {
                                Vec<ASR::expr_t*> count_i_args; count_i_args.reserve(al, 1);
                                count_i_args.push_back(al, intrinsic_array_function->m_args[1]);
                                size_i_1 = ASRUtils::EXPR(ASRUtils::make_IntrinsicArrayFunction_t_util(
                                    al, loc, static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Count),
                                    count_i_args.p, count_i_args.size(), 0, ASRUtils::expr_type(int32_one), nullptr));
                            }
                            allocate_dim.m_length = size_i_1;
                            allocate_dims.push_back(al, allocate_dim);
                        }
                        break;
                    }
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Shape): {
                        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(
                            intrinsic_array_function->m_type);
                        allocate_dims.reserve(al, n_dims);
                        for( size_t i = 0; i < n_dims; i++ ) {
                            ASR::dimension_t allocate_dim;
                            allocate_dim.loc = loc;
                            allocate_dim.m_start = int32_one;
                            allocate_dim.m_length = int32_one;
                            allocate_dims.push_back(al, allocate_dim);
                        }
                        break;
                    }
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Transpose): {
                        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(intrinsic_array_function->m_type);
                        LCOMPILERS_ASSERT(n_dims == 2);
                        allocate_dims.reserve(al, n_dims);
                        // Transpose swaps the dimensions
                        for (size_t i = 0; i < n_dims; i++) {
                            ASR::dimension_t allocate_dim;
                            allocate_dim.loc = loc;
                            allocate_dim.m_start = int32_one;
                            ASR::expr_t* size_i = ASRUtils::EXPR(ASR::make_ArraySize_t(
                                al, loc, intrinsic_array_function->m_args[0],
                                ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                                    al, loc, n_dims - i, ASRUtils::expr_type(int32_one))),
                                ASRUtils::expr_type(int32_one), nullptr));
    
                            allocate_dim.m_length = size_i;
                            allocate_dims.push_back(al, allocate_dim);
                        }
                        break;
                    }
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Cshift): {
                        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(intrinsic_array_function->m_type);
                        allocate_dims.reserve(al, n_dims);
                        for (size_t i = 0; i < n_dims; i++) {
                            ASR::dimension_t allocate_dim;
                            allocate_dim.loc = loc;
                            allocate_dim.m_start = int32_one;
    
                            ASR::expr_t* size_i = ASRUtils::EXPR(ASR::make_ArraySize_t(
                                al, loc, intrinsic_array_function->m_args[0],
                                ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                                    al, loc, i + 1, ASRUtils::expr_type(int32_one))),
                                ASRUtils::expr_type(int32_one), nullptr));
    
                            allocate_dim.m_length = size_i;
                            allocate_dims.push_back(al, allocate_dim);
                        }
                        break;
                    }
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Spread): {
                        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(intrinsic_array_function->m_type);
                        ASR::expr_t* dim_arg = intrinsic_array_function->m_args[1];
                        allocate_dims.reserve(al, n_dims);
                        if (dim_arg && (ASRUtils::expr_value(dim_arg) != nullptr)) {
                            // Compile time value of `dim`
                            ASRUtils::ASRBuilder b(al, intrinsic_array_function->base.base.loc);
                            ASR::IntegerConstant_t* dim_val = ASR::down_cast<ASR::IntegerConstant_t>(dim_arg);
                            size_t dim_index = dim_val->m_n;
                            ASR::expr_t* ncopies = intrinsic_array_function->m_args[2];
                            int ncopies_inserted = 0;
                            for (size_t i = 0; i < n_dims; i++) {
                                ASR::dimension_t allocate_dim;
                                allocate_dim.loc = loc;
                                allocate_dim.m_start = int32_one;
                                if ( i == dim_index - 1 ) {
                                    allocate_dim.m_length = ncopies;
                                    ncopies_inserted = 1;
                                } else {
                                    allocate_dim.m_length = b.ArraySize(intrinsic_array_function->m_args[0],
                                            b.i32(i + 1 - ncopies_inserted), ASRUtils::expr_type(int32_one));
                                }
                                allocate_dims.push_back(al, allocate_dim);
                            }
                        } else {
                            // Here `dim` is runtime so can't decide where to insert ncopies
                            // Just copy original dimensions
                            ASR::dimension_t* dims;
                            ASRUtils::extract_dimensions_from_ttype(intrinsic_array_function->m_type, dims);
                            for (size_t i = 0; i < n_dims; i++) {
                                allocate_dims.push_back(al, dims[i]);
                            }
                        }
                        break;
                    }
    
                    default: {
                        LCOMPILERS_ASSERT_MSG(false, "ASR::IntrinsicArrayFunctions::" +
                            ASRUtils::get_array_intrinsic_name(intrinsic_array_function->m_arr_intrinsic_id)
                            + " not handled yet in set_allocation_size");
                    }
                }
                break;
            }
            case ASR::exprType::StructInstanceMember: {
                ASR::StructInstanceMember_t* struct_instance_member_t =
                    ASR::down_cast<ASR::StructInstanceMember_t>(value);
                size_t n_dims = ASRUtils::extract_n_dims_from_ttype(struct_instance_member_t->m_type);
                allocate_dims.reserve(al, n_dims);
                if( ASRUtils::is_array(ASRUtils::expr_type(struct_instance_member_t->m_v)) ) {
                    value = struct_instance_member_t->m_v;
                }
                ASRUtils::ExprStmtDuplicator expr_duplicator(al);
                for( size_t i = 0; i < n_dims; i++ ) {
                    ASR::dimension_t allocate_dim;
                    allocate_dim.loc = loc;
                    allocate_dim.m_start = int32_one;
                    allocate_dim.m_length = ASRUtils::EXPR(ASR::make_ArraySize_t(
                        al, loc, expr_duplicator.duplicate_expr(
                            ASRUtils::get_past_array_physical_cast(value)),
                            ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                            al, loc, i + 1, ASRUtils::expr_type(int32_one))),
                        ASRUtils::expr_type(int32_one), nullptr));
                    allocate_dims.push_back(al, allocate_dim);
                }
                break;
            }
            case ASR::exprType::ArrayReshape: {
                ASR::ArrayReshape_t* array_reshape_t = ASR::down_cast<ASR::ArrayReshape_t>(value);
                size_t n_dims = ASRUtils::get_fixed_size_of_array(
                    ASRUtils::expr_type(array_reshape_t->m_shape));
                allocate_dims.reserve(al, n_dims);
                ASRUtils::ASRBuilder b(al, array_reshape_t->base.base.loc);
                for( size_t i = 0; i < n_dims; i++ ) {
                    ASR::dimension_t allocate_dim;
                    allocate_dim.loc = loc;
                    allocate_dim.m_start = int32_one;
                    allocate_dim.m_length = b.ArrayItem_01(array_reshape_t->m_shape, {b.i32(i + 1)});
                    allocate_dims.push_back(al, allocate_dim);
                }
                break;
            }
            case ASR::exprType::ArrayConstructor: {
                allocate_dims.reserve(al, 1);
                ASR::dimension_t allocate_dim;
                allocate_dim.loc = loc;
                allocate_dim.m_start = int32_one;
                allocate_dim.m_length = ASRUtils::get_ArrayConstructor_size(al,
                    ASR::down_cast<ASR::ArrayConstructor_t>(value));
                allocate_dims.push_back(al, allocate_dim);
                break;
            }
            case ASR::exprType::ArrayConstant: {
                allocate_dims.reserve(al, 1);
                ASR::dimension_t allocate_dim;
                allocate_dim.loc = loc;
                allocate_dim.m_start = int32_one;
                allocate_dim.m_length = ASRUtils::get_ArrayConstant_size(al,
                    ASR::down_cast<ASR::ArrayConstant_t>(value));
                allocate_dims.push_back(al, allocate_dim);
                break;
            }
            case ASR::exprType::Var: {
                ASRUtils::ASRBuilder b(al, value->base.loc);
                if ( ASRUtils::is_array(ASRUtils::expr_type(value))) {
                    ASR::dimension_t* m_dims;
                    size_t n_dims = ASRUtils::extract_dimensions_from_ttype(
                        ASRUtils::expr_type(value), m_dims);
                    allocate_dims.reserve(al, n_dims);
                    for( size_t i = 0; i < n_dims; i++ ) {
                        ASR::dimension_t allocate_dim;
                        allocate_dim.loc = loc;
                        allocate_dim.m_start = int32_one;
                        allocate_dim.m_length = b.ArrayUBound(value, i+1);
                        allocate_dims.push_back(al, allocate_dim);
                    }
                } else {
                    return false;
                }
                break;
            }
            default: {
                LCOMPILERS_ASSERT_MSG(false, "ASR::exprType::" + std::to_string(value->type)
                    + " not handled yet in set_allocation_size");
            }
        }
        return true;
    }


    ASR::stmt_t* create_do_loop(Allocator &al, const Location &loc, std::vector<ASR::expr_t*> do_loop_variables, ASR::expr_t* left_arr, ASR::expr_t* right_arr, int curr_idx) {
        ASRUtils::ASRBuilder b(al, loc);

        if (curr_idx == 1) {
            std::vector<ASR::expr_t*> vars;
            for (size_t i = 0; i < do_loop_variables.size(); i++) {
                vars.push_back(do_loop_variables[i]);
            }
            return b.DoLoop(do_loop_variables[curr_idx - 1], LBound(left_arr, curr_idx), UBound(left_arr, curr_idx), {
                b.Assignment(b.ArrayItem_01(left_arr, vars), b.ArrayItem_01(right_arr, vars))
            }, nullptr);
        }
        return b.DoLoop(do_loop_variables[curr_idx - 1], LBound(left_arr, curr_idx), UBound(left_arr, curr_idx), {
            create_do_loop(al, loc, do_loop_variables, left_arr, right_arr, curr_idx - 1)
        }, nullptr);
    }

    void traverse_call_args(Vec<ASR::call_arg_t>& x_m_args_vec, ASR::call_arg_t* x_m_args,
        size_t x_n_args, const std::string& name_hint, std::vector<bool> is_arg_intent_out = {}, bool is_func_bind_c = false) {
        /* For other frontends, we might need to traverse the arguments
           in reverse order. */
        for( size_t i = 0; i < x_n_args; i++ ) {
            ASR::expr_t* arg_expr = x_m_args[i].m_value;
            if ( x_m_args[i].m_value && is_descriptor_array_casted_to_pointer_to_data(x_m_args[i].m_value) &&
                 !is_func_bind_c && !ASRUtils::is_pointer(ASRUtils::expr_type(x_m_args[i].m_value)) &&
                 !ASR::is_a<ASR::FunctionParam_t>(*ASRUtils::get_past_array_physical_cast(x_m_args[i].m_value)) ) {
                ASR::ArrayPhysicalCast_t* array_physical_cast = ASR::down_cast<ASR::ArrayPhysicalCast_t>(arg_expr);
                ASR::expr_t* arg_expr_past_cast = ASRUtils::get_past_array_physical_cast(arg_expr);
                const Location& loc = arg_expr->base.loc;
                ASR::expr_t* array_var_temporary = create_temporary_variable_for_array(
                    al, arg_expr_past_cast, current_scope, name_hint, true);
                ASR::call_arg_t array_var_temporary_arg;
                array_var_temporary_arg.loc = loc;
                if( ASRUtils::is_pointer(ASRUtils::expr_type(array_var_temporary)) ) {
                    ASR::expr_t* casted_array_var_temporary_arg = ASRUtils::EXPR(ASR::make_ArrayPhysicalCast_t(al, loc,
                        array_var_temporary, ASR::array_physical_typeType::DescriptorArray, ASR::array_physical_typeType::PointerToDataArray,
                        array_physical_cast->m_type, nullptr));
                    array_var_temporary_arg.m_value = casted_array_var_temporary_arg;
                    x_m_args_vec.push_back(al, array_var_temporary_arg);
                    // This should be always true as we pass `true` to `create_temporary_variable_for_array`
                    ASRUtils::ASRBuilder b(al, arg_expr_past_cast->base.loc);
                    Vec<ASR::dimension_t> allocate_dims; allocate_dims.reserve(al, 1);
                    size_t target_n_dims = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(array_var_temporary));
                    if( !set_allocation_size(al, arg_expr_past_cast, allocate_dims, target_n_dims) ) {
                        current_body->push_back(al, ASRUtils::STMT(ASR::make_Associate_t(
                            al, loc, array_var_temporary, arg_expr_past_cast)));
                        return;
                    }
                    LCOMPILERS_ASSERT(target_n_dims == allocate_dims.size());
                    Vec<ASR::alloc_arg_t> alloc_args; alloc_args.reserve(al, 1);
                    ASR::alloc_arg_t alloc_arg;
                    alloc_arg.loc = arg_expr_past_cast->base.loc;
                    alloc_arg.m_a = array_var_temporary;
                    alloc_arg.m_dims = allocate_dims.p;
                    alloc_arg.n_dims = allocate_dims.size();
                    alloc_arg.m_len_expr = nullptr;
                    alloc_arg.m_type = nullptr;
                    alloc_args.push_back(al, alloc_arg);
                
                    Vec<ASR::expr_t*> dealloc_args; dealloc_args.reserve(al, 1);
                    dealloc_args.push_back(al, array_var_temporary);
                    ASR::expr_t* is_contiguous = ASRUtils::EXPR(ASR::make_ArrayIsContiguous_t(al, loc,
                        arg_expr_past_cast, ASRUtils::expr_type(array_var_temporary), nullptr));
                    ASR::expr_t* not_is_contiguous = ASRUtils::EXPR(ASR::make_LogicalNot_t(al, loc, is_contiguous,
                        ASRUtils::expr_type(is_contiguous), nullptr));
                    ASR::dimension_t* array_dims = nullptr;
                    int array_rank = ASRUtils::extract_dimensions_from_ttype(ASRUtils::expr_type(array_var_temporary), array_dims);
                    std::vector<ASR::expr_t*> do_loop_variables;
                    #define declare(var_name, type, intent)                                     \
                    b.Variable(fn_symtab, var_name, type, ASR::intentType::intent)
                    for (int i = 0; i < array_rank; i++) {
                        std::string var_name = current_scope->get_unique_name("__lcompilers_i_" + std::to_string(i));
                        do_loop_variables.push_back(b.Variable(current_scope, var_name, ASRUtils::expr_type(b.i32(0)), ASR::intentType::Local));
                    }
                    current_body->push_back(al,
                        b.If(not_is_contiguous, {
                            ASRUtils::STMT(ASR::make_ExplicitDeallocate_t(al,
                                array_var_temporary->base.loc, dealloc_args.p, dealloc_args.size())),
                            ASRUtils::STMT(ASR::make_Allocate_t(al,
                                array_var_temporary->base.loc, alloc_args.p, alloc_args.size(),
                                nullptr, nullptr, nullptr)),
                                create_do_loop(al, loc, do_loop_variables, array_var_temporary, arg_expr_past_cast, array_rank)
                        }, {
                            ASRUtils::STMT(ASR::make_Associate_t(
                                al, loc, array_var_temporary, arg_expr_past_cast))
                        })
                    );
                    if ( is_arg_intent_out.size() > 0 && is_arg_intent_out[i] ) {
                        body_after_curr_stmt->push_back(al, b.If(not_is_contiguous, {
                            create_do_loop(al, loc, do_loop_variables, arg_expr_past_cast, array_var_temporary, array_rank)
                        }, {}));
                    }
                } else {
                    x_m_args_vec.push_back(al, x_m_args[i]);
                }
            } else {
                x_m_args_vec.push_back(al, x_m_args[i]);
            }
        }
    }

    template <typename T>
    void visit_Call(const T& x, const std::string& name_hint) {
        LCOMPILERS_ASSERT(!x.m_dt || !ASRUtils::is_array(ASRUtils::expr_type(x.m_dt)));
        Vec<ASR::call_arg_t> x_m_args; x_m_args.reserve(al, x.n_args);
        std::vector<bool> is_arg_intent_out;
        if ( ASR::is_a<ASR::Function_t>(*ASRUtils::symbol_get_past_external(x.m_name)) ) {
            ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(ASRUtils::symbol_get_past_external(x.m_name));
            ASR::FunctionType_t* func_type = ASR::down_cast<ASR::FunctionType_t>(func->m_function_signature);
            bool is_func_bind_c = func_type->m_abi == ASR::abiType::BindC || func_type->m_deftype == ASR::deftypeType::Interface;
            for (size_t i = 0; i < func->n_args; i++ ) {
                if ( ASR::is_a<ASR::Var_t>(*func->m_args[i]) ) {
                    ASR::Var_t* var_ = ASR::down_cast<ASR::Var_t>(func->m_args[i]);
                    if ( ASR::is_a<ASR::Variable_t>(*var_->m_v) ) {
                        ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(var_->m_v);
                        is_arg_intent_out.push_back(
                            var->m_intent == ASR::intentType::Out ||
                            var->m_intent == ASR::intentType::InOut ||
                            var->m_intent == ASR::intentType::Unspecified
                        );
                    } else {
                        is_arg_intent_out.push_back(false);
                    }
                } else {
                    is_arg_intent_out.push_back(false);
                }
            }
            traverse_call_args(x_m_args, x.m_args, x.n_args,
                name_hint + ASRUtils::symbol_name(x.m_name), is_arg_intent_out, is_func_bind_c);
        } else {
            traverse_call_args(x_m_args, x.m_args, x.n_args,
                name_hint + ASRUtils::symbol_name(x.m_name));
        }

        T& xx = const_cast<T&>(x);
        xx.m_args = x_m_args.p;
        xx.n_args = x_m_args.size();
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
        visit_Call(x, "_subroutine_call_");
        ASR::CallReplacerOnExpressionsVisitor<CallVisitor>::visit_SubroutineCall(x);
    }

    void visit_FunctionCall(const ASR::FunctionCall_t& x) {
        visit_Call(x, "_function_call_");
        ASR::CallReplacerOnExpressionsVisitor<CallVisitor>::visit_FunctionCall(x);
    }

};

void pass_replace_array_passed_in_function_call(Allocator &al, ASR::TranslationUnit_t &unit,
                        const LCompilers::PassOptions& /*pass_options*/) {
    CallVisitor v(al);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor x(al);
    x.visit_TranslationUnit(unit);
}


} // namespace LCompilers
