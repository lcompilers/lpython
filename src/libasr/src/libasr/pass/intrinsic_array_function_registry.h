#ifndef LFORTRAN_PASS_INTRINSIC_ARRAY_FUNCTIONS_H
#define LFORTRAN_PASS_INTRINSIC_ARRAY_FUNCTIONS_H

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/asr_utils.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/intrinsic_function_registry.h>

#include <cmath>
#include <string>
#include <numeric>
#include <tuple>

namespace LCompilers {

namespace ASRUtils {

/************************* Intrinsic Array Functions **************************/
enum class IntrinsicArrayFunctions : int64_t {
    Any,
    MatMul,
    MaxLoc,
    MaxVal,
    Merge,
    MinLoc,
    MinVal,
    Product,
    Shape,
    Sum,
    Transpose,
    Pack,
    Unpack,
    Count,
    DotProduct,
    // ...
};

#define ARRAY_INTRINSIC_NAME_CASE(X)                                            \
    case (static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::X)) : {       \
        return #X;                                                              \
    }

inline std::string get_array_intrinsic_name(int x) {
    switch (x) {
        ARRAY_INTRINSIC_NAME_CASE(Any)
        ARRAY_INTRINSIC_NAME_CASE(MatMul)
        ARRAY_INTRINSIC_NAME_CASE(MaxLoc)
        ARRAY_INTRINSIC_NAME_CASE(MaxVal)
        ARRAY_INTRINSIC_NAME_CASE(Merge)
        ARRAY_INTRINSIC_NAME_CASE(MinLoc)
        ARRAY_INTRINSIC_NAME_CASE(MinVal)
        ARRAY_INTRINSIC_NAME_CASE(Product)
        ARRAY_INTRINSIC_NAME_CASE(Shape)
        ARRAY_INTRINSIC_NAME_CASE(Sum)
        ARRAY_INTRINSIC_NAME_CASE(Transpose)
        ARRAY_INTRINSIC_NAME_CASE(Pack)
        ARRAY_INTRINSIC_NAME_CASE(Unpack)
        ARRAY_INTRINSIC_NAME_CASE(Count)
        ARRAY_INTRINSIC_NAME_CASE(DotProduct)
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
            Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
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
        diag::Diagnostics& diag) {
        if( args.size() != 3 ) {
            append_error(diag, "`merge` intrinsic accepts 3 positional arguments, found " +
                std::to_string(args.size()), loc);
            return nullptr;
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
                append_error(diag, "All arguments of `merge` should be of same rank and dimensions", loc);
                return nullptr;
            }

            if( ASRUtils::extract_physical_type(tsource_type) == ASR::array_physical_typeType::FixedSizeArray &&
                ASRUtils::extract_physical_type(fsource_type) == ASR::array_physical_typeType::FixedSizeArray &&
                ASRUtils::get_fixed_size_of_array(tsource_mdims, tsource_ndims) !=
                    ASRUtils::get_fixed_size_of_array(fsource_mdims, fsource_ndims) ) {
                append_error(diag, "`tsource` and `fsource` arguments should have matching size", loc);
                return nullptr;
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
            append_error(diag, "`tsource` and `fsource` arguments to `merge` should be of same type, found " +
                ASRUtils::get_type_code(tsource_type) + ", " +
                ASRUtils::get_type_code(fsource_type), loc);
            return nullptr;
        }
        if( !ASRUtils::is_logical(*mask_type) ) {
            append_error(diag, "`mask` argument to `merge` should be of logical type, found " +
                ASRUtils::get_type_code(mask_type), loc);
            return nullptr;
        }

        return ASR::make_IntrinsicArrayFunction_t(al, loc,
            static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Merge),
            args.p, args.size(), 0, result_type, nullptr);
    }

    static inline ASR::expr_t* instantiate_Merge(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
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
            ASR::Character_t* return_char = ASR::down_cast<ASR::Character_t>(
                ASRUtils::type_get_past_allocatable(return_type));
            return_char->m_len = -2; return_char->m_len_expr = nullptr;

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
        auto result = declare("merge", type_get_past_allocatable(return_type), ReturnVar);

        {
            Vec<ASR::stmt_t *> if_body; if_body.reserve(al, 1);
            if_body.push_back(al, b.Assignment(result, tsource_arg));
            Vec<ASR::stmt_t *> else_body; else_body.reserve(al, 1);
            else_body.push_back(al, b.Assignment(result, fsource_arg));
            body.push_back(al, STMT(ASR::make_If_t(al, loc, mask_arg,
                if_body.p, if_body.n, else_body.p, else_body.n)));
        }

        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.Call(new_symbol, new_args, return_type, nullptr);
    }

} // namespace Merge

namespace ArrIntrinsic {

static inline void verify_array_int_real_cmplx(ASR::expr_t* array, ASR::ttype_t* return_type,
    const Location& loc, diag::Diagnostics& diagnostics, ASRUtils::IntrinsicArrayFunctions intrinsic_func_id) {
    std::string intrinsic_func_name = ASRUtils::get_array_intrinsic_name(static_cast<int>(intrinsic_func_id));
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
    std::string intrinsic_func_name = ASRUtils::get_array_intrinsic_name(static_cast<int>(intrinsic_func_id));
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
    std::string intrinsic_func_name = ASRUtils::get_array_intrinsic_name(static_cast<int>(intrinsic_func_id));
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
    std::string intrinsic_func_name = ASRUtils::get_array_intrinsic_name(static_cast<int>(intrinsic_func_id));
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
    const Location & /*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/,
    diag::Diagnostics& /*diag*/) {
    return nullptr;
}

static inline ASR::asr_t* create_ArrIntrinsic(
    Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
    diag::Diagnostics& diag,
    ASRUtils::IntrinsicArrayFunctions intrinsic_func_id) {
    std::string intrinsic_func_name = ASRUtils::get_array_intrinsic_name(static_cast<int>(intrinsic_func_id));
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
            append_error(diag, "dim argument to " + intrinsic_func_name + " must be a scalar and must not be an array",
                arg2->base.loc);
            return nullptr;
        }

        if( arg3_rank == 0 ) {
            append_error(diag, "mask argument to " + intrinsic_func_name + " must be an array and must not be a scalar",
                arg3->base.loc);
            return nullptr;
        }

        overload_id = id_array_dim_mask;
    }

    // TODO: Add a check for range of values axis can take
    // if axis is available at compile time

    ASR::expr_t *value = nullptr;
    bool runtime_dim = false;
    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, 3);
    ASR::expr_t *array_value = ASRUtils::expr_value(array);
    arg_values.push_back(al, array_value);
    if( arg2 ) {
        ASR::expr_t *arg2_value = ASRUtils::expr_value(arg2);
        runtime_dim = arg2_value == nullptr;
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
        for( int it = 0; it < (int) n_dims - 1; it++ ) {
            Vec<ASR::expr_t*> args_merge; args_merge.reserve(al, 3);
            ASRUtils::ASRBuilder b(al, loc);
            args_merge.push_back(al, b.ArraySize(args[0], b.i32(it+1), int32));
            args_merge.push_back(al, b.ArraySize(args[0], b.i32(it+2), int32));
            args_merge.push_back(al, b.iLt(b.i32(it+1), args[1]));
            ASR::expr_t* merge = EXPR(Merge::create_Merge(al, loc, args_merge, diag));
            ASR::dimension_t dim;
            dim.loc = array->base.loc;
            dim.m_start = b.i32(1);
            dim.m_length = runtime_dim ? merge : nullptr;
            dims.push_back(al, dim);
        }
        return_type = ASRUtils::duplicate_type(al, array_type, &dims, ASR::array_physical_typeType::DescriptorArray, true);
    }
    value = eval_ArrIntrinsic(al, loc, return_type, arg_values, diag);

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
    std::string intrinsic_func_name = ASRUtils::get_array_intrinsic_name(static_cast<int>(intrinsic_func_id));
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
        ASR::ttype_t* return_type_ = return_type;
        if( !ASRUtils::is_fixed_size_array(return_type) ) {
            bool is_allocatable = ASRUtils::is_allocatable(return_type);
            Vec<ASR::dimension_t> empty_dims;
            empty_dims.reserve(al, result_dims);
            for( int idim = 0; idim < result_dims; idim++ ) {
                ASR::dimension_t empty_dim;
                empty_dim.loc = loc;
                empty_dim.m_start = nullptr;
                empty_dim.m_length = nullptr;
                empty_dims.push_back(al, empty_dim);
            }
            return_type_ = ASRUtils::make_Array_t_util(al, loc,
                ASRUtils::extract_type(return_type_), empty_dims.p, empty_dims.size());
            if( is_allocatable ) {
                return_type_ = ASRUtils::TYPE(ASR::make_Allocatable_t(al, loc, return_type_));
            }
        }
        ASR::expr_t *result = declare("result", return_type_, Out);
        args.push_back(al, result);
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
        new_symbol = make_ASR_Function_t(new_name, fn_symtab, dep, args,
            body, return_var, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
    } else {
        new_symbol = make_Function_Without_ReturnVar_t(
            new_name, fn_symtab, dep, args,
            body, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
    }
    scope->add_symbol(new_name, new_symbol);
    return builder.Call(new_symbol, new_args, return_type, nullptr);
}

static inline void verify_MaxMinLoc_args(const ASR::IntrinsicArrayFunction_t& x,
        diag::Diagnostics& diagnostics) {
    std::string intrinsic_name = get_array_intrinsic_name(
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
            return b.i(index, type);
        } else {
            return b.ArrayConstant({b.i32(index)}, extract_type(type), false);
        }
    } else {
        return nullptr;
    }
}

