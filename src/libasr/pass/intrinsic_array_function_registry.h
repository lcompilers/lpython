#ifndef LFORTRAN_PASS_INTRINSIC_ARRAY_FUNCTIONS_H
#define LFORTRAN_PASS_INTRINSIC_ARRAY_FUNCTIONS_H

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/asr_utils.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/intrinsic_function_registry.h>

#include <cmath>
#include <string>
#include <tuple>

namespace LCompilers {

namespace ASRUtils {

/************************* Intrinsic Array Functions **************************/
enum class IntrinsicArrayFunctions : int64_t {
    Any,
    MaxLoc,
    MaxVal,
    Merge,
    MinLoc,
    MinVal,
    Product,
    Shape,
    Sum,
    // ...
};

#define ARRAY_INTRINSIC_NAME_CASE(X)                                            \
    case (static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::X)) : {       \
        return #X;                                                              \
    }

inline std::string get_array_intrinsic_name(int x) {
    switch (x) {
        ARRAY_INTRINSIC_NAME_CASE(Any)
        ARRAY_INTRINSIC_NAME_CASE(MaxLoc)
        ARRAY_INTRINSIC_NAME_CASE(MaxVal)
        ARRAY_INTRINSIC_NAME_CASE(Merge)
        ARRAY_INTRINSIC_NAME_CASE(MinLoc)
        ARRAY_INTRINSIC_NAME_CASE(MinVal)
        ARRAY_INTRINSIC_NAME_CASE(Product)
        ARRAY_INTRINSIC_NAME_CASE(Shape)
        ARRAY_INTRINSIC_NAME_CASE(Sum)
        default : {
            throw LCompilersException("pickle: intrinsic_id not implemented");
        }
    }
}

typedef ASR::expr_t* (ASRBuilder::*elemental_operation_func)(ASR::expr_t*,
    ASR::expr_t*, const Location&, ASR::expr_t*);

typedef void (*verify_array_func)(ASR::expr_t*, ASR::ttype_t*,
    const Location&, diag::Diagnostics&,
    ASRUtils::IntrinsicArrayFunctions);

typedef void (*verify_array_function)(
    const ASR::IntrinsicArrayFunction_t&,
    diag::Diagnostics&);

namespace ArrIntrinsic {

static inline void verify_array_int_real_cmplx(ASR::expr_t* array, ASR::ttype_t* return_type,
    const Location& loc, diag::Diagnostics& diagnostics, ASRUtils::IntrinsicArrayFunctions intrinsic_func_id) {
    std::string intrinsic_func_name = ASRUtils::get_intrinsic_name(static_cast<int>(intrinsic_func_id));
    ASR::ttype_t* array_type = ASRUtils::expr_type(array);
    ASRUtils::require_impl(ASRUtils::is_integer(*array_type) ||
        ASRUtils::is_real(*array_type) ||
        ASRUtils::is_complex(*array_type),
        "Input to " + intrinsic_func_name + " intrinsic must be of integer, real or complex type, found: " +
        ASRUtils::get_type_code(array_type), loc, diagnostics);
    int array_n_dims = ASRUtils::extract_n_dims_from_ttype(array_type);
    ASRUtils::require_impl(array_n_dims > 0, "Input to " + intrinsic_func_name + " intrinsic must always be an array",
        loc, diagnostics);
    ASRUtils::require_impl(ASRUtils::check_equal_type(
        return_type, array_type, false),
        intrinsic_func_name + " intrinsic must return an output of the same type as input", loc, diagnostics);
    int return_n_dims = ASRUtils::extract_n_dims_from_ttype(return_type);
    ASRUtils::require_impl(return_n_dims == 0,
    intrinsic_func_name + " intrinsic output for array only input should be a scalar, found an array of " +
    std::to_string(return_n_dims), loc, diagnostics);
}

static inline void verify_array_int_real(ASR::expr_t* array, ASR::ttype_t* return_type,
    const Location& loc, diag::Diagnostics& diagnostics, ASRUtils::IntrinsicArrayFunctions intrinsic_func_id) {
    std::string intrinsic_func_name = ASRUtils::get_intrinsic_name(static_cast<int>(intrinsic_func_id));
    ASR::ttype_t* array_type = ASRUtils::expr_type(array);
    ASRUtils::require_impl(ASRUtils::is_integer(*array_type) ||
        ASRUtils::is_real(*array_type),
        "Input to " + intrinsic_func_name + " intrinsic must be of integer or real type, found: " +
        ASRUtils::get_type_code(array_type), loc, diagnostics);
    int array_n_dims = ASRUtils::extract_n_dims_from_ttype(array_type);
    ASRUtils::require_impl(array_n_dims > 0, "Input to " + intrinsic_func_name + " intrinsic must always be an array",
        loc, diagnostics);
    ASRUtils::require_impl(ASRUtils::check_equal_type(
        return_type, array_type, false),
        intrinsic_func_name + " intrinsic must return an output of the same type as input", loc, diagnostics);
    int return_n_dims = ASRUtils::extract_n_dims_from_ttype(return_type);
    ASRUtils::require_impl(return_n_dims == 0,
    intrinsic_func_name + " intrinsic output for array only input should be a scalar, found an array of " +
    std::to_string(return_n_dims), loc, diagnostics);
}

static inline void verify_array_dim(ASR::expr_t* array, ASR::expr_t* dim,
    ASR::ttype_t* return_type, const Location& loc, diag::Diagnostics& diagnostics, ASRUtils::IntrinsicArrayFunctions intrinsic_func_id) {
    std::string intrinsic_func_name = ASRUtils::get_intrinsic_name(static_cast<int>(intrinsic_func_id));
    ASR::ttype_t* array_type = ASRUtils::expr_type(array);
    ASRUtils::require_impl(ASRUtils::is_integer(*array_type) ||
        ASRUtils::is_real(*array_type) ||
        ASRUtils::is_complex(*array_type),
        "Input to " + intrinsic_func_name + " intrinsic must be of integer, real or complex type, found: " +
        ASRUtils::get_type_code(array_type), loc, diagnostics);
    int array_n_dims = ASRUtils::extract_n_dims_from_ttype(array_type);
    ASRUtils::require_impl(array_n_dims > 0, "Input to " + intrinsic_func_name + " intrinsic must always be an array",
        loc, diagnostics);

    ASRUtils::require_impl(ASRUtils::is_integer(*ASRUtils::expr_type(dim)),
        "dim argument must be an integer", loc, diagnostics);

    ASRUtils::require_impl(ASRUtils::check_equal_type(
        return_type, array_type, false),
        intrinsic_func_name + " intrinsic must return an output of the same type as input", loc, diagnostics);
    int return_n_dims = ASRUtils::extract_n_dims_from_ttype(return_type);
    ASRUtils::require_impl(array_n_dims == return_n_dims + 1,
        intrinsic_func_name + " intrinsic output must return an array with dimension "
        "only 1 less than that of input array",
        loc, diagnostics);
}

static inline void verify_args(const ASR::IntrinsicArrayFunction_t& x, diag::Diagnostics& diagnostics,
    ASRUtils::IntrinsicArrayFunctions intrinsic_func_id, verify_array_func verify_array) {
    std::string intrinsic_func_name = ASRUtils::get_intrinsic_name(static_cast<int>(intrinsic_func_id));
    ASRUtils::require_impl(x.n_args >= 1, intrinsic_func_name + " intrinsic must accept at least one argument",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(x.m_args[0] != nullptr, "Array argument to " + intrinsic_func_name + " intrinsic cannot be nullptr",
        x.base.base.loc, diagnostics);
    const int64_t id_array = 0, id_array_dim = 1, id_array_mask = 2;
    const int64_t id_array_dim_mask = 3;
    switch( x.m_overload_id ) {
        case id_array:
        case id_array_mask: {
            if( x.m_overload_id == id_array_mask ) {
                ASRUtils::require_impl(x.n_args == 2 && x.m_args[1] != nullptr,
                    "mask argument cannot be nullptr", x.base.base.loc, diagnostics);
            }
            verify_array(x.m_args[0], x.m_type, x.base.base.loc, diagnostics, intrinsic_func_id);
            break;
        }
        case id_array_dim:
        case id_array_dim_mask: {
            if( x.m_overload_id == id_array_dim_mask ) {
                ASRUtils::require_impl(x.n_args == 3 && x.m_args[2] != nullptr,
                    "mask argument cannot be nullptr", x.base.base.loc, diagnostics);
            }
            ASRUtils::require_impl(x.n_args >= 2 && x.m_args[1] != nullptr,
                "dim argument to any intrinsic cannot be nullptr",
                x.base.base.loc, diagnostics);
            verify_array_dim(x.m_args[0], x.m_args[1], x.m_type, x.base.base.loc, diagnostics, intrinsic_func_id);
            break;
        }
        default: {
            require_impl(false, "Unrecognised overload id in " + intrinsic_func_name + " intrinsic",
                         x.base.base.loc, diagnostics);
        }
    }
    if( x.m_overload_id == id_array_mask ||
        x.m_overload_id == id_array_dim_mask ) {
        ASR::expr_t* mask = nullptr;
        if( x.m_overload_id == id_array_mask ) {
            mask = x.m_args[1];
        } else if( x.m_overload_id == id_array_dim_mask ) {
            mask = x.m_args[2];
        }
        ASR::dimension_t *array_dims, *mask_dims;
        ASR::ttype_t* array_type = ASRUtils::expr_type(x.m_args[0]);
        ASR::ttype_t* mask_type = ASRUtils::expr_type(mask);
        size_t array_n_dims = ASRUtils::extract_dimensions_from_ttype(array_type, array_dims);
        size_t mask_n_dims = ASRUtils::extract_dimensions_from_ttype(mask_type, mask_dims);
        ASRUtils::require_impl(ASRUtils::dimensions_equal(array_dims, array_n_dims, mask_dims, mask_n_dims),
            "The dimensions of array and mask arguments of " + intrinsic_func_name + " intrinsic must be same",
            x.base.base.loc, diagnostics);
    }
}

static inline ASR::expr_t *eval_ArrIntrinsic(Allocator & /*al*/,
    const Location & /*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/) {
    return nullptr;
}

static inline ASR::asr_t* create_ArrIntrinsic(
    Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err,
    ASRUtils::IntrinsicArrayFunctions intrinsic_func_id) {
    std::string intrinsic_func_name = ASRUtils::get_intrinsic_name(static_cast<int>(intrinsic_func_id));
    int64_t id_array = 0, id_array_dim = 1, id_array_mask = 2;
    int64_t id_array_dim_mask = 3;
    int64_t overload_id = id_array;

    ASR::expr_t* array = args[0];
    ASR::expr_t *arg2 = nullptr, *arg3 = nullptr;
    if( args.size() >= 2 ) {
        arg2 = args[1];
    }
    if( args.size() == 3 ) {
        arg3 = args[2];
    }

    if( !arg2 && arg3 ) {
        std::swap(arg2, arg3);
    }

    ASR::ttype_t* array_type = ASRUtils::expr_type(array);
    if( arg2 && !arg3 ) {
        size_t arg2_rank = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(arg2));
        if( arg2_rank == 0 ) {
            overload_id = id_array_dim;
        } else {
            overload_id = id_array_mask;
        }
    } else if( arg2 && arg3 ) {
        ASR::expr_t* arg2 = args[1];
        ASR::expr_t* arg3 = args[2];
        size_t arg2_rank = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(arg2));
        size_t arg3_rank = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(arg3));

        if( arg2_rank != 0 ) {
            err("dim argument to " + intrinsic_func_name + " must be a scalar and must not be an array",
                arg2->base.loc);
        }

        if( arg3_rank == 0 ) {
            err("mask argument to " + intrinsic_func_name + " must be an array and must not be a scalar",
                arg3->base.loc);
        }

        overload_id = id_array_dim_mask;
    }

    // TODO: Add a check for range of values axis can take
    // if axis is available at compile time

    ASR::expr_t *value = nullptr;
    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, 3);
    ASR::expr_t *array_value = ASRUtils::expr_value(array);
    arg_values.push_back(al, array_value);
    if( arg2 ) {
        ASR::expr_t *arg2_value = ASRUtils::expr_value(arg2);
        arg_values.push_back(al, arg2_value);
    }
    if( arg3 ) {
        ASR::expr_t* mask = arg3;
        ASR::expr_t *mask_value = ASRUtils::expr_value(mask);
        arg_values.push_back(al, mask_value);
    }

    ASR::ttype_t* return_type = nullptr;
    if( overload_id == id_array ||
        overload_id == id_array_mask ) {
        ASR::ttype_t* type = ASRUtils::type_get_past_allocatable(
            ASRUtils::type_get_past_pointer(array_type));
        return_type = ASRUtils::duplicate_type_without_dims(
                        al, type, loc);
    } else if( overload_id == id_array_dim ||
               overload_id == id_array_dim_mask ) {
        Vec<ASR::dimension_t> dims;
        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(array_type);
        dims.reserve(al, (int) n_dims - 1);
        for( int i = 0; i < (int) n_dims - 1; i++ ) {
            ASR::dimension_t dim;
            dim.loc = array->base.loc;
            dim.m_length = nullptr;
            dim.m_start = nullptr;
            dims.push_back(al, dim);
        }
        return_type = ASRUtils::duplicate_type(al, array_type, &dims);
    }
    value = eval_ArrIntrinsic(al, loc, return_type, arg_values);

    Vec<ASR::expr_t*> arr_intrinsic_args;
    arr_intrinsic_args.reserve(al, 3);
    arr_intrinsic_args.push_back(al, array);
    if( arg2 ) {
        arr_intrinsic_args.push_back(al, arg2);
    }
    if( arg3 ) {
        arr_intrinsic_args.push_back(al, arg3);
    }

    return ASRUtils::make_IntrinsicArrayFunction_t_util(al, loc,
        static_cast<int64_t>(intrinsic_func_id),
        arr_intrinsic_args.p, arr_intrinsic_args.n, overload_id, return_type, value);
}

static inline void generate_body_for_array_input(Allocator& al, const Location& loc,
    ASR::expr_t* array, ASR::expr_t* return_var, SymbolTable* fn_scope,
    Vec<ASR::stmt_t*>& fn_body, get_initial_value_func get_initial_value, elemental_operation_func elemental_operation) {
    ASRBuilder builder(al, loc);
    Vec<ASR::expr_t*> idx_vars;
    Vec<ASR::stmt_t*> doloop_body;
    builder.generate_reduction_intrinsic_stmts_for_scalar_output(loc,
        array, fn_scope, fn_body, idx_vars, doloop_body,
        [=, &al, &fn_body, &builder] {
            ASR::ttype_t* array_type = ASRUtils::expr_type(array);
            ASR::ttype_t* element_type = ASRUtils::duplicate_type_without_dims(al, array_type, loc);
            ASR::expr_t* initial_val = get_initial_value(al, element_type);
            ASR::stmt_t* return_var_init = builder.Assignment(return_var, initial_val);
            fn_body.push_back(al, return_var_init);
        },
        [=, &al, &idx_vars, &doloop_body, &builder] () {
            ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
            ASR::expr_t* elemental_operation_val = (builder.*elemental_operation)(return_var, array_ref, loc, nullptr);
            ASR::stmt_t* loop_invariant = builder.Assignment(return_var, elemental_operation_val);
            doloop_body.push_back(al, loop_invariant);
    });
}

static inline void generate_body_for_array_mask_input(Allocator& al, const Location& loc,
    ASR::expr_t* array, ASR::expr_t* mask, ASR::expr_t* return_var, SymbolTable* fn_scope,
    Vec<ASR::stmt_t*>& fn_body, get_initial_value_func get_initial_value, elemental_operation_func elemental_operation) {
    ASRBuilder builder(al, loc);
    Vec<ASR::expr_t*> idx_vars;
    Vec<ASR::stmt_t*> doloop_body;
    builder.generate_reduction_intrinsic_stmts_for_scalar_output(loc,
        array, fn_scope, fn_body, idx_vars, doloop_body,
        [=, &al, &fn_body, &builder] {
            ASR::ttype_t* array_type = ASRUtils::expr_type(array);
            ASR::ttype_t* element_type = ASRUtils::duplicate_type_without_dims(al, array_type, loc);
            ASR::expr_t* initial_val = get_initial_value(al, element_type);
            ASR::stmt_t* return_var_init = builder.Assignment(return_var, initial_val);
            fn_body.push_back(al, return_var_init);
        },
        [=, &al, &idx_vars, &doloop_body, &builder] () {
            ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
            ASR::expr_t* mask_ref = PassUtils::create_array_ref(mask, idx_vars, al);
            ASR::expr_t* elemental_operation_val = (builder.*elemental_operation)(return_var, array_ref, loc, nullptr);
            ASR::stmt_t* loop_invariant = builder.Assignment(return_var, elemental_operation_val);
            Vec<ASR::stmt_t*> if_mask;
            if_mask.reserve(al, 1);
            if_mask.push_back(al, loop_invariant);
            ASR::stmt_t* if_mask_ = ASRUtils::STMT(ASR::make_If_t(al, loc,
                                        mask_ref, if_mask.p, if_mask.size(),
                                        nullptr, 0));
            doloop_body.push_back(al, if_mask_);
    });
}

static inline void generate_body_for_array_dim_input(
    Allocator& al, const Location& loc,
    ASR::expr_t* array, ASR::expr_t* dim, ASR::expr_t* result,
    SymbolTable* fn_scope, Vec<ASR::stmt_t*>& fn_body, get_initial_value_func get_initial_value,
    elemental_operation_func elemental_operation) {
    ASRBuilder builder(al, loc);
    Vec<ASR::expr_t*> idx_vars, target_idx_vars;
    Vec<ASR::stmt_t*> doloop_body;
    builder.generate_reduction_intrinsic_stmts_for_array_output(
        loc, array, dim, fn_scope, fn_body,
        idx_vars, target_idx_vars, doloop_body,
        [=, &al, &fn_body, &builder] () {
            ASR::ttype_t* array_type = ASRUtils::expr_type(array);
            ASR::expr_t* initial_val = get_initial_value(al, array_type);
            ASR::stmt_t* result_init = builder.Assignment(result, initial_val);
            fn_body.push_back(al, result_init);
        },
        [=, &al, &idx_vars, &target_idx_vars, &doloop_body, &builder, &result] () {
            ASR::expr_t* result_ref = PassUtils::create_array_ref(result, target_idx_vars, al);
            ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
            ASR::expr_t* elemental_operation_val = (builder.*elemental_operation)(result_ref, array_ref, loc, nullptr);
            ASR::stmt_t* loop_invariant = builder.Assignment(result_ref, elemental_operation_val);
            doloop_body.push_back(al, loop_invariant);
        });
}

static inline void generate_body_for_array_dim_mask_input(
    Allocator& al, const Location& loc,
    ASR::expr_t* array, ASR::expr_t* dim,
    ASR::expr_t* mask, ASR::expr_t* result,
    SymbolTable* fn_scope, Vec<ASR::stmt_t*>& fn_body,
    get_initial_value_func get_initial_value, elemental_operation_func elemental_operation) {
    ASRBuilder builder(al, loc);
    Vec<ASR::expr_t*> idx_vars, target_idx_vars;
    Vec<ASR::stmt_t*> doloop_body;
    builder.generate_reduction_intrinsic_stmts_for_array_output(
        loc, array, dim, fn_scope, fn_body,
        idx_vars, target_idx_vars, doloop_body,
        [=, &al, &fn_body, &builder] () {
            ASR::ttype_t* array_type = ASRUtils::expr_type(array);
            ASR::expr_t* initial_val = get_initial_value(al, array_type);
            ASR::stmt_t* result_init = builder.Assignment(result, initial_val);
            fn_body.push_back(al, result_init);
        },
        [=, &al, &idx_vars, &target_idx_vars, &doloop_body, &builder, &result] () {
            ASR::expr_t* result_ref = PassUtils::create_array_ref(result, target_idx_vars, al);
            ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
            ASR::expr_t* mask_ref = PassUtils::create_array_ref(mask, idx_vars, al);
            ASR::expr_t* elemental_operation_val = (builder.*elemental_operation)(result_ref, array_ref, loc, nullptr);
            ASR::stmt_t* loop_invariant = builder.Assignment(result_ref, elemental_operation_val);
            Vec<ASR::stmt_t*> if_mask;
            if_mask.reserve(al, 1);
            if_mask.push_back(al, loop_invariant);
            ASR::stmt_t* if_mask_ = ASRUtils::STMT(ASR::make_If_t(al, loc,
                                        mask_ref, if_mask.p, if_mask.size(),
                                        nullptr, 0));
            doloop_body.push_back(al, if_mask_);
        }
    );
}

static inline ASR::expr_t* instantiate_ArrIntrinsic(Allocator &al,
        const Location &loc, SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
        ASR::ttype_t *return_type, Vec<ASR::call_arg_t>& new_args,
        int64_t overload_id, ASRUtils::IntrinsicArrayFunctions intrinsic_func_id,
        get_initial_value_func get_initial_value,
        elemental_operation_func elemental_operation) {
    std::string intrinsic_func_name = ASRUtils::get_intrinsic_name(static_cast<int>(intrinsic_func_id));
    ASRBuilder builder(al, loc);
    ASRBuilder& b = builder;
    int64_t id_array = 0, id_array_dim = 1, id_array_mask = 2;
    int64_t id_array_dim_mask = 3;

    ASR::ttype_t* arg_type = ASRUtils::type_get_past_allocatable(
        ASRUtils::type_get_past_pointer(arg_types[0]));
    int kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
    int rank = ASRUtils::extract_n_dims_from_ttype(arg_type);
    std::string new_name = intrinsic_func_name + "_" + std::to_string(kind) +
                            "_" + std::to_string(rank) +
                            "_" + std::to_string(overload_id);
    // Check if Function is already defined.
    {
        std::string new_func_name = new_name;
        int i = 1;
        while (scope->get_symbol(new_func_name) != nullptr) {
            ASR::symbol_t *s = scope->get_symbol(new_func_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            int orig_array_rank = ASRUtils::extract_n_dims_from_ttype(
                                    ASRUtils::expr_type(f->m_args[0]));
            if (ASRUtils::types_equal(ASRUtils::expr_type(f->m_args[0]),
                    arg_type) && orig_array_rank == rank) {
                return builder.Call(s, new_args, return_type, nullptr);
            } else {
                new_func_name += std::to_string(i);
                i++;
            }
        }
    }

    new_name = scope->get_unique_name(new_name, false);
    SymbolTable *fn_symtab = al.make_new<SymbolTable>(scope);

    Vec<ASR::expr_t*> args;
    args.reserve(al, 1);

    ASR::ttype_t* array_type = ASRUtils::duplicate_type_with_empty_dims(al, arg_type);
    fill_func_arg("array", array_type)
    if( overload_id == id_array_dim ||
        overload_id == id_array_dim_mask ) {
        ASR::ttype_t* dim_type = ASRUtils::TYPE(ASR::make_Integer_t(
                                    al, arg_type->base.loc, 4));
        fill_func_arg("dim", dim_type)
    }
    if( overload_id == id_array_mask ||
        overload_id == id_array_dim_mask ) {
        Vec<ASR::dimension_t> mask_dims;
        mask_dims.reserve(al, rank);
        for( int i = 0; i < rank; i++ ) {
            ASR::dimension_t mask_dim;
            mask_dim.loc = arg_type->base.loc;
            mask_dim.m_start = nullptr;
            mask_dim.m_length = nullptr;
            mask_dims.push_back(al, mask_dim);
        }
        ASR::ttype_t* mask_type = ASRUtils::TYPE(ASR::make_Logical_t(
                                    al, arg_type->base.loc, 4));
        if( mask_dims.size() > 0 ) {
            mask_type = ASRUtils::make_Array_t_util(
                            al, arg_type->base.loc, mask_type,
                            mask_dims.p, mask_dims.size());
        }
        fill_func_arg("mask", mask_type)
    }

    int result_dims = extract_n_dims_from_ttype(return_type);
    ASR::expr_t* return_var = nullptr;
    if( result_dims > 0 ) {
        fill_func_arg("result", return_type)
    } else if( result_dims == 0 ) {
        return_var = declare("result", return_type, ReturnVar);
    }

    Vec<ASR::stmt_t*> body;
    body.reserve(al, 1);
    ASR::expr_t* output_var = nullptr;
    if( return_var ) {
        output_var = return_var;
    } else {
        output_var = args[(int) args.size() - 1];
    }
    if( overload_id == id_array ) {
        generate_body_for_array_input(al, loc, args[0], output_var,
                                      fn_symtab, body, get_initial_value, elemental_operation);
    } else if( overload_id == id_array_dim ) {
        generate_body_for_array_dim_input(al, loc, args[0], args[1], output_var,
                                          fn_symtab, body, get_initial_value, elemental_operation);
    } else if( overload_id == id_array_dim_mask ) {
        generate_body_for_array_dim_mask_input(al, loc, args[0], args[1], args[2],
                                               output_var, fn_symtab, body, get_initial_value, elemental_operation);
    } else if( overload_id == id_array_mask ) {
        generate_body_for_array_mask_input(al, loc, args[0], args[1], output_var,
                                           fn_symtab, body, get_initial_value, elemental_operation);
    }

    Vec<char *> dep;
    dep.reserve(al, 1);
    // TODO: fill dependencies

    ASR::symbol_t *new_symbol = nullptr;
    if( return_var ) {
        new_symbol = make_Function_t(new_name, fn_symtab, dep, args,
            body, return_var, Source, Implementation, nullptr);
    } else {
        new_symbol = make_Function_Without_ReturnVar_t(
            new_name, fn_symtab, dep, args,
            body, Source, Implementation, nullptr);
    }
    scope->add_symbol(new_name, new_symbol);
    return builder.Call(new_symbol, new_args, return_type, nullptr);
}

static inline void verify_MaxMinLoc_args(const ASR::IntrinsicArrayFunction_t& x,
        diag::Diagnostics& diagnostics) {
    std::string intrinsic_name = get_intrinsic_name(
        static_cast<int>(x.m_arr_intrinsic_id));
    require_impl(x.n_args >= 1, "`"+ intrinsic_name +"` intrinsic "
        "must accept at least one argument", x.base.base.loc, diagnostics);
    require_impl(x.m_args[0], "`array` argument of `"+ intrinsic_name
        + "` intrinsic cannot be nullptr", x.base.base.loc, diagnostics);
    require_impl(x.m_args[1], "`dim` argument of `" + intrinsic_name
        + "` intrinsic cannot be nullptr", x.base.base.loc, diagnostics);
}

static inline ASR::expr_t *eval_MaxMinLoc(Allocator &al, const Location &loc,
        ASR::ttype_t *type, Vec<ASR::expr_t*> &args, int intrinsic_id) {
    ASRBuilder b(al, loc);
    if (all_args_evaluated(args) &&
            extract_n_dims_from_ttype(expr_type(args[0])) == 1) {
        // Only supported for arrays with rank 1
        ASR::ArrayConstant_t *arr = ASR::down_cast<ASR::ArrayConstant_t>(args[0]);
        std::vector<double> m_eles;
        for (size_t i = 0; i < arr->n_args; i++) {
            double ele = 0;
            if(extract_value(arr->m_args[i], ele)) {
                m_eles.push_back(ele);
            }
        }
        int index = 0;
        if (static_cast<int>(IntrinsicArrayFunctions::MaxLoc) == intrinsic_id) {
            index = std::distance(m_eles.begin(),
                std::max_element(m_eles.begin(), m_eles.end())) + 1;
        } else {
            index = std::distance(m_eles.begin(),
                std::min_element(m_eles.begin(), m_eles.end())) + 1;
        }
        if (!is_array(type)) {
            return i(index, type);
        } else {
            return b.ArrayConstant({i32(index)}, extract_type(type), false);
        }
    } else {
        return nullptr;
    }
}

static inline ASR::asr_t* create_MaxMinLoc(Allocator& al, const Location& loc,
        Vec<ASR::expr_t*>& args, int intrinsic_id,
        const std::function<void (const std::string &, const Location &)> err) {
    std::string intrinsic_name = get_intrinsic_name(static_cast<int>(intrinsic_id));
    ASR::ttype_t *array_type = expr_type(args[0]);
    if ( !is_array(array_type) ) {
        err("`array` argument of `"+ intrinsic_name +"` must be an array", loc);
    } else if ( !is_integer(*array_type) && !is_real(*array_type) ) {
        err("`array` argument of `"+ intrinsic_name +"` must be integer or "
            "real for now", loc);
    } else if ( args[2] || args[4] ) {
        err("`mask` and `back` keyword argument is not supported yet", loc);
    }
    ASR::ttype_t *return_type = nullptr;
    Vec<ASR::expr_t *> m_args; m_args.reserve(al, 1);
    m_args.push_back(al, args[0]);
    Vec<ASR::dimension_t> result_dims; result_dims.reserve(al, 1);
    ASR::dimension_t *m_dims;
    int n_dims = extract_dimensions_from_ttype(array_type, m_dims);
    int dim = 0, kind = 4; // default kind
    if (args[3]) {
        if (!extract_value(expr_value(args[3]), kind)) {
            err("Runtime value for `kind` argument is not supported yet", loc);
        }
    }
    if ( args[1] ) {
        if ( !ASR::is_a<ASR::Integer_t>(*expr_type(args[1])) ) {
            err("`dim` should be a scalar integer type", loc);
        } else if (!extract_value(expr_value(args[1]), dim)) {
            err("Runtime values for `dim` argument is not supported yet", loc);
        }
        if ( 1 > dim || dim > n_dims ) {
            err("`dim` argument of `"+ intrinsic_name +"` is out of "
                "array index range", loc);
        }
        if ( n_dims == 1 ) {
            return_type = TYPE(ASR::make_Integer_t(al, loc, kind)); // 1D
        } else {
            for ( int i = 1; i <= n_dims; i++ ) {
                if ( i == dim ) {
                    continue;
                }
                ASR::dimension_t tmp_dim;
                tmp_dim.loc = args[0]->base.loc;
                tmp_dim.m_start = m_dims[i - 1].m_start;
                tmp_dim.m_length = m_dims[i - 1].m_length;
                result_dims.push_back(al, tmp_dim);
            }
        }
        m_args.push_back(al, args[1]);
    } else {
        ASR::dimension_t tmp_dim;
        tmp_dim.loc = args[0]->base.loc;
        tmp_dim.m_start = i32(1);
        tmp_dim.m_length = i32(n_dims);
        result_dims.push_back(al, tmp_dim);
    }
    if ( !return_type ) {
        return_type = duplicate_type(al, TYPE(
            ASR::make_Integer_t(al, loc, kind)), &result_dims);
    }
    ASR::expr_t *m_value = eval_MaxMinLoc(al, loc, return_type, m_args,
        intrinsic_id);
    return make_IntrinsicArrayFunction_t_util(al, loc,
        intrinsic_id, m_args.p, m_args.n, 0, return_type, m_value);
}

static inline ASR::expr_t *instantiate_MaxMinLoc(Allocator &al,
        const Location &loc, SymbolTable *scope, int intrinsic_id,
        Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& m_args, int64_t /*overload_id*/) {
    std::string intrinsic_name = get_intrinsic_name(static_cast<int>(intrinsic_id));
    declare_basic_variables("_lcompilers_" + intrinsic_name)
    /*
     * max_index = 1; min_index
     * do i = 1, size(arr))
     *  do ...
     *    if (arr[i] > arr[max_index]) then
     *      max_index = i;
     *    end if
     * ------------------------------------
     *    if (arr[i] < arr[max_index]) then
     *      min_index = i;
     *    end if
     *  end ...
     * end do
     */
    fill_func_arg("array", arg_types[0]);
    int n_dims = extract_n_dims_from_ttype(arg_types[0]);
    ASR::ttype_t *type = extract_type(return_type);
    if (m_args.n > 1) {
        // TODO: Use overload_id
        fill_func_arg("dim", arg_types[1]);
    }
    ASR::expr_t *result = declare("result", return_type, ReturnVar);
    Vec<ASR::expr_t*> idx_vars, target_idx_vars;
    Vec<ASR::stmt_t*> doloop_body;
    if (m_args.n == 1) {
        b.generate_reduction_intrinsic_stmts_for_scalar_output(
            loc, args[0], fn_symtab, body, idx_vars, doloop_body,
            [=, &al, &body, &b] () {
                body.push_back(al, b.Assignment(result, i(1, type)));
            }, [=, &al, &b, &idx_vars, &doloop_body] () {
                std::vector<ASR::stmt_t *> if_body; if_body.reserve(n_dims);
                Vec<ASR::expr_t *> result_idx; result_idx.reserve(al, n_dims);
                for (int i = 0; i < n_dims; i++) {
                    ASR::expr_t *idx = b.ArrayItem_01(result, {i32(i+1)});
                    if (extract_kind_from_ttype_t(type) != 4) {
                        if_body.push_back(b.Assignment(idx, i2i(idx_vars[i], type)));
                        result_idx.push_back(al, i2i32(idx));
                    } else {
                        if_body.push_back(b.Assignment(idx, idx_vars[i]));
                        result_idx.push_back(al, idx);
                    }
                }
                ASR::expr_t *array_ref_01 = ArrayItem_02(args[0], idx_vars);
                ASR::expr_t *array_ref_02 = ArrayItem_02(args[0], result_idx);
                if (static_cast<int>(IntrinsicArrayFunctions::MaxLoc) == intrinsic_id) {
                    doloop_body.push_back(al, b.If(b.Gt(array_ref_01,
                        array_ref_02), if_body, {}));
                } else {
                    doloop_body.push_back(al, b.If(b.Lt(array_ref_01,
                        array_ref_02), if_body, {}));
                }
            });
    } else {
        int dim = 0;
        extract_value(expr_value(m_args[1].m_value), dim);
        b.generate_reduction_intrinsic_stmts_for_array_output(
            loc, args[0], args[1], fn_symtab, body, idx_vars,
            target_idx_vars, doloop_body,
            [=, &al, &body, &b] () {
                body.push_back(al, b.Assignment(result, i(1, type)));
            }, [=, &al, &b, &idx_vars, &target_idx_vars, &doloop_body] () {
                ASR::expr_t *result_ref, *array_ref_02;
                if (is_array(return_type)) {
                    result_ref = ArrayItem_02(result, target_idx_vars);
                    Vec<ASR::expr_t*> tmp_idx_vars;
                    tmp_idx_vars.from_pointer_n_copy(al, idx_vars.p, idx_vars.n);
                    tmp_idx_vars.p[dim - 1] = i2i32(result_ref);
                    array_ref_02 = ArrayItem_02(args[0], tmp_idx_vars);
                } else {
                    // 1D scalar output
                    result_ref = result;
                    array_ref_02 = b.ArrayItem_01(args[0], {result});
                }
                ASR::expr_t *array_ref_01 = ArrayItem_02(args[0], idx_vars);
                ASR::expr_t *res_idx = idx_vars.p[dim - 1];
                if (extract_kind_from_ttype_t(type) != 4) {
                    res_idx = i2i(res_idx, type);
                }
                if (static_cast<int>(IntrinsicArrayFunctions::MaxLoc) == intrinsic_id) {
                    doloop_body.push_back(al, b.If(b.Gt(array_ref_01, array_ref_02), {
                        b.Assignment(result_ref, res_idx)
                    }, {}));
                } else {
                    doloop_body.push_back(al, b.If(b.Lt(array_ref_01, array_ref_02), {
                        b.Assignment(result_ref, res_idx)
                    }, {}));
                }
            });
    }
    body.push_back(al, Return());
    ASR::symbol_t *fn_sym = make_Function_t(fn_name, fn_symtab, dep, args,
            body, result, Source, Implementation, nullptr);
    scope->add_symbol(fn_name, fn_sym);
    return b.Call(fn_sym, m_args, return_type, nullptr);
}

} // namespace ArrIntrinsic

namespace Shape {
    static inline void verify_args(const ASR::IntrinsicArrayFunction_t &x,
            diag::Diagnostics &diagnostics) {
        ASRUtils::require_impl(x.n_args == 1 || x.n_args == 2,
            "`shape` intrinsic accepts either 1 or 2 arguments",
            x.base.base.loc, diagnostics);
        ASRUtils::require_impl(x.m_args[0], "`source` argument of `shape` "
            "cannot be nullptr", x.base.base.loc, diagnostics);
        ASRUtils::require_impl(x.m_args[1], "`kind` argument of `shape` "
            "cannot be nullptr", x.base.base.loc, diagnostics);
    }