static inline ASR::asr_t* create_MaxMinLoc(Allocator& al, const Location& loc,
        Vec<ASR::expr_t*>& args, int intrinsic_id,
        diag::Diagnostics& diag) {
    std::string intrinsic_name = get_array_intrinsic_name(static_cast<int>(intrinsic_id));
    ASRUtils::ASRBuilder b(al, loc);
    ASR::ttype_t *array_type = expr_type(args[0]);
    if ( !is_array(array_type) ) {
        append_error(diag, "`array` argument of `"+ intrinsic_name +"` must be an array", loc);
        return nullptr;
    } else if ( !is_integer(*array_type) && !is_real(*array_type) ) {
        append_error(diag, "`array` argument of `"+ intrinsic_name +"` must be integer or "
            "real for now", loc);
        return nullptr;
    } else if ( args[2] || args[4] ) {
        append_error(diag, "`mask` and `back` keyword argument is not supported yet", loc);
        return nullptr;
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
            append_error(diag, "Runtime value for `kind` argument is not supported yet", loc);
            return nullptr;
        }
    }
    if ( args[1] ) {
        if ( !ASR::is_a<ASR::Integer_t>(*expr_type(args[1])) ) {
            append_error(diag, "`dim` should be a scalar integer type", loc);
            return nullptr;
        } else if (!extract_value(expr_value(args[1]), dim)) {
            append_error(diag, "Runtime values for `dim` argument is not supported yet", loc);
            return nullptr;
        }
        if ( 1 > dim || dim > n_dims ) {
            append_error(diag, "`dim` argument of `"+ intrinsic_name +"` is out of "
                "array index range", loc);
            return nullptr;
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
        tmp_dim.m_start = b.i32(1);
        tmp_dim.m_length = b.i32(n_dims);
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
    std::string intrinsic_name = get_array_intrinsic_name(static_cast<int>(intrinsic_id));
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
                body.push_back(al, b.Assignment(result, b.i(1, type)));
            }, [=, &al, &b, &idx_vars, &doloop_body] () {
                std::vector<ASR::stmt_t *> if_body; if_body.reserve(n_dims);
                Vec<ASR::expr_t *> result_idx; result_idx.reserve(al, n_dims);
                for (int i = 0; i < n_dims; i++) {
                    ASR::expr_t *idx = b.ArrayItem_01(result, {b.i32(i+1)});
                    if (extract_kind_from_ttype_t(type) != 4) {
                        if_body.push_back(b.Assignment(idx, b.i2i(idx_vars[i], type)));
                        result_idx.push_back(al, b.i2i32(idx));
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
                body.push_back(al, b.Assignment(result, b.i(1, type)));
            }, [=, &al, &b, &idx_vars, &target_idx_vars, &doloop_body] () {
                ASR::expr_t *result_ref, *array_ref_02;
                if (is_array(return_type)) {
                    result_ref = ArrayItem_02(result, target_idx_vars);
                    Vec<ASR::expr_t*> tmp_idx_vars;
                    tmp_idx_vars.from_pointer_n_copy(al, idx_vars.p, idx_vars.n);
                    tmp_idx_vars.p[dim - 1] = b.i2i32(result_ref);
                    array_ref_02 = ArrayItem_02(args[0], tmp_idx_vars);
                } else {
                    // 1D scalar output
                    result_ref = result;
                    array_ref_02 = b.ArrayItem_01(args[0], {result});
                }
                ASR::expr_t *array_ref_01 = ArrayItem_02(args[0], idx_vars);
                ASR::expr_t *res_idx = idx_vars.p[dim - 1];
                if (extract_kind_from_ttype_t(type) != 4) {
                    res_idx = b.i2i(res_idx, type);
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
    ASR::symbol_t *fn_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
    scope->add_symbol(fn_name, fn_sym);
    return b.Call(fn_sym, m_args, return_type, nullptr);
}

} // namespace ArrIntrinsic

namespace Shape {
    static inline void verify_args(const ASR::IntrinsicArrayFunction_t &x,
            diag::Diagnostics &diagnostics) {
        ASRUtils::require_impl(x.n_args == 1,
            "`shape` intrinsic accepts 1 argument",
            x.base.base.loc, diagnostics);
        ASRUtils::require_impl(x.m_args[0], "`source` argument of `shape` "
            "cannot be nullptr", x.base.base.loc, diagnostics);
    }

    static ASR::expr_t *eval_Shape(Allocator &al, const Location &loc,
            ASR::ttype_t *type, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        ASR::dimension_t *m_dims;
        size_t n_dims = extract_dimensions_from_ttype(expr_type(args[0]), m_dims);
        Vec<ASR::expr_t *> m_shapes; m_shapes.reserve(al, n_dims);
        if( n_dims == 0 ){
            return EXPR(ASR::make_ArrayConstant_t(al, loc, m_shapes.p, 0,
                type, ASR::arraystorageType::ColMajor));
        } else {
            for (size_t i = 0; i < n_dims; i++) {
                if (m_dims[i].m_length) {
                    ASR::expr_t *e = nullptr;
                    if (extract_kind_from_ttype_t(type) != 4) {
                        ASRUtils::ASRBuilder b(al, loc);
                        e = b.i2i(m_dims[i].m_length, extract_type(type));
                    } else {
                        e = m_dims[i].m_length;
                    }
                    m_shapes.push_back(al, e);
                }
            }
        }
        ASR::expr_t *value = nullptr;
        bool all_args_evaluated_ = all_args_evaluated(m_shapes);
        if (m_shapes.n > 0) {
            if (all_args_evaluated_) {
                value = EXPR(ASR::make_ArrayConstant_t(al, loc, m_shapes.p, m_shapes.n,
                    type, ASR::arraystorageType::ColMajor));
            } else {
                value = EXPR(ASR::make_ArrayConstructor_t(al, loc, m_shapes.p, m_shapes.n,
                    type, nullptr, ASR::arraystorageType::ColMajor));
            }
        }
        return value;
    }

    static inline ASR::asr_t* create_Shape(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            diag::Diagnostics& diag) {
        ASRBuilder b(al, loc);
        Vec<ASR::expr_t *>m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        int kind = 4; // default kind
        if (args[1]) {
            if (!ASR::is_a<ASR::Integer_t>(*expr_type(args[1]))) {
                append_error(diag, "`kind` argument of `shape` must be a scalar integer", loc);
                return nullptr;
            }
            if (!extract_value(args[1], kind)) {
                append_error(diag, "Only constant value for `kind` is supported for now", loc);
                return nullptr;
            }
        }
        // TODO: throw error for assumed size array
        int n_dims = extract_n_dims_from_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = b.Array({n_dims},
            TYPE(ASR::make_Integer_t(al, loc, kind)));
        ASR::expr_t *m_value = eval_Shape(al, loc, return_type, args, diag);
        return ASRUtils::make_IntrinsicArrayFunction_t_util(al, loc,
            static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Shape),
            m_args.p, m_args.n, 0, return_type, m_value);
    }

    static inline ASR::expr_t* instantiate_Shape(Allocator &al,
            const Location &loc, SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            ASR::ttype_t *return_type, Vec<ASR::call_arg_t>& new_args, int64_t) {
        declare_basic_variables("_lcompilers_shape");
        fill_func_arg("source", ASRUtils::duplicate_type_with_empty_dims(al,
            arg_types[0]));
        auto result = declare(fn_name, return_type, ReturnVar);
        int iter = extract_n_dims_from_ttype(arg_types[0]) + 1;
        auto i = declare("i", int32, Local);
        body.push_back(al, b.Assignment(i, b.i32(1)));
        body.push_back(al, b.While(b.iLt(i, b.i32(iter)), {
            b.Assignment(b.ArrayItem_01(result, {i}),
                b.ArraySize_2(args[0], i, extract_type(return_type))),
            b.Assignment(i, b.iAdd(i, b.i32(1)))
        }));
        body.push_back(al, Return());
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
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
        const Location & /*loc*/, ASR::ttype_t */*t*/, Vec<ASR::expr_t*>& /*args*/,
        diag::Diagnostics& /*diag*/) {
        return nullptr;
    }

    static inline ASR::asr_t* create_Any(
        Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
        diag::Diagnostics& diag) {
        int64_t overload_id = 0;
        Vec<ASR::expr_t*> any_args;
        any_args.reserve(al, 2);

        ASR::expr_t* array = args[0];
        ASR::expr_t* axis = nullptr;
        if( args.size() == 2 ) {
            axis = args[1];
        }
        if( ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(array)) == 0 ) {
            append_error(diag, "mask argument to any must be an array and must not be a scalar",
                array->base.loc);
            return nullptr;
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
        value = eval_Any(al, loc, logical_return_type, arg_values, diag);

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
                ASR::expr_t* logical_or = builder.LogicalOr(return_var, array_ref, loc);
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
            Vec<ASR::call_arg_t>& new_args, int64_t overload_id) {
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
            new_symbol = make_ASR_Function_t(new_name, fn_symtab, dep, args,
                body, return_var, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        } else {
            new_symbol = make_Function_Without_ReturnVar_t(
                new_name, fn_symtab, dep, args,
                body, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
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
        const Location & /*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/,
        diag::Diagnostics& /*diag*/) {
        return nullptr;
    }

    static inline ASR::asr_t* create_Sum(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            diag::Diagnostics& diag) {
        return ArrIntrinsic::create_ArrIntrinsic(al, loc, args, diag,
            IntrinsicArrayFunctions::Sum);
    }

    static inline ASR::expr_t* instantiate_Sum(Allocator &al,
            const Location &loc, SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            ASR::ttype_t *return_type, Vec<ASR::call_arg_t>& new_args,
            int64_t overload_id) {
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
        const Location & /*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/,
        diag::Diagnostics& /*diag*/) {
        return nullptr;
    }

    static inline ASR::asr_t* create_Product(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            diag::Diagnostics& diag) {
        return ArrIntrinsic::create_ArrIntrinsic(al, loc, args, diag,
            IntrinsicArrayFunctions::Product);
    }

    static inline ASR::expr_t* instantiate_Product(Allocator &al,
            const Location &loc, SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            ASR::ttype_t *return_type, Vec<ASR::call_arg_t>& new_args,
            int64_t overload_id) {
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
        const Location & /*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/,
        diag::Diagnostics& /*diag*/) {
        return nullptr;
    }

    static inline ASR::asr_t* create_MaxVal(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            diag::Diagnostics& diag) {
        return ArrIntrinsic::create_ArrIntrinsic(al, loc, args, diag,
            IntrinsicArrayFunctions::MaxVal);
    }

    static inline ASR::expr_t* instantiate_MaxVal(Allocator &al,
            const Location &loc, SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            ASR::ttype_t *return_type, Vec<ASR::call_arg_t>& new_args,
            int64_t overload_id) {
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
            diag::Diagnostics& diag) {
        return ArrIntrinsic::create_MaxMinLoc(al, loc, args,
            static_cast<int>(IntrinsicArrayFunctions::MaxLoc), diag);
    }

    static inline ASR::expr_t *instantiate_MaxLoc(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& m_args, int64_t overload_id) {
        return ArrIntrinsic::instantiate_MaxMinLoc(al, loc, scope,
            static_cast<int>(IntrinsicArrayFunctions::MaxLoc), arg_types, return_type,
            m_args, overload_id);
    }

} // namespace MaxLoc

namespace MinVal {

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t& x,
            diag::Diagnostics& diagnostics) {
        ArrIntrinsic::verify_args(x, diagnostics, IntrinsicArrayFunctions::MinVal,
            &ArrIntrinsic::verify_array_int_real);
    }

    static inline ASR::expr_t *eval_MinVal(Allocator & /*al*/,
        const Location & /*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/,
        diag::Diagnostics& /*diag*/) {
        return nullptr;
    }

    static inline ASR::asr_t* create_MinVal(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            diag::Diagnostics& diag) {
        return ArrIntrinsic::create_ArrIntrinsic(al, loc, args, diag,
            IntrinsicArrayFunctions::MinVal);
    }

    static inline ASR::expr_t* instantiate_MinVal(Allocator &al,
            const Location &loc, SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            ASR::ttype_t *return_type, Vec<ASR::call_arg_t>& new_args,
            int64_t overload_id) {
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
            diag::Diagnostics& diag) {
        return ArrIntrinsic::create_MaxMinLoc(al, loc, args,
            static_cast<int>(IntrinsicArrayFunctions::MinLoc), diag);
    }

    static inline ASR::expr_t *instantiate_MinLoc(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& m_args, int64_t overload_id) {
        return ArrIntrinsic::instantiate_MaxMinLoc(al, loc, scope,
            static_cast<int>(IntrinsicArrayFunctions::MinLoc), arg_types, return_type,
            m_args, overload_id);
    }

} // namespace MinLoc

namespace MatMul {

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t &x,
            diag::Diagnostics& diagnostics) {
        require_impl(x.n_args == 2, "`matmul` intrinsic accepts exactly"
            "two arguments", x.base.base.loc, diagnostics);
        require_impl(x.m_args[0], "`matrix_a` argument of `matmul` intrinsic "
            "cannot be nullptr", x.base.base.loc, diagnostics);
        require_impl(x.m_args[1], "`matrix_b` argument of `matmul` intrinsic "
            "cannot be nullptr", x.base.base.loc, diagnostics);
    }

    static inline ASR::expr_t *eval_MatMul(Allocator &,
        const Location &, ASR::ttype_t *, Vec<ASR::expr_t*>&, diag::Diagnostics&) {
        // TODO
        return nullptr;
    }

    static inline ASR::asr_t* create_MatMul(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            diag::Diagnostics& diag) {
        ASR::expr_t *matrix_a = args[0], *matrix_b = args[1];
        bool is_type_allocatable = false;
        if (ASRUtils::is_allocatable(matrix_a) || ASRUtils::is_allocatable(matrix_b)) {
            // TODO: Use Array type as return type instead of allocatable
            //  for both Array and Allocatable as input arguments.
            is_type_allocatable = true;
        }
        ASR::ttype_t *type_a = expr_type(matrix_a);
        ASR::ttype_t *type_b = expr_type(matrix_b);
        ASR::ttype_t *ret_type = nullptr;
        bool matrix_a_numeric = is_integer(*type_a) ||
                                is_real(*type_a) ||
                                is_complex(*type_a);
        bool matrix_a_logical = is_logical(*type_a);
        bool matrix_b_numeric = is_integer(*type_b) ||
                                is_real(*type_b) ||
                                is_complex(*type_b);
        bool matrix_b_logical = is_logical(*type_b);
        if (matrix_a_logical || matrix_b_logical) {
            // TODO
            append_error(diag, "The `matmul` intrinsic doesn't handle logical type yet", loc);
            return nullptr;
        }
        if ( !matrix_a_numeric && !matrix_a_logical ) {
            append_error(diag, "The argument `matrix_a` in `matmul` must be of type Integer, "
                "Real, Complex or Logical", matrix_a->base.loc);
            return nullptr;
        } else if ( matrix_a_numeric ) {
            if( !matrix_b_numeric ) {
                append_error(diag, "The argument `matrix_b` in `matmul` must be of type "
                    "Integer, Real or Complex if first matrix is of numeric "
                    "type", matrix_b->base.loc);
                    return nullptr;
            }
        } else {
            if( !matrix_b_logical ) {
                append_error(diag, "The argument `matrix_b` in `matmul` must be of type Logical"
                    " if first matrix is of Logical type", matrix_b->base.loc);
                return nullptr;
            }
        }
        if ( matrix_a_numeric || matrix_b_numeric ) {
            if ( is_complex(*type_a) ) {
                ret_type = extract_type(type_a);
            } else if ( is_complex(*type_b) ) {
                ret_type = extract_type(type_b);
            } else if ( is_real(*type_a) ) {
                ret_type = extract_type(type_a);
            } else if ( is_real(*type_b) ) {
                ret_type = extract_type(type_b);
            } else {
                ret_type = extract_type(type_a);
            }
        }
        LCOMPILERS_ASSERT(!matrix_a_logical && !matrix_b_logical)
        ASR::dimension_t* matrix_a_dims = nullptr;
        ASR::dimension_t* matrix_b_dims = nullptr;
        int matrix_a_rank = extract_dimensions_from_ttype(type_a, matrix_a_dims);
        int matrix_b_rank = extract_dimensions_from_ttype(type_b, matrix_b_dims);
        if ( matrix_a_rank != 1 && matrix_a_rank != 2 ) {
            append_error(diag, "`matmul` accepts arrays of rank 1 or 2 only, provided an array "
                "with rank, " + std::to_string(matrix_a_rank), matrix_a->base.loc);
            return nullptr;
        } else if ( matrix_b_rank != 1 && matrix_b_rank != 2 ) {
            append_error(diag, "`matmul` accepts arrays of rank 1 or 2 only, provided an array "
                "with rank, " + std::to_string(matrix_b_rank), matrix_b->base.loc);
            return nullptr;
        }

        ASRBuilder b(al, loc);
        Vec<ASR::dimension_t> result_dims; result_dims.reserve(al, 1);
        int overload_id = -1;
        if (matrix_a_rank == 1 && matrix_b_rank == 2) {
            overload_id = 1;
            if (!dimension_expr_equal(matrix_a_dims[0].m_length,
                    matrix_b_dims[0].m_length)) {
                int matrix_a_dim_1 = -1, matrix_b_dim_1 = -1;
                extract_value(matrix_a_dims[0].m_length, matrix_a_dim_1);
                extract_value(matrix_b_dims[0].m_length, matrix_b_dim_1);
                append_error(diag, "The argument `matrix_b` must be of dimension "
                    + std::to_string(matrix_a_dim_1) + ", provided an array "
                    "with dimension " + std::to_string(matrix_b_dim_1) +
                    " in `matrix_b('n', m)`", matrix_b->base.loc);
                return nullptr;
            } else {
                result_dims.push_back(al, b.set_dim(matrix_b_dims[1].m_start,
                    matrix_b_dims[1].m_length));
            }
        } else if (matrix_a_rank == 2) {
            overload_id = 2;
            if (!dimension_expr_equal(matrix_a_dims[1].m_length,
                    matrix_b_dims[0].m_length)) {
                int matrix_a_dim_2 = -1, matrix_b_dim_1 = -1;
                extract_value(matrix_a_dims[1].m_length, matrix_a_dim_2);
                extract_value(matrix_b_dims[0].m_length, matrix_b_dim_1);
                std::string err_dims = "('n', m)";
                if (matrix_b_rank == 1) err_dims = "('n')";
                append_error(diag, "The argument `matrix_b` must be of dimension "
                    + std::to_string(matrix_a_dim_2) + ", provided an array "
                    "with dimension " + std::to_string(matrix_b_dim_1) +
                    " in matrix_b" + err_dims, matrix_b->base.loc);
                return nullptr;
            }
            result_dims.push_back(al, b.set_dim(matrix_a_dims[0].m_start,
                matrix_a_dims[0].m_length));
            if (matrix_b_rank == 2) {
                overload_id = 3;
                result_dims.push_back(al, b.set_dim(matrix_b_dims[1].m_start,
                    matrix_b_dims[1].m_length));
            }
        } else {
            append_error(diag, "The argument `matrix_b` in `matmul` must be of rank 2, "
                "provided an array with rank, " + std::to_string(matrix_b_rank),
                matrix_b->base.loc);
            return nullptr;
        }
        ret_type = ASRUtils::duplicate_type(al, ret_type, &result_dims);
        if (is_type_allocatable) {
            ret_type = TYPE(ASR::make_Allocatable_t(al, loc, ret_type));
        }
        ASR::expr_t *value = eval_MatMul(al, loc, ret_type, args, diag);
        return make_IntrinsicArrayFunction_t_util(al, loc,
            static_cast<int64_t>(IntrinsicArrayFunctions::MatMul),
            args.p, args.n, overload_id, ret_type, value);
    }

    static inline ASR::expr_t *instantiate_MatMul(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*> &arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t> &m_args, int64_t overload_id) {
        /*
         *    2 x 3          3 x 2          2 x 2
         *   ------▶
         * [ 1, 2, 3 ]  *  [ 1, 2 ] │  =  [ 14, 20 ]
         * [ 2, 3, 4 ]     │ 2, 3 │ │     [ 20, 29 ]
         *                 [ 3, 4 ] ▼
         */
        declare_basic_variables("_lcompilers_matmul");
        fill_func_arg("matrix_a_m", duplicate_type_with_empty_dims(al, arg_types[0]));
        fill_func_arg("matrix_b_m", duplicate_type_with_empty_dims(al, arg_types[1]));
        ASR::ttype_t* return_type_ = return_type;
        if( !ASRUtils::is_fixed_size_array(return_type) ) {
            bool is_allocatable = ASRUtils::is_allocatable(return_type);
            Vec<ASR::dimension_t> empty_dims;
            int result_dims = 2;
            if( overload_id == 1 || overload_id == 2 ) {
                result_dims = 1;
            }
            empty_dims.reserve(al, result_dims);
            for( int idim = 0; idim < result_dims; idim++ ) {
                ASR::dimension_t empty_dim;
                empty_dim.loc = loc;
                empty_dim.m_start = nullptr;
                empty_dim.m_length = nullptr;
                empty_dims.push_back(al, empty_dim);
            }
            return_type_ = ASRUtils::make_Array_t_util(al, loc,
                ASRUtils::extract_type(return_type_), empty_dims.p, empty_dims.size());
            if( is_allocatable ) {
                return_type_ = ASRUtils::TYPE(ASR::make_Allocatable_t(al, loc, return_type_));
            }
        }
        ASR::expr_t *result = declare("result", return_type_, Out);
        args.push_back(al, result);
        ASR::expr_t *i = declare("i", int32, Local);
        ASR::expr_t *j = declare("j", int32, Local);
        ASR::expr_t *k = declare("k", int32, Local);
        ASR::dimension_t* matrix_a_dims = nullptr;
        ASR::dimension_t* matrix_b_dims = nullptr;
        extract_dimensions_from_ttype(arg_types[0], matrix_a_dims);
        extract_dimensions_from_ttype(arg_types[1], matrix_b_dims);
        ASR::expr_t *res_ref, *a_ref, *b_ref, *a_lbound, *b_lbound;
        ASR::expr_t *dim_mismatch_check, *a_ubound, *b_ubound;
        dim_mismatch_check = b.iEq(UBound(args[0], 2), UBound(args[1], 1));
        a_lbound = LBound(args[0], 1); a_ubound = UBound(args[0], 1);
        b_lbound = LBound(args[1], 2); b_ubound = UBound(args[1], 2);
        std::string assert_msg = "'MatMul' intrinsic dimension mismatch: "
            "please make sure the dimensions are ";
        Vec<ASR::dimension_t> alloc_dims; alloc_dims.reserve(al, 1);
        if ( overload_id == 1 ) {
            // r(j) = r(j) + a(k) * b(k, j)
            res_ref = b.ArrayItem_01(result,  {j});
            a_ref   = b.ArrayItem_01(args[0], {k});
            b_ref   = b.ArrayItem_01(args[1], {k, j});
            a_ubound = a_lbound;
            alloc_dims.push_back(al, b.set_dim(LBound(args[1], 2), UBound(args[1], 2)));
            dim_mismatch_check = b.iEq(UBound(args[0], 1), UBound(args[1], 1));
            assert_msg += "`matrix_a(k)` and `matrix_b(k, j)`";
        } else if ( overload_id == 2 ) {
            // r(i) = r(i) + a(i, k) * b(k)
            res_ref = b.ArrayItem_01(result,  {i});
            a_ref   = b.ArrayItem_01(args[0], {i, k});
            b_ref   = b.ArrayItem_01(args[1], {k});
            b_ubound = b_lbound = LBound(args[1], 1);
            alloc_dims.push_back(al, b.set_dim(LBound(args[0], 1), UBound(args[0], 1)));
            assert_msg += "`matrix_a(i, k)` and `matrix_b(k)`";
        } else {
            // r(i, j) = r(i, j) + a(i, k) * b(k, j)
            res_ref = b.ArrayItem_01(result,  {i, j});
            a_ref   = b.ArrayItem_01(args[0], {i, k});
            b_ref   = b.ArrayItem_01(args[1], {k, j});
            alloc_dims.push_back(al, b.set_dim(LBound(args[0], 1), UBound(args[0], 1)));
            alloc_dims.push_back(al, b.set_dim(LBound(args[1], 2), UBound(args[1], 2)));
            assert_msg += "`matrix_a(i, k)` and `matrix_b(k, j)`";
        }
        if (is_allocatable(result)) {
            body.push_back(al, b.Allocate(result, alloc_dims));
        }
        body.push_back(al, STMT(ASR::make_Assert_t(al, loc, dim_mismatch_check,
            EXPR(ASR::make_StringConstant_t(al, loc, s2c(al, assert_msg),
            character(assert_msg.size()))))));
        ASR::expr_t *mul_value;
        if (is_real(*expr_type(a_ref)) && is_integer(*expr_type(b_ref))) {
            mul_value = b.Mul(a_ref, b.i2r(b_ref, expr_type(a_ref)));
        } else if (is_real(*expr_type(b_ref)) && is_integer(*expr_type(a_ref))) {
            mul_value = b.Mul(b.i2r(a_ref, expr_type(b_ref)), b_ref);
        } else if (is_real(*expr_type(a_ref)) && is_complex(*expr_type(b_ref))){
            mul_value = b.Mul(EXPR(ASR::make_ComplexConstructor_t(al, loc, a_ref, b.f(0, expr_type(a_ref)), expr_type(b_ref), nullptr)), b_ref);
        } else if (is_complex(*expr_type(a_ref)) && is_real(*expr_type(b_ref))){
            mul_value = b.Mul(a_ref, EXPR(ASR::make_ComplexConstructor_t(al, loc, b_ref, b.f(0, expr_type(b_ref)), expr_type(a_ref), nullptr)));
        } else if (is_integer(*expr_type(a_ref)) && is_complex(*expr_type(b_ref))) {
            int kind = ASRUtils::extract_kind_from_ttype_t(expr_type(b_ref));
            ASR::ttype_t* real_type = TYPE(ASR::make_Real_t(al, loc, kind));
            mul_value = b.Mul(EXPR(ASR::make_ComplexConstructor_t(al, loc, b.i2r(a_ref, real_type), b.f(0, real_type), expr_type(b_ref), nullptr)), b_ref);
        } else if (is_complex(*expr_type(a_ref)) && is_integer(*expr_type(b_ref))) {
            int kind = ASRUtils::extract_kind_from_ttype_t(expr_type(a_ref));
            ASR::ttype_t* real_type = TYPE(ASR::make_Real_t(al, loc, kind));
            mul_value = b.Mul(a_ref, EXPR(ASR::make_ComplexConstructor_t(al, loc, b.i2r(b_ref, real_type), b.f(0, real_type), expr_type(a_ref), nullptr)));
        } else {
            mul_value = b.Mul(a_ref, b_ref);
        }
        body.push_back(al, b.DoLoop(i, a_lbound, a_ubound, {
            b.DoLoop(j, b_lbound, b_ubound, {
                b.Assign_Constant(res_ref, 0),
                b.DoLoop(k, LBound(args[1], 1), UBound(args[1], 1), {
                    b.Assignment(res_ref, b.Add(res_ref, mul_value))
                }),
            })
        }));
        body.push_back(al, Return());
        ASR::symbol_t *fn_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
                body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, fn_sym);
        return b.Call(fn_sym, m_args, return_type, nullptr);
    }

} // namespace MatMul

namespace Count {

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t &x,
            diag::Diagnostics& diagnostics) {
        require_impl(x.n_args == 1 || x.n_args == 2 || x.n_args == 3, "`count` intrinsic accepts "
            "one, two or three arguments", x.base.base.loc, diagnostics);
        require_impl(x.m_args[0], "`mask` argument of `count` intrinsic "
            "cannot be nullptr", x.base.base.loc, diagnostics);
    }

    static inline ASR::expr_t *eval_Count(Allocator &/*al*/,
        const Location &/*loc*/, ASR::ttype_t */*return_type*/, Vec<ASR::expr_t*>& /*args*/, diag::Diagnostics& /*diag*/) {
        // TODO
        return nullptr;
    }

    static inline ASR::asr_t* create_Count(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            diag::Diagnostics& diag) {
        int64_t id_mask = 0, id_mask_dim = 1;
        int64_t overload_id = id_mask;

        ASR::expr_t *mask = args[0], *dim_ = nullptr, *kind = nullptr;

        if (args.size() == 2) {
            dim_ = args[1];
        } else if (args.size() == 3) {
            dim_ = args[1];
            kind = args[2];
        }
        ASR::dimension_t* array_dims = nullptr;
        int array_rank = extract_dimensions_from_ttype(ASRUtils::expr_type(args[0]), array_dims);

        ASR::ttype_t* mask_type = ASRUtils::expr_type(mask);
        if ( dim_ != nullptr ) {
            size_t dim_rank = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(dim_));
            if (dim_rank != 0) {
                append_error(diag, "dim argument to count must be a scalar and must not be an array",
                    dim_->base.loc);
                return nullptr;
            }

            overload_id = id_mask_dim;
        }
        if (array_rank == 1) {
            overload_id = id_mask;
        }
        if ( kind != nullptr) {
            size_t kind_rank = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(kind));
            if (kind_rank != 0) {
                append_error(diag, "kind argument to count must be a scalar and must not be an array",
                    kind->base.loc);
                return nullptr;
            }
        }
        ASR::expr_t *value = nullptr;
        Vec<ASR::expr_t*> arg_values; arg_values.reserve(al, 2);
        ASR::expr_t *mask_value = ASRUtils::expr_value(mask);
        arg_values.push_back(al, mask_value);
        if( mask ) {
            ASR::expr_t *mask_value = ASRUtils::expr_value(mask);
            arg_values.push_back(al, mask_value);
        }

        ASR::ttype_t* return_type = nullptr;
        if( overload_id == id_mask ) {
            return_type = int32;
        } else if( overload_id == id_mask_dim ) {
            Vec<ASR::dimension_t> dims;
            size_t n_dims = ASRUtils::extract_n_dims_from_ttype(mask_type);
            dims.reserve(al, (int) n_dims - 1);
            for( int i = 0; i < (int) n_dims - 1; i++ ) {
                ASR::dimension_t dim;
                dim.loc = mask->base.loc;
                dim.m_length = nullptr;
                dim.m_start = nullptr;
                dims.push_back(al, dim);
            }
            return_type = ASRUtils::make_Array_t_util(al, loc,
                int32, dims.p, dims.n, ASR::abiType::Source,
                false);
        } else if ( kind ) {
            int kind_value = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(kind))->m_n;
            return_type = TYPE(ASR::make_Integer_t(al, loc, kind_value));
        }
        // value = eval_Count(al, loc, return_type, arg_values, diag);
        value = nullptr;

        Vec<ASR::expr_t*> arr_intrinsic_args; arr_intrinsic_args.reserve(al, 1);
        arr_intrinsic_args.push_back(al, mask);
        if( dim_ && array_rank != 1 ) {
            arr_intrinsic_args.push_back(al, dim_);
        }
        return make_IntrinsicArrayFunction_t_util(al, loc,
            static_cast<int64_t>(IntrinsicArrayFunctions::Count),
            arr_intrinsic_args.p, arr_intrinsic_args.n, overload_id, return_type, value);
    }

    static inline ASR::expr_t *instantiate_Count(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*> &arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t> &m_args, int64_t overload_id) {
        declare_basic_variables("_lcompilers_count");
        fill_func_arg("mask", duplicate_type_with_empty_dims(al, arg_types[0]));
        if (overload_id == 0) {
            ASR::expr_t *result = declare("result", return_type, ReturnVar);
            /*
                for array of rank 2, the following code is generated:
                result = 0
                do i = lbound(mask, 2), ubound(mask, 2)
                    do j = lbound(mask, 1), ubound(mask, 1)
                        if (mask(j, i)) then
                            result = result + 1
                        end if
                    end do
                end do
            */
            ASR::dimension_t* array_dims = nullptr;
            int array_rank = extract_dimensions_from_ttype(arg_types[0], array_dims);
            std::vector<ASR::expr_t*> do_loop_variables;
            for (int i = 0; i < array_rank; i++) {
                do_loop_variables.push_back(declare("i_" + std::to_string(i), int32, Local));
            }
            body.push_back(al, b.Assignment(result, b.i(0, return_type)));
            ASR::stmt_t* do_loop = PassUtils::create_do_loop_helper_count(al, loc, do_loop_variables, args[0], result, array_rank);
            body.push_back(al, do_loop);
            body.push_back(al, Return());
            ASR::symbol_t *fn_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
                    body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
            scope->add_symbol(fn_name, fn_sym);
            return b.Call(fn_sym, m_args, return_type, nullptr);
        } else {
            fill_func_arg("dim", duplicate_type_with_empty_dims(al, arg_types[1]));
            ASR::expr_t *result = declare("result", return_type, Out);
            args.push_back(al, result);
            /*
                for array of rank 3, the following code is generated:
                dim == 2
                do i = 1, ubound(mask, 1)
                    do k = 1, ubound(mask, 3)
                        c = 0
                        do j = 1, ubound(mask, 2)
                            if (mask(i, j, k)) then
                                c = c + 1
                            end if
                        end do
                        res(i, k) = c
                    end do
                end do
            */
            int dim = ASR::down_cast<ASR::IntegerConstant_t>(m_args[1].m_value)->m_n;
            ASR::dimension_t* array_dims = nullptr;
            int array_rank = extract_dimensions_from_ttype(arg_types[0], array_dims);
            std::vector<ASR::expr_t*> res_idx;
            for (int i = 0; i < array_rank - 1; i++) {
                res_idx.push_back(declare("i_" + std::to_string(i), int32, Local));
            }
            ASR::expr_t* j = declare("j", int32, Local);
            ASR::expr_t* c = declare("c", int32, Local);

            std::vector<ASR::expr_t*> idx; bool dim_found = false;
            for (int i = 0; i < array_rank; i++) {
                if (i == dim - 1) {
                    idx.push_back(j);
                    dim_found = true;
                } else {
                    dim_found ? idx.push_back(res_idx[i-1]):
                                idx.push_back(res_idx[i]);
                }
            }
            ASR::stmt_t* inner_most_do_loop = b.DoLoop(j, LBound(args[0], dim), UBound(args[0], dim), {
                b.If(b.ArrayItem_01(args[0], idx), {
                    b.Assignment(c, b.Add(c, b.i32(1))),
                }, {})
            });
            ASR::stmt_t* do_loop = PassUtils::create_do_loop_helper_count_dim(al, loc,
                                    idx, res_idx, inner_most_do_loop, c, args[0], result, 0, dim);
            body.push_back(al, do_loop);
            body.push_back(al, Return());
            ASR::symbol_t *fn_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
                    body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
            scope->add_symbol(fn_name, fn_sym);
            return b.Call(fn_sym, m_args, return_type, nullptr);
        }
    }

} // namespace Count