    static ASR::expr_t *eval_Shape(Allocator &al, const Location &loc,
            ASR::ttype_t *type, Vec<ASR::expr_t*> &args) {
        ASR::dimension_t *m_dims;
        size_t n_dims = extract_dimensions_from_ttype(expr_type(args[0]), m_dims);
        Vec<ASR::expr_t *> m_shapes; m_shapes.reserve(al, n_dims);
        for (size_t i = 0; i < n_dims; i++) {
            if (m_dims[i].m_length) {
                ASR::expr_t *e = nullptr;
                if (extract_kind_from_ttype_t(type) != 4) {
                    e = i2i(m_dims[i].m_length, extract_type(type));
                } else {
                    e = m_dims[i].m_length;
                }
                m_shapes.push_back(al, e);
            }
        }
        ASR::expr_t *value = nullptr;
        if (m_shapes.n > 0) {
            value = EXPR(ASR::make_ArrayConstant_t(al, loc, m_shapes.p, m_shapes.n,
                type, ASR::arraystorageType::ColMajor));
        }
        return value;
    }

    static inline ASR::asr_t* create_Shape(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        ASRBuilder b(al, loc);
        Vec<ASR::expr_t *>m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        int kind = 4; // default kind
        if (args[1]) {
            if (!ASR::is_a<ASR::Integer_t>(*expr_type(args[1]))) {
                err("`kind` argument of `shape` must be a scalar integer", loc);
            }
            if (!extract_value(args[1], kind)) {
                err("Only constant value for `kind` is supported for now", loc);
            }
        }
        // TODO: throw error for assumed size array
        int n_dims = extract_n_dims_from_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = b.Array({n_dims},
            TYPE(ASR::make_Integer_t(al, loc, kind)));
        ASR::expr_t *m_value = eval_Shape(al, loc, return_type, args);

        return ASRUtils::make_IntrinsicArrayFunction_t_util(al, loc,
            static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Shape),
            m_args.p, m_args.n, 0, return_type, m_value);
    }