namespace Pack {

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t &x,
            diag::Diagnostics& diagnostics) {
        require_impl(x.n_args == 2 || x.n_args == 3, "`pack` intrinsic accepts "
            "two or three arguments", x.base.base.loc, diagnostics);
        require_impl(x.m_args[0], "`array` argument of `pack` intrinsic "
            "cannot be nullptr", x.base.base.loc, diagnostics);
        require_impl(x.m_args[1], "`mask` argument of `pack` intrinsic "
            "cannot be nullptr", x.base.base.loc, diagnostics);
    }

    template<typename T>
    void populate_vector(std::vector<T> &a, ASR::expr_t *vector_a, int dim) {
        if (!vector_a) return;
        if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*vector_a)) {
            vector_a = ASR::down_cast<ASR::ArrayPhysicalCast_t>(vector_a)->m_arg;
        }
        LCOMPILERS_ASSERT(ASR::is_a<ASR::ArrayConstant_t>(*vector_a));
        ASR::ArrayConstant_t *a_const = ASR::down_cast<ASR::ArrayConstant_t>(vector_a);

        for (int i = 0; i < dim; i++) {
            ASR::expr_t* arg_a = a_const->m_args[i];

            if (ASR::is_a<ASR::IntegerConstant_t>(*arg_a)) {
                a[i] = ASR::down_cast<ASR::IntegerConstant_t>(arg_a)->m_n;
            } else if (ASR::is_a<ASR::RealConstant_t>(*arg_a)) {
                a[i] = ASR::down_cast<ASR::RealConstant_t>(arg_a)->m_r;
            } else if (ASR::is_a<ASR::LogicalConstant_t>(*arg_a)) {
                a[i] = ASR::down_cast<ASR::LogicalConstant_t>(arg_a)->m_value;
            } else {
                LCOMPILERS_ASSERT(false);
            }
        }
    }

    template<typename T>
    void populate_vector_complex(std::vector<T> &a, ASR::expr_t *vector_a, int dim) {
        if (!vector_a) return;
        if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*vector_a)) {
            vector_a = ASR::down_cast<ASR::ArrayPhysicalCast_t>(vector_a)->m_arg;
        }
        LCOMPILERS_ASSERT(ASR::is_a<ASR::ArrayConstant_t>(*vector_a));
        ASR::ArrayConstant_t *a_const = ASR::down_cast<ASR::ArrayConstant_t>(vector_a);

        for (int i = 0; i < dim; i++) {
            ASR::expr_t* arg_a = a_const->m_args[i];

            if (ASR::is_a<ASR::ComplexConstructor_t>(*arg_a)) {
                arg_a = ASR::down_cast<ASR::ComplexConstructor_t>(arg_a)->m_value;
            }
            if (arg_a && ASR::is_a<ASR::ComplexConstant_t>(*arg_a)) {
                ASR::ComplexConstant_t *c_a = ASR::down_cast<ASR::ComplexConstant_t>(arg_a);
                a[i] = {c_a->m_re, c_a->m_im};
            } else {
                LCOMPILERS_ASSERT(false);
            }
        }
    }

    template<typename T>
    void evaluate_Pack(std::vector<T> &a, std::vector<bool> &b, std::vector<T> &c, std::vector<T> &res) {
        int dim_array = a.size();
        int dim_vector = c.size();
        int i = 0;
        for (i = 0; i < dim_array; i++) {
            if (b[i]) res.push_back(a[i]);
        }

        for (i = res.size(); i < dim_vector; i++) {
            res.push_back(c[i]);
        }
    }

    static inline ASR::expr_t *eval_Pack(Allocator & al,
        const Location & loc, ASR::ttype_t *return_type, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        ASR::expr_t *array = args[0], *mask = args[1], *vector = args[2];
        ASR::ttype_t *type_array = ASRUtils::type_get_past_pointer(ASRUtils::type_get_past_allocatable(expr_type(array)));
        ASR::ttype_t *type_vector = ASRUtils::type_get_past_pointer(ASRUtils::type_get_past_allocatable(expr_type(vector)));
        ASR::ttype_t* type_a = ASRUtils::type_get_past_array(type_array);

        int kind = ASRUtils::extract_kind_from_ttype_t(type_a);
        int dim_array = ASRUtils::get_fixed_size_of_array(type_array);
        int dim_vector = 0;

        bool is_vector_present = false;
        if (vector) is_vector_present = true;
        if (is_vector_present) dim_vector = ASRUtils::get_fixed_size_of_array(type_vector);

        std::vector<bool> b(dim_array);
        populate_vector(b, mask, dim_array);

        if (ASRUtils::is_real(*type_a)) {
            if (kind == 4) {
                std::vector<float> a(dim_array), c(dim_vector), res;
                populate_vector(a, array, dim_array);
                populate_vector(c, vector, dim_vector);
                evaluate_Pack(a, b, c, res);
                Vec<ASR::expr_t*> values; values.reserve(al, res.size());
                for (auto it: res) {
                    values.push_back(al, EXPR(ASR::make_RealConstant_t(al, loc, it, real32)));
                }
                return EXPR(ASR::make_ArrayConstant_t(al, loc, values.p, values.n, return_type, ASR::arraystorageType::ColMajor));
            } else if (kind == 8) {
                std::vector<double> a(dim_array), c(dim_vector), res;
                populate_vector(a, array, dim_array);
                populate_vector(c, vector, dim_vector);
                evaluate_Pack(a, b, c, res);
                Vec<ASR::expr_t*> values; values.reserve(al, res.size());
                for (auto it: res) {
                    values.push_back(al, EXPR(ASR::make_RealConstant_t(al, loc, it, real64)));
                }
                return EXPR(ASR::make_ArrayConstant_t(al, loc, values.p, values.n, return_type, ASR::arraystorageType::ColMajor));
            } else {
                append_error(diag, "The `dot_product` intrinsic doesn't handle kind " + std::to_string(kind) + " yet", loc);
                return nullptr;
            }
        } else if (ASRUtils::is_integer(*type_a)) {
            if (kind == 4) {
                std::vector<int32_t> a(dim_array), c(dim_vector), res;
                populate_vector(a, array, dim_array);
                populate_vector(c, vector, dim_vector);
                evaluate_Pack(a, b, c, res);
                Vec<ASR::expr_t*> values; values.reserve(al, res.size());
                for (auto it: res) {
                    values.push_back(al, EXPR(ASR::make_IntegerConstant_t(al, loc, it, int32)));
                }
                return EXPR(ASR::make_ArrayConstant_t(al, loc, values.p, values.n, return_type, ASR::arraystorageType::ColMajor));
            } else if (kind == 8) {
                std::vector<int64_t> a(dim_array), c(dim_vector), res;
                populate_vector(a, array, dim_array);
                populate_vector(c, vector, dim_vector);
                evaluate_Pack(a, b, c, res);
                Vec<ASR::expr_t*> values; values.reserve(al, res.size());
                for (auto it: res) {
                    values.push_back(al, EXPR(ASR::make_IntegerConstant_t(al, loc, it, int64)));
                }
                return EXPR(ASR::make_ArrayConstant_t(al, loc, values.p, values.n, return_type, ASR::arraystorageType::ColMajor));
            } else {
                append_error(diag, "The `dot_product` intrinsic doesn't handle kind " + std::to_string(kind) + " yet", loc);
                return nullptr;
            }
        } else if (ASRUtils::is_logical(*type_a)) {
            std::vector<bool> a(dim_array), c(dim_vector), res;
            populate_vector(a, array, dim_array);
            populate_vector(c, vector, dim_vector);
            evaluate_Pack(a, b, c, res);
            Vec<ASR::expr_t*> values; values.reserve(al, res.size());
            for (auto it: res) {
                values.push_back(al, EXPR(ASR::make_LogicalConstant_t(al, loc, it, logical)));
            }
            return EXPR(ASR::make_ArrayConstant_t(al, loc, values.p, values.n, return_type, ASR::arraystorageType::ColMajor));
        } else if (ASRUtils::is_complex(*type_a)) {
            if (kind == 4) {
                std::vector<std::pair<float, float>> a(dim_array), c(dim_vector), res;
                populate_vector_complex(a, array, dim_array);
                populate_vector_complex(c, vector, dim_vector);
                evaluate_Pack(a, b, c, res);
                Vec<ASR::expr_t*> values; values.reserve(al, res.size());
                for (auto it: res) {
                    values.push_back(al, EXPR(ASR::make_ComplexConstant_t(al, loc, it.first, it.second, type_get_past_array(return_type))));
                }
                return EXPR(ASR::make_ArrayConstant_t(al, loc, values.p, values.n, return_type, ASR::arraystorageType::ColMajor));
            } else if (kind == 8) {
                std::vector<std::pair<double, double>> a(dim_array), c(dim_vector), res;
                populate_vector_complex(a, array, dim_array);
                populate_vector_complex(c, vector, dim_vector);
                evaluate_Pack(a, b, c, res);
                Vec<ASR::expr_t*> values; values.reserve(al, res.size());
                for (auto it: res) {
                    values.push_back(al, EXPR(ASR::make_ComplexConstant_t(al, loc, it.first, it.second, type_get_past_array(return_type))));
                }
                return EXPR(ASR::make_ArrayConstant_t(al, loc, values.p, values.n, return_type, ASR::arraystorageType::ColMajor));
            } else {
                append_error(diag, "The `dot_product` intrinsic doesn't handle kind " + std::to_string(kind) + " yet", loc);
                return nullptr;
            }
        } else {
            append_error(diag, "The `dot_product` intrinsic doesn't handle type " + ASRUtils::get_type_code(type_a) + " yet", loc);
            return nullptr;
        }
        return nullptr;
    }

    static inline ASR::asr_t* create_Pack(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            diag::Diagnostics& diag) {
        ASR::expr_t *array = args[0], *mask = args[1], *vector = args[2];
        bool is_type_allocatable = false;
        bool is_vector_present = false;
        if (ASRUtils::is_allocatable(array) || ASRUtils::is_allocatable(mask)) {
            // TODO: Use Array type as return type instead of allocatable
            //  for both Array and Allocatable as input arguments.
            is_type_allocatable = true;
        }
        if (vector) {
            is_vector_present = true;
        }

        ASR::ttype_t *type_array = expr_type(array);
        ASR::ttype_t *type_mask = expr_type(mask);
        ASR::ttype_t *type_vector = nullptr;
        if (is_vector_present) type_vector = expr_type(vector);
        ASR::ttype_t *ret_type = expr_type(array);
        bool mask_logical = is_logical(*type_mask);
        if( !mask_logical ) {
            append_error(diag, "The argument `mask` in `pack` must be of type Logical", mask->base.loc);
            return nullptr;
        }
        ASR::dimension_t* array_dims = nullptr;
        ASR::dimension_t* mask_dims = nullptr;
        ASR::dimension_t* vector_dims = nullptr;
        int array_rank = extract_dimensions_from_ttype(type_array, array_dims);
        int mask_rank = extract_dimensions_from_ttype(type_mask, mask_dims);
        int array_dim = -1, mask_dim = -1, fixed_size_array = -1;
        fixed_size_array = ASRUtils::get_fixed_size_of_array(type_array);
        extract_value(array_dims[0].m_length, array_dim);
        if (mask_rank == 0) {
            Vec<ASR::expr_t*> mask_expr; mask_expr.reserve(al, fixed_size_array);
            for (int i = 0; i < fixed_size_array; i++) {
                mask_expr.push_back(al, mask);
            }
            if (all_args_evaluated(mask_expr)) {
                mask = EXPR(ASR::make_ArrayConstant_t(al, mask->base.loc, mask_expr.p, mask_expr.n,
                        TYPE(ASR::make_Array_t(al, mask->base.loc, logical, array_dims, array_rank, ASR::array_physical_typeType::FixedSizeArray)),
                        ASR::arraystorageType::ColMajor));
            } else {
                mask = EXPR(ASR::make_ArrayConstructor_t(al, mask->base.loc, mask_expr.p, mask_expr.n,
                    TYPE(ASR::make_Array_t(al, mask->base.loc, logical, array_dims, array_rank, ASR::array_physical_typeType::FixedSizeArray)),
                    nullptr, ASR::arraystorageType::ColMajor));
            }
            type_mask = expr_type(mask);
            mask_rank = extract_dimensions_from_ttype(type_mask, mask_dims);
        }
        int vector_rank = 0;
        if (is_vector_present) {
            vector_rank = extract_dimensions_from_ttype(type_vector, vector_dims);
        }
        if (array_rank != mask_rank) {
            append_error(diag, "The argument `mask` must be of rank " + std::to_string(array_rank) +
                ", provided an array with rank, " + std::to_string(mask_rank), mask->base.loc);
            return nullptr;
        }
        if (!dimension_expr_equal(array_dims[0].m_length,
                mask_dims[0].m_length)) {
            append_error(diag, "The argument `mask` must be of dimension "
                + std::to_string(array_dim) + ", provided an array "
                "with dimension " + std::to_string(mask_dim), mask->base.loc);
            return nullptr;
        }
        if (is_vector_present && vector_rank != 1) {
            append_error(diag, "`pack` accepts vector of rank 1 only, provided an array "
                "with rank, " + std::to_string(vector_rank), vector->base.loc);
            return nullptr;
        }

        ASRBuilder b(al, loc);
        Vec<ASR::dimension_t> result_dims; result_dims.reserve(al, 1);
        int overload_id = 2;
        if (is_vector_present) {
            result_dims.push_back(al, b.set_dim(vector_dims[0].m_start, vector_dims[0].m_length));
            ret_type = ASRUtils::duplicate_type(al, ret_type, &result_dims);
        } else {
            Vec<ASR::expr_t*> args_count; args_count.reserve(al, 1); args_count.push_back(al, mask);
            ASR::expr_t* count = EXPR(Count::create_Count(al, loc, args_count, diag));
            result_dims.push_back(al, b.set_dim(array_dims[0].m_start, count));
            ret_type = ASRUtils::duplicate_type(al, ret_type, &result_dims, ASR::array_physical_typeType::DescriptorArray, true);
        }
        if (is_type_allocatable) {
            ret_type = TYPE(ASR::make_Allocatable_t(al, loc, ret_type));
        }
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, array); m_args.push_back(al, mask);
        if (is_vector_present) {
            m_args.push_back(al, vector);
            overload_id = 3;
        }
        ASR::expr_t *value = nullptr;
        if (all_args_evaluated(m_args)) {
            value = eval_Pack(al, loc, ret_type, m_args, diag);
        }
        return make_IntrinsicArrayFunction_t_util(al, loc,
            static_cast<int64_t>(IntrinsicArrayFunctions::Pack),
            m_args.p, m_args.n, overload_id, ret_type, value);
    }

    static inline ASR::expr_t *instantiate_Pack(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*> &arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t> &m_args, int64_t overload_id) {
        declare_basic_variables("_lcompilers_pack");
        fill_func_arg("array", duplicate_type_with_empty_dims(al, arg_types[0]));
        fill_func_arg("mask", duplicate_type_with_empty_dims(al, arg_types[1]));
        if (overload_id == 3) {
            fill_func_arg("vector", duplicate_type_with_empty_dims(al, arg_types[2]));
        }
        ASR::ttype_t* ret_type = return_type;
        if (overload_id == 2) {
            ret_type = ASRUtils::duplicate_type(al, return_type, nullptr, ASRUtils::extract_physical_type(return_type), true);
            LCOMPILERS_ASSERT(ASR::is_a<ASR::Array_t>(*ret_type));
            ASR::Array_t *ret_type_array = ASR::down_cast<ASR::Array_t>(ret_type);
            if (ASR::is_a<ASR::FunctionCall_t>(*ret_type_array->m_dims[0].m_length)) {
                ASR::FunctionCall_t *func_call = ASR::down_cast<ASR::FunctionCall_t>(ret_type_array->m_dims[0].m_length);
                if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*func_call->m_args[0].m_value)) {
                    ASR::ArrayPhysicalCast_t *array_cast = ASR::down_cast<ASR::ArrayPhysicalCast_t>(func_call->m_args[0].m_value);
                    array_cast->m_arg = args[1];
                    array_cast->m_old = ASRUtils::extract_physical_type(arg_types[1]);
                    array_cast->m_type = ASRUtils::duplicate_type_with_empty_dims(al, array_cast->m_type);

                    ret_type = TYPE(ASR::make_Array_t(al, loc, ret_type_array->m_type, ret_type_array->m_dims,
                                ret_type_array->n_dims, ret_type_array->m_physical_type));
                } else {
                    ret_type = return_type;
                }
            } else if (ASR::is_a<ASR::IntrinsicArrayFunction_t>(*ret_type_array->m_dims[0].m_length)) {
                ASR::IntrinsicArrayFunction_t *intrinsic_array = ASR::down_cast<ASR::IntrinsicArrayFunction_t>(ret_type_array->m_dims[0].m_length);
                if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*intrinsic_array->m_args[0])) {
                    ASR::ArrayPhysicalCast_t *array_cast = ASR::down_cast<ASR::ArrayPhysicalCast_t>(intrinsic_array->m_args[0]);
                    array_cast->m_arg = args[1];
                    array_cast->m_old = ASRUtils::extract_physical_type(arg_types[1]);
                    array_cast->m_type = ASRUtils::duplicate_type_with_empty_dims(al, array_cast->m_type);

                    ret_type = TYPE(ASR::make_Array_t(al, loc, ret_type_array->m_type, ret_type_array->m_dims,
                                ret_type_array->n_dims, ret_type_array->m_physical_type));
                } else {
                    ret_type = return_type;
                }
            } else {
                ret_type = return_type;
            }
        }
        ASR::expr_t *result = declare("result", ret_type, Out);
        args.push_back(al, result);
        /*
            For array of rank 2, the following code is generated:
            k = lbound(vector, 1)
            print *, k
            do i = lbound(array, 2), ubound(array, 2)
                do j = lbound(array, 1), ubound(array, 1)
                    ! print *, "mask(", j, ",", i, ") ", mask(j, i)
                    if (mask(j, i)) then
                        res(k) = array(j, i)
                        ! print *, "array(", j, ",", i, ") ", array(j, i)
                        ! print *, "res(", k, ") ", res(k)
                        k = k + 1
                    end if
                end do
            end do

            do i = k, ubound(vector, 1)
                res(k) = vector(k)
                k = k + 1
            end do
        */
        ASR::dimension_t* array_dims = nullptr;
        int array_rank = extract_dimensions_from_ttype(arg_types[0], array_dims);
        std::vector<ASR::expr_t*> do_loop_variables;
        for (int i = 0; i < array_rank; i++) {
            do_loop_variables.push_back(declare("i_" + std::to_string(i), int32, Local));
        }
        ASR::expr_t *k = declare("k", int32, Local);
        body.push_back(al, b.Assignment(k, b.i32(1)));
        ASR::stmt_t* do_loop = PassUtils::create_do_loop_helper_pack(al, loc, do_loop_variables, args[0], args[1], result, k, array_rank);
        body.push_back(al, do_loop);

        if (overload_id == 3) {
            body.push_back(al, b.DoLoop(do_loop_variables[0], k, UBound(args[2], 1), {
                b.Assignment(b.ArrayItem_01(result, {k}), b.ArrayItem_01(args[2], {k})),
                b.Assignment(k, b.Add(k, b.i32(1)))
            }));
        }
        body.push_back(al, Return());
        ASR::symbol_t *fn_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
                body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, fn_sym);
        return b.Call(fn_sym, m_args, return_type, nullptr);
    }

} // namespace Pack

namespace Unpack {

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t &x,
            diag::Diagnostics& diagnostics) {
        require_impl(x.n_args == 3, "`unpack` intrinsic accepts "
            "three arguments", x.base.base.loc, diagnostics);
        require_impl(x.m_args[0], "`vector` argument of `unpack` intrinsic "
            "cannot be nullptr", x.base.base.loc, diagnostics);
        require_impl(x.m_args[1], "`mask` argument of `unpack` intrinsic "
            "cannot be nullptr", x.base.base.loc, diagnostics);
        require_impl(x.m_args[2], "`field` argument of `unpack` intrinsic "
            "cannot be nullptr", x.base.base.loc, diagnostics);
    }

    template<typename T>
    void populate_vector(std::vector<T> &a, ASR::expr_t *vector_a, int dim) {
        if (!vector_a) return;
        if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*vector_a)) {
            vector_a = ASR::down_cast<ASR::ArrayPhysicalCast_t>(vector_a)->m_arg;
        }
        LCOMPILERS_ASSERT(ASR::is_a<ASR::ArrayConstant_t>(*vector_a));
        ASR::ArrayConstant_t *a_const = ASR::down_cast<ASR::ArrayConstant_t>(vector_a);

        for (int i = 0; i < dim; i++) {
            ASR::expr_t* arg_a = a_const->m_args[i];

            if (ASR::is_a<ASR::IntegerConstant_t>(*arg_a)) {
                a[i] = ASR::down_cast<ASR::IntegerConstant_t>(arg_a)->m_n;
            } else if (ASR::is_a<ASR::RealConstant_t>(*arg_a)) {
                a[i] = ASR::down_cast<ASR::RealConstant_t>(arg_a)->m_r;
            } else if (ASR::is_a<ASR::LogicalConstant_t>(*arg_a)) {
                a[i] = ASR::down_cast<ASR::LogicalConstant_t>(arg_a)->m_value;
            } else {
                LCOMPILERS_ASSERT(false);
            }
        }
    }

    template<typename T>
    void populate_vector_complex(std::vector<T> &a, ASR::expr_t *vector_a, int dim) {
        if (!vector_a) return;
        if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*vector_a)) {
            vector_a = ASR::down_cast<ASR::ArrayPhysicalCast_t>(vector_a)->m_arg;
        }
        LCOMPILERS_ASSERT(ASR::is_a<ASR::ArrayConstant_t>(*vector_a));
        ASR::ArrayConstant_t *a_const = ASR::down_cast<ASR::ArrayConstant_t>(vector_a);

        for (int i = 0; i < dim; i++) {
            ASR::expr_t* arg_a = a_const->m_args[i];

            if (ASR::is_a<ASR::ComplexConstructor_t>(*arg_a)) {
                arg_a = ASR::down_cast<ASR::ComplexConstructor_t>(arg_a)->m_value;
            }
            if (arg_a && ASR::is_a<ASR::ComplexConstant_t>(*arg_a)) {
                ASR::ComplexConstant_t *c_a = ASR::down_cast<ASR::ComplexConstant_t>(arg_a);
                a[i] = {c_a->m_re, c_a->m_im};
            } else {
                LCOMPILERS_ASSERT(false);
            }
        }
    }

    static inline ASR::expr_t *eval_Unpack(Allocator & al,
        const Location & loc, ASR::ttype_t *return_type, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        ASR::expr_t * vector= args[0], *mask = args[1], *field = args[2];
        ASR::ttype_t *type_vector = ASRUtils::type_get_past_pointer(ASRUtils::type_get_past_allocatable(expr_type(vector)));
        ASR::ttype_t *type_mask = ASRUtils::type_get_past_pointer(ASRUtils::type_get_past_allocatable(expr_type(mask)));
        ASR::ttype_t* type_a = ASRUtils::type_get_past_array(type_vector);

        int kind = ASRUtils::extract_kind_from_ttype_t(type_a);
        int dim_mask = ASRUtils::get_fixed_size_of_array(type_mask);
        int dim_vector = ASRUtils::get_fixed_size_of_array(type_vector);

        std::vector<bool> b(dim_mask);
        populate_vector(b, mask, dim_mask);

        if (ASRUtils::is_real(*type_a)) {
            if (kind == 4) {
                std::vector<float> a(dim_vector), c(dim_mask);
                populate_vector(a, vector, dim_vector);
                populate_vector(c, field, dim_mask);
                Vec<ASR::expr_t*> values; values.reserve(al, b.size());

                for (int i = 0; i < dim_mask; i++) {
                    if (b[i]) {
                        values.push_back(al, EXPR(ASR::make_RealConstant_t(al, loc, a[i], real32)));
                    } else {
                        values.push_back(al, EXPR(ASR::make_RealConstant_t(al, loc, c[i], real32)));
                    }
                }

                return EXPR(ASR::make_ArrayConstant_t(al, loc, values.p, values.n, return_type, ASR::arraystorageType::ColMajor));
            } else if (kind == 8) {
                std::vector<double> a(dim_vector), c(dim_mask);
                populate_vector(a, vector, dim_vector);
                populate_vector(c, field, dim_mask);
                Vec<ASR::expr_t*> values; values.reserve(al, b.size());

                for (int i = 0; i < dim_mask; i++) {
                    if (b[i]) {
                        values.push_back(al, EXPR(ASR::make_RealConstant_t(al, loc, a[i], real64)));
                    } else {
                        values.push_back(al, EXPR(ASR::make_RealConstant_t(al, loc, c[i], real64)));
                    }
                }
                return EXPR(ASR::make_ArrayConstant_t(al, loc, values.p, values.n, return_type, ASR::arraystorageType::ColMajor));
            } else {
                append_error(diag, "The `unpack` intrinsic doesn't handle kind " + std::to_string(kind) + " yet", loc);
                return nullptr;
            }
        } else if (ASRUtils::is_integer(*type_a)) {
            if (kind == 4) {
                std::vector<int32_t> a(dim_vector), c(dim_mask);
                populate_vector(a, vector, dim_vector);
                populate_vector(c, field, dim_mask);
                Vec<ASR::expr_t*> values; values.reserve(al, b.size());

                for (int i = 0; i < dim_mask; i++) {
                    if (b[i]) {
                        values.push_back(al, EXPR(ASR::make_IntegerConstant_t(al, loc, a[i], int32)));
                    } else {
                        values.push_back(al, EXPR(ASR::make_IntegerConstant_t(al, loc, c[i], int32)));
                    }
                }
                return EXPR(ASR::make_ArrayConstant_t(al, loc, values.p, values.n, return_type, ASR::arraystorageType::ColMajor));
            } else if (kind == 8) {
                std::vector<int64_t> a(dim_vector), c(dim_mask);
                populate_vector(a, vector, dim_vector);
                populate_vector(c, field, dim_mask);
                Vec<ASR::expr_t*> values; values.reserve(al, b.size());

                for (int i = 0; i < dim_mask; i++) {
                    if (b[i]) {
                        values.push_back(al, EXPR(ASR::make_IntegerConstant_t(al, loc, a[i], int64)));
                    } else {
                        values.push_back(al, EXPR(ASR::make_IntegerConstant_t(al, loc, c[i], int64)));
                    }
                }
                return EXPR(ASR::make_ArrayConstant_t(al, loc, values.p, values.n, return_type, ASR::arraystorageType::ColMajor));
            } else {
                append_error(diag, "The `unpack` intrinsic doesn't handle kind " + std::to_string(kind) + " yet", loc);
                return nullptr;
            }
        } else if (ASRUtils::is_logical(*type_a)) {
            std::vector<bool> a(dim_vector), c(dim_mask);
            populate_vector(a, vector, dim_vector);
            populate_vector(c, field, dim_mask);
            Vec<ASR::expr_t*> values; values.reserve(al, b.size());

            for (int i = 0; i < dim_mask; i++) {
                if (b[i]) {
                    values.push_back(al, EXPR(ASR::make_LogicalConstant_t(al, loc, a[i], logical)));
                } else {
                    values.push_back(al, EXPR(ASR::make_LogicalConstant_t(al, loc, c[i], logical)));
                }
            }
            return EXPR(ASR::make_ArrayConstant_t(al, loc, values.p, values.n, return_type, ASR::arraystorageType::ColMajor));

        } else if (ASRUtils::is_complex(*type_a)) {
            if (kind == 4) {
                std::vector<std::pair<float, float>> a(dim_vector), c(dim_mask);
                populate_vector_complex(a, vector, dim_vector);
                populate_vector_complex(c, field, dim_mask);
                Vec<ASR::expr_t*> values; values.reserve(al, b.size());

                for (int i = 0; i < dim_mask; i++) {
                    if (b[i]) {
                        values.push_back(al, EXPR(ASR::make_ComplexConstant_t(al, loc, a[i].first, a[i].second, type_get_past_array(return_type))));
                    } else {
                        values.push_back(al, EXPR(ASR::make_ComplexConstant_t(al, loc, c[i].first, c[i].second, type_get_past_array(return_type))));
                    }
                }
                return EXPR(ASR::make_ArrayConstant_t(al, loc, values.p, values.n, return_type, ASR::arraystorageType::ColMajor));
            } else if (kind == 8) {
                std::vector<std::pair<double, double>> a(dim_vector), c(dim_mask);
                populate_vector_complex(a, vector, dim_vector);
                populate_vector_complex(c, field, dim_mask);
                Vec<ASR::expr_t*> values; values.reserve(al, b.size());

                for (int i = 0; i < dim_mask; i++) {
                    if (b[i]) {
                        values.push_back(al, EXPR(ASR::make_ComplexConstant_t(al, loc, a[i].first, a[i].second, type_get_past_array(return_type))));
                    } else {
                        values.push_back(al, EXPR(ASR::make_ComplexConstant_t(al, loc, c[i].first, c[i].second, type_get_past_array(return_type))));
                    }
                }
                return EXPR(ASR::make_ArrayConstant_t(al, loc, values.p, values.n, return_type, ASR::arraystorageType::ColMajor));
            } else {
                append_error(diag, "The `unpack` intrinsic doesn't handle kind " + std::to_string(kind) + " yet", loc);
                return nullptr;
            }
        } else {
            append_error(diag, "The `unpack` intrinsic doesn't handle type " + ASRUtils::get_type_code(type_a) + " yet", loc);
        }
        return nullptr;
    }

    static inline ASR::asr_t* create_Unpack(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            diag::Diagnostics& diag) {
        ASR::expr_t *vector = args[0], *mask = args[1], *field = args[2];
        bool is_type_allocatable = false;
        if (ASRUtils::is_allocatable(field) || ASRUtils::is_allocatable(mask)) {
            // TODO: Use Array type as return type instead of allocatable
            //  for both Array and Allocatable as input arguments.
            is_type_allocatable = true;
        }

        ASR::ttype_t *type_vector = expr_type(vector);
        ASR::ttype_t *type_mask = expr_type(mask);
        ASR::ttype_t *type_field = expr_type(field);
        ASR::ttype_t *ret_type = type_field;
        bool mask_logical = is_logical(*type_mask);
        if( !mask_logical ) {
            append_error(diag, "The argument `mask` in `unpack` must be of type Logical", mask->base.loc);
            return nullptr;
        }
        ASR::dimension_t* vector_dims = nullptr;
        ASR::dimension_t* mask_dims = nullptr;
        ASR::dimension_t* field_dims = nullptr;
        int vector_rank = extract_dimensions_from_ttype(type_vector, vector_dims);
        int mask_rank = extract_dimensions_from_ttype(type_mask, mask_dims);
        int field_rank = extract_dimensions_from_ttype(type_field, field_dims);
        int vector_dim = -1, mask_dim = -1, field_dim = -1;
        extract_value(vector_dims[0].m_length, vector_dim);
        extract_value(mask_dims[0].m_length, mask_dim);
        extract_value(field_dims[0].m_length, field_dim);
        if (vector_rank != 1) {
            append_error(diag, "`unpack` accepts vector of rank 1 only, provided an array "
                "with rank, " + std::to_string(vector_rank), vector->base.loc);
            return nullptr;
        }
        if (mask_rank == 0) {
            append_error(diag, "The argument `mask` in `unpack` must be an array and not a scalar", mask->base.loc);
        }
        if (field_rank != mask_rank) {
            append_error(diag, "The argument `field` must be of rank " + std::to_string(mask_rank) +
                ", provided an array with rank, " + std::to_string(field_rank), mask->base.loc);
            return nullptr;
        }
        if (!dimension_expr_equal(field_dims[0].m_length,
                mask_dims[0].m_length)) {
            append_error(diag, "The argument `field` must be of dimension "
                + std::to_string(mask_dim) + ", provided an array "
                "with dimension " + std::to_string(field_dim), mask->base.loc);
            return nullptr;
        }
        ASRBuilder b(al, loc);
        Vec<ASR::dimension_t> result_dims; result_dims.reserve(al, 1);
        int overload_id = 2;
        for (int i = 0; i < mask_rank; i++) {
            result_dims.push_back(al, b.set_dim(mask_dims[i].m_start, mask_dims[i].m_length));
        }
        ret_type = ASRUtils::duplicate_type(al, ret_type, &result_dims);
        if (is_type_allocatable) {
            ret_type = TYPE(ASR::make_Allocatable_t(al, loc, ret_type));
        }
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 3);
        m_args.push_back(al, vector); m_args.push_back(al, mask); m_args.push_back(al, field);
        ASR::expr_t *value = nullptr;
        if (all_args_evaluated(m_args)) {
            value = eval_Unpack(al, loc, ret_type, m_args, diag);
        }
        return make_IntrinsicArrayFunction_t_util(al, loc,
            static_cast<int64_t>(IntrinsicArrayFunctions::Unpack),
            m_args.p, m_args.n, overload_id, ret_type, value);
    }

    static inline ASR::expr_t *instantiate_Unpack(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*> &arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t> &m_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_unpack");
        fill_func_arg("vector", duplicate_type_with_empty_dims(al, arg_types[0]));
        fill_func_arg("mask", duplicate_type_with_empty_dims(al, arg_types[1]));
        fill_func_arg("field", duplicate_type_with_empty_dims(al, arg_types[2]));
        ASR::expr_t *result = declare("result", return_type, Out);
        args.push_back(al, result);
        /*
            For array of rank 2, the following code is generated:
            k = lbound(vector, 1)
            res = field
            do i = lbound(mask, 2), ubound(mask, 2)
                do j = lbound(mask, 1), ubound(mask, 1)
                    print *, "mask(", j, i, ") = ", mask(j, i)
                    if (mask(j, i)) then
                        res(j, i) = vector(k)
                        k = k + 1
                    end if
                end do
            end do
        */
        ASR::dimension_t* array_dims = nullptr;
        int mask_rank = extract_dimensions_from_ttype(arg_types[1], array_dims);
        std::vector<ASR::expr_t*> do_loop_variables;
        for (int i = 0; i < mask_rank; i++) {
            do_loop_variables.push_back(declare("i_" + std::to_string(i), int32, Local));
        }
        ASR::expr_t *k = declare("k", int32, Local);
        body.push_back(al, b.Assignment(k, LBound(args[0], 1)));
        body.push_back(al, b.Assignment(result, args[2]));
        ASR::stmt_t* do_loop = PassUtils::create_do_loop_helper_unpack(al, loc, do_loop_variables, args[0], args[1], result, k, mask_rank);
        body.push_back(al, do_loop);
        body.push_back(al, Return());
        ASR::symbol_t *fn_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
                body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, fn_sym);
        return b.Call(fn_sym, m_args, return_type, nullptr);
    }

} // namespace Unpack