    static inline ASR::expr_t* instantiate_Shape(Allocator &al,
            const Location &loc, SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            ASR::ttype_t *return_type, Vec<ASR::call_arg_t>& new_args,
            int64_t, ASR::expr_t* compile_time_value) {
        if (compile_time_value) {
            return compile_time_value;
        }
        declare_basic_variables("_lcompilers_shape");
        fill_func_arg("source", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        int iter = extract_n_dims_from_ttype(arg_types[0]) + 1;
        auto i = declare("i", int32, Local);
        body.push_back(al, b.Assignment(i, i32(1)));
        body.push_back(al, b.While(iLt(i, i32(iter)), {
            b.Assignment(b.ArrayItem_01(result, {i}),
                ArraySize_2(args[0], i, extract_type(return_type))),
            b.Assignment(i, iAdd(i, i32(1)))
        }));
        body.push_back(al, Return());

        ASR::symbol_t *f_sym = make_Function_t(fn_name, fn_symtab, dep, args,
            body, result, Source, Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Shape

namespace Any {

    static inline void verify_array(ASR::expr_t* array, ASR::ttype_t* return_type,
        const Location& loc, diag::Diagnostics& diagnostics) {
        ASR::ttype_t* array_type = ASRUtils::expr_type(array);
        ASRUtils::require_impl(ASRUtils::is_logical(*array_type),
            "Input to Any intrinsic must be of logical type, found: " + ASRUtils::get_type_code(array_type),
            loc, diagnostics);
        int array_n_dims = ASRUtils::extract_n_dims_from_ttype(array_type);
        ASRUtils::require_impl(array_n_dims > 0, "Input to Any intrinsic must always be an array",
            loc, diagnostics);
        ASRUtils::require_impl(ASRUtils::is_logical(*return_type),
            "Any intrinsic must return a logical output", loc, diagnostics);
        int return_n_dims = ASRUtils::extract_n_dims_from_ttype(return_type);
        ASRUtils::require_impl(return_n_dims == 0,
        "Any intrinsic output for array only input should be a scalar",
        loc, diagnostics);
    }

    static inline void verify_array_dim(ASR::expr_t* array, ASR::expr_t* dim,
        ASR::ttype_t* return_type, const Location& loc, diag::Diagnostics& diagnostics) {
        ASR::ttype_t* array_type = ASRUtils::expr_type(array);
        ASRUtils::require_impl(ASRUtils::is_logical(*ASRUtils::type_get_past_pointer(array_type)),
            "Input to Any intrinsic must be of logical type, found: " + ASRUtils::get_type_code(array_type),
            loc, diagnostics);
        int array_n_dims = ASRUtils::extract_n_dims_from_ttype(array_type);
        ASRUtils::require_impl(array_n_dims > 0, "Input to Any intrinsic must always be an array",
            loc, diagnostics);

        ASRUtils::require_impl(ASR::is_a<ASR::Integer_t>(*ASRUtils::type_get_past_pointer(ASRUtils::expr_type(dim))),
            "dim argument must be an integer", loc, diagnostics);

        ASRUtils::require_impl(ASRUtils::is_logical(*return_type),
            "Any intrinsic must return a logical output", loc, diagnostics);
        int return_n_dims = ASRUtils::extract_n_dims_from_ttype(return_type);
        ASRUtils::require_impl(array_n_dims == return_n_dims + 1,
            "Any intrinsic output must return a logical array with dimension "
            "only 1 less than that of input array",
            loc, diagnostics);
    }

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args >= 1, "Any intrinsic must accept at least one argument",
            x.base.base.loc, diagnostics);
        ASRUtils::require_impl(x.m_args[0] != nullptr, "Array argument to any intrinsic cannot be nullptr",
            x.base.base.loc, diagnostics);
        switch( x.m_overload_id ) {
            case 0: {
                verify_array(x.m_args[0], x.m_type, x.base.base.loc, diagnostics);
                break;
            }
            case 1: {
                ASRUtils::require_impl(x.n_args == 2 && x.m_args[1] != nullptr,
                    "dim argument to any intrinsic cannot be nullptr",
                    x.base.base.loc, diagnostics);
                verify_array_dim(x.m_args[0], x.m_args[1], x.m_type, x.base.base.loc, diagnostics);
                break;
            }
            default: {
                require_impl(false, "Unrecognised overload id in Any intrinsic",
                            x.base.base.loc, diagnostics);
            }
        }
    }

    static inline ASR::expr_t *eval_Any(Allocator & /*al*/,
        const Location & /*loc*/, ASR::ttype_t */*t*/, Vec<ASR::expr_t*>& /*args*/) {
        return nullptr;
    }

    static inline ASR::asr_t* create_Any(
        Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
        const std::function<void (const std::string &, const Location &)> err) {
        int64_t overload_id = 0;
        Vec<ASR::expr_t*> any_args;
        any_args.reserve(al, 2);

        ASR::expr_t* array = args[0];
        ASR::expr_t* axis = nullptr;
        if( args.size() == 2 ) {
            axis = args[1];
        }
        if( ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(array)) == 0 ) {
            err("mask argument to any must be an array and must not be a scalar",
                array->base.loc);
        }

        // TODO: Add a check for range of values axis can take
        // if axis is available at compile time

        ASR::expr_t *value = nullptr;
        Vec<ASR::expr_t*> arg_values;
        arg_values.reserve(al, 2);
        ASR::expr_t *array_value = ASRUtils::expr_value(array);
        arg_values.push_back(al, array_value);
        if( axis ) {
            ASR::expr_t *axis_value = ASRUtils::expr_value(axis);
            arg_values.push_back(al, axis_value);
        }

        ASR::ttype_t* logical_return_type = nullptr;
        if( axis == nullptr ) {
            overload_id = 0;
            logical_return_type = ASRUtils::TYPE(ASR::make_Logical_t(
                                    al, loc, 4));
        } else {
            overload_id = 1;
            Vec<ASR::dimension_t> dims;
            size_t n_dims = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(array));
            dims.reserve(al, (int) n_dims - 1);
            for( int i = 0; i < (int) n_dims - 1; i++ ) {
                ASR::dimension_t dim;
                dim.loc = array->base.loc;
                dim.m_length = nullptr;
                dim.m_start = nullptr;
                dims.push_back(al, dim);
            }
            if( dims.size() > 0 ) {
                logical_return_type = ASRUtils::make_Array_t_util(al, loc,
                    ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)), dims.p, dims.size());
            } else {
                logical_return_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4));
            }
        }
        value = eval_Any(al, loc, logical_return_type, arg_values);

        any_args.push_back(al, array);
        if( axis ) {
            any_args.push_back(al, axis);
        }

        return ASRUtils::make_IntrinsicArrayFunction_t_util(al, loc,
            static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Any),
            any_args.p, any_args.n, overload_id, logical_return_type, value);
    }

    static inline void generate_body_for_scalar_output(Allocator& al, const Location& loc,
        ASR::expr_t* array, ASR::expr_t* return_var, SymbolTable* fn_scope,
        Vec<ASR::stmt_t*>& fn_body) {
        ASRBuilder builder(al, loc);
        Vec<ASR::expr_t*> idx_vars;
        Vec<ASR::stmt_t*> doloop_body;
        builder.generate_reduction_intrinsic_stmts_for_scalar_output(loc,
            array, fn_scope, fn_body, idx_vars, doloop_body,
            [=, &al, &fn_body, &builder] () {
                ASR::expr_t* logical_false = make_ConstantWithKind(
                    make_LogicalConstant_t, make_Logical_t, false, 4, loc);
                ASR::stmt_t* return_var_init = builder.Assignment(return_var, logical_false);
                fn_body.push_back(al, return_var_init);
            },
            [=, &al, &idx_vars, &doloop_body, &builder] () {
                ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
                ASR::expr_t* logical_or = builder.Or(return_var, array_ref, loc);
                ASR::stmt_t* loop_invariant = builder.Assignment(return_var, logical_or);
                doloop_body.push_back(al, loop_invariant);
            }
        );
    }

    static inline void generate_body_for_array_output(Allocator& al, const Location& loc,
        ASR::expr_t* array, ASR::expr_t* dim, ASR::expr_t* result,
        SymbolTable* fn_scope, Vec<ASR::stmt_t*>& fn_body) {
        ASRBuilder builder(al, loc);
        Vec<ASR::expr_t*> idx_vars, target_idx_vars;
        Vec<ASR::stmt_t*> doloop_body;
        builder.generate_reduction_intrinsic_stmts_for_array_output(
            loc, array, dim, fn_scope, fn_body,
            idx_vars, target_idx_vars, doloop_body,
            [=, &al, &fn_body, &builder] {
                ASR::expr_t* logical_false = make_ConstantWithKind(
                    make_LogicalConstant_t, make_Logical_t, false, 4, loc);
                ASR::stmt_t* result_init = builder.Assignment(result, logical_false);
                fn_body.push_back(al, result_init);
            },
            [=, &al, &idx_vars, &target_idx_vars, &doloop_body, &result, &builder] () {
                ASR::expr_t* result_ref = PassUtils::create_array_ref(result, target_idx_vars, al);
                ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
                ASR::expr_t* logical_or = builder.ElementalOr(result_ref, array_ref, loc);
                ASR::stmt_t* loop_invariant = builder.Assignment(result_ref, logical_or);
                doloop_body.push_back(al, loop_invariant);
            });
    }

    static inline ASR::expr_t* instantiate_Any(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *logical_return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t overload_id,
            ASR::expr_t* compile_time_value) {
        if (compile_time_value) {
            return compile_time_value;
        }
        ASRBuilder builder(al, loc);
        ASRBuilder& b = builder;
        ASR::ttype_t* arg_type = arg_types[0];
        int kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
        int rank = ASRUtils::extract_n_dims_from_ttype(arg_type);
        std::string new_name = "any_" + std::to_string(kind) +
                                "_" + std::to_string(rank) +
                                "_" + std::to_string(overload_id);
        // Check if Function is already defined.
        {
            std::string new_func_name = new_name;
            int i = 1;
            while (scope->get_symbol(new_func_name) != nullptr) {
                ASR::symbol_t *s = scope->get_symbol(new_func_name);
                ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
                int orig_array_rank = ASRUtils::extract_n_dims_from_ttype(
                                        ASRUtils::expr_type(f->m_args[0]));
                if (ASRUtils::types_equal(ASRUtils::expr_type(f->m_args[0]),
                        arg_type) && orig_array_rank == rank) {
                    return builder.Call(s, new_args, logical_return_type, nullptr);
                } else {
                    new_func_name += std::to_string(i);
                    i++;
                }
            }
        }

        new_name = scope->get_unique_name(new_name, false);
        SymbolTable *fn_symtab = al.make_new<SymbolTable>(scope);

        Vec<ASR::expr_t*> args;
        int result_dims = 0;
        {
            args.reserve(al, 1);
            ASR::ttype_t* mask_type = ASRUtils::duplicate_type_with_empty_dims(al, arg_type);
            fill_func_arg("mask", mask_type);
            if( overload_id == 1 ) {
                ASR::ttype_t* dim_type = ASRUtils::expr_type(new_args[1].m_value);
                LCOMPILERS_ASSERT(ASR::is_a<ASR::Integer_t>(*dim_type));
                [[maybe_unused]] int kind = ASRUtils::extract_kind_from_ttype_t(dim_type);
                LCOMPILERS_ASSERT(kind == 4);
                fill_func_arg("dim", dim_type);

                Vec<ASR::dimension_t> dims;
                size_t n_dims = ASRUtils::extract_n_dims_from_ttype(arg_type);
                dims.reserve(al, (int) n_dims - 1);
                for( int i = 0; i < (int) n_dims - 1; i++ ) {
                    ASR::dimension_t dim;
                    dim.loc = new_args[0].m_value->base.loc;
                    dim.m_length = nullptr;
                    dim.m_start = nullptr;
                    dims.push_back(al, dim);
                }
                result_dims = dims.size();
                if( result_dims > 0 ) {
                    fill_func_arg("result", logical_return_type);
                }
            }
        }

        ASR::expr_t* return_var = nullptr;
        if( result_dims == 0 ) {
            return_var = declare(new_name, logical_return_type, ReturnVar);
        }

        Vec<ASR::stmt_t*> body;
        body.reserve(al, 1);
        if( overload_id == 0 || return_var ) {
            generate_body_for_scalar_output(al, loc, args[0], return_var, fn_symtab, body);
        } else if( overload_id == 1 ) {
            generate_body_for_array_output(al, loc, args[0], args[1], args[2], fn_symtab, body);
        } else {
            LCOMPILERS_ASSERT(false);
        }

        Vec<char *> dep;
        dep.reserve(al, 1);
        // TODO: fill dependencies

        ASR::symbol_t *new_symbol = nullptr;
        if( return_var ) {
            new_symbol = make_Function_t(new_name, fn_symtab, dep, args,
                body, return_var, Source, Implementation, nullptr);
        } else {
            new_symbol = make_Function_Without_ReturnVar_t(
                new_name, fn_symtab, dep, args,
                body, Source, Implementation, nullptr);
        }
        scope->add_symbol(new_name, new_symbol);
        return builder.Call(new_symbol, new_args, logical_return_type, nullptr);
    }

} // namespace Any