namespace DotProduct {

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t &x,
            diag::Diagnostics& diagnostics) {
        require_impl(x.n_args == 2, "`dot_product` intrinsic accepts exactly"
            "two arguments", x.base.base.loc, diagnostics);
        require_impl(x.m_args[0], "`vector_a` argument of `dot_product` intrinsic "
            "cannot be nullptr", x.base.base.loc, diagnostics);
        require_impl(x.m_args[1], "`vector_b` argument of `dot_product` intrinsic "
            "cannot be nullptr", x.base.base.loc, diagnostics);
    }

    template<typename T>
    void populate_vector_complex(std::vector<T> &a, std::vector<T>& b, ASR::expr_t *vector_a, ASR::expr_t* vector_b, int dim) {
        if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*vector_a)) {
            vector_a = ASR::down_cast<ASR::ArrayPhysicalCast_t>(vector_a)->m_arg;
        }
        if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*vector_b)) {
            vector_b = ASR::down_cast<ASR::ArrayPhysicalCast_t>(vector_b)->m_arg;
        }
        LCOMPILERS_ASSERT(ASR::is_a<ASR::ArrayConstant_t>(*vector_a) && ASR::is_a<ASR::ArrayConstant_t>(*vector_b));
        ASR::ArrayConstant_t *a_const = ASR::down_cast<ASR::ArrayConstant_t>(vector_a);
        ASR::ArrayConstant_t *b_const = ASR::down_cast<ASR::ArrayConstant_t>(vector_b);

        for (int i = 0; i < dim; i++) {
            ASR::expr_t* arg_a = a_const->m_args[i];
            ASR::expr_t* arg_b = b_const->m_args[i];

            if (ASR::is_a<ASR::ComplexConstructor_t>(*arg_a)) {
                arg_a = ASR::down_cast<ASR::ComplexConstructor_t>(arg_a)->m_value;
            }
            if (ASR::is_a<ASR::ComplexConstructor_t>(*arg_b)) {
                arg_b = ASR::down_cast<ASR::ComplexConstructor_t>(arg_b)->m_value;
            }

            if (arg_a && arg_b && ASR::is_a<ASR::ComplexConstant_t>(*arg_a)) {
                ASR::ComplexConstant_t *c_a = ASR::down_cast<ASR::ComplexConstant_t>(arg_a);
                ASR::ComplexConstant_t *c_b = ASR::down_cast<ASR::ComplexConstant_t>(arg_b);
                a[i] = {c_a->m_re, c_a->m_im};
                b[i] = {c_b->m_re, c_b->m_im};
            } else {
                LCOMPILERS_ASSERT(false);
            }
        }
    }

    template<typename T>
    void populate_vector(std::vector<T> &a, std::vector<T>& b, ASR::expr_t *vector_a, ASR::expr_t* vector_b, int dim) {
        if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*vector_a)) {
            vector_a = ASR::down_cast<ASR::ArrayPhysicalCast_t>(vector_a)->m_arg;
        }
        if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*vector_b)) {
            vector_b = ASR::down_cast<ASR::ArrayPhysicalCast_t>(vector_b)->m_arg;
        }
        LCOMPILERS_ASSERT(ASR::is_a<ASR::ArrayConstant_t>(*vector_a) && ASR::is_a<ASR::ArrayConstant_t>(*vector_b));
        ASR::ArrayConstant_t *a_const = ASR::down_cast<ASR::ArrayConstant_t>(vector_a);
        ASR::ArrayConstant_t *b_const = ASR::down_cast<ASR::ArrayConstant_t>(vector_b);

        for (int i = 0; i < dim; i++) {
            ASR::expr_t* arg_a = a_const->m_args[i];
            ASR::expr_t* arg_b = b_const->m_args[i];

            if (ASR::is_a<ASR::IntegerConstant_t>(*arg_a)) {
                a[i] = ASR::down_cast<ASR::IntegerConstant_t>(arg_a)->m_n;
                b[i] = ASR::down_cast<ASR::IntegerConstant_t>(arg_b)->m_n;
            } else if (ASR::is_a<ASR::RealConstant_t>(*arg_a)) {
                a[i] = ASR::down_cast<ASR::RealConstant_t>(arg_a)->m_r;
                b[i] = ASR::down_cast<ASR::RealConstant_t>(arg_b)->m_r;
            } else if (ASR::is_a<ASR::LogicalConstant_t>(*arg_a)) {
                a[i] = ASR::down_cast<ASR::LogicalConstant_t>(arg_a)->m_value;
                b[i] = ASR::down_cast<ASR::LogicalConstant_t>(arg_b)->m_value;
            } else {
                LCOMPILERS_ASSERT(false);
            }
        }
    }

    static inline ASR::expr_t *eval_DotProduct(Allocator & al,
        const Location & loc, ASR::ttype_t *return_type, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        ASR::expr_t *vector_a = args[0], *vector_b = args[1];
        ASR::ttype_t *type_vector_a = ASRUtils::type_get_past_pointer(ASRUtils::type_get_past_allocatable(expr_type(vector_a)));
        ASR::ttype_t* type_a = ASRUtils::type_get_past_array(type_vector_a);

        int kind = ASRUtils::extract_kind_from_ttype_t(type_a);
        int dim = ASRUtils::get_fixed_size_of_array(type_vector_a);

        if (ASRUtils::is_real(*type_a)) {
            if (kind == 4) {
                std::vector<float> a(dim), b(dim);
                populate_vector(a, b, vector_a, vector_b, dim);
                float result = std::inner_product(a.begin(), a.end(), b.begin(), 0.0f);
                return make_ConstantWithType(make_RealConstant_t, result, return_type, loc);
            } else if (kind == 8) {
                std::vector<double> a(dim), b(dim);
                populate_vector(a, b, vector_a, vector_b, dim);
                double result = std::inner_product(a.begin(), a.end(), b.begin(), 0.0);
                return make_ConstantWithType(make_RealConstant_t, result, return_type, loc);
            } else {
                append_error(diag, "The `dot_product` intrinsic doesn't handle kind " + std::to_string(kind) + " yet", loc);
                return nullptr;
            }
        } else if (ASRUtils::is_integer(*type_a)) {
            if (kind == 4) {
                std::vector<int32_t> a(dim), b(dim);
                populate_vector(a, b, vector_a, vector_b, dim);
                int32_t result = std::inner_product(a.begin(), a.end(), b.begin(), 0);
                return make_ConstantWithType(make_IntegerConstant_t, result, return_type, loc);
            } else if (kind == 8) {
                std::vector<int64_t> a(dim), b(dim);
                populate_vector(a, b, vector_a, vector_b, dim);
                int64_t result = std::inner_product(a.begin(), a.end(), b.begin(), 0);
                return make_ConstantWithType(make_IntegerConstant_t, result, return_type, loc);
            } else {
                append_error(diag, "The `dot_product` intrinsic doesn't handle kind " + std::to_string(kind) + " yet", loc);
                return nullptr;
            }
        } else if (ASRUtils::is_logical(*type_a)) {
            std::vector<bool> a(dim), b(dim);
            populate_vector(a, b, vector_a, vector_b, dim);
            bool result = false;
            for (int i = 0; i < dim; i++) {
                result = result || (a[i] && b[i]);
            }
            return make_ConstantWithType(make_LogicalConstant_t, result, return_type, loc);
        } else if (ASRUtils::is_complex(*type_a)) {
            if (kind == 4) {
                std::vector<std::pair<float, float>> a(dim), b(dim);
                populate_vector_complex(a, b, vector_a, vector_b, dim);
                std::pair<float, float> result = {0.0f, 0.0f};
                for (int i = 0; i < dim; i++) {
                    result.first += a[i].first * b[i].first + (a[i].second * b[i].second);
                    result.second += a[i].first * b[i].second + ((-a[i].second)* b[i].first);
                }
                return EXPR(make_ComplexConstant_t(al, loc, result.first, result.second, return_type));
            } else if (kind == 8) {
                std::vector<std::pair<double, double>> a(dim), b(dim);
                populate_vector_complex(a, b, vector_a, vector_b, dim);
                std::pair<double, double> result = {0.0, 0.0};
                for (int i = 0; i < dim; i++) {
                    result.first += a[i].first * b[i].first + (a[i].second * b[i].second);
                    result.second += a[i].first * b[i].second + ((-a[i].second)* b[i].first);
                }
                return EXPR(make_ComplexConstant_t(al, loc, result.first, result.second, return_type));
            } else {
                append_error(diag, "The `dot_product` intrinsic doesn't handle kind " + std::to_string(kind) + " yet", loc);
                return nullptr;
            }
        } else {
            append_error(diag, "The `dot_product` intrinsic doesn't handle type " + ASRUtils::get_type_code(type_a) + " yet", loc);
            return nullptr;
        }
        return nullptr;
    }

    static inline ASR::asr_t* create_DotProduct(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            diag::Diagnostics& diag) {
        ASR::expr_t *matrix_a = args[0], *matrix_b = args[1];
        ASR::ttype_t *type_a = expr_type(matrix_a);
        ASR::ttype_t *type_b = expr_type(matrix_b);
        ASR::ttype_t *ret_type = nullptr;
        bool matrix_a_numeric = is_integer(*type_a) ||
                                is_real(*type_a) ||
                                is_complex(*type_a);
        bool matrix_a_logical = is_logical(*type_a);
        bool matrix_b_numeric = is_integer(*type_b) ||
                                is_real(*type_b) ||
                                is_complex(*type_b);
        bool matrix_b_logical = is_logical(*type_b);
        if ( !matrix_a_numeric && !matrix_a_logical ) {
            append_error(diag, "The argument `matrix_a` in `dot_product` must be of type Integer, "
                "Real, Complex or Logical", matrix_a->base.loc);
            return nullptr;
        } else if ( matrix_a_numeric ) {
            if( !matrix_b_numeric ) {
                append_error(diag, "The argument `matrix_b` in `dot_product` must be of type "
                    "Integer, Real or Complex if first matrix is of numeric "
                    "type", matrix_b->base.loc);
                    return nullptr;
            }
        } else {
            if( !matrix_b_logical ) {
                append_error(diag, "The argument `matrix_b` in `dot_product` must be of type Logical"
                    " if first matrix is of Logical type", matrix_b->base.loc);
                return nullptr;
            }
        }
        ret_type = extract_type(type_a);
        ASR::dimension_t* matrix_a_dims = nullptr;
        ASR::dimension_t* matrix_b_dims = nullptr;
        int matrix_a_rank = extract_dimensions_from_ttype(type_a, matrix_a_dims);
        int matrix_b_rank = extract_dimensions_from_ttype(type_b, matrix_b_dims);
        if ( matrix_a_rank != 1) {
            append_error(diag, "`dot_product` accepts arrays of rank 1 only, provided an array "
                "with rank, " + std::to_string(matrix_a_rank), matrix_a->base.loc);
            return nullptr;
        } else if ( matrix_b_rank != 1 ) {
            append_error(diag, "`dot_product` accepts arrays of rank 1 only, provided an array "
                "with rank, " + std::to_string(matrix_b_rank), matrix_b->base.loc);
            return nullptr;
        }

        int overload_id = 1;
        int matrix_a_dim_1 = -1, matrix_b_dim_1 = -1;
        if ( !dimension_expr_equal(matrix_a_dims[0].m_length, matrix_b_dims[0].m_length) ) {
            append_error(diag, "The argument `matrix_b` must be of dimension "
                + std::to_string(matrix_a_dim_1) + ", provided an array "
                "with dimension " + std::to_string(matrix_b_dim_1) +
                " in `matrix_b('n')`", matrix_b->base.loc);
            return nullptr;
        }

        ASR::expr_t *value = nullptr;
        if (all_args_evaluated(args)) {
            value = eval_DotProduct(al, loc, ret_type, args, diag);
        }
        return make_IntrinsicArrayFunction_t_util(al, loc,
            static_cast<int64_t>(IntrinsicArrayFunctions::DotProduct),
            args.p, args.n, overload_id, ret_type, value);
    }

    static inline ASR::expr_t *instantiate_DotProduct(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*> &arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t> &m_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_dot_product");
        fill_func_arg("matrix_a", duplicate_type_with_empty_dims(al, arg_types[0]));
        fill_func_arg("matrix_b", duplicate_type_with_empty_dims(al, arg_types[1]));
        ASR::expr_t *result = declare("result", return_type, ReturnVar);
        ASR::expr_t *i = declare("i", int32, Local);
        /*
            res = 0
            do i = LBound(matrix_a, 1), UBound(matrix_a, 1)
                res = res + matrix_a(i) * matrix_b(i)
            end do
        */
        if (is_logical(*return_type)) {
            body.push_back(al, b.Assignment(result, ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, loc, false, return_type))));
            body.push_back(al, b.DoLoop(i, LBound(args[0], 1), UBound(args[0], 1), {
                b.Assignment(result, b.LogicalOr(result, b.And(b.ArrayItem_01(args[0], {i}), b.ArrayItem_01(args[1], {i})), loc))
            }));
        } else if (is_complex(*return_type)) {
            body.push_back(al, b.Assignment(result, EXPR(ASR::make_ComplexConstant_t(al, loc, 0.0, 0.0, return_type))));

            Vec<ASR::call_arg_t> new_args_conjg; new_args_conjg.reserve(al, 1);
            ASR::call_arg_t call_arg; call_arg.loc = loc;
            call_arg.m_value = b.ArrayItem_01(args[0], {i});
            new_args_conjg.push_back(al, call_arg);

            Vec<ASR::ttype_t*> arg_types_conjg; arg_types_conjg.reserve(al, 1);
            arg_types_conjg.push_back(al, return_type);

            ASR::expr_t* func_call_conjg = Conjg::instantiate_Conjg(al, loc, scope, arg_types_conjg, return_type, new_args_conjg, 0);
            body.push_back(al, b.DoLoop(i, LBound(args[0], 1), UBound(args[0], 1), {
                b.Assignment(result, b.Add(result, EXPR(ASR::make_ComplexBinOp_t(al, loc, func_call_conjg, ASR::binopType::Mul, b.ArrayItem_01(args[1], {i}), return_type, nullptr))))
            }, nullptr));
        } else {
            if (is_real(*return_type)) {
                body.push_back(al, b.Assignment(result, make_ConstantWithType(make_RealConstant_t, 0.0, return_type, loc)));
            } else {
                body.push_back(al, b.Assignment(result, make_ConstantWithType(make_IntegerConstant_t, 0, return_type, loc)));
            }
            body.push_back(al, b.DoLoop(i, LBound(args[0], 1), UBound(args[0], 1), {
                b.Assignment(result, b.Add(result, b.Mul(b.ArrayItem_01(args[0], {i}), b.ArrayItem_01(args[1], {i}))))
            }, nullptr));
        }
        body.push_back(al, Return());
        ASR::symbol_t *fn_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
                body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, fn_sym);
        return b.Call(fn_sym, m_args, return_type, nullptr);
    }

} // namespace DotProduct

namespace Transpose {

    static inline void verify_args(const ASR::IntrinsicArrayFunction_t &x,
            diag::Diagnostics& diagnostics) {
        require_impl(x.n_args == 1, "`transpose` intrinsic accepts exactly"
            "one arguments", x.base.base.loc, diagnostics);
        require_impl(x.m_args[0], "`matrix` argument of `transpose` intrinsic "
            "cannot be nullptr", x.base.base.loc, diagnostics);
    }

    static inline ASR::expr_t *eval_Transpose(Allocator &/*al*/,
        const Location &/*loc*/, ASR::ttype_t */*return_type*/, Vec<ASR::expr_t*>& /*args*/, diag::Diagnostics& /*diag*/) {
        // TODO
        return nullptr;
    }

    static inline ASR::asr_t* create_Transpose(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            diag::Diagnostics& diag) {
        ASR::expr_t *matrix_a = args[0];
        bool is_type_allocatable = false;
        if (ASRUtils::is_allocatable(matrix_a)) {
            // TODO: Use Array type as return type instead of allocatable
            //  for both Array and Allocatable as input arguments.
            is_type_allocatable = true;
        }
        ASR::ttype_t *type_a = expr_type(matrix_a);
        ASR::ttype_t *ret_type = nullptr;
        ret_type = extract_type(type_a);
        ASR::dimension_t* matrix_a_dims = nullptr;
        int matrix_a_rank = extract_dimensions_from_ttype(type_a, matrix_a_dims);
        if ( matrix_a_rank != 2 ) {
            append_error(diag, "`transpose` accepts arrays of rank 2 only, provided an array "
                "with rank, " + std::to_string(matrix_a_rank), matrix_a->base.loc);
            return nullptr;
        }
        ASRBuilder b(al, loc);
        Vec<ASR::dimension_t> result_dims; result_dims.reserve(al, 2);
        int overload_id = 2;
        result_dims.push_back(al, b.set_dim(matrix_a_dims[0].m_start,
        matrix_a_dims[1].m_length));
        result_dims.push_back(al, b.set_dim(matrix_a_dims[1].m_start,
            matrix_a_dims[0].m_length));
        ret_type = ASRUtils::duplicate_type(al, ret_type, &result_dims);
        if (is_type_allocatable) {
            ret_type = TYPE(ASR::make_Allocatable_t(al, loc, ret_type));
        }
        ASR::expr_t *value = nullptr;
        if (all_args_evaluated(args)) {
            value = eval_Transpose(al, loc, ret_type, args, diag);
        }
        return make_IntrinsicArrayFunction_t_util(al, loc,
            static_cast<int64_t>(IntrinsicArrayFunctions::Transpose),
            args.p, args.n, overload_id, ret_type, value);
    }