namespace Sum {

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t& x,
            diag::Diagnostics& diagnostics) {
        ArrIntrinsic::verify_args(x, diagnostics, IntrinsicArrayFunctions::Sum,
            &ArrIntrinsic::verify_array_int_real_cmplx);
    }

    static inline ASR::expr_t *eval_Sum(Allocator & /*al*/,
        const Location & /*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/) {
        return nullptr;
    }

    static inline ASR::asr_t* create_Sum(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        return ArrIntrinsic::create_ArrIntrinsic(al, loc, args, err,
            IntrinsicArrayFunctions::Sum);
    }

    static inline ASR::expr_t* instantiate_Sum(Allocator &al,
            const Location &loc, SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            ASR::ttype_t *return_type, Vec<ASR::call_arg_t>& new_args,
            int64_t overload_id, ASR::expr_t* compile_time_value) {
        if (compile_time_value) {
            return compile_time_value;
        }
        return ArrIntrinsic::instantiate_ArrIntrinsic(al, loc, scope, arg_types,
            return_type, new_args, overload_id, IntrinsicArrayFunctions::Sum,
            &get_constant_zero_with_given_type, &ASRBuilder::ElementalAdd);
    }

} // namespace Sum

namespace Product {

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t& x,
            diag::Diagnostics& diagnostics) {
        ArrIntrinsic::verify_args(x, diagnostics, IntrinsicArrayFunctions::Product,
            &ArrIntrinsic::verify_array_int_real_cmplx);
    }

    static inline ASR::expr_t *eval_Product(Allocator & /*al*/,
        const Location & /*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/) {
        return nullptr;
    }

    static inline ASR::asr_t* create_Product(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        return ArrIntrinsic::create_ArrIntrinsic(al, loc, args, err,
            IntrinsicArrayFunctions::Product);
    }

    static inline ASR::expr_t* instantiate_Product(Allocator &al,
            const Location &loc, SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            ASR::ttype_t *return_type, Vec<ASR::call_arg_t>& new_args,
            int64_t overload_id, ASR::expr_t* compile_time_value) {
        if (compile_time_value) {
            return compile_time_value;
        }
        return ArrIntrinsic::instantiate_ArrIntrinsic(al, loc, scope, arg_types,
            return_type, new_args, overload_id, IntrinsicArrayFunctions::Product,
            &get_constant_one_with_given_type, &ASRBuilder::ElementalMul);
    }

} // namespace Product

namespace MaxVal {

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t& x,
            diag::Diagnostics& diagnostics) {
        ArrIntrinsic::verify_args(x, diagnostics, IntrinsicArrayFunctions::MaxVal,
            &ArrIntrinsic::verify_array_int_real);
    }

    static inline ASR::expr_t *eval_MaxVal(Allocator & /*al*/,
        const Location & /*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/) {
        return nullptr;
    }

    static inline ASR::asr_t* create_MaxVal(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        return ArrIntrinsic::create_ArrIntrinsic(al, loc, args, err,
            IntrinsicArrayFunctions::MaxVal);
    }

    static inline ASR::expr_t* instantiate_MaxVal(Allocator &al,
            const Location &loc, SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            ASR::ttype_t *return_type, Vec<ASR::call_arg_t>& new_args,
            int64_t overload_id, ASR::expr_t* compile_time_value) {
        if (compile_time_value) {
            return compile_time_value;
        }
        return ArrIntrinsic::instantiate_ArrIntrinsic(al, loc, scope, arg_types,
            return_type, new_args, overload_id, IntrinsicArrayFunctions::MaxVal,
            &get_minimum_value_with_given_type, &ASRBuilder::ElementalMax);
    }

} // namespace MaxVal

namespace MaxLoc {

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t& x,
            diag::Diagnostics& diagnostics) {
        ArrIntrinsic::verify_MaxMinLoc_args(x, diagnostics);
    }

    static inline ASR::asr_t* create_MaxLoc(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        return ArrIntrinsic::create_MaxMinLoc(al, loc, args,
            static_cast<int>(IntrinsicArrayFunctions::MaxLoc), err);
    }

    static inline ASR::expr_t *instantiate_MaxLoc(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& m_args, int64_t overload_id,
            ASR::expr_t* compile_time_value) {
        if (compile_time_value) {
            return compile_time_value;
        }
        return ArrIntrinsic::instantiate_MaxMinLoc(al, loc, scope,
            static_cast<int>(IntrinsicArrayFunctions::MaxLoc), arg_types, return_type,
            m_args, overload_id);
    }

} // namespace MaxLoc

namespace Merge {

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t& x,
            diag::Diagnostics& diagnostics) {
        const Location& loc = x.base.base.loc;
        ASR::expr_t *tsource = x.m_args[0], *fsource = x.m_args[1], *mask = x.m_args[2];
        ASR::ttype_t *tsource_type = ASRUtils::expr_type(tsource);
        ASR::ttype_t *fsource_type = ASRUtils::expr_type(fsource);
        ASR::ttype_t *mask_type = ASRUtils::expr_type(mask);
        int tsource_ndims, fsource_ndims;
        ASR::dimension_t *tsource_mdims = nullptr, *fsource_mdims = nullptr;
        tsource_ndims = ASRUtils::extract_dimensions_from_ttype(tsource_type, tsource_mdims);
        fsource_ndims = ASRUtils::extract_dimensions_from_ttype(fsource_type, fsource_mdims);
        if( tsource_ndims > 0 && fsource_ndims > 0 ) {
            ASRUtils::require_impl(tsource_ndims == fsource_ndims,
                "All arguments of `merge` should be of same rank and dimensions", loc, diagnostics);

            if( ASRUtils::extract_physical_type(tsource_type) == ASR::array_physical_typeType::FixedSizeArray &&
                ASRUtils::extract_physical_type(fsource_type) == ASR::array_physical_typeType::FixedSizeArray ) {
                ASRUtils::require_impl(ASRUtils::get_fixed_size_of_array(tsource_mdims, tsource_ndims) ==
                    ASRUtils::get_fixed_size_of_array(fsource_mdims, fsource_ndims),
                    "`tsource` and `fsource` arguments should have matching size", loc, diagnostics);
            }
        }

        ASRUtils::require_impl(ASRUtils::check_equal_type(tsource_type, fsource_type),
            "`tsource` and `fsource` arguments to `merge` should be of same type, found " +
            ASRUtils::get_type_code(tsource_type) + ", " +
            ASRUtils::get_type_code(fsource_type), loc, diagnostics);
        ASRUtils::require_impl(ASRUtils::is_logical(*mask_type),
            "`mask` argument to `merge` should be of logical type, found " +
                ASRUtils::get_type_code(mask_type), loc, diagnostics);
    }

    static inline ASR::expr_t* eval_Merge(
        Allocator &/*al*/, const Location &/*loc*/, ASR::ttype_t *,
            Vec<ASR::expr_t*>& args) {
        LCOMPILERS_ASSERT(args.size() == 3);
        ASR::expr_t *tsource = args[0], *fsource = args[1], *mask = args[2];
        if( ASRUtils::is_array(ASRUtils::expr_type(mask)) ) {
            return nullptr;
        }

        bool mask_value = false;
        if( ASRUtils::is_value_constant(mask, mask_value) ) {
            if( mask_value ) {
                return tsource;
            } else {
                return fsource;
            }
        }
        return nullptr;
    }

    static inline ASR::asr_t* create_Merge(Allocator& al, const Location& loc,
        Vec<ASR::expr_t*>& args,
        const std::function<void (const std::string &, const Location &)> err) {
        if( args.size() != 3 ) {
            err("`merge` intrinsic accepts 3 positional arguments, found " +
                std::to_string(args.size()), loc);
        }

        ASR::expr_t *tsource = args[0], *fsource = args[1], *mask = args[2];
        ASR::ttype_t *tsource_type = ASRUtils::expr_type(tsource);
        ASR::ttype_t *fsource_type = ASRUtils::expr_type(fsource);
        ASR::ttype_t *mask_type = ASRUtils::expr_type(mask);
        ASR::ttype_t* result_type = tsource_type;
        int tsource_ndims, fsource_ndims, mask_ndims;
        ASR::dimension_t *tsource_mdims = nullptr, *fsource_mdims = nullptr, *mask_mdims = nullptr;
        tsource_ndims = ASRUtils::extract_dimensions_from_ttype(tsource_type, tsource_mdims);
        fsource_ndims = ASRUtils::extract_dimensions_from_ttype(fsource_type, fsource_mdims);
        mask_ndims = ASRUtils::extract_dimensions_from_ttype(mask_type, mask_mdims);
        if( tsource_ndims > 0 && fsource_ndims > 0 ) {
            if( tsource_ndims != fsource_ndims ) {
                err("All arguments of `merge` should be of same rank and dimensions", loc);
            }

            if( ASRUtils::extract_physical_type(tsource_type) == ASR::array_physical_typeType::FixedSizeArray &&
                ASRUtils::extract_physical_type(fsource_type) == ASR::array_physical_typeType::FixedSizeArray &&
                ASRUtils::get_fixed_size_of_array(tsource_mdims, tsource_ndims) !=
                    ASRUtils::get_fixed_size_of_array(fsource_mdims, fsource_ndims) ) {
                err("`tsource` and `fsource` arguments should have matching size", loc);
            }
        } else {
            if( tsource_ndims > 0 && fsource_ndims == 0 ) {
                result_type = tsource_type;
            } else if( tsource_ndims == 0 && fsource_ndims > 0 ) {
                result_type = fsource_type;
            } else if( tsource_ndims == 0 && fsource_ndims == 0 && mask_ndims > 0 ) {
                Vec<ASR::dimension_t> mask_mdims_vec;
                mask_mdims_vec.from_pointer_n(mask_mdims, mask_ndims);
                result_type = ASRUtils::duplicate_type(al, tsource_type, &mask_mdims_vec,
                    ASRUtils::extract_physical_type(mask_type), true);
                if( ASR::is_a<ASR::Allocatable_t>(*mask_type) ) {
                    result_type = ASRUtils::TYPE(ASR::make_Allocatable_t(al, loc, result_type));
                }
            }
        }
        if( !ASRUtils::check_equal_type(tsource_type, fsource_type) ) {
            err("`tsource` and `fsource` arguments to `merge` should be of same type, found " +
                ASRUtils::get_type_code(tsource_type) + ", " +
                ASRUtils::get_type_code(fsource_type), loc);
        }
        if( !ASRUtils::is_logical(*mask_type) ) {
            err("`mask` argument to `merge` should be of logical type, found " +
                ASRUtils::get_type_code(mask_type), loc);
        }

        return ASR::make_IntrinsicArrayFunction_t(al, loc,
            static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Merge),
            args.p, args.size(), 0, result_type, nullptr);
    }

    static inline ASR::expr_t* instantiate_Merge(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/,
            ASR::expr_t* compile_time_value) {
        if (compile_time_value) {
            return compile_time_value;
        }
        LCOMPILERS_ASSERT(arg_types.size() == 3);

        // Array inputs should be elementalised in array_op pass already
        LCOMPILERS_ASSERT( !ASRUtils::is_array(arg_types[2]) );
        ASR::ttype_t *tsource_type = ASRUtils::duplicate_type(al, arg_types[0]);
        ASR::ttype_t *fsource_type = ASRUtils::duplicate_type(al, arg_types[1]);
        ASR::ttype_t *mask_type = ASRUtils::duplicate_type(al, arg_types[2]);
        if( ASR::is_a<ASR::Character_t>(*tsource_type) ) {
            ASR::Character_t* tsource_char = ASR::down_cast<ASR::Character_t>(tsource_type);
            ASR::Character_t* fsource_char = ASR::down_cast<ASR::Character_t>(fsource_type);
            tsource_char->m_len_expr = nullptr; fsource_char->m_len_expr = nullptr;
            tsource_char->m_len = -2; fsource_char->m_len = -2;
        }
        std::string new_name = "_lcompilers_merge_" + get_type_code(tsource_type);

        declare_basic_variables(new_name);
        if (scope->get_symbol(new_name)) {
            ASR::symbol_t *s = scope->get_symbol(new_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var), nullptr);
        }

        auto tsource_arg = declare("tsource", tsource_type, In);
        args.push_back(al, tsource_arg);
        auto fsource_arg = declare("fsource", fsource_type, In);
        args.push_back(al, fsource_arg);
        auto mask_arg = declare("mask", mask_type, In);
        args.push_back(al, mask_arg);
        // TODO: In case of Character type, set len of ReturnVar to len(tsource) expression
        auto result = declare("merge", tsource_type, ReturnVar);

        {
            Vec<ASR::stmt_t *> if_body; if_body.reserve(al, 1);
            if_body.push_back(al, b.Assignment(result, tsource_arg));
            Vec<ASR::stmt_t *> else_body; else_body.reserve(al, 1);
            else_body.push_back(al, b.Assignment(result, fsource_arg));
            body.push_back(al, STMT(ASR::make_If_t(al, loc, mask_arg,
                if_body.p, if_body.n, else_body.p, else_body.n)));
        }

        ASR::symbol_t *new_symbol = make_Function_t(fn_name, fn_symtab, dep, args,
            body, result, Source, Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.Call(new_symbol, new_args, return_type, nullptr);
    }

} // namespace Merge