    static inline ASR::expr_t *instantiate_Transpose(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*> &arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t> &m_args, int64_t /*overload_id*/) {
        /*
            do i = lbound(m,1), ubound(m,1)
                do j = lbound(m,2), ubound(m,2)
                    result(j,i) = m(i,j)
                end do
            end do
         */
        declare_basic_variables("_lcompilers_transpose");
        fill_func_arg("matrix_a_t", duplicate_type_with_empty_dims(al, arg_types[0]));
        ASR::ttype_t* return_type_ = return_type;
        if( !ASRUtils::is_fixed_size_array(return_type) ) {
            bool is_allocatable = ASRUtils::is_allocatable(return_type);
            Vec<ASR::dimension_t> empty_dims;
            empty_dims.reserve(al, 2);
            for( int idim = 0; idim < 2; idim++ ) {
                ASR::dimension_t empty_dim;
                empty_dim.loc = loc;
                empty_dim.m_start = nullptr;
                empty_dim.m_length = nullptr;
                empty_dims.push_back(al, empty_dim);
            }
            return_type_ = ASRUtils::make_Array_t_util(al, loc,
                ASRUtils::extract_type(return_type_), empty_dims.p, empty_dims.size());
            if( is_allocatable ) {
                return_type_ = ASRUtils::TYPE(ASR::make_Allocatable_t(al, loc, return_type_));
            }
        }
        ASR::expr_t *result = declare("result", return_type_, Out);
        args.push_back(al, result);
        ASR::expr_t *i = declare("i", int32, Local);
        ASR::expr_t *j = declare("j", int32, Local);
        body.push_back(al, b.DoLoop(i, LBound(args[0], 1), UBound(args[0], 1), {
            b.DoLoop(j, LBound(args[0], 2), UBound(args[0], 2), {
                b.Assignment(b.ArrayItem_01(result, {j, i}), b.ArrayItem_01(args[0], {i, j}))
            }, nullptr)
        }, nullptr));
        body.push_back(al, Return());
        ASR::symbol_t *fn_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
                body, nullptr, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, fn_sym);
        return b.Call(fn_sym, m_args, return_type, nullptr);
    }

} // namespace Transpose