namespace MinVal {

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t& x,
            diag::Diagnostics& diagnostics) {
        ArrIntrinsic::verify_args(x, diagnostics, IntrinsicArrayFunctions::MinVal,
            &ArrIntrinsic::verify_array_int_real);
    }

    static inline ASR::expr_t *eval_MinVal(Allocator & /*al*/,
        const Location & /*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/) {
        return nullptr;
    }

    static inline ASR::asr_t* create_MinVal(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        return ArrIntrinsic::create_ArrIntrinsic(al, loc, args, err,
            IntrinsicArrayFunctions::MinVal);
    }

    static inline ASR::expr_t* instantiate_MinVal(Allocator &al,
            const Location &loc, SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            ASR::ttype_t *return_type, Vec<ASR::call_arg_t>& new_args,
            int64_t overload_id, ASR::expr_t* compile_time_value) {
        if (compile_time_value) {
            return compile_time_value;
        }
        return ArrIntrinsic::instantiate_ArrIntrinsic(al, loc, scope, arg_types,
            return_type, new_args, overload_id, IntrinsicArrayFunctions::MinVal,
            &get_maximum_value_with_given_type, &ASRBuilder::ElementalMin);
    }

} // namespace MinVal

namespace MinLoc {

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t& x,
            diag::Diagnostics& diagnostics) {
        ArrIntrinsic::verify_MaxMinLoc_args(x, diagnostics);
    }

    static inline ASR::asr_t* create_MinLoc(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        return ArrIntrinsic::create_MaxMinLoc(al, loc, args,
            static_cast<int>(IntrinsicArrayFunctions::MinLoc), err);
    }

    static inline ASR::expr_t *instantiate_MinLoc(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& m_args, int64_t overload_id,
            ASR::expr_t* compile_time_value) {
        if (compile_time_value) {
            return compile_time_value;
        }
        return ArrIntrinsic::instantiate_MaxMinLoc(al, loc, scope,
            static_cast<int>(IntrinsicArrayFunctions::MinLoc), arg_types, return_type,
            m_args, overload_id);
    }

} // namespace MinLoc

namespace IntrinsicArrayFunctionRegistry {

    static const std::map<int64_t, std::tuple<impl_function,
            verify_array_function>>& intrinsic_function_by_id_db = {
        {static_cast<int64_t>(IntrinsicArrayFunctions::Any),
            {&Any::instantiate_Any, &Any::verify_args}},
        {static_cast<int64_t>(IntrinsicArrayFunctions::MaxLoc),
            {&MaxLoc::instantiate_MaxLoc, &MaxLoc::verify_args}},
        {static_cast<int64_t>(IntrinsicArrayFunctions::MaxVal),
            {&MaxVal::instantiate_MaxVal, &MaxVal::verify_args}},
        {static_cast<int64_t>(IntrinsicArrayFunctions::Merge),
            {&Merge::instantiate_Merge, &Merge::verify_args}},
        {static_cast<int64_t>(IntrinsicArrayFunctions::MinLoc),
            {&MinLoc::instantiate_MinLoc, &MinLoc::verify_args}},
        {static_cast<int64_t>(IntrinsicArrayFunctions::MinVal),
            {&MinVal::instantiate_MinVal, &MinVal::verify_args}},
        {static_cast<int64_t>(IntrinsicArrayFunctions::Product),
            {&Product::instantiate_Product, &Product::verify_args}},
        {static_cast<int64_t>(IntrinsicArrayFunctions::Shape),
            {&Shape::instantiate_Shape, &Shape::verify_args}},
        {static_cast<int64_t>(IntrinsicArrayFunctions::Sum),
            {&Sum::instantiate_Sum, &Sum::verify_args}},
    };

    static const std::map<std::string, std::tuple<create_intrinsic_function,
            eval_intrinsic_function>>& function_by_name_db = {
        {"any", {&Any::create_Any, &Any::eval_Any}},
        {"maxloc", {&MaxLoc::create_MaxLoc, nullptr}},
        {"maxval", {&MaxVal::create_MaxVal, &MaxVal::eval_MaxVal}},
        {"merge", {&Merge::create_Merge, &Merge::eval_Merge}},
        {"minloc", {&MinLoc::create_MinLoc, nullptr}},
        {"minval", {&MinVal::create_MinVal, &MinVal::eval_MinVal}},
        {"product", {&Product::create_Product, &Product::eval_Product}},
        {"shape", {&Shape::create_Shape, &Shape::eval_Shape}},
        {"sum", {&Sum::create_Sum, &Sum::eval_Sum}},
    };

    static inline bool is_intrinsic_function(const std::string& name) {
        return function_by_name_db.find(name) != function_by_name_db.end();
    }

    static inline create_intrinsic_function get_create_function(const std::string& name) {
        return  std::get<0>(function_by_name_db.at(name));
    }

    static inline impl_function get_instantiate_function(int64_t id) {
        if( intrinsic_function_by_id_db.find(id) == intrinsic_function_by_id_db.end() ) {
            return nullptr;
        }
        return std::get<0>(intrinsic_function_by_id_db.at(id));
    }

    static inline verify_array_function get_verify_function(int64_t id) {
        return std::get<1>(intrinsic_function_by_id_db.at(id));
    }

    /*
        The function gives the index of the dim a.k.a axis argument
        for the intrinsic with the given id. Most of the time
        dim is specified via second argument (i.e., index 1) but
        still its better to encapsulate it in the following
        function and then call it to get the index of the dim
        argument whenever needed. This helps in limiting
        the API changes of the intrinsic to this function only.
    */
    static inline int get_dim_index(IntrinsicArrayFunctions id) {
        if( id == IntrinsicArrayFunctions::Any ||
            id == IntrinsicArrayFunctions::Sum ||
            id == IntrinsicArrayFunctions::Product ||
            id == IntrinsicArrayFunctions::MaxVal ||
            id == IntrinsicArrayFunctions::MinVal) {
            return 1;
        } else {
            LCOMPILERS_ASSERT(false);
        }
        return -1;
    }

    static inline bool handle_dim(IntrinsicArrayFunctions id) {
        // Dim argument is already handled for the following
        if( id == IntrinsicArrayFunctions::Shape  ||
            id == IntrinsicArrayFunctions::MaxLoc ||
            id == IntrinsicArrayFunctions::MinLoc ) {
            return false;
        } else {
            return true;
        }
    }

    static inline bool is_elemental(int64_t id) {
        IntrinsicArrayFunctions id_ = static_cast<IntrinsicArrayFunctions>(id);
        return (id_ == IntrinsicArrayFunctions::Merge);
    }

} // namespace IntrinsicArrayFunctionRegistry

} // namespace ASRUtils

} // namespace LCompilers

#endif // LFORTRAN_PASS_INTRINSIC_ARRAY_FUNCTIONS_H