namespace IntrinsicArrayFunctionRegistry {

    static const std::map<int64_t, std::tuple<impl_function,
            verify_array_function>>& intrinsic_function_by_id_db = {
        {static_cast<int64_t>(IntrinsicArrayFunctions::Any),
            {&Any::instantiate_Any, &Any::verify_args}},
        {static_cast<int64_t>(IntrinsicArrayFunctions::MatMul),
            {&MatMul::instantiate_MatMul, &MatMul::verify_args}},
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
        {static_cast<int64_t>(IntrinsicArrayFunctions::Transpose),
            {&Transpose::instantiate_Transpose, &Transpose::verify_args}},
        {static_cast<int64_t>(IntrinsicArrayFunctions::Pack),
            {&Pack::instantiate_Pack, &Pack::verify_args}},
        {static_cast<int64_t>(IntrinsicArrayFunctions::Unpack),
            {&Unpack::instantiate_Unpack, &Unpack::verify_args}},
        {static_cast<int64_t>(IntrinsicArrayFunctions::Count),
            {&Count::instantiate_Count, &Count::verify_args}},
        {static_cast<int64_t>(IntrinsicArrayFunctions::DotProduct),
            {&DotProduct::instantiate_DotProduct, &DotProduct::verify_args}},
    };

    static const std::map<std::string, std::tuple<create_intrinsic_function,
            eval_intrinsic_function>>& function_by_name_db = {
        {"any", {&Any::create_Any, &Any::eval_Any}},
        {"matmul", {&MatMul::create_MatMul, &MatMul::eval_MatMul}},
        {"maxloc", {&MaxLoc::create_MaxLoc, nullptr}},
        {"maxval", {&MaxVal::create_MaxVal, &MaxVal::eval_MaxVal}},
        {"merge", {&Merge::create_Merge, &Merge::eval_Merge}},
        {"minloc", {&MinLoc::create_MinLoc, nullptr}},
        {"minval", {&MinVal::create_MinVal, &MinVal::eval_MinVal}},
        {"product", {&Product::create_Product, &Product::eval_Product}},
        {"shape", {&Shape::create_Shape, &Shape::eval_Shape}},
        {"sum", {&Sum::create_Sum, &Sum::eval_Sum}},
        {"transpose", {&Transpose::create_Transpose, &Transpose::eval_Transpose}},
        {"pack", {&Pack::create_Pack, &Pack::eval_Pack}},
        {"unpack", {&Unpack::create_Unpack, &Unpack::eval_Unpack}},
        {"count", {&Count::create_Count, &Count::eval_Count}},
        {"dot_product", {&DotProduct::create_DotProduct, &DotProduct::eval_DotProduct}},
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
            id == IntrinsicArrayFunctions::MinVal ||
            id == IntrinsicArrayFunctions::Count) {
            return 1; // dim argument index
        } else if( id == IntrinsicArrayFunctions::MatMul || id == IntrinsicArrayFunctions::Transpose || id == IntrinsicArrayFunctions::Pack ||
            id == IntrinsicArrayFunctions::Unpack ) {
            return 2; // return variable index
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
