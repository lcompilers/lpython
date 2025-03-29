#ifndef LIBASR_PASS_INTRINSIC_FUNCTIONS_H
#define LIBASR_PASS_INTRINSIC_FUNCTIONS_H

#include <libasr/asr_builder.h>
#include <libasr/casting_utils.h>
#include <math.h>

namespace LCompilers::ASRUtils {

/*
To add a new function implementation,

1. Create a new namespace like, `Sin`, `LogGamma` in this file.
2. In the above created namespace add `eval_*`, `instantiate_*`, and `create_*`.
3. Then register in the maps present in `IntrinsicElementalFunctionRegistry`.

You can use helper macros and define your own helper macros to reduce
the code size.
*/

enum class IntrinsicElementalFunctions : int64_t {
    ObjectType,
    Kind, // if kind is reordered, update `extract_kind` in `asr_utils.h`
    Mod, // if mod is reordered, update `pass/openmp.cpp`
    Rank,
    Sin,
    Cos,
    Tan,
    Asin,
    Acos,
    Atan,
    Sinh,
    Cosh,
    Tanh,
    Atan2,
    Asinh,
    Sind,
    Cosd,
    Tand,
    Asind,
    Acosd,
    Atand,
    Acosh,
    Atanh,
    Erf,
    Erfc,
    ErfcScaled,
    Gamma,
    Log,
    Log10,
    LogGamma,
    Trunc,
    Fix,
    Abs,
    Aimag,
    Dreal,
    Exp,
    Exp2,
    Expm1,
    FMA,
    FlipSign,
    Trailz,
    Isnan,
    Nearest,
    Spacing,
    Modulo,
    BesselJ0,
    BesselJ1,
    BesselJN,
    BesselY0,
    BesselY1,
    BesselYN,
    SameTypeAs,
    Merge,
    Mvbits,
    MoveAlloc,
    Mergebits,
    Shiftr,
    Rshift,
    Shiftl,
    Dshiftl,
    Dshiftr,
    Ishft,
    OutOfRange,
    Bgt,
    Blt,
    Bge,
    Ble,
    Lgt,
    Llt,
    Lge,
    Lle,
    Exponent,
    Fraction,
    SetExponent,
    Not,
    Iand,
    Ior,
    Ieor,
    Ibclr,
    Ibset,
    Btest,
    Ibits,
    Leadz,
    ToLowerCase,
    Digits,
    Rrspacing,
    Repeat,
    StringContainsSet,
    StringFindSet,
    SubstrIndex,
    Hypot,
    SelectedIntKind,
    SelectedRealKind,
    SelectedCharKind,
    Adjustl,
    Adjustr,
    StringLenTrim,
    StringTrim,
    Ichar,
    Char,
    Achar,
    MinExponent,
    MaxExponent,
    FloorDiv,
    ListIndex,
    Partition,
    ListReverse,
    ListPop,
    ListReserve,
    DictKeys,
    DictValues,
    SetAdd,
    SetRemove,
    Max,
    Min,
    Radix,
    IsContiguous,
    StorageSize,
    Scale,
    Dprod,
    Range,
    Sign,
    CompilerVersion,
    CompilerOptions,
    CommandArgumentCount,
    SignFromValue,
    Logical,
    Nint,
    Idnint,
    Aint,
    Anint,
    Dim,
    Sqrt,
    Sngl,
    Ifix,
    Idint,
    Floor,
    Ceiling,
    Ishftc,
    Maskr,
    Maskl,
    Epsilon,
    Precision,
    Tiny,
    BitSize,
    NewLine,
    Conjg,
    Huge,
    Popcnt,
    Poppar,
    Real,
    SymbolicSymbol,
    SymbolicAdd,
    SymbolicSub,
    SymbolicMul,
    SymbolicDiv,
    SymbolicPow,
    SymbolicPi,
    SymbolicE,
    SymbolicInteger,
    SymbolicDiff,
    SymbolicExpand,
    SymbolicSin,
    SymbolicCos,
    SymbolicLog,
    SymbolicExp,
    SymbolicAbs,
    SymbolicHasSymbolQ,
    SymbolicAddQ,
    SymbolicMulQ,
    SymbolicPowQ,
    SymbolicLogQ,
    SymbolicSinQ,
    SymbolicGetArgument,
    Int,
    Present,
    And,
    Or,
    Xor
    // ...
};

typedef ASR::expr_t* (*impl_function)(
    Allocator&, const Location &,
    SymbolTable*, Vec<ASR::ttype_t*>&, ASR::ttype_t *,
    Vec<ASR::call_arg_t>&, int64_t);

typedef ASR::expr_t* (*eval_intrinsic_function)(
    Allocator&, const Location &, ASR::ttype_t *,
    Vec<ASR::expr_t*>&, diag::Diagnostics&);

typedef ASR::asr_t* (*create_intrinsic_function)(
    Allocator&, const Location&,
    Vec<ASR::expr_t*>&,
    diag::Diagnostics&);

typedef void (*verify_function)(
    const ASR::IntrinsicElementalFunction_t&,
    diag::Diagnostics&);

typedef ASR::expr_t* (*get_initial_value_func)(Allocator&, ASR::ttype_t*);

namespace UnaryIntrinsicFunction {

static inline ASR::expr_t* instantiate_functions(Allocator &al,
        const Location &loc, SymbolTable *scope, std::string new_name,
        ASR::ttype_t *arg_type, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
    std::string c_func_name;
    switch (arg_type->type) {
        case ASR::ttypeType::Complex : {
            if (ASRUtils::extract_kind_from_ttype_t(arg_type) == 4) {
                c_func_name = "_lfortran_c" + new_name;
            } else {
                c_func_name = "_lfortran_z" + new_name;
            }
            break;
        }
        default : {
            if (ASRUtils::extract_kind_from_ttype_t(arg_type) == 4) {
                c_func_name = "_lfortran_s" + new_name;
            } else {
                c_func_name = "_lfortran_d" + new_name;
            }
        }
    }
    new_name = "_lcompilers_" + new_name + "_" + type_to_str_python(arg_type);

    declare_basic_variables(new_name);
    if (scope->get_symbol(new_name)) {
        ASR::symbol_t *s = scope->get_symbol(new_name);
        ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
        return b.Call(s, new_args, expr_type(f->m_return_var));
    }
    fill_func_arg("x", arg_type);
    auto result = declare(new_name, ASRUtils::extract_type(return_type), ReturnVar);

    {
        SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);
        Vec<ASR::expr_t*> args_1;
        {
            args_1.reserve(al, 1);
            ASR::expr_t *arg = b.Variable(fn_symtab_1, "x", arg_type,
                ASR::intentType::In, ASR::abiType::BindC, true);
            args_1.push_back(al, arg);
        }

        ASR::expr_t *return_var_1 = b.Variable(fn_symtab_1, c_func_name,
            return_type, ASRUtils::intent_return_var, ASR::abiType::BindC, false);

        SetChar dep_1; dep_1.reserve(al, 1);
        Vec<ASR::stmt_t*> body_1; body_1.reserve(al, 1);
        ASR::symbol_t *s = make_ASR_Function_t(c_func_name, fn_symtab_1, dep_1, args_1,
            body_1, return_var_1, ASR::abiType::BindC, ASR::deftypeType::Interface, s2c(al, c_func_name));
        fn_symtab->add_symbol(c_func_name, s);
        dep.push_back(al, s2c(al, c_func_name));
        body.push_back(al, b.Assignment(result, b.Call(s, args, return_type)));
    }

    ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
        body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
    scope->add_symbol(fn_name, new_symbol);
    return b.Call(new_symbol, new_args, return_type);
}

static inline ASR::asr_t* create_UnaryFunction(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args, eval_intrinsic_function eval_function,
    int64_t intrinsic_id, int64_t overload_id, ASR::ttype_t* type, diag::Diagnostics& diag) {
    ASR::expr_t *value = nullptr;
    if (ASRUtils::all_args_evaluated(args)) {
        Vec<ASR::expr_t*> arg_values; arg_values.reserve(al, 1);
        arg_values.push_back(al, ASRUtils::expr_value(args[0]));
        value = eval_function(al, loc, type, arg_values, diag);
    }

    return ASRUtils::make_IntrinsicElementalFunction_t_util(al, loc, intrinsic_id,
        args.p, args.n, overload_id, type, value);
}

static inline ASR::symbol_t *create_KMP_function(Allocator &al,
        const Location &loc, SymbolTable *scope)
{
    /*
     * Knuth-Morris-Pratt (KMP) string-matching
     * This function takes two parameters:
     *     the sub-string or pattern string and the target string,
     * then returns the position of the first occurrence of the
     * string in the pattern.
     */
    declare_basic_variables("KMP_string_matching");
    fill_func_arg("target_string", character(-2));
    fill_func_arg("pattern", character(-2));

    auto result = declare("result", int32, ReturnVar);
    auto pi_len = declare("pi_len", int32, Local);
    auto i = declare("i", int32, Local);
    auto j = declare("j", int32, Local);
    auto s_len = declare("s_len", int32, Local);
    auto pat_len = declare("pat_len", int32, Local);
    auto flag = declare("flag", logical, Local);
    auto lps = declare("lps", List(int32), Local);

    body.push_back(al, b.Assignment(s_len, b.StringLen(args[0])));
    body.push_back(al, b.Assignment(pat_len, b.StringLen(args[1])));
    body.push_back(al, b.Assignment(result, b.i_neg(b.i32(-1), int32)));
    body.push_back(al, b.If(b.Eq(pat_len, b.i32(0)), {
            b.Assignment(result, b.i32(0)), b.Return()
        }, {
            b.If(b.Eq(s_len, b.i32(0)), { b.Return() }, {})
        }));
    body.push_back(al, b.Assignment(lps,
        EXPR(ASR::make_ListConstant_t(al, loc, nullptr, 0, List(int32)))));
    body.push_back(al, b.Assignment(i, b.i32(0)));
    body.push_back(al, b.While(b.LtE(i, b.Sub(pat_len, b.i32(1))), {
        b.Assignment(i, b.Add(i, b.i32(1))),
        b.ListAppend(lps, b.i32(0))
    }));
    body.push_back(al, b.Assignment(flag, b.bool_t(0, logical)));
    body.push_back(al, b.Assignment(i, b.i32(1)));
    body.push_back(al, b.Assignment(pi_len, b.i32(0)));
    body.push_back(al, b.While(b.Lt(i, pat_len), {
        b.If(b.Eq(b.StringItem(args[1], b.Add(i, b.i32(1))),
                 b.StringItem(args[1], b.Add(pi_len, b.i32(1)))), {
            b.Assignment(pi_len, b.Add(pi_len, b.i32(1))),
            b.Assignment(b.ListItem(lps, i, int32), pi_len),
            b.Assignment(i, b.Add(i, b.i32(1)))
        }, {
            b.If(b.NotEq(pi_len, b.i32(0)), {
                b.Assignment(pi_len, b.ListItem(lps, b.Sub(pi_len, b.i32(1)), int32))
            }, {
                b.Assignment(i, b.Add(i, b.i32(1)))
            })
        })
    }));
    body.push_back(al, b.Assignment(j, b.i32(0)));
    body.push_back(al, b.Assignment(i, b.i32(0)));
    body.push_back(al, b.While(b.And(b.GtE(b.Sub(s_len, i),
            b.Sub(pat_len, j)), b.Not(flag)), {
        b.If(b.Eq(b.StringItem(args[1], b.Add(j, b.i32(1))),
                b.StringItem(args[0], b.Add(i, b.i32(1)))), {
            b.Assignment(i, b.Add(i, b.i32(1))),
            b.Assignment(j, b.Add(j, b.i32(1)))
        }, {}),
        b.If(b.Eq(j, pat_len), {
            b.Assignment(result, b.Sub(i, j)),
            b.Assignment(flag, b.bool_t(1, logical)),
            b.Assignment(j, b.ListItem(lps, b.Sub(j, b.i32(1)), int32))
        }, {
            b.If(b.And(b.Lt(i, s_len), b.NotEq(b.StringItem(args[1], b.Add(j, b.i32(1))),
                    b.StringItem(args[0], b.Add(i, b.i32(1))))), {
                b.If(b.NotEq(j, b.i32(0)), {
                    b.Assignment(j, b.ListItem(lps, b.Sub(j, b.i32(1)), int32))
                }, {
                    b.Assignment(i, b.Add(i, b.i32(1)))
                })
            }, {})
        })
    }));
    body.push_back(al, b.Return());
    ASR::symbol_t *fn_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
        body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
    scope->add_symbol(fn_name, fn_sym);
    return fn_sym;
}

static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x,
        diag::Diagnostics& diagnostics) {
    const Location& loc = x.base.base.loc;
    ASRUtils::require_impl(x.n_args == 1,
        "Elemental intrinsics must have only 1 input argument",
        loc, diagnostics);

    ASR::ttype_t* input_type = ASRUtils::expr_type(x.m_args[0]);
    ASR::ttype_t* output_type = x.m_type;
    ASRUtils::require_impl(ASRUtils::check_equal_type(input_type, output_type, true),
        "The input and output type of elemental intrinsic, " +
        std::to_string(static_cast<int64_t>(x.m_intrinsic_id)) +
        " must exactly match, input type: " +
        ASRUtils::get_type_code(input_type) + " output type: " + ASRUtils::get_type_code(output_type),
        loc, diagnostics);
}

} // namespace UnaryIntrinsicFunction

// `X` is the name of the function in the IntrinsicElementalFunctions enum and
// we use the same name for `create_X` and other places
// `eval_X` is the name of the function in the `std` namespace for compile
//  numerical time evaluation
// `lc_rt_name` is the name that we use in the C runtime library
#define create_unary_function(X, eval_X, lc_rt_name)                            \
namespace X {                                                                   \
    static inline ASR::expr_t *eval_##X(Allocator &al, const Location &loc,     \
            ASR::ttype_t *t, Vec<ASR::expr_t*> &args,                           \
            diag::Diagnostics& /*diag*/) {                                      \
        double rv = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;          \
        ASRUtils::ASRBuilder b(al, loc);                                        \
        return b.f_t(std::eval_X(rv), t);                                       \
    }                                                                           \
    static inline ASR::expr_t* instantiate_##X (Allocator &al,                  \
            const Location &loc, SymbolTable *scope,                            \
            Vec<ASR::ttype_t*> &arg_types, ASR::ttype_t *return_type,           \
            Vec<ASR::call_arg_t> &new_args, int64_t overload_id) {              \
        return UnaryIntrinsicFunction::instantiate_functions(al, loc, scope,    \
            #lc_rt_name, arg_types[0], return_type, new_args, overload_id);     \
    }                                                                           \
} // namespace X

create_unary_function(Trunc, trunc, trunc)
create_unary_function(Gamma, tgamma, gamma)
create_unary_function(LogGamma, lgamma, log_gamma)
create_unary_function(Log10, log10, log10)
create_unary_function(Erf, erf, erf)
create_unary_function(Erfc, erfc, erfc)

namespace Isnan{
    static inline ASR::expr_t *eval_Isnan(Allocator &al, const Location &loc,
            ASR::ttype_t *t, Vec<ASR::expr_t*> &args,
            diag::Diagnostics& /*diag*/) {
        double rv = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
        ASRUtils::ASRBuilder b(al, loc);
        return b.bool_t(std::isnan(rv), t);
    }
    static inline ASR::expr_t* instantiate_Isnan(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*> &arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t> &new_args, int64_t overload_id) {
        return UnaryIntrinsicFunction::instantiate_functions(al, loc, scope,
            "is_nan", arg_types[0], return_type, new_args, overload_id);
    }
}

namespace ObjectType {

     static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 1,
            "type() takes only 1 argument `object`",
            x.base.base.loc, diagnostics);
    }

    static ASR::expr_t *eval_ObjectType(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*>& /*args*/, diag::Diagnostics& /*diag*/) {
        ASRBuilder b(al, loc);
        std::string object_type = "<class '";
        switch (t1->type) {
            case ASR::ttypeType::Integer : {
                object_type += "int"; break;
            } case ASR::ttypeType::Real : {
                object_type += "float"; break;
            } case ASR::ttypeType::String : {
                object_type += "str"; break;
            } case ASR::ttypeType::List : {
                object_type += "list"; break;
            } case ASR::ttypeType::Dict : {
               object_type += "dict"; break;
            } default: {
                LCOMPILERS_ASSERT_MSG(false, "Unsupported type");
                break;
            }
        }
        object_type += "'>";
        return b.StringConstant(object_type, character(object_type.length()));
    }

    static inline ASR::asr_t* create_ObjectType(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() != 1) {
            append_error(diag, "type() takes exactly 1 argument `object` for now", loc);
        }
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t *> arg_values;


        m_value = eval_ObjectType(al, loc, expr_type(args[0]), arg_values, diag);


        return ASR::make_IntrinsicElementalFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicElementalFunctions::ObjectType),
            args.p, args.n, 0, ASRUtils::expr_type(m_value), m_value);
    }

} // namespace ObjectType

namespace Fix {
    static inline ASR::expr_t *eval_Fix(Allocator &al, const Location &loc,
            ASR::ttype_t *t, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        LCOMPILERS_ASSERT(args.size() == 1);
        double rv = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
        double val;
        if (rv > 0.0) {
            val = floor(rv);
        } else {
            val = ceil(rv);
        }
        return make_ConstantWithType(make_RealConstant_t, val, t, loc);
    }

    static inline ASR::expr_t* instantiate_Fix (Allocator &al,
            const Location &loc, SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            ASR::ttype_t *return_type, Vec<ASR::call_arg_t>& new_args,
            int64_t overload_id) {
        ASR::ttype_t* arg_type = arg_types[0];
        return UnaryIntrinsicFunction::instantiate_functions(al, loc, scope,
            "fix", arg_type, return_type, new_args, overload_id);
    }

} // namespace Fix

// `X` is the name of the function in the IntrinsicElementalFunctions enum and
// we use the same name for `create_X` and other places
// `stdeval` is the name of the function in the `std` namespace for compile
//  numerical time evaluation
// `lcompilers_name` is the name that we use in the C runtime library
#define create_trig(X, stdeval, lcompilers_name)                                \
namespace X {                                                                   \
    static inline ASR::expr_t *eval_##X(Allocator &al, const Location &loc,     \
            ASR::ttype_t *t, Vec<ASR::expr_t*>& args,                           \
            diag::Diagnostics& /*diag*/) {                                      \
        LCOMPILERS_ASSERT(args.size() == 1);                                    \
        double rv = -1;                                                         \
        if( ASRUtils::extract_value(args[0], rv) ) {                            \
            double val = std::stdeval(rv);                                      \
            return make_ConstantWithType(make_RealConstant_t, val, t, loc);     \
        } else {                                                                \
            std::complex<double> crv;                                           \
            if( ASRUtils::extract_value(args[0], crv) ) {                       \
                std::complex<double> val = std::stdeval(crv);                   \
                return ASRUtils::EXPR(ASR::make_ComplexConstant_t(              \
                    al, loc, val.real(), val.imag(), t));                       \
            }                                                                   \
        }                                                                       \
        return nullptr;                                                         \
    }                                                                           \
    static inline ASR::expr_t* instantiate_##X (Allocator &al,                  \
            const Location &loc, SymbolTable *scope,                            \
            Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,           \
            Vec<ASR::call_arg_t>& new_args,int64_t overload_id)  {              \
        ASR::ttype_t* arg_type = arg_types[0];                                  \
        return UnaryIntrinsicFunction::instantiate_functions(al, loc, scope,    \
            #lcompilers_name, arg_type, return_type, new_args, overload_id);    \
    }                                                                           \
} // namespace X

create_trig(Sin, sin, sin)
create_trig(Cos, cos, cos)
create_trig(Tan, tan, tan)
create_trig(Asin, asin, asin)
create_trig(Acos, acos, acos)
create_trig(Atan, atan, atan)
create_trig(Sinh, sinh, sinh)
create_trig(Cosh, cosh, cosh)
create_trig(Tanh, tanh, tanh)
create_trig(Asinh, asinh, asinh)
create_trig(Acosh, acosh, acosh)
create_trig(Atanh, atanh, atanh)
create_trig(Log, log, log)

namespace MathIntrinsicFunction{
    static inline ASR::expr_t* instantiate_functions(Allocator &al, const Location &loc,
            SymbolTable *scope, std::string lcompiler_name, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        std::string c_func_name;
        if (ASRUtils::extract_kind_from_ttype_t(arg_types[0]) == 4) {
            c_func_name = "_lfortran_s" + lcompiler_name;
        } else {
            c_func_name = "_lfortran_d" + lcompiler_name;
        }
        std::string new_name = "_lcompilers_" + lcompiler_name + "_"+ type_to_str_python(arg_types[0]);

        declare_basic_variables(new_name);
        if (scope->get_symbol(new_name)) {
            ASR::symbol_t *s = scope->get_symbol(new_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var));
        }
        fill_func_arg("x", arg_types[0]);
        auto result = declare(new_name, return_type, ReturnVar);
        {
            ASR::symbol_t *s = b.create_c_func(c_func_name, fn_symtab, return_type, arg_types.size(), arg_types);
            fn_symtab->add_symbol(c_func_name, s);
            dep.push_back(al, s2c(al, c_func_name));
            body.push_back(al, b.Assignment(result, b.Call(s, args, return_type)));
        }

        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.Call(new_symbol, new_args, return_type);
    }
}

#define create_math_bindc(math_func, stdeval, degree, lcompilers_name)                                  \
namespace math_func {                                                                                   \
    static inline ASR::expr_t *eval_##math_func(Allocator &al, const Location &loc,                     \
            ASR::ttype_t *t, Vec<ASR::expr_t*>& args,                                                   \
            diag::Diagnostics& /*diag*/) {                                                              \
        LCOMPILERS_ASSERT(args.size() == 1);                                                            \
        double rv = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;                                  \
        double result = stdeval(rv);                                                                    \
        if ( degree == 1 ) {                                                                            \
            double PI = 3.14159265358979323846;                                                         \
            result = result * 180.0/PI;                                                                 \
        } else if ( degree == 2 ) {                                                                     \
            double PI = 3.14159265358979323846;                                                         \
            result = stdeval( ( rv * PI ) / 180.0 );                                                    \
        }                                                                                               \
        return make_ConstantWithType(make_RealConstant_t, result, t, loc);                              \
    }                                                                                                   \
    static inline ASR::expr_t* instantiate_##math_func (Allocator &al,                                  \
            const Location &loc, SymbolTable *scope,                                                    \
            Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,                                   \
            Vec<ASR::call_arg_t>& new_args,int64_t overload_id)  {                                      \
        return MathIntrinsicFunction::instantiate_functions(al, loc, scope,                             \
            #lcompilers_name, arg_types, return_type, new_args, overload_id);                           \
    }                                                                                                   \
} // namespace math_func

/*
    Degree acts as a switch
    - if degree = 1, math function output is in degrees
    - if degree = 2, math function input is in degrees
    - degree = 0, implies no change in input or output ( radians )
*/

create_math_bindc(BesselJ0, j0, 0, bessel_j0)
create_math_bindc(BesselJ1, j1, 0, bessel_j1)
create_math_bindc(BesselY0, y0, 0, bessel_y0)
create_math_bindc(BesselY1, y1, 0, bessel_y1)
create_math_bindc(Asind, asin, 1, asind)
create_math_bindc(Acosd, acos, 1, acosd)
create_math_bindc(Atand, atan, 1, atand)
create_math_bindc(Sind, sin, 2, sind)
create_math_bindc(Cosd, cos, 2, cosd)
create_math_bindc(Tand, tan, 2, tand)

namespace Aimag {

    static inline ASR::expr_t *eval_Aimag(Allocator &al, const Location &loc,
            ASR::ttype_t *t, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        ASRUtils::ASRBuilder b(al, loc);
        std::complex<double> crv;
        if( ASRUtils::extract_value(args[0], crv) ) {
            ASR::ComplexConstant_t *c = ASR::down_cast<ASR::ComplexConstant_t>(ASRUtils::expr_value(args[0]));
            return make_ConstantWithType(make_RealConstant_t, c->m_im, t, loc);
        } else {
            return nullptr;
        }
    }

    static inline ASR::expr_t* instantiate_Aimag(Allocator &al,
            const Location &loc, SymbolTable* scope,
            Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t> &new_args,int64_t /*overload_id*/)  {
        declare_basic_variables("_lcompilers_aimag_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        body.push_back(al, b.Assignment(result, EXPR(ASR::make_ComplexIm_t(al, loc,
            args[0], return_type, nullptr))));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Aimag

namespace Atan2 {
    static inline ASR::expr_t *eval_Atan2(Allocator &al, const Location &loc,
            ASR::ttype_t *t, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        LCOMPILERS_ASSERT(args.size() == 2);
        double rv = -1, rv2 = -1;
        if( ASRUtils::extract_value(args[0], rv) && ASRUtils::extract_value(args[1], rv2) ) {
            double val = std::atan2(rv,rv2);
            return make_ConstantWithType(make_RealConstant_t, val, t, loc);
        }
        return nullptr;
    }
    static inline ASR::expr_t* instantiate_Atan2 (Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args,int64_t /*overload_id*/) {
        ASR::ttype_t* arg_type = arg_types[0];
        std::string c_func_name;
        std::string new_name = "atan2";
        switch (arg_type->type) {
            case ASR::ttypeType::Complex : {
                if (ASRUtils::extract_kind_from_ttype_t(arg_type) == 4) {
                    c_func_name = "_lfortran_c" + new_name;
                } else {
                    c_func_name = "_lfortran_z" + new_name;
                }
                break;
            }
            default : {
                if (ASRUtils::extract_kind_from_ttype_t(arg_type) == 4) {
                    c_func_name = "_lfortran_s" + new_name;
                } else {
                    c_func_name = "_lfortran_d" + new_name;
                }
            }
        }
        new_name = "_lcompilers_" + new_name + "_" + type_to_str_python(arg_type);

        declare_basic_variables(new_name);
        if (scope->get_symbol(new_name)) {
            ASR::symbol_t *s = scope->get_symbol(new_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var));
        }
        fill_func_arg("x", arg_type);
        fill_func_arg("y", arg_type);
        auto result = declare(new_name, return_type, ReturnVar);
        {
            ASR::symbol_t *s = b.create_c_func(c_func_name, fn_symtab, return_type, arg_types.size(), arg_types);
            fn_symtab->add_symbol(c_func_name, s);
            dep.push_back(al, s2c(al, c_func_name));
            body.push_back(al, b.Assignment(result, b.Call(s, args, return_type)));
        }

        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
        body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.Call(new_symbol, new_args, return_type);
    }
}

namespace Abs {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        const Location& loc = x.base.base.loc;
        ASRUtils::require_impl(x.n_args == 1,
            "Elemental intrinsics must have only 1 input argument",
            loc, diagnostics);

        ASR::ttype_t* input_type = ASRUtils::expr_type(x.m_args[0]);
        ASR::ttype_t* output_type = x.m_type;
        std::string input_type_str = ASRUtils::get_type_code(input_type);
        std::string output_type_str = ASRUtils::get_type_code(output_type);
        if (ASRUtils::is_complex(*input_type)) {
            ASRUtils::require_impl(ASRUtils::is_real(*output_type),
                "Abs intrinsic must return output of real for complex input, found: " + output_type_str,
                loc, diagnostics);
            int input_kind = ASRUtils::extract_kind_from_ttype_t(input_type);
            int output_kind = ASRUtils::extract_kind_from_ttype_t(output_type);
            ASRUtils::require_impl(input_kind == output_kind,
                "The input and output type of Abs intrinsic must be of same kind, input kind: " +
                std::to_string(input_kind) + " output kind: " + std::to_string(output_kind),
                loc, diagnostics);
        } else {
            ASRUtils::require_impl(ASRUtils::check_equal_type(input_type, output_type, true),
                "The input and output type of Abs intrinsic must exactly match, input type: " +
                input_type_str + " output type: " + output_type_str, loc, diagnostics);
        }
    }

    static ASR::expr_t *eval_Abs(Allocator &al, const Location &loc,
            ASR::ttype_t *t, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::expr_t* arg = args[0];
        if (ASRUtils::is_real(*expr_type(arg))) {
            double rv = ASR::down_cast<ASR::RealConstant_t>(arg)->m_r;
            double val = std::abs(rv);
            return make_ConstantWithType(make_RealConstant_t, val, t, loc);
        } else if (ASRUtils::is_integer(*expr_type(arg))) {
            int64_t rv = ASR::down_cast<ASR::IntegerConstant_t>(arg)->m_n;
            int64_t val = std::abs(rv);
            return make_ConstantWithType(make_IntegerConstant_t, val, t, loc);
        } else if (ASRUtils::is_complex(*expr_type(arg))) {
            double re = ASR::down_cast<ASR::ComplexConstant_t>(arg)->m_re;
            double im = ASR::down_cast<ASR::ComplexConstant_t>(arg)->m_im;
            std::complex<double> x(re, im);
            double result = std::abs(x);
            return make_ConstantWithType(make_RealConstant_t, result, t, loc);
        } else {
            return nullptr;
        }
    }

    static inline ASR::asr_t* create_Abs(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            diag::Diagnostics& diag) {
        if (args.size() != 1) {
            append_error(diag, "Intrinsic abs function accepts exactly 1 argument", loc);
            return nullptr;
        }
        ASR::ttype_t *type = ASRUtils::expr_type(args[0]);
        if (!ASRUtils::is_integer(*type) && !ASRUtils::is_real(*type)
                && !ASRUtils::is_complex(*type)) {
            append_error(diag, "Argument of the abs function must be Integer, Real or Complex",
                args[0]->base.loc);
            return nullptr;
        }
        // if the argument is "complex" (scalar or array or pointer ...)
        // the return argument is of type "real", so we change "type" accordingly
        if (is_complex(*type)) {
            ASR::ttype_t* real_type = TYPE(ASR::make_Real_t(al, type->base.loc,
                                        ASRUtils::extract_kind_from_ttype_t(type)));
            if (ASR::is_a<ASR::Array_t>(*type)) {
                ASR::Array_t* e = ASR::down_cast<ASR::Array_t>(type);
                type = TYPE(ASR::make_Array_t(al, type->base.loc, real_type,
                                    e->m_dims, e->n_dims, e->m_physical_type));
            } else {
                type = real_type;
            }
        }
        // array_struct_temporary: TODO: Calculate type according to input arguments
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args, eval_Abs,
            static_cast<int64_t>(IntrinsicElementalFunctions::Abs), 0,
            ASRUtils::type_get_past_allocatable(type), diag);
    }

    static inline ASR::expr_t* instantiate_Abs(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        std::string func_name = "_lcompilers_abs_" + type_to_str_python(arg_types[0]);
        declare_basic_variables(func_name);
        if (scope->get_symbol(func_name)) {
            ASR::symbol_t *s = scope->get_symbol(func_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var), nullptr);
        }
        fill_func_arg("x", arg_types[0]);
        auto result = declare(func_name, return_type, ReturnVar);
        /*
            * if (x >= 0) then
            *     r = x
            * else
            *     r = -x
            * end if
        */
        if (is_integer(*arg_types[0]) || is_real(*arg_types[0])) {
            if (is_integer(*arg_types[0])) {
                body.push_back(al, b.If(b.GtE(args[0], b.i_t(0, arg_types[0])), {
                    b.Assignment(result, args[0])
                }, {
                    b.Assignment(result, b.i_neg(args[0], arg_types[0]))
                }));
            } else {
                body.push_back(al, b.If(b.GtE(args[0], b.f_t(0, arg_types[0])), {
                    b.Assignment(result, args[0])
                }, {
                    b.Assignment(result, b.f_neg(args[0], arg_types[0]))
                }));
            }
        } else {
            // * Complex type: `r = (real(x)**2 + aimag(x)**2)**0.5`
            ASR::ttype_t *real_type = TYPE(ASR::make_Real_t(al, loc,
                                        ASRUtils::extract_kind_from_ttype_t(arg_types[0])));
            ASR::down_cast<ASR::Variable_t>(ASR::down_cast<ASR::Var_t>(result)->m_v)->m_type = return_type = real_type;
            body.push_back(al, b.Assignment(result,
                b.Pow(b.Add(b.Pow(EXPR(ASR::make_ComplexRe_t(al, loc,
                args[0], real_type, nullptr)), b.f_t(2.0, real_type)), b.Pow(EXPR(ASR::make_ComplexIm_t(al, loc,
                args[0], real_type, nullptr)), b.f_t(2.0, real_type))), b.f_t(0.5, real_type))));
        }
        ASR::symbol_t *f_sym = make_ASR_Function_t(func_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(func_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Abs

namespace Radix {

    static ASR::expr_t *eval_Radix(Allocator &al, const Location &loc,
            ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& /*diag*/) {
        ASRUtils::ASRBuilder b(al, loc);
        return b.i32(2);
    }

}  // namespace Radix

namespace StorageSize {

    static ASR::expr_t *eval_StorageSize(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t kind = ASRUtils::extract_kind_from_ttype_t(expr_type(args[0]));
        if (is_character(*expr_type(args[0]))) {
            int64_t len = ASR::down_cast<ASR::String_t>(ASRUtils::type_get_past_array(expr_type(args[0])))->m_len;
            return make_ConstantWithType(make_IntegerConstant_t, 8*len, t1, loc);
        } else if (is_complex(*expr_type(args[0]))) {
            if (kind == 4) return make_ConstantWithType(make_IntegerConstant_t, 64, t1, loc);
            else if (kind == 8) return make_ConstantWithType(make_IntegerConstant_t, 128, t1, loc);
            else return make_ConstantWithType(make_IntegerConstant_t, -1, t1, loc);
        } else {
            if (kind == 1) return make_ConstantWithType(make_IntegerConstant_t, 8, t1, loc);
            else if (kind == 2) return make_ConstantWithType(make_IntegerConstant_t, 16, t1, loc);
            else if (kind == 4) return make_ConstantWithType(make_IntegerConstant_t, 32, t1, loc);
            else if (kind == 8) return make_ConstantWithType(make_IntegerConstant_t, 64, t1, loc);
            else return make_ConstantWithType(make_IntegerConstant_t, -1, t1, loc);
        }
    }

}  // namespace StorageSize

namespace Scale {
    static ASR::expr_t *eval_Scale(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        double value_X = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
        int64_t value_I = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        double result = value_X * std::pow(2, value_I);
        ASRUtils::ASRBuilder b(al, loc);
        return b.f_t(result, arg_type);
    }

    static inline ASR::expr_t* instantiate_Scale(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_scale_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("i", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = scale(x, y)
        * r = x * 2**y
        */

       //TODO: Radix for most of the device is 2, so we can use the b.i2r_t(2, real32) instead of args[1]. Fix (find a way to get the radix of the device and use it here)
        body.push_back(al, b.Assignment(result, b.Mul(args[0], b.i2r_t(b.Pow(b.i_t(2, arg_types[1]), args[1]), return_type))));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args, body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }
}  // namespace Scale

namespace Dprod {
    static ASR::expr_t *eval_Dprod(Allocator &al, const Location &loc,
            ASR::ttype_t* return_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        double value_X = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
        double value_Y = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
        if (ASRUtils::extract_kind_from_ttype_t(expr_type(args[0])) != 4 ||
            ASRUtils::extract_kind_from_ttype_t(expr_type(args[1])) != 4) {
            append_error(diag, "Arguments to dprod must be real(4)", loc);
            return nullptr;
        }
        double result = value_X * value_Y;
        ASRUtils::ASRBuilder b(al, loc);
        return b.f_t(result, return_type);
    }

    static inline ASR::expr_t* instantiate_Dprod(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_dprod_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        if (ASRUtils::extract_kind_from_ttype_t(arg_types[0]) != 4 ||
            ASRUtils::extract_kind_from_ttype_t(arg_types[1]) != 4) {
            LCompilersException("Arguments to dprod must be default real");
            return nullptr;
        }
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = dprod(x, y)
        * r = x * y
        */
        body.push_back(al, b.Assignment(result, b.r2r_t(b.Mul(args[0],args[1]), real64)));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args, body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

}  // namespace Dprod

namespace SameTypeAs {

    static ASR::expr_t *eval_SameTypeAs(Allocator &/*al*/, const Location &loc,
            ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& diag) {
        append_error(diag, "same_type_as is not implemented yet", loc);
        return nullptr;
    }

} // namespace SameTypeAs

namespace Range {

    static ASR::expr_t *eval_Range(Allocator &al, const Location &loc,
            ASR::ttype_t */*return_type*/, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        ASRUtils::ASRBuilder b(al, loc);
        int64_t range_val = -1;
        ASR::ttype_t *arg_type = expr_type(args[0]);
        int32_t kind = extract_kind_from_ttype_t(arg_type);
        if ( is_integer(*arg_type) ) {
            switch ( kind ) {
                case 1: {
                    range_val = 2; break;
                } case 2: {
                    range_val = 4; break;
                } case 4: {
                    range_val = 9; break;
                } case 8: {
                    range_val = 18; break;
                } default: {
                    break;
                }
            }
        } else if ( is_real(*arg_type) || is_complex(*arg_type) ) {
            switch ( kind ) {
                case 4: {
                    range_val = 37; break;
                } case 8: {
                    range_val = 307; break;
                } default: {
                    break;
                }
            }
        }
        return b.i32(range_val);
    }

}  // namespace Range

namespace OutOfRange
{

    static ASR::expr_t* eval_OutOfRange(Allocator& al, const Location& loc,
        ASR::ttype_t* return_type, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/){
        ASRUtils::ASRBuilder b(al, loc);
        long long max_val_int = 0;
        long long min_val_int = 0;
        double max_val_float = 0.0;
        double min_val_float = 0.0;

        ASR::ttype_t* arg_type_1 = expr_type(args[0]);
        ASR::ttype_t* arg_type_2 = expr_type(args[1]);
        int32_t kind_2 = extract_kind_from_ttype_t(arg_type_2);
        switch (kind_2) {
            case 1:
                max_val_int = 127;
                min_val_int = -128;
                break;
            case 2:
                max_val_int = 32767;
                min_val_int = -32768;
                break;
            case 4:
                max_val_int = 2147483647;
                min_val_int = -2147483648;
                max_val_float = 3.4028234663852886e+38;
                min_val_float = -3.4028234663852886e+38;
                break;
            case 8:
                max_val_int = 9223372036854775807;
                min_val_int = -9223372036854775807;
                max_val_float = 1.7976931348623157e+308;
                min_val_float = -1.7976931348623157e+308;
                break;
        }
        if (is_integer(*arg_type_1)) {
            int64_t value = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
            if (is_integer(*arg_type_2)) {
                if (value > max_val_int || value < min_val_int) {
                    return b.bool_t(1, return_type);
                }
            } else if (is_real(*arg_type_2)) {
                if ((double) value > max_val_float || (double) value < min_val_float) {
                    return b.bool_t(1, return_type);
                }
            }
        } else if (is_real(*arg_type_1)) {
            double value = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            if (is_integer(*arg_type_2)) {
                if (value > (double) max_val_int || value < (double) min_val_int) {
                    return b.bool_t(1, return_type);
                }
            } else if (is_real(*arg_type_2)) {
                if (value > max_val_float || value < min_val_float) {
                    return b.bool_t(1, return_type);
                }
            }
        }

        return b.bool_t(0, return_type);
    }

    static inline ASR::expr_t* instantiate_OutOfRange(Allocator& al, const Location& loc,
            SymbolTable* scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t* return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {

        declare_basic_variables("_lcompilers_out_of_range_" + type_to_str_python(arg_types[0]));

        fill_func_arg("value", arg_types[0]);
        fill_func_arg("mold", arg_types[1]);
        fill_func_arg("round", arg_types[2]);

        auto result = declare(fn_name, return_type, ReturnVar);

        ASR::expr_t* max_val = nullptr;
        ASR::expr_t* min_val = nullptr;
        int kind1 = extract_kind_from_ttype_t(arg_types[0]);
        int kind2 = extract_kind_from_ttype_t(arg_types[1]);

        if (is_integer(*arg_types[1])) {
        if (kind2 == 4) {
            if (kind1 == 4) {
                max_val = b.i32(2147483647);
                min_val = b.i32(-2147483648);
                body.push_back(al,
                    b.If(b.Or(b.Gt(args[0], max_val), b.Lt(args[0], min_val)),
                        { b.Assignment(result, b.bool_t(true, return_type)) },
                        { b.Assignment(result, b.bool_t(false, return_type)) }));
            } else {
                max_val = b.i64(2147483647);
                min_val = b.i64(-2147483648);
                body.push_back(al,
                    b.If(b.Or(b.Gt(args[0], max_val), b.Lt(args[0], min_val)),
                        { b.Assignment(result, b.bool_t(true, return_type)) },
                        { b.Assignment(result, b.bool_t(false, return_type)) }));
            }
        } else if (kind2 == 8) {
            max_val = b.i64(9223372036854775807);
            min_val = b.i64(-9223372036854775807);
            body.push_back(al,
                b.If(b.Or(b.Gt(b.i2i_t(args[0], arg_types[1]), max_val), b.Lt(b.i2i_t(args[0], arg_types[1]), min_val)),
                    { b.Assignment(result, b.bool_t(true, return_type)) },
                    { b.Assignment(result, b.bool_t(false, return_type)) }));
        }
        } else if (is_real(*arg_types[1])) {
            if (kind2 == 4) {
                if (kind1 == 4) {
                    max_val = b.f32(3.4028235e+38f);
                    min_val = b.f32(-3.4028235e+38f);
                    body.push_back(al, b.If(b.Or(b.Gt(args[0], max_val), b.Lt(args[0], min_val)),
                     { b.Assignment(result, b.bool_t(true, return_type)) },
                     { b.Assignment(result, b.bool_t(false, return_type)) }));
                } else if (kind1 == 8) {
                    max_val = b.f64(3.4028235e+38f);
                    min_val = b.f64(-3.4028235e+38f);
                    body.push_back(al, b.If(b.Or(b.Gt(args[0], max_val), b.Lt(args[0], min_val)),
                    { b.Assignment(result, b.bool_t(true, return_type))},
                    { b.Assignment(result, b.bool_t(false, return_type))}));
                }
            } else if (kind2 == 8) {
                max_val = b.f64(1.7976931348623157e+308);
                min_val = b.f64(-1.7976931348623157e+308);
                body.push_back(al, b.If( b.Or(b.Gt(b.i2i_t(args[0], arg_types[1]), max_val), b.Lt(b.i2i_t(args[0], arg_types[1]), min_val)),
                { b.Assignment(result, b.bool_t(true, return_type)) },
            { b.Assignment(result, b.bool_t(false, return_type)) }));
            }
        }

        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.Call(new_symbol, new_args, return_type);
    }


}  // namespace OutOfRange

namespace CompilerVersion {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 0,
            "compiler_version() takes no argument",
            x.base.base.loc, diagnostics);
    }

    static ASR::expr_t *eval_CompilerVersion(Allocator &al, const Location &loc,
            ASR::ttype_t */*t1*/, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& /*diag*/) {
        ASRUtils::ASRBuilder b(al, loc);
        std::string version = LFORTRAN_VERSION;
        return b.StringConstant("LFortran version " + version, character(-1));
    }

    static inline ASR::asr_t* create_CompilerVersion(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        ASR::ttype_t *return_type = character(-1);
        ASR::expr_t *m_value = nullptr;
        return_type = ASRUtils::extract_type(return_type);
        m_value = eval_CompilerVersion(al, loc, return_type, args, diag);
            if (diag.has_error()) {
                return nullptr;
            }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::CompilerVersion),
                nullptr, 0, 0, return_type, m_value);
    }
} // namespace CompilerVersion

namespace CompilerOptions {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 0,
            "compiler_options() takes no argument",
            x.base.base.loc, diagnostics);
    }

    static ASR::expr_t *eval_CompilerOptions(Allocator &al, const Location &loc,
            ASR::ttype_t */*t1*/, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& /*diag*/) {
        ASRUtils::ASRBuilder b(al, loc);
        return b.StringConstant(lcompilers_commandline_options, character(-1));
    }

    static inline ASR::asr_t* create_CompilerOptions(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        ASR::ttype_t *return_type = character(-1);
        ASR::expr_t *m_value = nullptr;
        return_type = ASRUtils::extract_type(return_type);
        m_value = eval_CompilerOptions(al, loc, return_type, args, diag);
        if (diag.has_error()) {
            return nullptr;
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::CompilerOptions),
                nullptr, 0, 0, return_type, m_value);
    }
} // namespace CompilerOptions

namespace CommandArgumentCount {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 0,
            "command_argument_count() takes no argument",
            x.base.base.loc, diagnostics);
    }

    static inline ASR::asr_t* create_CommandArgumentCount(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& /*args*/, diag::Diagnostics& diag) {
        ASR::ttype_t *return_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
        ASR::expr_t *m_value = nullptr;
        return_type = ASRUtils::extract_type(return_type);
        m_value = nullptr;
        if (diag.has_error()) {
            return nullptr;
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::CommandArgumentCount),
                nullptr, 0, 0, return_type, m_value);
    }

    static inline ASR::expr_t* instantiate_CommandArgumentCount(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/){
        std::string c_func_name;
        c_func_name = "_lfortran_command_argument_count";
        std::string new_name = "_lcompilers_command_argument_count_";

        declare_basic_variables(new_name);
        if (scope->get_symbol(new_name)) {
            ASR::symbol_t *s = scope->get_symbol(new_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var));
        }
        auto result = declare(new_name, return_type, ReturnVar);
        {
            ASR::symbol_t *s = b.create_c_func(c_func_name, fn_symtab, return_type, 0, arg_types);
            fn_symtab->add_symbol(c_func_name, s);
            dep.push_back(al, s2c(al, c_func_name));
            body.push_back(al, b.Assignment(result, b.Call(s, args, return_type)));
        }

        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.Call(new_symbol, new_args, return_type);
    }
} // namespace CommandArgumentCount

namespace Sign {

    static ASR::expr_t *eval_Sign(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        if (ASRUtils::is_real(*t1)) {
            double rv1 = std::abs(ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r);
            double rv2 = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
            rv1 = copysign(rv1, rv2);
            return make_ConstantWithType(make_RealConstant_t, rv1, t1, loc);
        } else {
            int64_t iv1 = std::abs(ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n);
            int64_t iv2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
            if (iv2 < 0) iv1 = -iv1;
            return make_ConstantWithType(make_IntegerConstant_t, iv1, t1, loc);
        }
    }

    static inline ASR::expr_t* instantiate_Sign(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_sign_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        if (is_real(*arg_types[0])) {
            Vec<ASR::expr_t*> args; args.reserve(al, 2);
            visit_expr_list(al, new_args, args);
            return ASRUtils::EXPR(ASR::make_RealCopySign_t(al, loc, args[0], args[1], arg_types[0], nullptr));
        } else {
            /*
            * r = abs(x)
            * if (y < 0) then
            *     r = -r
            * end if
            */
            body.push_back(al, b.If(b.GtE(args[0], b.i_t(0, arg_types[0])), {
                b.Assignment(result, args[0])
            }, {
                b.Assignment(result, b.i_neg(args[0], arg_types[0]))
            }));
            body.push_back(al, b.If(b.Lt(args[1], b.i_t(0, arg_types[0])), {
                b.Assignment(result, b.i_neg(result, arg_types[0]))
            }, {}));

            ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
                body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
            scope->add_symbol(fn_name, f_sym);
            return b.Call(f_sym, new_args, return_type, nullptr);
        }
    }

} // namespace Sign

namespace Shiftr {

    static ASR::expr_t *eval_Shiftr(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int kind = ASRUtils::extract_kind_from_ttype_t(ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_type);
        if(val2 < 0) {
            append_error(diag, "The shift argument of 'shiftr' intrinsic must be non-negative integer", args[1]->base.loc);
            return nullptr;
        }
        int k_val = kind * 8;
        if (val2 > k_val) {
            diag.add(diag::Diagnostic("The shift argument of 'shiftr' intrinsic must be less than or equal to the bit size of the integer", diag::Level::Error,
            diag::Stage::Semantic, {diag::Label("Shift value is " + std::to_string(val2) +
            ", but bit size of integer is " + std::to_string(k_val), { args[1]->base.loc })}));
            return nullptr;
        }
        int64_t val = val1 >> val2;
        return make_ConstantWithType(make_IntegerConstant_t, val, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Shiftr(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("");
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = shiftr(x, y)
        * r = x >> y
        */
        body.push_back(al, b.Assignment(result, b.BitRshift(args[0], b.i2i_t(args[1], arg_types[0]), arg_types[0])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);

    }

    static inline ASR::expr_t* SHIFTR(ASRBuilder& b, ASR::expr_t* i, ASR::expr_t* shift, SymbolTable* scope) {
        return b.CallIntrinsic(scope, {expr_type(i), expr_type(shift)}, {i, shift}, expr_type(i), 0, Shiftr::instantiate_Shiftr);
    }

} // namespace Shiftr

namespace Rshift {

    static ASR::expr_t *eval_Rshift(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int kind = ASRUtils::extract_kind_from_ttype_t(ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_type);
        if(val2 < 0) {
            append_error(diag, "The shift argument of 'rshift' intrinsic must be non-negative integer", args[1]->base.loc);
            return nullptr;
        }
        int k_val = kind * 8;
        if (val2 > k_val) {
            diag.add(diag::Diagnostic("The shift argument of 'rshift' intrinsic must be less than or equal to the bit size of the integer", diag::Level::Error,
            diag::Stage::Semantic, {diag::Label("Shift value is " + std::to_string(val2) +
            ", but bit size of integer is " + std::to_string(k_val), { args[1]->base.loc })}));
            return nullptr;
        }
        int64_t val = val1 >> val2;
        return make_ConstantWithType(make_IntegerConstant_t, val, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Rshift(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("");
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = rshift(x, y)
        * r = x >> y
        */
        body.push_back(al, b.Assignment(result, b.BitRshift(args[0], args[1], arg_types[0])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);

    }

} // namespace Rshift

namespace Shiftl {

    static ASR::expr_t *eval_Shiftl(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int kind = ASRUtils::extract_kind_from_ttype_t(ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_type);
        if(val2 < 0) {
            append_error(diag, "The shift argument of 'shiftl' intrinsic must be non-negative integer", args[1]->base.loc);
            return nullptr;
        }
        int k_val = kind * 8;
        if (val2 > k_val) {
            diag.add(diag::Diagnostic("The shift argument of 'shiftl' intrinsic must be less than or equal to the bit size of the integer", diag::Level::Error,
            diag::Stage::Semantic, {diag::Label("Shift value is " + std::to_string(val2) +
            ", but bit size of integer is " + std::to_string(k_val), { args[1]->base.loc })}));
            return nullptr;
        }
        int64_t val = val1 << val2;
        return make_ConstantWithType(make_IntegerConstant_t, val, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Shiftl(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_shiftl_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = shiftl(x, y)
        * r = x << y
        */
        body.push_back(al, b.Assignment(result, b.BitLshift(args[0], b.i2i_t(args[1], arg_types[0]), arg_types[0])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);

    }

    static inline ASR::expr_t* SHIFTL(ASRBuilder& b, ASR::expr_t* i, ASR::expr_t* shift, SymbolTable* scope) {
        return b.CallIntrinsic(scope, {expr_type(i), expr_type(shift)}, {i, shift}, expr_type(i), 0, Shiftl::instantiate_Shiftl);
    }

} // namespace Shiftl

namespace Dshiftl {

    static ASR::expr_t *eval_Dshiftl(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int64_t shift = ASR::down_cast<ASR::IntegerConstant_t>(args[2])->m_n;
        int kind = ASRUtils::extract_kind_from_ttype_t(ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_type);
        if(shift < 0){
            append_error(diag, "The shift argument of 'dshiftl' intrinsic must be non-negative integer", args[2]->base.loc);
            return nullptr;
        }
        int k_val = kind * 8;
        if (shift > k_val) {
            append_error(diag, "The shift argument of 'dshiftl' intrinsic must be less than or equal to the bit size of the integer", args[2]->base.loc);
            return nullptr;
        }
        int64_t val = (val1 << shift) | (val2 >> (k_val - shift));
        return make_ConstantWithType(make_IntegerConstant_t, val, t1, loc);
    }


    static inline ASR::expr_t* instantiate_Dshiftl(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_dshiftl_" + type_to_str_python(arg_types[0]));
        fill_func_arg("i", arg_types[0]);
        fill_func_arg("j", arg_types[1]);
        fill_func_arg("shift", arg_types[2]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = Dshiftl(x, y, shift)
        * r = x << shift | y >> (32 - shift) ! kind = 4
        * r = x << shift | y >> (64 - shift) ! kind = 8
        */
        body.push_back(al, b.Assignment(result, b.BitLshift(args[0], b.i2i_t(args[2], return_type), return_type)));
        body.push_back(al, b.If(b.Eq(b.i32(extract_kind_from_ttype_t(arg_types[0])), b.i32(4)), {
            b.Assignment(result, b.Or(result, b.BitRshift(args[1], b.Sub(b.i_t(32, return_type), args[2]), return_type)))
        }, {
            b.Assignment(result, b.Or(result, b.BitRshift(args[1], b.Sub(b.i_t(64, return_type), args[2]), return_type)))
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);

    }

} // namespace Dshiftl

namespace Dshiftr {

    static ASR::expr_t *eval_Dshiftr(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int64_t shift = ASR::down_cast<ASR::IntegerConstant_t>(args[2])->m_n;
        int kind1 = ASRUtils::extract_kind_from_ttype_t(ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_type);
        int kind2 = ASRUtils::extract_kind_from_ttype_t(ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_type);
        if(kind1 != kind2) {
            append_error(diag, "The kind of first argument of 'dshiftr' intrinsic must be the same as second argument", loc);
            return nullptr;
        }
        if(shift < 0){
            append_error(diag, "The shift argument of 'dshiftr' intrinsic must be non-negative integer", args[2]->base.loc);
            return nullptr;
        }
        int64_t k_val = kind1 * 8;
        if (shift > k_val) {
            append_error(diag, "The shift argument of 'dshiftr' intrinsic must be less than or equal to the bit size of the integer", args[2]->base.loc);
            return nullptr;
        }
        int64_t rightmostI = val1 & ((1LL << shift) - 1);
        int64_t result = rightmostI << (k_val - shift);
        int64_t leftmostJ;
        if (val2 < 0) {
            leftmostJ = (val2 >> shift) & ((1LL << (k_val - shift)) - 1LL);
        } else {
            leftmostJ = val2 >> shift;
        }
        result |= leftmostJ;
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }


    static inline ASR::expr_t* instantiate_Dshiftr(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_dshiftr_" + type_to_str_python(arg_types[0]));
        fill_func_arg("i", arg_types[0]);
        fill_func_arg("j", arg_types[1]);
        fill_func_arg("shift", arg_types[2]);
        auto final_result = declare("x", int64, Local);
        auto result = declare(fn_name , return_type, ReturnVar);

        body.push_back(al, b.Assignment(final_result, b.BitLshift(b.And(b.i2i_t(args[0], int64),
            b.Sub(b.BitLshift(b.i64(1), b.i2i_t(args[2], int64), int64), b.i64(1))),
            b.Sub(b.Mul(b.i64(extract_kind_from_ttype_t(arg_types[0])), b.i64(8)), b.i2i_t(args[2], int64)), int64)));

        body.push_back(al, b.If(b.Lt(b.i2i_t(args[1], int64), b.i64(0)), {
            b.Assignment(final_result, b.Or(final_result, b.And(b.BitRshift(b.i2i_t(args[1], int64),
            b.Sub(b.Mul((b.i64(extract_kind_from_ttype_t(arg_types[0]))), b.i64(8)), b.i2i_t(args[2], int64)), int64),
            b.Sub(b.BitLshift(b.i64(1), b.Sub(b.Mul((b.i64(extract_kind_from_ttype_t(arg_types[0]))), b.i64(8)),
            b.i2i_t(args[2], int64)), int64), b.i64(1)))))
        }, {
            b.Assignment(final_result, b.Or(final_result, b.BitRshift(b.i2i_t(args[1], int64),
            b.Sub(b.Mul((b.i64(extract_kind_from_ttype_t(arg_types[0]))), b.i64(8)), b.i2i_t(args[2], int64)), int64)))
        }));
        body.push_back(al, b.Assignment(result, b.i2i_t(final_result, return_type)));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);

    }

} // namespace Dshiftr

namespace Dreal {

    static inline ASR::expr_t *eval_Dreal(Allocator &al, const Location &loc,
        ASR::ttype_t *t, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        ASRUtils::ASRBuilder b(al, loc);
        std::complex<double> crv;
        int kind = ASRUtils::extract_kind_from_ttype_t(ASR::down_cast<ASR::ComplexConstant_t>(args[0])->m_type);
        if(kind == 4){
            append_error(diag, "The argument of 'dreal' intrinsic must be of kind 8", loc);
            return nullptr;
        }
        if( ASRUtils::extract_value(args[0], crv) ) {
            double result = std::real(crv);
            return make_ConstantWithType(make_RealConstant_t, result, t, loc);
        } else {
            return nullptr;
        }
    }

    static inline ASR::expr_t* instantiate_Dreal(Allocator &al, const Location &loc,
            SymbolTable* scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t> &new_args,int64_t /*overload_id*/)  {
        declare_basic_variables("_lcompilers_dreal_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        body.push_back(al, b.Assignment(result, b.c2r_t(args[0], real64)));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Dreal


namespace Ishft {

    static ASR::expr_t *eval_Ishft(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int64_t val;
        if(val2<=0){
            val2 = val2 * -1;
            val = val1 >> val2;
        } else {
            val = val1 << val2;
        }
        return make_ConstantWithType(make_IntegerConstant_t, val, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Ishft(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_ishft_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = ishft(x, y)
        * if ( y <= 0) {
        *   r = x >> ( -1 * y)
        * } else {
        *   r = x << y
        * }
        */
        body.push_back(al, b.If(b.LtE(args[1], b.i_t(0, arg_types[0])), {
            b.Assignment(result, b.BitRshift(args[0], b.Mul(b.i_t(-1, arg_types[0]), args[1]), arg_types[0]))
        }, {
            b.Assignment(result, b.BitLshift(args[0], args[1], arg_types[0]))
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Ishft

namespace Bgt {

    static ASR::expr_t *eval_Bgt(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        bool result = false;
        if (val1 * val2 > 0 || ((val1 * val2 == 0) && (val1 > 0 || val2 > 0))) {
            if (val1 > val2) {
                result = true;
            }
        } else {
            if (val1 < val2) {
                result = true;
            }
        }
        return make_ConstantWithType(make_LogicalConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Bgt(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t */*return_type*/,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_bgt_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, logical, ReturnVar);
        body.push_back(al, b.Assignment(result, b.bool_t(0, logical)));
        body.push_back(al, b.If(b.Or(b.Gt(b.Mul(args[0], args[1]), b.i_t(0, arg_types[0])), b.And(b.Eq(b.Mul(args[0], args[1]), b.i_t(0, arg_types[0])), b.Or(b.Gt(args[0], b.i_t(0, arg_types[0])), b.Gt(args[1], b.i_t(0, arg_types[0]))))), {
            b.If(b.Gt(args[0], args[1]), {
                b.Assignment(result, b.bool_t(1, logical))
            }, {})
        }, {
            b.If(b.Lt(args[0], args[1]), {
                b.Assignment(result, b.bool_t(1, logical))
            }, {})
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, logical, nullptr);
    }

} // namespace Bgt

namespace Blt {

    static ASR::expr_t *eval_Blt(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        bool result = false;
        if (val1 * val2 > 0 || ((val1 * val2 == 0) && (val1 > 0 || val2 > 0))) {
            if (val1 < val2) {
                result = true;
            }
        } else {
            if (val1 > val2) {
                result = true;
            }
        }
        return make_ConstantWithType(make_LogicalConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Blt(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t */*return_type*/,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_blt_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, logical, ReturnVar);
        body.push_back(al, b.Assignment(result, b.bool_t(0, logical)));
        body.push_back(al, b.If(b.Or(b.Gt(b.Mul(args[0], args[1]), b.i_t(0, arg_types[0])), b.And(b.Eq(b.Mul(args[0], args[1]), b.i_t(0, arg_types[0])), b.Or(b.Gt(args[0], b.i_t(0, arg_types[0])), b.Gt(args[1], b.i_t(0, arg_types[0]))))), {
            b.If(b.Lt(args[0], args[1]), {
                b.Assignment(result, b.bool_t(1, logical))
            }, {})
        }, {
            b.If(b.Gt(args[0], args[1]), {
                b.Assignment(result, b.bool_t(1, logical))
            }, {})
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, logical, nullptr);
    }

} // namespace Blt

namespace Bge {

    static ASR::expr_t *eval_Bge(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        bool result = false;
        if (val1 * val2 > 0 || ((val1 * val2 == 0) && (val1 > 0 || val2 > 0))) {
            if (val1 >= val2) {
                result = true;
            }
        } else {
            if (val1 <= val2) {
                result = true;
            }
        }
        return make_ConstantWithType(make_LogicalConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Bge(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t */*return_type*/,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_bge_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, logical, ReturnVar);
        body.push_back(al, b.Assignment(result, b.bool_t(0, logical)));
        body.push_back(al, b.If(b.Or(b.Gt(b.Mul(args[0], args[1]), b.i_t(0, arg_types[0])), b.And(b.Eq(b.Mul(args[0], args[1]), b.i_t(0, arg_types[0])), b.Or(b.Gt(args[0], b.i_t(0, arg_types[0])), b.Gt(args[1], b.i_t(0, arg_types[0]))))), {
            b.If(b.GtE(args[0], args[1]), {
                b.Assignment(result, b.bool_t(1, logical))
            }, {})
        }, {
            b.If(b.LtE(args[0], args[1]), {
                b.Assignment(result, b.bool_t(1, logical))
            }, {})
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, logical, nullptr);
    }

} // namespace Bge

namespace Present {

    static ASR::expr_t *eval_Present(Allocator &/*al*/, const Location &/*loc*/,
        ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& /*diag*/) { return nullptr; }

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args != 1)  {
            ASRUtils::require_impl(false, "Unexpected number of args, Present takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Present(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        ASR::expr_t* arg = args[0];
        if (!ASR::is_a<ASR::Var_t>(*arg)) {
            diag.semantic_error_label(
                "Argument to 'present' must be a variable, but got an expression",
                {arg->base.loc},
                "Expected a variable here"
            );

            return nullptr;
        }

        ASR::symbol_t* sym = ASR::down_cast<ASR::Var_t>(arg)->m_v;
        ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(sym);
        if (var->m_presence != ASR::presenceType::Optional) {
            diag.semantic_error_label(
                "Argument to 'present' must be an optional dummy argument",
                {arg->base.loc},
                "This variable is not 'optional'"
            );

            return nullptr;
        }

        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;

        ASR::ttype_t* type_ = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;

        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);

        return ASR::make_IntrinsicElementalFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicElementalFunctions::Present),
            m_args.p, m_args.n, 0, return_type, m_value);
    }

    static inline ASR::expr_t* instantiate_Present(Allocator &/*al*/, const Location &/*loc*/,
            SymbolTable */*scope*/, Vec<ASR::ttype_t*>& /*arg_types*/, ASR::ttype_t */*return_type*/,
            Vec<ASR::call_arg_t>& /*new_args*/, int64_t /*overload_id*/) { return nullptr;}
}

namespace Ble {

    static ASR::expr_t *eval_Ble(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        bool result = false;
        if (val1 * val2 > 0 || ((val1 * val2 == 0) && (val1 > 0 || val2 > 0))) {
            if (val1 <= val2) {
                result = true;
            }
        } else {
            if (val1 >= val2) {
                result = true;
            }
        }
        return make_ConstantWithType(make_LogicalConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Ble(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t */*return_type*/,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_ble_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, logical, ReturnVar);
        body.push_back(al, b.Assignment(result, b.bool_t(0, logical)));
        body.push_back(al, b.If(b.Or(b.Gt(b.Mul(args[0], args[1]), b.i_t(0, arg_types[0])), b.And(b.Eq(b.Mul(args[0], args[1]), b.i_t(0, arg_types[0])), b.Or(b.Gt(args[0], b.i_t(0, arg_types[0])), b.Gt(args[1], b.i_t(0, arg_types[0]))))), {
            b.If(b.LtE(args[0], args[1]), {
                b.Assignment(result, b.bool_t(1, logical))
            }, {})
        }, {
            b.If(b.GtE(args[0], args[1]), {
                b.Assignment(result, b.bool_t(1, logical))
            }, {})
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, logical, nullptr);
    }

} // namespace Ble

namespace Lgt {

    static ASR::expr_t *eval_Lgt(Allocator &al, const Location &loc,
        ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        char* string_A = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        char* string_B = ASR::down_cast<ASR::StringConstant_t>(args[1])->m_s;
        bool result = false;
        if (strcmp(string_A, string_B) > 0) {
            result = true;
        }
        return make_ConstantWithType(make_LogicalConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Lgt(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_lgt_" + type_to_str_python(type_get_past_allocatable(arg_types[0])));
        fill_func_arg("x", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        fill_func_arg("y", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        auto result = declare(fn_name, return_type, ReturnVar);
        body.push_back(al, b.Assignment(result, b.Gt(args[0], args[1])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Lgt

namespace Llt {

    static ASR::expr_t *eval_Llt(Allocator &al, const Location &loc,
        ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        char* string_A = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        char* string_B = ASR::down_cast<ASR::StringConstant_t>(args[1])->m_s;
        bool result = false;
        if (strcmp(string_A, string_B) < 0) {
            result = true;
        }
        return make_ConstantWithType(make_LogicalConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Llt(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_llt_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        fill_func_arg("y", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        auto result = declare(fn_name, return_type, ReturnVar);
        body.push_back(al, b.Assignment(result, b.Lt(args[0], args[1])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, logical, nullptr);
    }

} // namespace Llt

namespace Lge {

    static ASR::expr_t *eval_Lge(Allocator &al, const Location &loc,
        ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        char* string_A = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        char* string_B = ASR::down_cast<ASR::StringConstant_t>(args[1])->m_s;
        bool result = false;
        if (strcmp(string_A, string_B) >= 0) {
            result = true;
        }
        return make_ConstantWithType(make_LogicalConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Lge(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_lge_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        fill_func_arg("y", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        auto result = declare(fn_name, return_type, ReturnVar);
        body.push_back(al, b.Assignment(result, b.GtE(args[0], args[1])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, logical, nullptr);
    }

} // namespace Lge

namespace Lle {

    static ASR::expr_t *eval_Lle(Allocator &al, const Location &loc,
        ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        char* string_A = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        char* string_B = ASR::down_cast<ASR::StringConstant_t>(args[1])->m_s;
        bool result = false;
        if (strcmp(string_A, string_B) <= 0) {
            result = true;
        }
        return make_ConstantWithType(make_LogicalConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Lle(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_lle_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        fill_func_arg("y", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        auto result = declare(fn_name, return_type, ReturnVar);
        body.push_back(al, b.Assignment(result, b.LtE(args[0], args[1])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, logical, nullptr);
    }

} // namespace Lle

namespace Int {

    static ASR::expr_t *eval_Int(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int64_t i = -1;
        if (ASR::is_a<ASR::IntegerConstant_t>(*args[0])) {
            i = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(args[0]))->m_n;
            return make_ConstantWithType(make_IntegerConstant_t, i, t1, loc);
        } else if (ASR::is_a<ASR::RealConstant_t>(*args[0])) {
            i = ASR::down_cast<ASR::RealConstant_t>(ASRUtils::expr_value(args[0]))->m_r;
            return make_ConstantWithType(make_IntegerConstant_t, i, t1, loc);
        } else if (ASR::is_a<ASR::ComplexConstant_t>(*args[0])) {
            i = ASR::down_cast<ASR::ComplexConstant_t>(ASRUtils::expr_value(args[0]))->m_re;
            return make_ConstantWithType(make_IntegerConstant_t, i, t1, loc);
        } else {
            append_error(diag, "Invalid argument to `int` intrinsic", loc);
            return nullptr;
        }
    }

    static inline ASR::expr_t* instantiate_Int(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_int_" + type_to_str_python(arg_types[0]));
        fill_func_arg("a", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        if (is_integer(*arg_types[0])) {
            body.push_back(al,b.Assignment(result, b.i2i_t(args[0], return_type)));
        } else if (is_real(*arg_types[0])) {
            body.push_back(al,b.Assignment(result, b.r2i_t(args[0], return_type)));
        } else if (is_complex(*arg_types[0])) {
            body.push_back(al,b.Assignment(result, b.c2i_t(args[0], return_type)));
        } else {
            return nullptr;
        }

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

}  // namespace Int

namespace Not {

    static ASR::expr_t *eval_Not(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t val = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t result = ~val;
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Not(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_not_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = not(x)
        * r = ~x
        */
        body.push_back(al, b.Assignment(result, b.Not(args[0])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

    static inline ASR::expr_t* NOT(ASRBuilder &b, ASR::expr_t* x, SymbolTable* scope) {
        return b.CallIntrinsic(scope, {expr_type(x)}, {x}, expr_type(x), 0, Not::instantiate_Not);
    }

} // namespace Not

namespace Iand {

    static ASR::expr_t *eval_Iand(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int64_t result;
        result = val1 & val2;
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Iand(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_iand_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = iand(x, y)
        * r = x & y
        */
        body.push_back(al, b.Assignment(result, b.And(args[0], args[1])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

    static inline ASR::expr_t* IAND(ASRBuilder &b, ASR::expr_t* i, ASR::expr_t* j, SymbolTable* scope) {
        return b.CallIntrinsic(scope, {expr_type(i), expr_type(j)}, {i, j}, expr_type(i), 0, Iand::instantiate_Iand);
    }

} // namespace Iand

namespace And {

    static ASR::expr_t *eval_And(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        if(ASR::is_a<ASR::IntegerConstant_t>(*args[0])){
            int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
            int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
            int64_t result;
            result = val1 & val2;
            return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
        } else {
            bool val1 = ASR::down_cast<ASR::LogicalConstant_t>(args[0])->m_value;
            bool val2 = ASR::down_cast<ASR::LogicalConstant_t>(args[1])->m_value;
            bool result;
            result = val1 & val2;
            return make_ConstantWithType(make_LogicalConstant_t, result, t1, loc);
        }
    }

    static inline ASR::expr_t* instantiate_And(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_and_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = and(x, y)
        * r = x & y
        */
        body.push_back(al, b.Assignment(result, b.And(args[0], args[1])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace And

namespace Ior {

    static ASR::expr_t *eval_Ior(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int64_t result;
        result = val1 | val2;
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Ior(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_ior_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = ior(x, y)
        * r = x | y
        */
        body.push_back(al, b.Assignment(result, b.Or(args[0], args[1])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

    static inline ASR::expr_t* IOR(ASRBuilder& b, ASR::expr_t* i, ASR::expr_t* j, SymbolTable* scope) {
        return b.CallIntrinsic(scope, {expr_type(i), expr_type(j)}, {i, j}, expr_type(i), 0, Ior::instantiate_Ior);
    }

} // namespace Ior

namespace Or {

    static ASR::expr_t *eval_Or(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        if(ASR::is_a<ASR::IntegerConstant_t>(*args[0])){
            int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
            int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
            int64_t result;
            result = val1 | val2;
            return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
        } else {
            bool val1 = ASR::down_cast<ASR::LogicalConstant_t>(args[0])->m_value;
            bool val2 = ASR::down_cast<ASR::LogicalConstant_t>(args[1])->m_value;
            bool result;
            result = val1 || val2;
            return make_ConstantWithType(make_LogicalConstant_t, result, t1, loc);
        }
    }

    static inline ASR::expr_t* instantiate_Or(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_or_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = or(x, y)
        * r = x | y
        */
        body.push_back(al, b.Assignment(result, b.Or(args[0], args[1])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Or

namespace Ieor {

    static ASR::expr_t *eval_Ieor(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int64_t result;
        result = val1 ^ val2;
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Ieor(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_ieor_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = ieor(x, y)
        * r = x ^ y
        */
        body.push_back(al, b.Assignment(result, b.Xor(args[0], args[1])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Ieor

namespace Xor {

    static ASR::expr_t *eval_Xor(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        if(ASR::is_a<ASR::IntegerConstant_t>(*args[0])){
            int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
            int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
            int64_t result;
            result = val1 ^ val2;
            return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
        } else {
            bool val1 = ASR::down_cast<ASR::LogicalConstant_t>(args[0])->m_value;
            bool val2 = ASR::down_cast<ASR::LogicalConstant_t>(args[1])->m_value;
            bool result;
            result = val1 ^ val2;
            return make_ConstantWithType(make_LogicalConstant_t, result, t1, loc);
        }
    }

    static inline ASR::expr_t* instantiate_Xor(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_xor_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = xor(x, y)
        * r = x ^ y
        */
        body.push_back(al, b.Assignment(result, b.Xor(args[0], args[1])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Xor

namespace Ibclr {

    static ASR::expr_t *eval_Ibclr(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int64_t result;
        if ( val2 < 0 ) {
            diag.semantic_error_label("`pos` argument of `ibclr` intrinsic must be non-negative", {loc}, "");
        }
        result = val1 & ~(1 << val2);
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Ibclr(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_ibclr_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = ibclr(x, y)
        * r = x & ~( 1 << y )
        */
        body.push_back(al, b.Assignment(result, b.And(args[0], b.Not(b.BitLshift(b.i_t(1, arg_types[0]), b.i2i_t(args[1], arg_types[0]), return_type)))));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Ibclr

namespace Ibset {

    static ASR::expr_t *eval_Ibset(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int64_t result;
        if ( val2 < 0 ) {
            diag.semantic_error_label("`pos` argument of `ibset` intrinsic must be non-negative", {loc}, "");
        }
        result = val1 | (1 << val2);
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Ibset(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_ibset_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = ibset(x, y)
        * r = x | ( 1 << y )
        */
        body.push_back(al, b.Assignment(result, b.Or(args[0], b.BitLshift(b.i_t(1, arg_types[0]), b.i2i_t(args[1], arg_types[0]), return_type))));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Ibset

namespace Btest {

    static ASR::expr_t *eval_Btest(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        bool result;
        if ( val2 < 0 ) {
            diag.semantic_error_label("`pos` argument of `btest` intrinsic must be non-negative", {loc}, "");
        }
        if ((val1 & (1 << val2)) == 0) result = false;
        else result = true;
        return make_ConstantWithType(make_LogicalConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Btest(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_btest_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = btest(x, y)
        * r = (( x  & ( 1 << y )) == 0) ? .false. : .true.
        */
        body.push_back(al, b.If(b.Eq(b.And(args[0], b.BitLshift(b.i_t(1, arg_types[0]), b.i2i_t(args[1], arg_types[0]), arg_types[0])), b.i_t(0, arg_types[0])), {
            b.Assignment(result, b.bool_t(0, return_type))
        }, {
            b.Assignment(result, b.bool_t(1, return_type))
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Btest

namespace Ibits {

    static ASR::expr_t *eval_Ibits(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int64_t val1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t val2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int64_t val3 = ASR::down_cast<ASR::IntegerConstant_t>(args[2])->m_n;
        if ( val2 < 0 ) {
            diag.semantic_error_label("`pos` argument of `ibits` intrinsic must be non-negative", {loc}, "");
        }
        if ( val3 < 0 ) {
            diag.semantic_error_label("`len` argument of `ibits` intrinsic must be non-negative", {loc}, "");
        }
        int64_t result;
        result = (val1 >> val2) & ((1 << val3) - 1);
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Ibits(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_ibits_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        fill_func_arg("z", arg_types[2]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = ibits(x, y, z)
        * r = ( x >> y ) & ( ( 1 << z ) - 1 )
        */
        body.push_back(al, b.Assignment(result, b.And(b.BitRshift(args[0], b.i2i_t(args[1], arg_types[0]), return_type), b.Sub(b.BitLshift(b.i_t(1, arg_types[0]), b.i2i_t(args[2], arg_types[0]), return_type), b.i_t(1, arg_types[0])))));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Ibits

namespace Aint {

    static ASR::expr_t *eval_Aint(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        double rv = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
        ASRUtils::ASRBuilder b(al, loc);
        return b.f_t(std::trunc(rv), arg_type);
    }

    static inline ASR::expr_t* instantiate_Aint(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_aint_" + type_to_str_python(arg_types[0]));
        fill_func_arg("a", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);

        // Cast: Real -> Integer -> Real
        // TODO: this approach doesn't work for numbers > i64_max
        body.push_back(al, b.Assignment(result, b.i2r_t(b.r2i_t(args[0], int64), return_type)));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

}  // namespace Aint

namespace Anint {

    static ASR::expr_t *eval_Anint(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        double rv = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
        ASRUtils::ASRBuilder b(al, loc);
        return b.f_t(std::round(rv), arg_type);
    }

    static inline ASR::expr_t* instantiate_Anint(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_anint_" + type_to_str_python(arg_types[0]));
        fill_func_arg("a", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * if (x > 0) then
        *     r = aint(x+0.5)
        * else
        *     r = aint(x-0.5)
        * end if
        */
        body.push_back(al, b.If(b.Gt(args[0], b.f_t(0, arg_types[0])), {
            b.Assignment(result, b.CallIntrinsic(scope, {arg_types[0]}, {b.Add(args[0], b.f_t(0.5, arg_types[0]))},
                                        return_type, 0, Aint::instantiate_Aint))
        }, {
            b.Assignment(result, b.CallIntrinsic(scope, {arg_types[0]}, {b.Sub(args[0], b.f_t(0.5, arg_types[0]))},
                                        return_type, 0, Aint::instantiate_Aint))
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

}  // namespace Anint

namespace Nint {

    static ASR::expr_t *eval_Nint(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
        double rv = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
        double near_integer = std::round(rv);

        if (kind == 4) {
            if (near_integer < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
                near_integer > static_cast<double>(std::numeric_limits<int32_t>::max())) {
                diag.semantic_error_label("Result of `nint` overflows its kind(" + std::to_string(kind) + ")", {loc}, "");
            }
        } else if (kind == 8) {
            if (near_integer < static_cast<double>(std::numeric_limits<int64_t>::min()) ||
                near_integer > static_cast<double>(std::numeric_limits<int64_t>::max())) {
                diag.semantic_error_label("Result of `nint` overflows its kind(" + std::to_string(kind) + ")", {loc}, "");
            }
        } else {
            diag.semantic_error_label("Unsupported integer kind", {loc}, "");
        }

        int64_t result = int64_t(near_integer);
        return make_ConstantWithType(make_IntegerConstant_t, result, arg_type, loc);
    }

    static inline ASR::expr_t* instantiate_Nint(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_nint_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = nint(x)
        * r = int(anint(x))
        */
        body.push_back(al,b.Assignment(result, b.r2i_t(b.CallIntrinsic(scope, {arg_types[0]}, {args[0]}, arg_types[0], 0, Anint::instantiate_Anint), return_type)));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }
} // namespace Nint

namespace Idnint {

    static ASR::expr_t *eval_Idnint(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        if (ASRUtils::extract_kind_from_ttype_t(expr_type(args[0])) != 8 ) {
            diag.semantic_error_label("`idnint` takes argument of kind 8", {loc}, "");
        }
        double rv = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
        double near_integer = std::round(rv);

        if (near_integer < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
            near_integer > static_cast<double>(std::numeric_limits<int32_t>::max())) {
            diag.semantic_error_label("Result of `idnint` overflows its kind 4", {loc}, "");
        }

        int32_t result = int32_t(near_integer);
        return make_ConstantWithType(make_IntegerConstant_t, result, arg_type, loc);
    }

    static inline ASR::expr_t* instantiate_Idnint(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_idnint_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        if (ASRUtils::extract_kind_from_ttype_t(arg_types[0]) != 8) {
            throw LCompilersException("argument of `idnint` must have kind equals to 8");
            return nullptr;
        }
        /*
        * r = idnint(x)
        * r = int(anint(x))
        */
        body.push_back(al,b.Assignment(result, b.r2i_t(b.CallIntrinsic(scope, {arg_types[0]}, {args[0]}, arg_types[0], 0, Anint::instantiate_Anint), return_type)));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }
} // namespace Idnint

namespace Logical {

    static ASR::expr_t *eval_Logical(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        bool result = ASR::down_cast<ASR::LogicalConstant_t>(args[0])->m_value;
        return make_ConstantWithType(make_LogicalConstant_t, result, arg_type, loc);
    }

    static inline ASR::expr_t* instantiate_Logical(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_logical_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        body.push_back(al,b.Assignment(result, b.bool_t(args[0], return_type)));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }
} // namespace Logical

namespace Floor {

    static ASR::expr_t *eval_Floor(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        float val = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
        int64_t result = int64_t(val);
        if(val<=0.0 && val!=result) {
            result = result-1;
        }
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Floor(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_floor_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = floor(x)
        * r = int(x)
        * if(x <= 0.00 && x != r){
        *  r = int(x) - 1
        * }
        */
        body.push_back(al, b.Assignment(result, b.r2i_t(args[0], return_type)));
        body.push_back(al, b.If(b.And(b.LtE(args[0], b.f_t(0, arg_types[0])), b.NotEq(b.i2r_t(b.r2i_t(args[0], return_type), arg_types[0]), args[0])),
        {
            b.Assignment(result, b.Sub(b.r2i_t(args[0], return_type), b.i_t(1, return_type)))
        }, {}));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);

    }
    static inline ASR::expr_t* FLOOR(ASRBuilder &b, ASR::expr_t* x, ASR::ttype_t* t, SymbolTable* scope) {
        return b.CallIntrinsic(scope, {expr_type(x)}, {x}, t, 0, Floor::instantiate_Floor);
    }

} // namespace Floor

namespace Ceiling {

    static ASR::expr_t *eval_Ceiling(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        double val = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
        double difference = val - double(int(val));
        int64_t result;
        if (difference == 0.0) {
            result = int(val);
        } else if(val <= 0.0){
            result = int(val);
        } else{
            result = int(val) + 1;
        }
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Ceiling(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_ceiling_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = Ceiling(x)
        * if(x >= 0.00){
        *   if(x == int(x)){
        *       r = int(x)
        *   } else {
        *       r = int(x) + 1
        *   }
        * } else {
        *   r = int(x)
        * }
        */
        body.push_back(al, b.If(b.GtE(args[0], b.f_t(0, arg_types[0])),
        {
            b.If(b.Eq(args[0],
                b.i2r_t(b.r2i_t(args[0], return_type), arg_types[0])
                ),
            {
                b.Assignment(result, b.r2i_t(args[0], return_type))
            }, {
                b.Assignment(result, b.Add(b.r2i_t(args[0], return_type), b.i_t(1, return_type)))
            })
        }, {
            b.Assignment(result, b.r2i_t(args[0], return_type))
        }));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);

    }

} // namespace Ceiling

namespace Dim {

    static ASR::expr_t *eval_Dim(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        if (is_real(*t1)) {
            double a = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            double b = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
            double result;
            double zero = 0.0;
            if (a > b) {
                result = a - b;
            } else {
                result = zero;
            }
            return make_ConstantWithType(make_RealConstant_t, result, t1, loc);
        }
        LCOMPILERS_ASSERT(is_integer(*t1));
        int64_t a = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t b = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int64_t result;
        if (a > b) {
            result = a - b;
        } else {
            result = 0;
        }
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Dim(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_dim_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = Dim(x)
        * if (x > y) {
        *   r = x - y
        * } else {
        *   r = 0
        * }
        */
        if (is_real(*arg_types[0])) {
            body.push_back(al, b.If(b.Gt(args[0], b.r2r_t(args[1], arg_types[0])), {
                b.Assignment(result, b.Sub(args[0], b.r2r_t(args[1], arg_types[0])))
            }, {
                b.Assignment(result, b.f_t(0.0, arg_types[0]))
            }));
        } else {
            body.push_back(al, b.If(b.Gt(args[0], b.i2i_t(args[1], arg_types[0])), {
                b.Assignment(result, b.Sub(args[0], b.i2i_t(args[1], arg_types[0])))
            }, {
                b.Assignment(result, b.i_t(0, arg_types[0]))
            }));
        }

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);

    }

} // namespace Dim

namespace Sqrt {

    static ASR::expr_t *eval_Sqrt(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        ASRUtils::ASRBuilder b(al, loc);
        if (is_real(*arg_type)) {
            double val = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            if (val < 0.0) {
                append_error(diag, "Argument of `sqrt` has a negative argument", loc);
                return nullptr;
            }
            return b.f_t(std::sqrt(val), arg_type);
        } else {
            std::complex<double> crv;
            if( ASRUtils::extract_value(args[0], crv) ) {
                std::complex<double> val = std::sqrt(crv);
                return ASRUtils::EXPR(ASR::make_ComplexConstant_t(
                    al, loc, val.real(), val.imag(), arg_type));
            } else {
                return nullptr;
            }
        }
    }

    static inline ASR::expr_t* instantiate_Sqrt(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t overload_id) {
        ASR::ttype_t* arg_type = arg_types[0];
        if (is_real(*arg_type)) {
            return EXPR(ASR::make_RealSqrt_t(al, loc,
                new_args[0].m_value, return_type, nullptr));
        } else {
            return UnaryIntrinsicFunction::instantiate_functions(al, loc, scope,
                "sqrt", arg_type, return_type, new_args, overload_id);
        }
    }

}  // namespace Sqrt

namespace Exponent {

    static ASR::expr_t* eval_Exponent(Allocator& al, const Location& loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        ASR::ttype_t* arguement_type = expr_type(args[0]);
        int32_t kind = extract_kind_from_ttype_t(arguement_type);

        if (kind == 4) {
            float x = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            if (x == 0.0) {
                return make_ConstantWithType(make_IntegerConstant_t, 0, arg_type, loc);
            }
            int32_t ix;
            std::memcpy(&ix, &x, sizeof(ix));
            int32_t exponent = ((ix >> 23) & 0xff) - 126;
            return make_ConstantWithType(make_IntegerConstant_t, exponent, arg_type, loc);
        }
        else if (kind == 8) {
            double x = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            if (x == 0.0) {
                return make_ConstantWithType(make_IntegerConstant_t, 0, arg_type, loc);
            }
            int64_t ix;
            std::memcpy(&ix, &x, sizeof(ix));
            int64_t exponent = ((ix >> 52) & 0x7ff) - 1022;
            return make_ConstantWithType(make_IntegerConstant_t, exponent, arg_type, loc);
        }
        return nullptr;
    }


    static inline ASR::expr_t* instantiate_Exponent(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompiler_optimization_exponent_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        int32_t kind = extract_kind_from_ttype_t(arg_types[0]);
        /*
            if (x == 0.0) then
                result = 0
            else
                result = iand(shiftr(transfer(x, 0), 23), int(Z'0FF', kind=4)) - 126 ! for real kind = 4
                result = iand(shiftr(transfer(x, 0), 52), int(Z'7FF', kind=8)) - 1022 ! for real kind = 8
            end if
        */
        if (kind == 8) {
                body.push_back(al, b.If(b.Eq(args[0], b.f_t(0.0, arg_types[0])), {
                b.Assignment(result, b.i32(0))
            }, {
                b.Assignment(result, b.i2i_t(b.Sub(b.And(b.BitRshift(ASRUtils::EXPR(ASR::make_BitCast_t(al, loc, args[0], b.i64(0), nullptr, int64, nullptr)),
                    b.i64(52), int64), b.i64(0x7FF)), b.i64(1022)), int32))
            }));
        } else {
                body.push_back(al, b.If(b.Eq(args[0], b.f_t(0.0, arg_types[0])), {
                b.Assignment(result, b.i32(0))
            }, {
                b.Assignment(result, b.Sub(b.And(b.BitRshift(ASRUtils::EXPR(ASR::make_BitCast_t(al, loc, args[0], b.i32(0), nullptr, int32, nullptr)),
                b.i32(23), int32), b.i32(0x0FF)), b.i32(126)))
            }));
        }

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }
}  // namespace Exponent

namespace Fraction {
    static ASR::expr_t *eval_Fraction(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        ASR::ttype_t* arguement_type = expr_type(args[0]);
        int32_t kind = extract_kind_from_ttype_t(arguement_type);
        if (kind == 4) {
            float x = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            int32_t exponent;
            if (x == 0.0) {
                exponent = 0;
                float result =  x * std::pow((2), (-1*(exponent)));
                return make_ConstantWithType(make_RealConstant_t, result, arg_type, loc);
            }
            else{
                int32_t ix;
                std::memcpy(&ix, &x, sizeof(ix));
                exponent = ((ix >> 23) & 0xff) - 126;
                float result =  x * std::pow((2), (-1*(exponent)));
                return make_ConstantWithType(make_RealConstant_t, result, arg_type, loc);
            }
        }
        else if (kind == 8) {
            double x = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            int64_t exponent;
            if (x == 0.0) {
                exponent = 0;
                double result =  x * std::pow((2), (-1*(exponent)));
                return make_ConstantWithType(make_RealConstant_t, result, arg_type, loc);
            }
            else{
                int64_t ix;
                std::memcpy(&ix, &x, sizeof(ix));
                exponent = ((ix >> 52) & 0x7ff) - 1022;
                double result =  x * std::pow((2), (-1*(exponent)));
                return make_ConstantWithType(make_RealConstant_t, result, arg_type, loc);
            }
        }
        return nullptr;
    }

    static inline ASR::expr_t* instantiate_Fraction(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_fraction_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = fraction(x, y)
        * r = x * radix(x)**(-exp(x))
        */
        ASR::expr_t* func_call_exponent = b.CallIntrinsic(scope, {arg_types[0]}, {args[0]}, int32, 0, Exponent::instantiate_Exponent);
        body.push_back(al, b.Assignment(result, b.Mul(args[0], b.Pow(b.i2r_t(b.i32(2),return_type), b.Mul(b.i2r_t(b.i32(-1), return_type),b.i2r_t(func_call_exponent, return_type))))));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args, body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }
}  // namespace Fraction

namespace SetExponent {
    static ASR::expr_t *eval_SetExponent(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        ASR::ttype_t* arguement_type = expr_type(args[0]);
        int32_t kind = extract_kind_from_ttype_t(arguement_type);
        if (kind == 4) {
            float x = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            int32_t I = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
            int32_t exponent;
            if (x == 0.0) {
                exponent = 0;
                float result1 =  x * std::pow((2), (-1*(exponent)));
                float result = result1 * std::pow((2), I);
                return make_ConstantWithType(make_RealConstant_t, result, arg_type, loc);
            } else {
                int32_t ix;
                std::memcpy(&ix, &x, sizeof(ix));
                exponent = ((ix >> 23) & 0xff) - 126;
                float result1 =  x * std::pow((2), (-1*(exponent)));
                float result = result1 * std::pow((2), I);
                return make_ConstantWithType(make_RealConstant_t, result, arg_type, loc);
            }
        }
        else if (kind == 8) {
            double x = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            int64_t I = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
            int64_t exponent;
            if (x == 0.0) {
                exponent = 0;
                double result1 =  x * std::pow((2), (-1*(exponent)));
                double result = result1 * std::pow((2), I);
                return make_ConstantWithType(make_RealConstant_t, result, arg_type, loc);
            } else {
                int64_t ix;
                std::memcpy(&ix, &x, sizeof(ix));
                exponent = ((ix >> 52) & 0x7ff) - 1022;
                double result1 =  x * std::pow((2), (-1*(exponent)));
                double result = result1 * std::pow((2), I);
                return make_ConstantWithType(make_RealConstant_t, result, arg_type, loc);
            }
        }
        return nullptr;
    }

    static inline ASR::expr_t* instantiate_SetExponent(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_setexponent_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("i", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);

        /*
        * r = setexponent(x, I)
        * r = fraction(x) * radix(x)**(I)
        */
        ASR::expr_t* func_call_fraction = b.CallIntrinsic(scope, {arg_types[0]}, {args[0]}, return_type, 0, Fraction::instantiate_Fraction);
        body.push_back(al, b.Assignment(result, b.Mul(func_call_fraction, b.Pow(b.i2r_t(b.i32(2),return_type),b.i2r_t(args[1], return_type)))));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args, body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }
}  // namespace SetExponent

namespace Sngl {

    static ASR::expr_t *eval_Sngl(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        ASRUtils::ASRBuilder b(al, loc);
        double val = ASR::down_cast<ASR::RealConstant_t>(expr_value(args[0]))->m_r;
        return b.f_t(val, arg_type);
    }

    static inline ASR::expr_t* instantiate_Sngl(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_sngl_" + type_to_str_python(arg_types[0]));
        fill_func_arg("a", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        body.push_back(al, b.Assignment(result, b.r2r_t(args[0], real32)));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

}  // namespace Sngl

namespace Ifix {

    static ASR::expr_t *eval_Ifix(Allocator &al, const Location &loc,
            ASR::ttype_t* /*arg_type*/, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int val = ASR::down_cast<ASR::RealConstant_t>(expr_value(args[0]))->m_r;
        if (ASRUtils::extract_kind_from_ttype_t(expr_type(args[0])) != 4) {
            append_error(diag, "first argument of `ifix` must have kind equals to 4", loc);
            return nullptr;
        }
        return make_ConstantWithType(make_IntegerConstant_t, val, int32, loc);
    }

    static inline ASR::expr_t* instantiate_Ifix(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_ifix_" + type_to_str_python(arg_types[0]));
        if (ASRUtils::extract_kind_from_ttype_t(arg_types[0]) != 4) {
            throw LCompilersException("first argument of `ifix` must have kind equals to 4");
            return nullptr;
        }
        fill_func_arg("a", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        body.push_back(al, b.Assignment(result, b.r2i_t(args[0], int32)));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

}  // namespace Ifix

namespace Idint {

    static ASR::expr_t *eval_Idint(Allocator &al, const Location &loc,
            ASR::ttype_t* /*arg_type*/, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int val = ASR::down_cast<ASR::RealConstant_t>(expr_value(args[0]))->m_r;
        int kind = ASRUtils::extract_kind_from_ttype_t(expr_type(args[0]));
        if(kind == 4) {
            append_error(diag, "first argument of `idint` must have kind equals to 8", loc);
            return nullptr;
        }
        return make_ConstantWithType(make_IntegerConstant_t, val, int32, loc);
    }

    static inline ASR::expr_t* instantiate_Idint(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_idint_" + type_to_str_python(arg_types[0]));
        fill_func_arg("a", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);

        int kind = ASRUtils::extract_kind_from_ttype_t(arg_types[0]);
        if(kind == 4) {
            throw LCompilersException("first argument of `idint` must have kind equals to 8");
            return nullptr;
        }
        body.push_back(al, b.Assignment(result, b.r2i_t(args[0], int32)));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

}

namespace FMA {

    static ASR::expr_t *eval_FMA(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        double a = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
        double b = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
        double c = ASR::down_cast<ASR::RealConstant_t>(args[2])->m_r;
        return make_ConstantWithType(make_RealConstant_t, a + b*c, t1, loc);
    }

    static inline ASR::expr_t* instantiate_FMA(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_optimization_fma_" + type_to_str_python(arg_types[0]));
        fill_func_arg("a", arg_types[0]);
        fill_func_arg("b", arg_types[0]);
        fill_func_arg("c", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
         * result = a + b*c
        */
        body.push_back(al, b.Assignment(result,
        b.Add(args[0], b.Mul(args[1], args[2]))));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace FMA


namespace SignFromValue {

    static ASR::expr_t *eval_SignFromValue(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        if (is_real(*t1)) {
            double a = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            double b = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
            a = (b < 0 ? -a : a);
            return make_ConstantWithType(make_RealConstant_t, a, t1, loc);
        }
        int64_t a = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t b = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        a = (b < 0 ? -a : a);
        return make_ConstantWithType(make_IntegerConstant_t, a, t1, loc);

    }

    static inline ASR::expr_t* instantiate_SignFromValue(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_optimization_signfromvalue_" + type_to_str_python(arg_types[0]));
        fill_func_arg("a", arg_types[0]);
        fill_func_arg("b", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        elemental real(real32) function signfromvaluer32r32(a, b) result(d)
            real(real32), intent(in) :: a, b
            d = a * asignr32(1.0_real32, b)
        end function
        */
        if (is_real(*arg_types[0])) {
            body.push_back(al, b.If(b.Lt(args[1], b.f_t(0.0, arg_types[1])), {
                b.Assignment(result, b.f_neg(args[0], arg_types[0]))
            }, {
                b.Assignment(result, args[0])
            }));
        } else {
            body.push_back(al, b.If(b.Lt(args[1], b.i_t(0, arg_types[1])), {
                b.Assignment(result, b.i_neg(args[0], arg_types[0]))
            }, {
                b.Assignment(result, args[0])
            }));
        }
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace SignFromValue


namespace FlipSign {

    static ASR::expr_t *eval_FlipSign(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int a = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        double b = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
        if (a % 2 == 1) b = -b;
        return make_ConstantWithType(make_RealConstant_t, b, t1, loc);
    }

    static inline ASR::expr_t* instantiate_FlipSign(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_optimization_flipsign_" + type_to_str_python(arg_types[1]));
        fill_func_arg("signal", arg_types[0]);
        fill_func_arg("variable", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        real(real32) function flipsigni32r32(signal, variable)
            integer(int32), intent(in) :: signal
            real(real32), intent(out) :: variable
            integer(int32) :: q
            q = signal/2
            flipsigni32r32 = variable
            if (signal - 2*q == 1 ) flipsigni32r32 = -variable
        end subroutine
        */

        body.push_back(al, b.If(b.Eq(b.Sub(args[0], b.Mul(b.i_t(2, arg_types[0]), b.Div(args[0], b.i_t(2, arg_types[0])))), b.i_t(1, arg_types[0])), {
            b.Assignment(result, b.f_neg(args[1], arg_types[1]))
        }, {
            b.Assignment(result, args[1])
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace FlipSign

namespace FloorDiv {

    static ASR::expr_t *eval_FloorDiv(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        ASR::ttype_t *type1 = ASRUtils::expr_type(args[0]);
        ASR::ttype_t *type2 = ASRUtils::expr_type(args[1]);
        bool is_real1 = is_real(*type1);
        bool is_real2 = is_real(*type2);
        bool is_int1 = is_integer(*type1);
        bool is_int2 = is_integer(*type2);
        bool is_unsigned_int1 = is_unsigned_integer(*type1);
        bool is_unsigned_int2 = is_unsigned_integer(*type2);
        bool is_logical1 = is_logical(*type1);
        bool is_logical2 = is_logical(*type2);


        if (is_int1 && is_int2) {
            int64_t a = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
            int64_t b = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
            if (b == 0) {
                append_error(diag, "Division by `0` is not allowed", loc);
                return nullptr;
            }
            return make_ConstantWithType(make_IntegerConstant_t, a / b, t1, loc);
        } else if (is_unsigned_int1 && is_unsigned_int2) {
            int64_t a = ASR::down_cast<ASR::UnsignedIntegerConstant_t>(args[0])->m_n;
            int64_t b = ASR::down_cast<ASR::UnsignedIntegerConstant_t>(args[1])->m_n;
            if (b == 0) {
                append_error(diag, "Division by `0` is not allowed", loc);
                return nullptr;
            }
            return make_ConstantWithType(make_UnsignedIntegerConstant_t, a / b, t1, loc);
        } else if (is_logical1 && is_logical2) {
            bool a = ASR::down_cast<ASR::LogicalConstant_t>(args[0])->m_value;
            bool b = ASR::down_cast<ASR::LogicalConstant_t>(args[1])->m_value;
            if (b == 0) {
                append_error(diag, "Division by `0` is not allowed", loc);
                return nullptr;
            }
            return make_ConstantWithType(make_LogicalConstant_t, a / b, t1, loc);
        } else if (is_real1 && is_real2) {
            double a = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            double b = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
            if (b == 0.0) {
                append_error(diag, "Division by `0` is not allowed", loc);
                return nullptr;
            }
            double r = a / b;
            int64_t result = (int64_t)r;
            if ( r >= 0.0 || (double)result == r) {
                return make_ConstantWithType(make_RealConstant_t, (double)result, t1, loc);
            }
            return make_ConstantWithType(make_RealConstant_t, (double)(result - 1), t1, loc);
        }
        return nullptr;
    }

    static inline ASR::expr_t* instantiate_FloorDiv(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_optimization_floordiv_" + type_to_str_python(arg_types[1]));
        fill_func_arg("a", arg_types[0]);
        fill_func_arg("b", arg_types[1]);
        auto r = declare("r", real64, Local);
        auto tmp = declare("tmp", int64, Local);
        auto result = declare("result", return_type, ReturnVar);
        /*
        @overload
        def _lpython_floordiv(a: i32, b: i32) -> i32:
            r: f64 # f32 rounds things up and gives incorrect tmps
            tmp: i64
            result: i32
            r = float(a)/float(b)
            tmp = i64(r)
            if r < 0.0 and f64(tmp) != r:
                tmp = tmp - 1
            result = i32(tmp)
            return result
        */
        body.push_back(al, b.Assignment(r, b.Div(CastingUtil::perform_casting(args[0], real64, al, loc),
            CastingUtil::perform_casting(args[1], real64, al, loc))));
        body.push_back(al, b.Assignment(tmp, b.r2i_t(r, int64)));
        body.push_back(al, b.If(b.And(b.Lt(r, b.f_t(0.0, real64)), b.NotEq(b.i2r_t(tmp, real64), r)), {
                b.Assignment(tmp, b.Sub(tmp, b.i32(1)))
            }, {}));
        body.push_back(al, b.Assignment(result, CastingUtil::perform_casting(tmp, return_type, al, loc)));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace FloorDiv

namespace Mod {

    static ASR::expr_t *eval_Mod(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        bool is_real1 = is_real(*ASRUtils::expr_type(args[0]));
        bool is_real2 = is_real(*ASRUtils::expr_type(args[1]));
        bool is_int1 = is_integer(*ASRUtils::expr_type(args[0]));
        bool is_int2 = is_integer(*ASRUtils::expr_type(args[1]));

        if (is_int1 && is_int2) {
            int64_t a = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
            int64_t b = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
            return make_ConstantWithType(make_IntegerConstant_t, a % b, t1, loc);
        } else if (is_real1 && is_real2) {
            double a = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            double b = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
            return make_ConstantWithType(make_RealConstant_t, std::fmod(a, b), t1, loc);
        }
        return nullptr;
    }

    static inline ASR::expr_t* instantiate_Mod(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_optimization_mod_" + type_to_str_python(arg_types[1]));
        fill_func_arg("a", arg_types[0]);
        fill_func_arg("p", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        function modi32i32(a, p) result(d)
            integer(int32), intent(in) :: a, p
            integer(int32) :: q
            q = a/p
            d = a - p*q
        end function
        */
        int kind = ASRUtils::extract_kind_from_ttype_t(arg_types[1]);
        int kind2 = ASRUtils::extract_kind_from_ttype_t(arg_types[0]);
        int upper_kind = std::max(kind, kind2);
        if (is_real(*arg_types[1])) {
            ASR::ttype_t* new_type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, upper_kind));
            ASR::expr_t* arg0 = b.r2r_t(args[0], new_type);
            ASR::expr_t* arg1 = b.r2r_t(args[1], new_type);

            if (upper_kind == 4) {
                body.push_back(al, b.Assignment(result, b.Sub(arg0, b.Mul(arg1, b.i2r_t(b.r2i_t(b.Div(arg0, arg1), real32), real32)))));
            } else {
                body.push_back(al, b.Assignment(result, b.Sub(arg0, b.Mul(arg1, b.i2r_t(b.r2i_t(b.Div(arg0, arg1), real64), real64)))));
            }
        } else {
            ASR::ttype_t* new_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, upper_kind));
            ASR::expr_t* arg0 = b.i2i_t(args[0], new_type);
            ASR::expr_t* arg1 = b.i2i_t(args[1], new_type);

            body.push_back(al, b.Assignment(result, b.Sub(arg0, b.Mul(arg1, b.Div(arg0, arg1)))));
        }

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }
    static inline ASR::expr_t* MOD(ASRBuilder &b, ASR::expr_t* a, ASR::expr_t* p, SymbolTable* scope) {
        return b.CallIntrinsic(scope, {expr_type(a), expr_type(p)}, {a, p}, expr_type(a), 0, Mod::instantiate_Mod);
    }

} // namespace Mod

namespace Popcnt {

    template <typename T>
    int compute_count(T mask, int64_t val) {
        int count = 0;
        while (mask != 0) {
            if (val & mask) count++;
            mask = mask << 1;
        }
        return count;
    }

    static ASR::expr_t *eval_Popcnt(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int kind = ASRUtils::extract_kind_from_ttype_t(ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_type);
        int64_t val = static_cast<int64_t>(ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n);
        int64_t mask1 = 1;
        int32_t mask2 = 1;
        int count = 0;
        if (val < 0) {
            count = kind == 4 ? compute_count(mask2, val) : compute_count(mask1, val);
        } else {
            while (val) {
                count += val & 1;
                val >>= 1;
            }
        }
        return make_ConstantWithType(make_IntegerConstant_t, count, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Popcnt(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
            declare_basic_variables("_lcompilers_popcnt_" + type_to_str_python(arg_types[0]));
        fill_func_arg("i", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        auto count = declare("j", arg_types[0], Local);
        auto val = declare("k", arg_types[0], Local);
        auto mask = declare("l", arg_types[0], Local);
        /*
        function popcnt(i) result(r)
        integer, intent(in) :: i
        integer :: r, count, mask
        count = 0
        mask = 1
        if (i >= 0) then
            ! For positive numbers
            do while (i /= 0)
                count = count + mod(i, 2)
                i = i / 2
            end do
        else
            ! For negative numbers
            do while (mask /= 0)
                if ((i .and. mask) /= 0) then
                    count = count + 1
                end if
                mask = mask * 2
            end do
        end if
        r = count
        end function popcnt
        */
        body.push_back(al, b.Assignment(count, b.i_t(0,arg_types[0])));
        body.push_back(al, b.Assignment(val, args[0]));
        body.push_back(al, b.Assignment(mask, b.i_t(1,arg_types[0])));
        body.push_back(al, b.If(b.GtE(args[0], b.i_t(0,arg_types[0])), {
            b.While(b.NotEq(val, b.i_t(0, arg_types[0])), {
                b.Assignment(count, b.Add(count, Mod::MOD(b, val, b.i_t(2, arg_types[0]), scope))),
                b.Assignment(val, b.Div(val, b.i_t(2, arg_types[0])))
                })
        }, {
            b.While(b.NotEq(mask, b.i_t(0, arg_types[0])), {
                b.If(b.NotEq(b.i_t(0,arg_types[0]), (b.And(val,mask))), {b.Assignment(count, b.Add(count, b.i_t(1, arg_types[0])))},
                            {}),
                b.Assignment(mask, b.BitLshift(mask, b.i_t(1, arg_types[0]), arg_types[0]))
            })
        }));
        body.push_back(al, b.Assignment(result, b.i2i_t(count, return_type)));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args, body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }
    static inline ASR::expr_t* POPCNT(ASRBuilder &b, ASR::expr_t* a, ASR::ttype_t *return_type, SymbolTable* scope) {
        return b.CallIntrinsic(scope, {expr_type(a)}, {a}, return_type, 0, Popcnt::instantiate_Popcnt);
    }
} // namespace Popcnt

namespace Maskl {
    static ASR::expr_t* eval_Maskl(Allocator& al, const Location& loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        int32_t kind = ASRUtils::extract_kind_from_ttype_t(t1);
        int64_t i = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        if ((kind == 4 && i > 32) || (kind == 8 && i > 64)) {
            diag.semantic_error_label(" first argument of `maskl` must be less than or equal to the BIT_SIZE of INTEGER(KIND="
            + std::to_string(kind) + ")", {loc}, "");
            return nullptr;
        } else if (i < 0) {
            diag.semantic_error_label("first argument of `maskl` must be nonnegative", {loc}, "");
            return nullptr;
        } else {
            int64_t bit_size = (i <= 32 && kind == 4) ? 32 : 64;
            int64_t result;
            if (i == 0) {
                result = 0;
            } else {
                result = (i == bit_size) ? -1 : ((~0ULL) << (bit_size - i));
            }
            return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
        }
    }

    static inline ASR::expr_t* instantiate_Maskl(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_maskl_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = Maskl(x)
        * r = (x == 64) ? -1 : ((1 << x) - 1) << (64 - x)
        */
        // For return_type is 8 result is always 64 bit
        /*if (x == 32 .or. x == 64) then
        *    res = -1
        * else
        *    if (x <= 32) then             ! 32 bit result
        *        res = ishft(1, x) - 1 
        *        res = ishft(res, 32 - x)
        *    else                            ! 64 bit result     
        *        res = ishft(1, x) - 1 
        *        res = ishft(res, 64 - x)
        *    end if
        *end if
        */
    //    std::cout<<ASRUtils::extract_kind_from_ttype_t(return_type)<<" "<<ASRUtils::extract_kind_from_ttype_t(arg_types[0])<<std::endl;
       if (ASRUtils::extract_kind_from_ttype_t(return_type) == 8) {
            body.push_back(al, b.If(b.Or((b.Eq(b.i2i_t(args[0], return_type), b.i_t(32, return_type))),(b.Eq(b.i2i_t(args[0], return_type), b.i_t(64, return_type)))), {
                b.Assignment(result, b.i_t(-1, return_type))
            }, {
                b.Assignment(result, b.BitLshift(b.Sub(b.BitLshift(b.i_t(1, return_type), b.i2i_t(args[0], return_type), return_type), b.i_t(1, return_type)),
                    b.Sub(b.i_t(64, return_type), b.i2i_t(args[0], return_type)), return_type))
            }));
        } else {
            body.push_back(al,b.If(b.Or((b.Eq(b.i2i_t(args[0], return_type), b.i_t(32, return_type))),(b.Eq(b.i2i_t(args[0], return_type), b.i_t(64, return_type)))), {
                b.Assignment(result, b.i_t(-1, return_type))
            }, {
                b.If(b.LtE(b.i2i_t(args[0], return_type), b.i_t(32, return_type)), 
                    { b.Assignment(result, b.BitLshift(b.Sub(b.BitLshift(b.i_t(1, return_type), b.i2i_t(args[0], return_type), return_type), b.i_t(1, return_type)),
                        b.Sub(b.i_t(32, return_type), b.i2i_t(args[0], return_type)), return_type)) },
                    { b.Assignment(result, b.BitLshift(b.Sub(b.BitLshift(b.i_t(1, return_type), b.i2i_t(args[0], return_type), return_type), b.i_t(1, return_type)),
                        b.Sub(b.i_t(64, return_type), b.i2i_t(args[0], return_type)), return_type)) } )
            }));
                
        }
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args, body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

}  // namespace Maskl

namespace Maskr {
    static ASR::expr_t* eval_Maskr(Allocator& al, const Location& loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        int32_t kind = ASRUtils::extract_kind_from_ttype_t(t1);
        int64_t i = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        if (((kind == 4) && i > 32) || (kind == 8 && i > 64)) {
            diag.semantic_error_label("first argument of `maskr` must be less than or equal to the BIT_SIZE of INTEGER(KIND="
            + std::to_string(kind) + ")", {loc}, "");
            return nullptr;
        } else if (i < 0) {
            diag.semantic_error_label("first argument of `maskr` must be nonnegative", {loc}, "");
            return nullptr;
        }
        if(i == 64){
            return make_ConstantWithType(make_IntegerConstant_t, -1, t1, loc);
        }
        int64_t one = 1;
        int64_t result = (one << i) - one;
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Maskr(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_maskr_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = Maskr(x)
        * r = (1 << x) - 1
        */
        body.push_back(al, b.If((b.Eq(b.i2i_t(args[0], return_type), b.i_t(64, return_type))), {
            b.Assignment(result, b.i_t(-1, return_type))
        }, {
            b.Assignment(result, b.Sub(b.BitLshift(b.i_t(1, return_type), b.i2i_t(args[0], return_type), return_type), b.i_t(1, return_type)))
        }));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args, body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

}  // namespace Maskr

namespace Merge {

    static inline ASR::expr_t* eval_Merge(Allocator &, const Location &,
            ASR::ttype_t *, Vec<ASR::expr_t*>& args, diag::Diagnostics &) {
        bool mask = ASR::down_cast<ASR::LogicalConstant_t>(args[2])->m_value;
        ASR::expr_t *tsource = args[0], *fsource = args[1];
        if (mask) {
            return tsource;
        } else {
            return fsource;
        }
    }

    static inline ASR::expr_t* instantiate_Merge(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        ASR::ttype_t *tsource_type = nullptr, *fsource_type = nullptr, *mask_type = nullptr;
        tsource_type = ASRUtils::duplicate_type(al,
            ASRUtils::extract_type(arg_types[0]));
        fsource_type = ASRUtils::duplicate_type(al,
            ASRUtils::extract_type(arg_types[1]));
        mask_type = ASRUtils::duplicate_type(al,
            ASRUtils::extract_type(arg_types[2]));
        if( ASR::is_a<ASR::String_t>(*tsource_type) ) {
            ASR::String_t* tsource_char = ASR::down_cast<ASR::String_t>(tsource_type);
            ASR::String_t* fsource_char = ASR::down_cast<ASR::String_t>(fsource_type);
            tsource_char->m_len_expr = nullptr; fsource_char->m_len_expr = nullptr;
            tsource_char->m_len = -2; fsource_char->m_len = -2;
            ASR::String_t* return_char = ASR::down_cast<ASR::String_t>(
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
        // TODO: In case of String type, set len of ReturnVar to len(tsource) expression
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

namespace Spacing {

    static ASR::expr_t *eval_Spacing(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(args[0]));
        if (kind == 4) {
            float x = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            float result = std::fabs(std::nextafterf(x, std::numeric_limits<float>::infinity()) - x);
            return make_ConstantWithType(make_RealConstant_t, result, t1, loc);
        } else {
            double x = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            double result = std::fabs(std::nextafter(x, std::numeric_limits<double>::infinity()) - x);
            return make_ConstantWithType(make_RealConstant_t, result, t1, loc);
        }
    }

    static inline ASR::expr_t* instantiate_Spacing(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_spacing_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, arg_types[0], ReturnVar);
        /*
        function spacing(x) result(result)
            real :: x
            real :: result
            result = abs(nextafter(x, infinity) - x)
        end function
        */
       throw LCompilersException("`Spacing` intrinsic is not yet implemented for runtime values");

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Spacing

namespace Trailz {

    static ASR::expr_t *eval_Trailz(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t a = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t kind = ASRUtils::extract_kind_from_ttype_t(t1);
        int64_t trailing_zeros = ASRUtils::compute_trailing_zeros(a, kind);
        set_kind_to_ttype_t(t1, 4);
        return make_ConstantWithType(make_IntegerConstant_t, trailing_zeros, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Trailz(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_trailz_" + type_to_str_python(arg_types[0]));
        fill_func_arg("n", arg_types[0]);
        ASR::expr_t* n_val = declare("n_val", arg_types[0], Local);
        body.push_back(al, b.Assignment(n_val, args[0]));
        auto result = declare(fn_name, arg_types[0], ReturnVar);
        // This is not the most efficient way to do this, but it works for now.
        /*
        function trailz(n) result(result)
            integer :: n
            integer :: result
            integer :: k
            k = kind(n)
            result = 0
            if (n == 0) then
                if (k == 4) then
                    result = 32
                else
                    result = 64
                end if
            else
                do while (mod(n,2) == 0)
                    n = n/2
                    result = result + 1
                end do
            end if
        end function
        */
        body.push_back(al, b.Assignment(result, b.i_t(0, arg_types[0])));
        body.push_back(al, b.If(b.Eq(n_val, b.i_t(0, arg_types[0])), {
            b.Assignment(result, b.i_t(8*ASRUtils::extract_kind_from_ttype_t(arg_types[0]), arg_types[0]))
        }, {
            b.While(b.Eq(b.CallIntrinsic(scope, {arg_types[0], arg_types[0]
            }, {
            n_val, b.i_t(2, arg_types[0])}, return_type, 0, Mod::instantiate_Mod), b.i_t(0, arg_types[0])),
            {
                b.Assignment(n_val, b.Div(n_val, b.i_t(2, arg_types[0]))),
                b.Assignment(result, b.Add(result, b.i_t(1, arg_types[0])))
            })
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Trailz

namespace Nearest {

    static ASR::expr_t *eval_Nearest(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int64_t kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(args[0]));
        double s = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
        if (s == 0.0) {
            append_error(diag, "`S` argument of nearest() must be non-zero", loc);
            return nullptr;
        }
        if (kind == 4) {
            float x = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            float result = 0.0;
            if (s > 0) result = x + std::fabs(std::nextafterf(x, std::numeric_limits<float>::infinity()) - x);
            else result = x - std::fabs(std::nextafterf(x, -std::numeric_limits<float>::infinity()) - x);
            return make_ConstantWithType(make_RealConstant_t, result, t1, loc);
        } else {
            double x = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            double result = 0.0;
            if (s > 0) result = x + std::fabs(std::nextafter(x, std::numeric_limits<double>::infinity()) - x);
            else result = x - std::fabs(std::nextafter(x, -std::numeric_limits<double>::infinity()) - x);
            return make_ConstantWithType(make_RealConstant_t, result, t1, loc);
        }
    }

    static inline ASR::expr_t* instantiate_Nearest(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_optimization_nearest_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("s", arg_types[1]);
        auto result = declare(fn_name, arg_types[0], ReturnVar);
        /*
        function nearest(x, s) result(result)
            real :: x, s
            real :: result
            result = ?
        end function
        */
        throw LCompilersException("`Nearest` intrinsic is not yet implemented for runtime values");

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Nearest

namespace Modulo {

    static ASR::expr_t *eval_Modulo(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        if (is_integer(*ASRUtils::expr_type(args[0])) && is_integer(*ASRUtils::expr_type(args[1]))) {
            int64_t a = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
            int64_t b = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
            if (b == 0) {
                append_error(diag, "Second argument of modulo cannot be 0", loc);
            }
            return make_ConstantWithType(make_IntegerConstant_t, a - b * std::floor(std::real(a)/b), t1, loc);
        } else if (is_real(*ASRUtils::expr_type(args[0])) && is_real(*ASRUtils::expr_type(args[1]))) {
            double a = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            double b = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
            if (b == 0) {
                append_error(diag, "Second argument of modulo cannot be 0", loc);
            }
            return make_ConstantWithType(make_RealConstant_t, a - b * std::floor(a/b), t1, loc);
        }
        return nullptr;
    }

    static inline ASR::expr_t* instantiate_Modulo(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_optimization_modulo_" + type_to_str_python(arg_types[0]));
        fill_func_arg("a", arg_types[0]);
        fill_func_arg("p", arg_types[1]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        function modulo(a, p) result(d)
            d = a - p * floor(a/p)
        end function
        */
        if (is_real(*arg_types[0])) {
            body.push_back(al, b.Assignment(result, b.Sub(args[0], b.Mul(b.r2r_t(args[1], arg_types[0]) , b.i2r_t(Floor::FLOOR(b, b.Div(args[0], b.r2r_t(args[1], arg_types[0])), int32, scope), arg_types[1])))));
        } else {
            body.push_back(al, b.Assignment(result, b.Sub(args[0], b.Mul(b.i2i_t(args[1], arg_types[0]), Floor::FLOOR(b, b.Div(b.i2r_t(args[0], real32), b.i2r_t(args[1], real32)), int32, scope)))));
        }

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

}  // namespace Modulo

namespace BesselJN {

    static ASR::expr_t *eval_BesselJN(Allocator& al, const Location& loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        return make_ConstantWithType(make_RealConstant_t, jn(ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n, ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r), t1, loc);
    }

    static inline ASR::expr_t* instantiate_BesselJN(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        std::string c_func_name;
        if (ASRUtils::extract_kind_from_ttype_t(arg_types[1]) == 4) {
            c_func_name = "_lfortran_sbesseljn";
        } else {
            c_func_name = "_lfortran_dbesseljn";
        }
        std::string new_name = "_lcompilers_bessel_jn_"+ type_to_str_python(arg_types[1]);

        declare_basic_variables(new_name);
        if (scope->get_symbol(new_name)) {
            ASR::symbol_t *s = scope->get_symbol(new_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var));
        }
        fill_func_arg("n", arg_types[0]);
        fill_func_arg("x", arg_types[1]);
        auto result = declare(new_name, return_type, ReturnVar);
        {
            ASR::symbol_t *s = b.create_c_func(c_func_name, fn_symtab, return_type, 2, arg_types);
            fn_symtab->add_symbol(c_func_name, s);
            dep.push_back(al, s2c(al, c_func_name));
            body.push_back(al, b.Assignment(result, b.Call(s, args, return_type)));
        }

        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.Call(new_symbol, new_args, return_type);
    }

} // namespace BesselJN

namespace BesselYN {

    static ASR::expr_t *eval_BesselYN(Allocator& al, const Location& loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*>& args, diag::Diagnostics& /*diag*/) {
        return make_ConstantWithType(make_RealConstant_t, yn(ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n, ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r), t1, loc);
    }

    static inline ASR::expr_t* instantiate_BesselYN(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        std::string c_func_name;
        if (ASRUtils::extract_kind_from_ttype_t(arg_types[1]) == 4) {
            c_func_name = "_lfortran_sbesselyn";
        } else {
            c_func_name = "_lfortran_dbesselyn";
        }
        std::string new_name = "_lcompilers_bessel_yn_"+ type_to_str_python(arg_types[1]);

        declare_basic_variables(new_name);
        if (scope->get_symbol(new_name)) {
            ASR::symbol_t *s = scope->get_symbol(new_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var));
        }
        fill_func_arg("n", arg_types[0]);
        fill_func_arg("x", arg_types[1]);
        auto result = declare(new_name, return_type, ReturnVar);
        {
            ASR::symbol_t *s = b.create_c_func(c_func_name, fn_symtab, return_type, 2, arg_types);
            fn_symtab->add_symbol(c_func_name, s);
            dep.push_back(al, s2c(al, c_func_name));
            body.push_back(al, b.Assignment(result, b.Call(s, args, return_type)));
        }

        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.Call(new_symbol, new_args, return_type);
    }

} // namespace BesselYN

namespace Poppar {

    static ASR::expr_t *eval_Poppar(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        ASR::expr_t* count = Popcnt::eval_Popcnt(al, loc, t1, args, diag);
        int result = ASR::down_cast<ASR::IntegerConstant_t>(count)->m_n;
        result = result % 2 == 0 ? 0 : 1;
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Poppar(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
            declare_basic_variables("_lcompilers_poppar_" + type_to_str_python(arg_types[0]));
        fill_func_arg("i", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        function poppar(n) result(result)
            integer, intent(in) :: n
            integer :: result
            integer :: count
            count = popcnt(n)
            result = mod(count, 2)
        end function
        */
        ASR::expr_t *func_call_poppar =Popcnt::POPCNT(b, args[0], return_type, scope);
        body.push_back(al, b.Assignment(result, Mod::MOD(b, func_call_poppar, b.i_t(2, return_type), scope)));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Poppar

namespace Real {

    static ASR::expr_t *eval_Real(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        if (ASR::is_a<ASR::IntegerConstant_t>(*args[0])) {
            double i = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(args[0]))->m_n;
            return make_ConstantWithType(make_RealConstant_t, i, t1, loc);
        } else if (ASR::is_a<ASR::RealConstant_t>(*args[0])) {
            ASR::RealConstant_t *r = ASR::down_cast<ASR::RealConstant_t>(ASRUtils::expr_value(args[0]));
            return make_ConstantWithType(make_RealConstant_t, r->m_r, t1, loc);
        } else if (ASR::is_a<ASR::ComplexConstant_t>(*args[0])) {
            ASR::ComplexConstant_t *c = ASR::down_cast<ASR::ComplexConstant_t>(ASRUtils::expr_value(args[0]));
            return make_ConstantWithType(make_RealConstant_t, c->m_re, t1, loc);
        } else {
            append_error(diag, "Invalid argument to `real` intrinsic", loc);
            return nullptr;
        }
    }

    static inline ASR::expr_t* instantiate_Real(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_real_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        function real(a) result(result)
            real :: a
            real :: result
            result = a
        end function
        */
        if (is_integer(*arg_types[0])) {
            body.push_back(al, b.Assignment(result, b.i2r_t(args[0], return_type)));
        } else if (is_real(*arg_types[0])) {
            body.push_back(al, b.Assignment(result, b.r2r_t(args[0], return_type)));
        } else if (is_complex(*arg_types[0])) {
            body.push_back(al, b.Assignment(result, EXPR(ASR::make_ComplexRe_t(al, loc,
            args[0], return_type, nullptr))));
        }
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Real

namespace Mvbits {

    static ASR::expr_t *eval_Mvbits(Allocator &/*al*/, const Location &/*loc*/,
            ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& /*diag*/) {
        return nullptr;
    }

    static inline ASR::expr_t* instantiate_Mvbits(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        std::string c_func_name;
        if (ASRUtils::extract_kind_from_ttype_t(arg_types[0]) == 4) {
            c_func_name = "_lfortran_mvbits32";
        } else {
            c_func_name = "_lfortran_mvbits64";
        }
        std::string new_name = "_lcompilers_mvbits_" + type_to_str_python(arg_types[0]);
        declare_basic_variables(new_name);
        fill_func_arg("from", arg_types[0]);
        fill_func_arg("frompos", arg_types[1]);
        fill_func_arg("len", arg_types[2]);
        fill_func_arg("to", arg_types[3]);
        fill_func_arg("topos", arg_types[4]);
        auto result = declare(new_name, ASRUtils::extract_type(return_type), ReturnVar);
        {
            SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);
            Vec<ASR::expr_t*> args_1;
            {
                args_1.reserve(al, 5);
                ASR::expr_t *arg = b.Variable(fn_symtab_1, "from", arg_types[0],
                    ASR::intentType::In, ASR::abiType::BindC, true);
                args_1.push_back(al, arg);
                arg = b.Variable(fn_symtab_1, "frompos", arg_types[1],
                    ASR::intentType::In, ASR::abiType::BindC, true);
                args_1.push_back(al, arg);
                arg = b.Variable(fn_symtab_1, "len", arg_types[2],
                    ASR::intentType::In, ASR::abiType::BindC, true);
                args_1.push_back(al, arg);
                arg = b.Variable(fn_symtab_1, "to", arg_types[3],
                    ASR::intentType::In, ASR::abiType::BindC, true);
                args_1.push_back(al, arg);
                arg = b.Variable(fn_symtab_1, "topos", arg_types[4],
                    ASR::intentType::In, ASR::abiType::BindC, true);
                args_1.push_back(al, arg);
            }

            ASR::expr_t *return_var_1 = b.Variable(fn_symtab_1, c_func_name,
                return_type, ASRUtils::intent_return_var, ASR::abiType::BindC, false);

            SetChar dep_1; dep_1.reserve(al, 1);
            Vec<ASR::stmt_t*> body_1; body_1.reserve(al, 1);
            ASR::symbol_t *s = make_ASR_Function_t(c_func_name, fn_symtab_1, dep_1, args_1,
                body_1, return_var_1, ASR::abiType::BindC, ASR::deftypeType::Interface, s2c(al, c_func_name));
            fn_symtab->add_symbol(c_func_name, s);
            dep.push_back(al, s2c(al, c_func_name));
            body.push_back(al, b.Assignment(result, b.Call(s, args, return_type)));
        }
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

    static inline ASR::expr_t* MVBITS(ASRBuilder &b, ASR::expr_t* from, ASR::expr_t* frompos,
            ASR::expr_t* len, ASR::expr_t* to, ASR::expr_t* topos, SymbolTable *scope) {
        return b.CallIntrinsic( scope, {ASRUtils::expr_type(from), ASRUtils::expr_type(frompos),
            ASRUtils::expr_type(len), ASRUtils::expr_type(to), ASRUtils::expr_type(topos)},
            {from, frompos, len, to, topos}, ASRUtils::expr_type(to), 0, Mvbits::instantiate_Mvbits);
    }

} // namespace Mvbits

namespace MoveAlloc {

    static ASR::expr_t *eval_MoveAlloc(Allocator &/*al*/, const Location &/*loc*/,
            ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& /*diag*/) {
        return nullptr;
    }

    static inline ASR::expr_t* instantiate_MoveAlloc(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {

        std::string new_name = "_lcompilers_move_alloc_" + type_to_str_python(arg_types[0]);
        declare_basic_variables(new_name);
        fill_func_arg("from", arg_types[0]);
        fill_func_arg("to", arg_types[1]);
        auto result = declare(new_name, arg_types[0], ReturnVar);
        body.push_back(al, b.Assignment(result, args[0]));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace MoveAlloc

namespace Mergebits {

    static int compute_merge_bits(int a, int b, int mask, int total_bits) {
        int result = 0;
        int k = 0;
        while (k < total_bits) {
            if (mask & (1 << k)) {
                result |= (a & (1 << k));
            } else {
                result |= (b & (1 << k));
            }
            k++;
        }
        return result;
    }

    static ASR::expr_t *eval_Mergebits(Allocator &al, const Location &loc,
        ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int kind = ASRUtils::extract_kind_from_ttype_t(ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_type);
        int a = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int b = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int mask = ASR::down_cast<ASR::IntegerConstant_t>(args[2])->m_n;
        int result = 0;
        result = compute_merge_bits(a, b, mask, kind * 8);
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Mergebits(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_mergebits_" + type_to_str_python(arg_types[0]));
        fill_func_arg("a", arg_types[0]);
        fill_func_arg("b", arg_types[1]);
        fill_func_arg("mask", arg_types[2]);
        auto result = declare(fn_name, return_type, ReturnVar);
        auto itr = declare("i", arg_types[0], Local);
        auto mask = declare("m", arg_types[0], Local);
        auto numberofbits = declare("n", arg_types[0], Local);

        if(ASRUtils::extract_kind_from_ttype_t(arg_types[0]) != ASRUtils::extract_kind_from_ttype_t(arg_types[1])){
            throw LCompilersException("The second argument of 'merge_bits' intrinsic must be the same type and kind as first argument");
        }
        if(ASRUtils::extract_kind_from_ttype_t(arg_types[0]) != ASRUtils::extract_kind_from_ttype_t(arg_types[2])){
            throw LCompilersException("The third argument of 'merge_bits' intrinsic must be the same type and kind as first argument");
        }

        body.push_back(al, b.Assignment(result, b.i_t(0, arg_types[0])));
        body.push_back(al, b.Assignment(itr, b.i_t(0, arg_types[0])));
        body.push_back(al, b.Assignment(mask, args[2]));
        body.push_back(al, b.Assignment(numberofbits, b.Mul(b.i_t(8, arg_types[0]),
                           b.i_t(ASRUtils::extract_kind_from_ttype_t(arg_types[0]), arg_types[0]))));
        body.push_back(al, b.While(b.Lt(itr, numberofbits), {
            b.If(b.NotEq(b.i_t(0, arg_types[0]), b.And(mask, b.BitLshift(b.i_t(1, arg_types[0]), itr, arg_types[0]))), {
                b.Assignment(result, b.Or(result, b.And(args[0], b.BitLshift(b.i_t(1, arg_types[0]), itr, arg_types[0]))))
            }, {
                b.Assignment(result, b.Or(result, b.And(args[1], b.BitLshift(b.i_t(1, arg_types[0]), itr, arg_types[0]))))
            }),
            b.Assignment(itr, b.Add(itr, b.i_t(1, arg_types[0]))),
        }));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Mergebits

namespace Leadz {

    static ASR::expr_t *eval_Leadz(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t a = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t kind = ASRUtils::extract_kind_from_ttype_t(t1);
        int64_t leading_zeros = ASRUtils::compute_leading_zeros(a, kind);
        set_kind_to_ttype_t(t1, 4);
        return make_ConstantWithType(make_IntegerConstant_t, leading_zeros, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Leadz(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_leadz_" + type_to_str_python(arg_types[0]));
        fill_func_arg("n", arg_types[0]);
        auto result = declare(fn_name, arg_types[0], ReturnVar);
        auto total_bits = declare("r", arg_types[0], Local);
        auto number = declare("num", arg_types[0], Local);
        /*
        function leadz(n) result(result)
            integer :: n, k, total_bits
            integer :: result
            k = kind(n)
            total_bits = 32
            if (k == 8) total_bits = 64
            if (n<0) then
                result = 0
            else
                do while (total_bits > 0)
                    if (mod(n,2) == 0) then
                        result = result + 1
                    else
                        result = 0
                    end if
                    n = n/2
                    total_bits = total_bits - 1
                end do
            end if
        end function
        */
        body.push_back(al, b.Assignment(result, b.i_t(0, arg_types[0])));
        body.push_back(al, b.Assignment(number, args[0]));
        body.push_back(al, b.Assignment(total_bits, b.i_t(8*ASRUtils::extract_kind_from_ttype_t(arg_types[0]), arg_types[0])));
        body.push_back(al, b.If(b.Lt(number, b.i_t(0, arg_types[0])), {
            b.Assignment(result, b.i_t(0, arg_types[0]))
        }, {
            b.While(b.Gt(total_bits, b.i_t(0, arg_types[0])), {
                b.If(b.Eq(b.CallIntrinsic(scope, {arg_types[0], arg_types[0]},
                    {number, b.i_t(2, arg_types[0])}, return_type, 0, Mod::instantiate_Mod), b.i_t(0, arg_types[0])), {
                    b.Assignment(result, b.Add(result, b.i_t(1, arg_types[0])))
                }, {
                    b.Assignment(result, b.i_t(0, arg_types[0]))
                }),
                b.Assignment(number, b.Div(number, b.i_t(2, arg_types[0]))),
                b.Assignment(total_bits, b.Sub(total_bits, b.i_t(1, arg_types[0]))),
            }),
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Leadz

namespace Ishftc {

    static uint64_t cutoff_extra_bits(uint64_t num, uint32_t bits_size, uint32_t max_bits_size) {
        if (bits_size == max_bits_size) {
            return num;
        }
        return (num & ((1lu << bits_size) - 1lu));
    }

    static ASR::expr_t *eval_Ishftc(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        uint64_t val = (uint64_t)ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t shift_signed = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int kind = ASRUtils::extract_kind_from_ttype_t(ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_type);
        uint32_t bits_size = (uint32_t)ASR::down_cast<ASR::IntegerConstant_t>(args[2])->m_n;
        uint32_t max_bits_size = 64;
        if (bits_size > (uint32_t)(8 * kind)) {
            append_error(diag, "The SIZE argument must be greater than zero and less than or equal to BIT_SIZE('I')", loc);
            return nullptr;
        }
        if(std::abs(shift_signed) > bits_size){
            append_error(diag, "The SHIFT argument must be less than or equal to the of SIZE argument", loc);
            return nullptr;
        }
        bool negative_shift = (shift_signed < 0);
        uint32_t shift = abs(shift_signed);

        if (shift > max_bits_size) {
            append_error(diag, "The absolute value of SHIFT argument must be less than SIZE", loc);
            return nullptr;
        }

        val = cutoff_extra_bits(val, bits_size, max_bits_size);
        uint64_t result;
        if (negative_shift) {
            result = (val >> shift) | cutoff_extra_bits(val << (bits_size - shift), bits_size, max_bits_size);
        } else {
            result = cutoff_extra_bits(val << shift, bits_size, max_bits_size) | ((val >> (bits_size - shift)));
        }
        if (kind == 1) {
            result = static_cast<int8_t>(result);
        } else if (kind == 2) {
            result = static_cast<int16_t>(result);
        } else if (kind == 4) {
            result = static_cast<int32_t>(result);
        } 
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Ishftc(Allocator & al, const Location & loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        std::string c_func_name;
        if(ASRUtils::extract_kind_from_ttype_t(arg_types[1]) == 4){
            c_func_name = "_lfortran_sishftc";
        } else {
            c_func_name = "_lfortran_dishftc";
        }
        std::string new_name = "_lcompilers_ishftc_"+ type_to_str_python(arg_types[1]);

        declare_basic_variables(new_name);
        if (scope->get_symbol(new_name)) {
            ASR::symbol_t *s = scope->get_symbol(new_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var));
        }
        fill_func_arg("n", arg_types[0]);
        fill_func_arg("x", arg_types[1]);
        fill_func_arg("size", arg_types[2]);
        auto result = declare(new_name, return_type, ReturnVar);
        {
            ASR::symbol_t *s = b.create_c_func(c_func_name, fn_symtab, return_type, 3, arg_types);
            fn_symtab->add_symbol(c_func_name, s);
            dep.push_back(al, s2c(al, c_func_name));
            body.push_back(al, b.Assignment(result, b.Call(s, args, return_type)));
        }

        ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.Call(new_symbol, new_args, return_type);
    }

} // namespace Ishftc

namespace Hypot {

    static ASR::expr_t *eval_Hypot(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int kind = ASRUtils::extract_kind_from_ttype_t(t1);
        if (kind == 4) {
            float a = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            float b = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
            return make_ConstantWithType(make_RealConstant_t, std::hypot(a, b), t1, loc);
        } else {
            double a = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            double b = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
            return make_ConstantWithType(make_RealConstant_t, std::hypot(a, b), t1, loc);
        }
    }

    static inline ASR::expr_t* instantiate_Hypot(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_optimization_hypot_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, arg_types[0], ReturnVar);
        /*
            real function hypot_(x,y) result(hypot)
            real :: x,y
            hypot = sqrt(x*x + y*y)
            end function
        */
        body.push_back(al, b.Assignment(result, b.CallIntrinsic(scope, {
            ASRUtils::expr_type(b.Add(b.Mul(args[0], args[0]), b.Mul(args[1], args[1])))
        }, {
            b.Add(b.Mul(args[0], args[0]), b.Mul(args[1], args[1]))
        }, return_type, 0, Sqrt::instantiate_Sqrt)));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Hypot

namespace ToLowerCase {

    static ASR::expr_t *eval_ToLowerCase(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {

        char* str = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        std::transform(str, str + std::strlen(str), str, [](unsigned char c) { return std::tolower(c); });
        return make_ConstantWithType(make_StringConstant_t, str, t1, loc);
    }

    static inline ASR::expr_t* instantiate_ToLowerCase(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("");
        fill_func_arg("s", arg_types[0]);
        ASR::ttype_t* char_type = ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, 0, nullptr, ASR::string_physical_typeType::PointerString));
        auto result = declare(fn_name, char_type, ReturnVar);
        auto itr = declare("i", int32, Local);

        /*
        function toLowerCase(str) result(result)
            character(len=5) :: str
            character(len=len(str)) :: result
            integer :: i, ln
            i = 1
            ln = len(str)
            result = str
            do while (i < ln)
                if (result(i:i) >= 'A' .and. result(i:i) <= 'Z') then
                    result(i:i) = char(ichar(result(i:i)) + ichar('a') - ichar('A'))
                end if
                i = i + 1
            end do
            print*, result
        end function
        */

        body.push_back(al, b.Assignment(itr, b.i32(1)));
        body.push_back(al, b.While(b.LtE(itr, b.StringLen(args[0])), {
            b.If(b.And(b.GtE(ASRUtils::EXPR(ASR::make_Ichar_t(al, loc, ASRUtils::EXPR(ASR::make_StringItem_t(al, loc, args[0], itr, char_type, nullptr)), int32, nullptr)), b.Ichar("A", arg_types[0], int32)),
                b.LtE(ASRUtils::EXPR(ASR::make_Ichar_t(al, loc, ASRUtils::EXPR(ASR::make_StringItem_t(al, loc, args[0], itr, char_type, nullptr)), int32, nullptr)), b.Ichar("Z", arg_types[0], int32))), {
                b.Assignment(result, b.StringConcat(result, ASRUtils::EXPR(ASR::make_StringChr_t(al, loc,
            b.Sub(b.Add(ASRUtils::EXPR(ASR::make_Ichar_t(al, loc, ASRUtils::EXPR(ASR::make_StringItem_t(al, loc, args[0], itr, char_type, nullptr)), int32, nullptr)), b.Ichar("a", arg_types[0], int32)),
            b.Ichar("A", arg_types[0], int32)), return_type, nullptr)), char_type))
            }, {
                b.Assignment(result, b.StringConcat(result, ASRUtils::EXPR(ASR::make_StringItem_t(al, loc, args[0], itr, char_type, nullptr)), char_type))
            }),
            b.Assignment(itr, b.Add(itr, b.i32(1))),
        }));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace ToLowerCase

namespace SelectedIntKind {

    static ASR::expr_t *eval_SelectedIntKind(Allocator &al, const Location &loc,
            ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t val = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        ASRUtils::ASRBuilder b(al, loc);
        int64_t result;
        if (val <= 2) {
            result = 1;
        } else if (val <= 4) {
            result = 2;
        } else if (val <= 9) {
            result = 4;
        } else {
            result = 8;
        }
        return b.i32(result);
    }

    static inline ASR::expr_t* instantiate_SelectedIntKind(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("");
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, int32, ReturnVar);
        auto number = declare("num", arg_types[0], Local);
        body.push_back(al, b.Assignment(number, args[0]));
        body.push_back(al, b.If(b.LtE(number, b.i_t(2, arg_types[0])), {
            b.Assignment(result, b.i32(1))
        }, {
            b.If(b.LtE(number, b.i_t(4, arg_types[0])), {
                b.Assignment(result, b.i32(2))
            }, {
                b.If(b.LtE(number, b.i_t(9, arg_types[0])), {
                    b.Assignment(result, b.i32(4))
                }, {
                    b.Assignment(result, b.i32(8))
                })
            })
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace SelectedIntKind

namespace SelectedRealKind {

    static inline ASR::expr_t *eval_SelectedRealKind(Allocator &al, const Location &loc,
            ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t kind;
        ASRUtils::ASRBuilder b(al, loc);
        int64_t p = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t r = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        int64_t radix = ASR::down_cast<ASR::IntegerConstant_t>(args[2])->m_n;

        if (p < 7 && r < 38 && radix == 2) {
            kind = 4;
        } else if (p < 16 && r < 308 && radix == 2) {
            kind = 8;
        } else if (radix != 2) {
            kind = -5;
        } else {
            kind = -1;
        }
        return b.i32(kind);
    }

    static inline ASR::expr_t* instantiate_SelectedRealKind(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("");
        fill_func_arg("x", arg_types[0]);
        fill_func_arg("y", arg_types[1]);
        fill_func_arg("z", arg_types[2]);
        auto result = declare(fn_name, int32, ReturnVar);
        auto p = declare("p", arg_types[0], Local);
        auto r = declare("r", arg_types[1], Local);
        auto radix = declare("radix", arg_types[2], Local);

        body.push_back(al, b.Assignment(p, args[0]));
        body.push_back(al, b.Assignment(r, args[1]));
        body.push_back(al, b.Assignment(radix, args[2]));
        body.push_back(al, b.If(b.And(b.And(b.Lt(p, b.i_t(7, arg_types[0])), b.Lt(r, b.i_t(38, arg_types[1]))), b.Eq(radix, b.i_t(2, arg_types[2]))), {
            b.Assignment(result, b.i32(4))
        }, {
            b.If( b.And(b.And(b.Lt(p, b.i_t(15, arg_types[0])), b.Lt(r, b.i_t(308, arg_types[1]))), b.Eq(radix, b.i_t(2, arg_types[2]))), {
                b.Assignment(result, b.i32(8))
            }, {
                b.If(b.NotEq(radix, b.i_t(2, arg_types[2])), {
                    b.Assignment(result, b.i32(-5))
                }, {
                    b.Assignment(result, b.i32(-1))
                })
            })
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace SelectedRealKind

namespace SelectedCharKind {

    static inline ASR::expr_t *eval_SelectedCharKind(Allocator &al, const Location &loc,
            ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t kind;
        ASRUtils::ASRBuilder b(al, loc);
        char* name = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        std::string lowercase_name = to_lower(name);
        if (lowercase_name == "ascii" || lowercase_name == "default") {
            kind = 1;
        } else if (lowercase_name == "iso_10646") {
            kind = 4;
        } else {
            kind = -1;
        }
        return b.i32(kind);
    }

    static inline ASR::expr_t* instantiate_SelectedCharKind(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_selected_char_kind_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);

        ASR::expr_t* func_call_lowercase = b.CallIntrinsic(scope, {arg_types[0]},
                                    {args[0]}, arg_types[0], 0, ToLowerCase::instantiate_ToLowerCase);
        body.push_back(al, b.If(b.Or(b.Eq(func_call_lowercase, b.StringConstant("ascii", arg_types[0])),
            b.Eq(func_call_lowercase, b.StringConstant("default", arg_types[0]))), {
            b.Assignment(result, b.i_t(1, return_type))
        }, {
            b.If(b.Eq(func_call_lowercase, b.StringConstant("iso_10646", arg_types[0])), {
                b.Assignment(result, b.i_t(4, return_type))
            }, {
                b.Assignment(result, b.i_t(-1, return_type))
            })
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace SelectedCharKind

namespace Kind {

    static ASR::expr_t *eval_Kind(Allocator &al, const Location &loc,
            ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int result = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(args[0]));
        return make_ConstantWithType(make_IntegerConstant_t, result, int32, loc);
    }

} // namespace Kind

namespace Rank {

    static ASR::expr_t *eval_Rank(Allocator &al, const Location &loc,
            ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        ASRUtils::ASRBuilder b(al, loc);
        return b.i_t(extract_n_dims_from_ttype(expr_type(args[0])), int32);
    }

} // namespace Rank

namespace BitSize {

    static ASR::expr_t *eval_BitSize(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(args[0]));
        return make_ConstantWithType(make_IntegerConstant_t, 8*kind, t1, loc);
    }

} // namespace BitSize

namespace NewLine {

    static ASR::expr_t *eval_NewLine(Allocator &al, const Location &loc,
            ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& /*diag*/) {
        char* new_line_str = (char*)"\n";
        return make_ConstantWithType(make_StringConstant_t, new_line_str, ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, 1, nullptr, ASR::string_physical_typeType::PointerString)), loc);
    }

} // namespace NewLine

namespace Adjustl {

    static ASR::expr_t *eval_Adjustl(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        char* str = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        size_t len = std::strlen(str);
        size_t first_non_space = 0;
        while (first_non_space < len && std::isspace(str[first_non_space])) {
            first_non_space++;
        }
        std::string res(len, ' ');
        char* result = s2c(al, res);
        std::strncpy(result, str + first_non_space, len - first_non_space);
        return make_ConstantWithType(make_StringConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Adjustl(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_adjustl_" + type_to_str_python(arg_types[0]));
        fill_func_arg("str", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        return_type = TYPE(ASR::make_String_t(al, loc, 1, -3, EXPR(ASR::make_StringLen_t(al, loc, args[0], int32, nullptr)), ASR::string_physical_typeType::PointerString));
        auto result = declare("result", return_type, ReturnVar);
        auto itr = declare("i", int32, Local);
        auto tmp = declare("tmp", int32, Local);

        /*
            function adjustl_(s) result(r)
                character(len=*), intent(in) :: s
                character(len=len(s)) :: r
                integer :: i, tmp
                i = 1
                do while (i <= len(s))
                    if (isspace(s(i:i))) then
                        i = i + 1
                    else
                        exit
                    end if
                end do
                if i <= len(s) then
                    tmp = len(s) - i + 1
                    r(1:tmp) = s(i:len(s))
                end if
            end function
        */

        body.push_back(al, b.Assignment(itr, b.i32(1)));
        body.push_back(al, b.While(b.LtE(itr, b.StringLen(args[0])), {
            b.If(b.Eq(ASRUtils::EXPR(ASR::make_Ichar_t(al, loc,
                ASRUtils::EXPR(ASR::make_StringItem_t(al, loc, args[0], itr,
                ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)), nullptr)), int32, nullptr)),
                b.Ichar(" ", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, 1, nullptr, ASR::string_physical_typeType::PointerString)), int32)), {
                b.Assignment(itr, b.Add(itr, b.i32(1)))
            }, {
                b.Exit()
            }),
        }));

        body.push_back(al, b.If(b.LtE(itr, b.StringLen(args[0])), {
            b.Assignment(tmp, b.Add(b.Sub(b.StringLen(args[0]), itr), b.i32(1))),
            b.Assignment(b.StringSection(result, b.i32(0), tmp), b.StringSection(args[0], b.Sub(itr, b.i32(1)), b.StringLen(args[0])))
        }, {}));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return_type = TYPE(ASR::make_String_t(al, loc, 1, -3, EXPR(ASR::make_StringLen_t(al, loc, new_args[0].m_value, int32, nullptr)), ASR::string_physical_typeType::PointerString));
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace AdjustL

namespace Adjustr {

    static ASR::expr_t *eval_Adjustr(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        char* str = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        size_t len = std::strlen(str);
        int last_non_space = len - 1;
        while (last_non_space >= 0 && std::isspace(str[last_non_space])) {
            last_non_space--;
        }
        std::string res(len, ' ');
        char* result = s2c(al, res);
        if (last_non_space != -1) {
            int tmp = len - 1 - last_non_space;
            for (int i = 0; i <= last_non_space; i++) {
                result[i + tmp] = str[i];
            }
        }
        return make_ConstantWithType(make_StringConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Adjustr(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_adjustr_" + type_to_str_python(arg_types[0]));
        fill_func_arg("str", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        return_type = TYPE(ASR::make_String_t(al, loc, 1, -3, EXPR(ASR::make_StringLen_t(al, loc, args[0], int32, nullptr)), ASR::string_physical_typeType::PointerString));
        auto result = declare("result", return_type, ReturnVar);
        auto itr = declare("i", int32, Local);
        auto tmp = declare("tmp", int32, Local);

        /*
            function adjustr_(s) result(r)
                character(len=*), intent(in) :: s
                character(len=len(s)) :: r
                integer :: i, tmp
                i = len(s)
                do while (i >= 1)
                    if isspace(s(i:i)) then
                        i = i - 1
                    else
                        exit
                    end if
                end do
                if i /= 0 then
                    tmp = len(s) - i + 1
                    r(tmp:len(s)) = s(1:i)
                end if
            end function
        */

        body.push_back(al, b.Assignment(itr, b.StringLen(args[0])));
        body.push_back(al, b.While(b.GtE(itr, b.i32(1)), {
            b.If(b.Eq(ASRUtils::EXPR(ASR::make_Ichar_t(al, loc,
                ASRUtils::EXPR(ASR::make_StringItem_t(al, loc, args[0], itr,
                ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)), nullptr)), int32, nullptr)),
                b.Ichar(" ", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, 1, nullptr, ASR::string_physical_typeType::PointerString)), int32)), {
                b.Assignment(itr, b.Sub(itr, b.i32(1)))
            }, {
                b.Exit()
            }),
        }));

        body.push_back(al, b.If(b.NotEq(itr, b.i32(0)), {
            b.Assignment(tmp, b.Add(b.Sub(b.StringLen(args[0]), itr), b.i32(1))),
            b.Assignment(b.StringSection(result, b.Sub(tmp, b.i32(1)), b.StringLen(args[0])),
                b.StringSection(args[0], b.i32(0), itr))
        }, {}));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return_type = TYPE(ASR::make_String_t(al, loc, 1, -3, EXPR(ASR::make_StringLen_t(al, loc, new_args[0].m_value, int32, nullptr)), ASR::string_physical_typeType::PointerString));
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Adjustr

namespace StringLenTrim {

    static ASR::expr_t *eval_StringLenTrim(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        char* str = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        size_t len = std::strlen(str);
        for (int i = len - 1; i >= 0; i--) {
            if (!std::isspace(str[i])) {
                return make_ConstantWithType(make_IntegerConstant_t, i + 1, t1, loc);
            }
        }
        return make_ConstantWithType(make_IntegerConstant_t, 0, t1, loc);
    }

    static inline ASR::expr_t* instantiate_StringLenTrim(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_len_trim_" + type_to_str_python(arg_types[0]));
        fill_func_arg("str", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        auto result = declare("result", return_type, ReturnVar);

        /*
            function len_trim(string) result(r)
                character(len=*), intent(in) :: string
                r = len(string)
                if (r/= 0) then
                    do while(string(r:r) == " ")
                        r = r - 1
                        if (r == 0) exit
                    end do
                end if
            end function
        */

        body.push_back(al, b.Assignment(result, b.StringLen(args[0])));
        body.push_back(al, b.If(b.NotEq(result, b.i32(0)), {
            b.While(b.Eq(b.StringItem(args[0], result), b.StringConstant(" ", arg_types[0])), {
                b.Assignment(result, b.Sub(result, b.i32(1))),
                b.If(b.Eq(result, b.i32(0)), {
                    b.Exit()
                }, {})
            })
        }, {}));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }
    static inline ASR::expr_t* StringLenTrim(ASRBuilder &b, ASR::expr_t* a, ASR::ttype_t *return_type, SymbolTable* scope) {
        return b.CallIntrinsic(scope, {expr_type(a)}, {a}, return_type, 0, StringLenTrim::instantiate_StringLenTrim);
    }

} // namespace StringLenTrim

namespace StringTrim {

    static ASR::expr_t *eval_StringTrim(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        char* str = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        size_t len = strlen(str);
        if (len > 0) {
            char* endptr = str + len - 1;
            while (endptr >= str && std::isspace(*endptr)) {
                *endptr = '\0';
                --endptr;
            }
        }
        return make_ConstantWithType(make_StringConstant_t, str, t1, loc);
    }

    static inline ASR::expr_t* instantiate_StringTrim(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_trim_" + type_to_str_python(arg_types[0]));
        fill_func_arg("str", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        ASR::expr_t* func_call_lentrim = StringLenTrim::StringLenTrim(b, args[0], int32, scope);
        return_type = TYPE(ASR::make_String_t(al, loc, 1, -3, func_call_lentrim, ASR::string_physical_typeType::PointerString));
        auto result = declare("result", return_type, ReturnVar);

        /*
            function trim(string) result(r)
                character(len=*), intent(in) :: string
                l = len_trim(string)
                r = x(1:l)
            end function
        */

        body.push_back(al, b.Assignment(result, b.StringSection(args[0], b.i32(0), func_call_lentrim)));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        return_type = TYPE(ASR::make_String_t(al, loc, 1, -3, EXPR(ASR::make_StringLen_t(al, loc, new_args[0].m_value, int32, nullptr)), ASR::string_physical_typeType::PointerString));
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace StringTrim

namespace Ichar {

    static ASR::expr_t *eval_Ichar(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        char* str = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        int64_t len = std::strlen(str);
        if (len != 1) {
            append_error(diag, "Argument to Ichar must have length one", loc);
            return nullptr;
        }
        char first_char = str[0];
        int result = (int)first_char;
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Ichar(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_ichar_" + type_to_str_python(arg_types[0]));
        fill_func_arg("str", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        auto result = declare("result", return_type, ReturnVar);
        auto itr = declare("i", int32, Local);
        body.push_back(al, b.Assignment(itr, b.i32(1)));
        body.push_back(al, b.Assignment(result, b.i2i_t(
            ASRUtils::EXPR(ASR::make_Ichar_t(al, loc, ASRUtils::EXPR(ASR::make_StringItem_t(al, loc, args[0], itr,
            ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)), nullptr)), int32, nullptr)), return_type)));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Ichar

namespace Char {

    static ASR::expr_t *eval_Char(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t i = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        char str = i;
        std::string svalue;
        svalue += str;
        Str s;
        s.from_str_view(svalue);
        char *result = s.c_str(al);
        return make_ConstantWithType(make_StringConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Char(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_char_" + type_to_str_python(arg_types[0]));
        fill_func_arg("i", arg_types[0]);
        auto result = declare("result", return_type, ReturnVar);

        body.push_back(al, b.Assignment(result, ASRUtils::EXPR(ASR::make_StringChr_t(al, loc, b.i2i_t(args[0], int32), return_type, nullptr))));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Char

namespace Achar {

    static ASR::expr_t *eval_Achar(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int64_t i = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        std::string svalue(1, static_cast<char>(i));
        return make_ConstantWithType(make_StringConstant_t, s2c(al, svalue), t1, loc);
    }

    static inline ASR::expr_t* instantiate_Achar(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_achar_" + type_to_str_python(arg_types[0]));
        fill_func_arg("i", arg_types[0]);
        auto result = declare("result", return_type, ReturnVar);
        body.push_back(al, b.Assignment(result, ASRUtils::EXPR(ASR::make_StringChr_t(al, loc, args[0], return_type, nullptr))));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Achar

namespace Digits {

    static ASR::expr_t *eval_Digits(Allocator &al, const Location &loc,
            ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        ASR::ttype_t *type1 = ASRUtils::expr_type(args[0]);
        int kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(args[0]));
        if (is_integer(*type1)) {
            if (kind == 1) {
                return make_ConstantWithType(make_IntegerConstant_t, 7, int32, loc);
            } else if (kind == 2) {
                return make_ConstantWithType(make_IntegerConstant_t, 15, int32, loc);
            } else if (kind == 4) {
                return make_ConstantWithType(make_IntegerConstant_t, 31, int32, loc);
            } else if (kind == 8) {
                return make_ConstantWithType(make_IntegerConstant_t, 63, int32, loc);
            } else {
                append_error(diag, "Kind "+ std::to_string(kind) + " not supported for type Integer", loc);
            }
        } else if (is_real(*type1)) {
            if (kind == 4) {
                return make_ConstantWithType(make_IntegerConstant_t, 24, int32, loc);
            } else if (kind == 8) {
                return make_ConstantWithType(make_IntegerConstant_t, 53, int32, loc);
            } else {
                append_error(diag, "Kind "+ std::to_string(kind) + " not supported for type Real", loc);
            }
        } else {
            append_error(diag, "Argument to `digits` intrinsic must be real or integer", loc);
        }
        return nullptr;
    }

    static inline ASR::expr_t* instantiate_Digits(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_optimization_digits_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, int32, ReturnVar);
        int kind = ASRUtils::extract_kind_from_ttype_t(arg_types[0]);
        if (is_integer(*arg_types[0])) {
            if (kind == 4) {
                body.push_back(al, b.Assignment(result, b.i32(31)));
            } else if (kind == 8) {
                body.push_back(al, b.Assignment(result, b.i32(63)));
            }
        } else if (is_real(*arg_types[0])) {
            if (kind == 4) {
                body.push_back(al, b.Assignment(result, b.i32(24)));
            } else if (kind == 8) {
                body.push_back(al, b.Assignment(result, b.i32(53)));
            }
        }
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Digits

namespace Rrspacing {

    static ASR::expr_t *eval_Rrspacing(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(args[0]));
        double digits = 0.0;
        double fraction = 0.0;
        digits = (kind == 4) ? 24.00 : 53.00;
        if (kind == 4) {
            float x = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            int32_t exponent;
            if (x == 0.0) {
                exponent = 0;
                fraction =  x * std::pow((2), (-1*(exponent)));
            }
            else{
                int32_t ix;
                std::memcpy(&ix, &x, sizeof(ix));
                exponent = ((ix >> 23) & 0xff) - 126;
                fraction =  x * std::pow((2), (-1*(exponent)));
            }
        }
        else if (kind == 8) {
            double x = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            int64_t exponent;
            if (x == 0.0) {
                exponent = 0;
                fraction =  x * std::pow((2), (-1*(exponent)));
            }
            else{
                int64_t ix;
                std::memcpy(&ix, &x, sizeof(ix));
                exponent = ((ix >> 52) & 0x7ff) - 1022;
                fraction =  x * std::pow((2), (-1*(exponent)));
            }
        }
        fraction = std::abs(fraction);
        double radix = 2.00;
        double result = fraction * std::pow(radix, digits);
        return make_ConstantWithType(make_RealConstant_t, result, arg_type, loc);

    }

    static inline ASR::expr_t* instantiate_Rrspacing(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_optimization_rrspacing_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = rrspacing(X)
        * r = abs(fraction(X)) * (radix(X)**digits(X))
        */
        body.push_back(al, b.Assignment(result, b.Mul(b.CallIntrinsic(scope, {arg_types[0]}, {
            b.CallIntrinsic(scope, {arg_types[0]}, {args[0]}, return_type, 0, Fraction::instantiate_Fraction)},
            return_type, 0, Abs::instantiate_Abs), b.Pow(b.i2r_t(b.i32(2), return_type),
            b.i2r_t(b.CallIntrinsic(scope, {arg_types[0]}, {args[0]}, int32, 0, Digits::instantiate_Digits),
            return_type)))));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);

    }

} // namespace Rrspacing

namespace Repeat {

    static ASR::expr_t *eval_Repeat(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        char* str = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        int64_t n = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
        size_t len = std::strlen(str);
        size_t new_len = len*n;
        char* result = new char[new_len+1];
        for (size_t i=0; i<new_len; i++) {
            result[i] = str[i%len];
        }
        result[new_len] = '\0';
        return make_ConstantWithType(make_StringConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_Repeat(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        auto func_name = "_lcompilers_optimization_repeat_" + type_to_str_python(arg_types[0])
             + type_to_str_python(arg_types[1]);
        declare_basic_variables(func_name);
        if (scope->get_symbol(func_name)) {
            ASR::symbol_t *s = scope->get_symbol(func_name);
            return b.Call(s, new_args, return_type, nullptr);
        }
        fill_func_arg("x", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -10, nullptr, ASR::string_physical_typeType::PointerString)));
        fill_func_arg("y", arg_types[1]);
        auto result = declare(fn_name, ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -3,
            ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc,
                ASRUtils::EXPR(ASR::make_StringLen_t(al, loc, args[0], ASRUtils::expr_type(args[1]), nullptr)),
                ASR::binopType::Mul, args[1], ASRUtils::expr_type(args[1]), nullptr)), ASR::string_physical_typeType::PointerString)), ReturnVar);
        auto i = declare("i", int32, Local);
        auto j = declare("j", int32, Local);
        auto m = declare("m", int32, Local);
        auto cnt = declare("cnt", int32, Local);
        /*
            function repeat_(s, n) result(r)
                character(len=*), intent(in) :: s
                integer, intent(in) :: n
                character(len=n*len(s)) :: r
                integer :: i, j, m, cnt
                m = len(s)
                i = 1
                j = m
                cnt = 0
                do while (cnt < n)
                    r(i:j) = s(1:len(s))
                    i = j + 1
                    j = i + m - 1
                    cnt = cnt + 1
                end do
            end function
        */

        body.push_back(al, b.Assignment(m, b.StringLen(args[0])));
        body.push_back(al, b.Assignment(i, b.i32(1)));
        body.push_back(al, b.Assignment(j, m));
        body.push_back(al, b.Assignment(cnt, b.i32(0)));
        body.push_back(al, b.While(b.Lt(cnt, CastingUtil::perform_casting(args[1], int32, al, loc)), {
            b.Assignment(b.StringSection(result, b.Sub(i, b.i32(1)), j),
                b.StringSection(args[0], b.i32(0), b.StringLen(args[0]))),
            b.Assignment(i, b.Add(j, b.i32(1))),
            b.Assignment(j, b.Sub(b.Add(i, m), b.i32(1))),
            b.Assignment(cnt, b.Add(cnt, b.i32(1))),
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Repeat

namespace StringContainsSet {

    static ASR::expr_t *eval_StringContainsSet(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        char* string = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        char* set = ASR::down_cast<ASR::StringConstant_t>(args[1])->m_s;
        bool back = ASR::down_cast<ASR::LogicalConstant_t>(args[2])->m_value;
        size_t len = std::strlen(string);
        int64_t result = 0;
        if (back) {
            for (size_t i=0; i<len; i++) {
                if (std::strchr(set, string[len-i-1]) == nullptr) {
                    result = len-i;
                    break;
                }
            }
        } else {
            for (size_t i=0; i<len; i++) {
                if (std::strchr(set, string[i]) == nullptr) {
                    result = i+1;
                    break;
                }
            }
        }
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_StringContainsSet(Allocator &al, const Location &loc,
            SymbolTable* scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_verify_" + type_to_str_python(arg_types[0]));
        fill_func_arg("str", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        fill_func_arg("set", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        fill_func_arg("back", ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)));
        fill_func_arg("kind", int32);
        auto result = declare(fn_name, return_type, ReturnVar);
        auto matched = declare("matched", logical, Local);
        auto i = declare("i", int32, Local);
        auto j = declare("j", int32, Local);
        /*
            function StringContainsSet_(string, set, back, kind) result(result)
                character(len=*) :: string
                character(len=*) :: set
                logical, optional :: back
                integer(kind) :: result = 0
                integer :: i, j
                logical :: matched
                if (back) then
                    i = len(string)
                    do while (i >= 1)
                        matched = .false.
                        j = 1
                        do while (j <= len(set))
                            if (string(i:i) == set(j:j)) then
                                matched = .true.
                            end if
                            j = j + 1
                        end do
                        if (.not. matched) then
                            result = i
                            exit
                        end if
                        i = i - 1
                    end do
                else
                    i = 1
                    do while (i <= len(string))
                        matched = .false.
                        j = 1
                        do while (j <= len(set))
                            if (string(i:i) == set(j:j)) then
                                matched = .true.
                            end if
                            j = j + 1
                        end do
                        if (.not. matched) then
                            result = i
                            exit
                        end if
                        i = i + 1
                    end do
                end if
            end function
        */
        body.push_back(al, b.Assignment(result, b.i_t(0, return_type)));
        body.push_back(al, b.If(b.Eq(args[2], b.bool_t(1, logical)), {
            b.Assignment(i, b.StringLen(args[0])),
            b.While(b.GtE(i, b.i_t(1, return_type)), {
                b.Assignment(matched, b.bool_t(0, logical)),
                b.Assignment(j, b.i_t(1, return_type)),
                b.While(b.LtE(j, b.StringLen(args[1])), {
                    b.If(b.Eq(b.StringSection(args[0], b.Sub(i, b.i_t(1, return_type)), i),
                    b.StringSection(args[1], b.Sub(j, b.i_t(1, return_type)), j)), {
                        b.Assignment(matched, b.bool_t(1, logical))
                    }, {}),
                    b.Assignment(j, b.Add(j, b.i_t(1, return_type))),
                }),
                b.If(b.Eq(matched, b.bool_t(0, logical)), {
                    b.Assignment(result, i),
                    b.Exit()
                }, {}),
                b.Assignment(i, b.Sub(i, b.i_t(1, return_type))),
            }),
        }, {
            b.Assignment(i, b.i_t(1, return_type)),
            b.While(b.LtE(i, b.StringLen(args[0])), {
                b.Assignment(matched, b.bool_t(0, logical)),
                b.Assignment(j, b.i_t(1, return_type)),
                b.While(b.LtE(j, b.StringLen(args[1])), {
                    b.If(b.Eq(b.StringSection(args[0], b.Sub(i, b.i_t(1, return_type)), i),
                    b.StringSection(args[1], b.Sub(j, b.i_t(1, return_type)), j)), {
                        b.Assignment(matched, b.bool_t(1, logical))
                    }, {}),
                    b.Assignment(j, b.Add(j, b.i_t(1, return_type))),
                }),
                b.If(b.Eq(matched, b.bool_t(0, logical)), {
                    b.Assignment(result, i),
                    b.Exit()
                }, {}),
                b.Assignment(i, b.Add(i, b.i_t(1, return_type)))
            }),
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace StringContainsSet

namespace StringFindSet {

    static ASR::expr_t *eval_StringFindSet(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        char* string = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        char* set = ASR::down_cast<ASR::StringConstant_t>(args[1])->m_s;
        bool back = ASR::down_cast<ASR::LogicalConstant_t>(args[2])->m_value;
        size_t len = std::strlen(string);
        int64_t result = 0;
        if (back) {
            for (size_t i=0; i<len; i++) {
                if (std::strchr(set, string[len-i-1]) != nullptr) {
                    result = len-i;
                    break;
                }
            }
        } else {
            for (size_t i=0; i<len; i++) {
                if (std::strchr(set, string[i]) != nullptr) {
                    result = i+1;
                    break;
                }
            }
        }
        return make_ConstantWithType(make_IntegerConstant_t, result, t1, loc);
    }

    static inline ASR::expr_t* instantiate_StringFindSet(Allocator &al, const Location &loc,
            SymbolTable* scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_scan_" + type_to_str_python(arg_types[0]));
        fill_func_arg("str", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        fill_func_arg("set", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        fill_func_arg("back", ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)));
        fill_func_arg("kind", int32);
        auto result = declare(fn_name, return_type, ReturnVar);
        auto i = declare("i", int32, Local);
        auto j = declare("j", int32, Local);
        /*
            function StringFindSet_(string, set, back, kind) result(r)
                character(len=*) :: string
                character(len=*) :: set
                logical, optional :: back
                integer(kind) :: r = 0
                integer :: i, j
                if (back) then
                    i = len(string)
                    do while (i >= 1)
                        j = 1
                        do while (j <= len(set))
                            if (string(i:i) == set(j:j)) then
                                r = i
                                exit
                            end if
                            j = j + 1
                        end do
                        if (r /= 0) exit
                        i = i - 1
                    end do
                else
                    i = 1
                    do while (i <= len(string))
                        j = 1
                        do while (j <= len(set))
                            if (string(i:i) == set(j:j)) then
                                r = i
                                exit
                            end if
                            j = j + 1
                        end do
                        if (r /= 0) exit
                        i = i + 1
                    end do
                end if
            end function
        */

        body.push_back(al, b.Assignment(result, b.i_t(0, return_type)));
        body.push_back(al, b.If(b.Eq(args[2], b.bool_t(1, arg_types[2])), {
            b.Assignment(i, b.StringLen(args[0])),
            b.While(b.GtE(i, b.i_t(1, return_type)), {
                b.Assignment(j, b.i_t(1, return_type)),
                b.While(b.LtE(j, b.StringLen(args[1])), {
                    b.If(b.Eq(b.StringSection(args[0], b.Sub(i, b.i_t(1, return_type)), i),
                    b.StringSection(args[1], b.Sub(j, b.i_t(1, return_type)), j)), {
                        b.Assignment(result, i),
                        b.Exit()
                    }, {}),
                    b.Assignment(j, b.Add(j, b.i_t(1, return_type))),
                }),
                b.If(b.NotEq(result, b.i_t(0, return_type)), {
                    b.Exit()
                }, {}),
                b.Assignment(i, b.Sub(i, b.i_t(1, return_type)))
            }),
        }, {
            b.Assignment(i, b.i_t(1, return_type)),
            b.While(b.LtE(i, b.StringLen(args[0])), {
                b.Assignment(j, b.i_t(1, return_type)),
                b.While(b.LtE(j, b.StringLen(args[1])), {
                    b.If(b.Eq(b.StringSection(args[0], b.Sub(i, b.i_t(1, return_type)), i),
                    b.StringSection(args[1], b.Sub(j, b.i_t(1, return_type)), j)), {
                        b.Assignment(result, i),
                        b.Exit()
                    }, {}),
                    b.Assignment(j, b.Add(j, b.i_t(1, return_type))),
                }),
                b.If(b.NotEq(result, b.i_t(0, return_type)), {
                    b.Exit()
                }, {}),
                b.Assignment(i, b.Add(i, b.i_t(1, return_type)))
            }),
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace StringFindSet

namespace SubstrIndex {

    static ASR::expr_t *eval_SubstrIndex(Allocator &al, const Location &loc,
            ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        char* string = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
        char* substring = ASR::down_cast<ASR::StringConstant_t>(args[1])->m_s;
        bool back = ASR::down_cast<ASR::LogicalConstant_t>(args[2])->m_value;
        int64_t kind = ASR::down_cast<ASR::IntegerConstant_t>(args[3])->m_n;
        size_t len = std::strlen(string);
        int64_t result = 0;
        if (back) {
            for (size_t i=0; i<len; i++) {
                if (std::strncmp(string+len-i-1, substring, std::strlen(substring)) == 0) {
                    result = len-i;
                    break;
                }
            }
        } else {
            for (size_t i=0; i<len; i++) {
                if (std::strncmp(string+i, substring, std::strlen(substring)) == 0) {
                    result = i+1;
                    break;
                }
            }
        }

        ASR::ttype_t* return_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, kind));
        return make_ConstantWithType(make_IntegerConstant_t, result, return_type, loc);
    }

    static inline ASR::expr_t* instantiate_SubstrIndex(Allocator &al, const Location &loc,
            SymbolTable* scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_index_" + type_to_str_python(arg_types[0]));
        fill_func_arg("str", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        fill_func_arg("substr", ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
        fill_func_arg("back", ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)));
        fill_func_arg("kind", int32);
        auto idx = declare(fn_name, return_type, ReturnVar);
        auto found = declare("found", arg_types[2], Local);
        auto i = declare("i", int32, Local);
        auto j = declare("j", int32, Local);
        auto k = declare("k", int32, Local);
        auto pos = declare("pos", int32, Local);

        /*
            function SubstrIndex_(string, substring, back, kind) result(r)
                character(len=*) :: string
                character(len=*) :: substring
                logical, optional :: back
                integer(kind) :: idx = 0
                integer :: i, j, k, pos, len_str, len_sub, idx
                logical :: found = .true.
                i = 1
                len_str = len(string)
                len_sub = len(substring)

                if (len_str < len_sub) then
                    found = .false.
                end if

                do while (i < len_str .and. found)
                    k = 0
                    j = 1
                    do while (j <= len_sub .and. found)
                        pos = i + k
                        if( string(pos:pos) /= substring(j:j) ) then
                            found = .false.
                        end if
                        k = k + 1
                        j = j + 1
                    end do
                    if (found) then
                        idx = i
                        if (back .eqv. .true.) then
                            found = .true.
                        else
                            found = .false.
                        end if
                    else
                        found = .true.
                    end if
                    i = i + 1
                end do
            end function
        */
        body.push_back(al, b.Assignment(idx, b.i_t(0, return_type)));
        body.push_back(al, b.Assignment(i, b.i_t(1, return_type)));
        body.push_back(al, b.Assignment(found, b.bool_t(1, arg_types[2])));
        body.push_back(al, b.If(b.Lt(b.StringLen(args[0]), b.StringLen(args[1])), {
            b.Assignment(found, b.bool_t(0, arg_types[2]))
        }, {}));

        body.push_back(al, b.While(b.And(b.Lt(i, b.Add(b.StringLen(args[0]), b.i32(1))), b.Eq(found, b.bool_t(1, arg_types[2]))), {
            b.Assignment(k, b.i_t(0, return_type)),
            b.Assignment(j, b.i_t(1, return_type)),
            b.While(b.And(b.LtE(j, b.StringLen(args[1])), b.Eq(found, b.bool_t(1, arg_types[2]))), {
                b.Assignment(pos, b.Add(i, k)),
                b.If(b.NotEq(
                    b.StringSection(args[0], b.Sub(pos, b.i_t(1, return_type)), pos),
                    b.StringSection(args[1], b.Sub(j, b.i_t(1, return_type)), j)), {
                        b.Assignment(found, b.bool_t(0, arg_types[2]))
                }, {}),
                b.Assignment(j, b.Add(j, b.i_t(1, return_type))),
                b.Assignment(k, b.Add(k, b.i_t(1, return_type))),
            }),
            b.If(b.Eq(found, b.bool_t(1, arg_types[2])), {
                b.Assignment(idx, i),
                b.Assignment(found, args[2])
            }, {
                b.Assignment(found, b.bool_t(1, arg_types[2]))
            }),
            b.Assignment(i, b.Add(i, b.i_t(1, return_type))),
        }));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, idx, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace SubstrIndex

namespace MinExponent {

    static ASR::expr_t *eval_MinExponent(Allocator &al, const Location &loc,
            ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int m_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(args[0]));
        int result;
        if (m_kind == 4) {
            result = std::numeric_limits<float>::min_exponent;
        } else {
            result = std::numeric_limits<double>::min_exponent;
        }
        return make_ConstantWithType(make_IntegerConstant_t, result, int32, loc);
    }

} // namespace MinExponent

namespace MaxExponent {

    static ASR::expr_t *eval_MaxExponent(Allocator &al, const Location &loc,
            ASR::ttype_t* /*t1*/, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        int m_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(args[0]));
        int result;
        if (m_kind == 4) {
            result = std::numeric_limits<float>::max_exponent;
        } else {
            result = std::numeric_limits<double>::max_exponent;
        }
        return make_ConstantWithType(make_IntegerConstant_t, result, int32, loc);
    }

} // namespace MaxExponent

#define create_exp_macro(X, stdeval)                                                      \
namespace X {                                                                             \
    static inline ASR::expr_t* eval_##X(Allocator &al, const Location &loc,               \
            ASR::ttype_t *t, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {      \
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));                            \
        double rv = -1;                                                                   \
        if( ASRUtils::extract_value(args[0], rv) ) {                                      \
            double val = std::stdeval(rv);                                                \
            return ASRUtils::EXPR(ASR::make_RealConstant_t(al, loc, val, t));             \
        }                                                                                 \
        return nullptr;                                                                   \
    }                                                                                     \
} // namespace X

create_exp_macro(Exp2, exp2)
create_exp_macro(Expm1, expm1)

namespace Exp {

    static inline ASR::expr_t* eval_Exp(Allocator &al, const Location &loc,
            ASR::ttype_t *t, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        double rv = -1;
        if( ASRUtils::extract_value(args[0], rv) ) {
            double val = std::exp(rv);
            return ASRUtils::EXPR(ASR::make_RealConstant_t(al, loc, val, t));
        } else {
            std::complex<double> crv;
            if( ASRUtils::extract_value(args[0], crv) ) {
                std::complex<double> val = std::exp(crv);
                return ASRUtils::EXPR(ASR::make_ComplexConstant_t(
                    al, loc, val.real(), val.imag(), t));
            }
        }
        return nullptr;
    }

    static inline ASR::expr_t* instantiate_Exp(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t overload_id) {
        if (is_real(*arg_types[0])) {
            Vec<ASR::expr_t *> args; args.reserve(al, 1);
            args.push_back(al, new_args[0].m_value);
            return EXPR(ASR::make_IntrinsicElementalFunction_t(al, loc,
                static_cast<int64_t>(IntrinsicElementalFunctions::Exp),
                args.p, 1, overload_id, return_type, nullptr));
        } else {
            return UnaryIntrinsicFunction::instantiate_functions(al, loc, scope,
                "exp", arg_types[0], return_type, new_args, overload_id);
        }
    }

} // namespace Exp

namespace ErfcScaled {

    static inline ASR::expr_t* eval_ErfcScaled(Allocator &al, const Location &loc,
            ASR::ttype_t *t, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        double val = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
        double result = std::exp(std::pow(val, 2)) * std::erfc(val);
        return make_ConstantWithType(make_RealConstant_t, result, t, loc);
    }

    static inline ASR::expr_t* instantiate_ErfcScaled(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_erfc_scaled_" + type_to_str_python(arg_types[0]));
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        /*
        * r = erfc_scaled(x)
        * r = exp(x**2) * erfc(x)
        */

        body.push_back(al, b.Assignment(result, b.Mul(
            b.CallIntrinsic(scope, {arg_types[0]}, {b.Pow(args[0], b.f_t(2, arg_types[0]))}, arg_types[0], 0, Exp::instantiate_Exp),
            b.CallIntrinsic(scope, {arg_types[0]}, {args[0]}, arg_types[0], 0, Erfc::instantiate_Erfc))));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args, body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);

    }

} // namespace ErfcScaled

namespace ListIndex {

static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
    ASRUtils::require_impl(x.n_args <= 4, "Call to list.index must have at most four arguments",
        x.base.base.loc, diagnostics);
    ASR::ttype_t* arg0_type = ASRUtils::expr_type(x.m_args[0]);
    ASRUtils::require_impl(ASR::is_a<ASR::List_t>(*arg0_type) &&
        ASRUtils::check_equal_type(ASRUtils::expr_type(x.m_args[1]), ASRUtils::get_contained_type(arg0_type)),
        "First argument to list.index must be of list type and "
        "second argument must be of same type as list elemental type",
        x.base.base.loc, diagnostics);
    if(x.n_args >= 3) {
        ASRUtils::require_impl(
            ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(x.m_args[2])),
            "Third argument to list.index must be an integer",
            x.base.base.loc, diagnostics);
    }
    if(x.n_args == 4) {
        ASRUtils::require_impl(
            ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(x.m_args[3])),
            "Fourth argument to list.index must be an integer",
            x.base.base.loc, diagnostics);
    }
    ASRUtils::require_impl(ASR::is_a<ASR::Integer_t>(*x.m_type),
        "Return type of list.index must be an integer",
        x.base.base.loc, diagnostics);
}

static inline ASR::expr_t *eval_list_index(Allocator &/*al*/,
    const Location &/*loc*/, ASR::ttype_t */*t*/, Vec<ASR::expr_t*>& /*args*/, diag::Diagnostics& /*diag*/) {
    // TODO: To be implemented for ListConstant expression
    return nullptr;
}


static inline ASR::asr_t* create_ListIndex(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    diag::Diagnostics& diag) {
    int64_t overload_id = 0;
    ASR::expr_t* list_expr = args[0];
    ASR::ttype_t *type = ASRUtils::expr_type(list_expr);
    ASR::ttype_t *list_type = ASR::down_cast<ASR::List_t>(type)->m_type;
    ASR::ttype_t *ele_type = ASRUtils::expr_type(args[1]);
    if (!ASRUtils::check_equal_type(ele_type, list_type)) {
        std::string fnd = ASRUtils::get_type_code(ele_type);
        std::string org = ASRUtils::get_type_code(list_type);
        append_error(diag,
            "Type mismatch in 'index', the types must be compatible "
            "(found: '" + fnd + "', expected: '" + org + "')", loc);
        return nullptr;
    }
    if (args.size() >= 3) {
        overload_id = 1;
        if(!ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(args[2]))) {
            append_error(diag, "Third argument to list.index must be an integer", loc);
            return nullptr;
        }
    }
    if (args.size() == 4) {
        overload_id = 2;
        if(!ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(args[3]))) {
            append_error(diag, "Fourth argument to list.index must be an integer", loc);
            return nullptr;
        }
    }
    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, args.size());
    for( size_t i = 0; i < args.size(); i++ ) {
        arg_values.push_back(al, ASRUtils::expr_value(args[i]));
    }
    ASR::ttype_t *to_type = int32;
    ASR::expr_t* compile_time_value = eval_list_index(al, loc, to_type, arg_values, diag);
    return ASR::make_IntrinsicElementalFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicElementalFunctions::ListIndex),
            args.p, args.size(), overload_id, to_type, compile_time_value);
}

} // namespace ListIndex

namespace ListReverse {

static inline ASR::expr_t *eval_ListReverse(Allocator &/*al*/,
    const Location &/*loc*/, ASR::ttype_t */*t*/, Vec<ASR::expr_t*>& /*args*/, diag::Diagnostics& /*diag*/) {
    // TODO: To be implemented for ListConstant expression
    return nullptr;
}

} // namespace ListReverse

namespace ListPop {

static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
    ASRUtils::require_impl(x.n_args <= 2, "Call to list.pop must have at most one argument",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASR::is_a<ASR::List_t>(*ASRUtils::expr_type(x.m_args[0])),
        "Argument to list.pop must be of list type",
        x.base.base.loc, diagnostics);
    switch(x.m_overload_id) {
        case 0:
            break;
        case 1:
            ASRUtils::require_impl(ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(x.m_args[1])),
            "Argument to list.pop must be an integer",
            x.base.base.loc, diagnostics);
            break;
    }
    ASRUtils::require_impl(ASRUtils::check_equal_type(x.m_type,
            ASRUtils::get_contained_type(ASRUtils::expr_type(x.m_args[0]))),
        "Return type of list.pop must be of same type as list's element type",
        x.base.base.loc, diagnostics);
}

static inline ASR::expr_t *eval_list_pop(Allocator &/*al*/,
    const Location &/*loc*/, ASR::ttype_t */*t*/, Vec<ASR::expr_t*>& /*args*/, diag::Diagnostics& /*diag*/) {
    // TODO: To be implemented for ListConstant expression
    return nullptr;
}

static inline ASR::asr_t* create_ListPop(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    diag::Diagnostics& diag) {
    if (args.size() > 2) {
        append_error(diag, "Call to list.pop must have at most one argument", loc);
        return nullptr;
    }
    if (args.size() == 2 &&
        !ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(args[1]))) {
        append_error(diag, "Argument to list.pop must be an integer", loc);
        return nullptr;
    }

    ASR::expr_t* list_expr = args[0];
    ASR::ttype_t *type = ASRUtils::expr_type(list_expr);
    ASR::ttype_t *list_type = ASR::down_cast<ASR::List_t>(type)->m_type;

    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, args.size());
    for( size_t i = 0; i < args.size(); i++ ) {
        arg_values.push_back(al, ASRUtils::expr_value(args[i]));
    }
    ASR::ttype_t *to_type = list_type;
    ASR::expr_t* compile_time_value = eval_list_pop(al, loc, to_type, arg_values, diag);
    int64_t overload_id = (args.size() == 2);
    return ASR::make_IntrinsicElementalFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicElementalFunctions::ListPop),
            args.p, args.size(), overload_id, to_type, compile_time_value);
}

} // namespace ListPop

namespace ListReserve {

static inline ASR::expr_t *eval_ListReserve(Allocator &/*al*/,
    const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/, diag::Diagnostics& /*diag*/) {
    // TODO: To be implemented for ListConstant expression
    return nullptr;
}

} // namespace ListReserve

namespace DictKeys {

static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
    ASRUtils::require_impl(x.n_args == 1, "Call to dict.keys must have no argument",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASR::is_a<ASR::Dict_t>(*ASRUtils::expr_type(x.m_args[0])),
        "Argument to dict.keys must be of dict type",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASR::is_a<ASR::List_t>(*x.m_type) &&
        ASRUtils::check_equal_type(ASRUtils::get_contained_type(x.m_type),
        ASRUtils::get_contained_type(ASRUtils::expr_type(x.m_args[0]), 0)),
        "Return type of dict.keys must be of list of dict key element type",
        x.base.base.loc, diagnostics);
}

static inline ASR::expr_t *eval_dict_keys(Allocator &/*al*/,
    const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/, diag::Diagnostics& /*diag*/) {
    // TODO: To be implemented for DictConstant expression
    return nullptr;
}

static inline ASR::asr_t* create_DictKeys(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    diag::Diagnostics& diag) {
    if (args.size() != 1) {
        append_error(diag, "Call to dict.keys must have no argument", loc);
        return nullptr;
    }

    ASR::expr_t* dict_expr = args[0];
    ASR::ttype_t *type = ASRUtils::expr_type(dict_expr);
    ASR::ttype_t *dict_keys_type = ASR::down_cast<ASR::Dict_t>(type)->m_key_type;

    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, args.size());
    for( size_t i = 0; i < args.size(); i++ ) {
        arg_values.push_back(al, ASRUtils::expr_value(args[i]));
    }
    ASR::ttype_t *to_type = List(dict_keys_type);
    ASR::expr_t* compile_time_value = eval_dict_keys(al, loc, to_type, arg_values, diag);
    return ASR::make_IntrinsicElementalFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicElementalFunctions::DictKeys),
            args.p, args.size(), 0, to_type, compile_time_value);
}

} // namespace DictKeys

namespace DictValues {

static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
    ASRUtils::require_impl(x.n_args == 1, "Call to dict.values must have no argument",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASR::is_a<ASR::Dict_t>(*ASRUtils::expr_type(x.m_args[0])),
        "Argument to dict.values must be of dict type",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASR::is_a<ASR::List_t>(*x.m_type) &&
        ASRUtils::check_equal_type(ASRUtils::get_contained_type(x.m_type),
        ASRUtils::get_contained_type(ASRUtils::expr_type(x.m_args[0]), 1)),
        "Return type of dict.values must be of list of dict value element type",
        x.base.base.loc, diagnostics);
}

static inline ASR::expr_t *eval_dict_values(Allocator &/*al*/,
    const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/, diag::Diagnostics& /*diag*/) {
    // TODO: To be implemented for DictConstant expression
    return nullptr;
}

static inline ASR::asr_t* create_DictValues(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    diag::Diagnostics& diag) {
    if (args.size() != 1) {
        append_error(diag, "Call to dict.values must have no argument", loc);
        return nullptr;
    }

    ASR::expr_t* dict_expr = args[0];
    ASR::ttype_t *type = ASRUtils::expr_type(dict_expr);
    ASR::ttype_t *dict_values_type = ASR::down_cast<ASR::Dict_t>(type)->m_value_type;

    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, args.size());
    for( size_t i = 0; i < args.size(); i++ ) {
        arg_values.push_back(al, ASRUtils::expr_value(args[i]));
    }
    ASR::ttype_t *to_type = List(dict_values_type);
    ASR::expr_t* compile_time_value = eval_dict_values(al, loc, to_type, arg_values, diag);
    return ASR::make_IntrinsicElementalFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicElementalFunctions::DictValues),
            args.p, args.size(), 0, to_type, compile_time_value);
}

} // namespace DictValues

namespace SetAdd {

static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
    ASRUtils::require_impl(x.n_args == 2, "Call to set.add must have exactly one argument",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASR::is_a<ASR::Set_t>(*ASRUtils::expr_type(x.m_args[0])),
        "First argument to set.add must be of set type",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASRUtils::check_equal_type(ASRUtils::expr_type(x.m_args[1]),
            ASRUtils::get_contained_type(ASRUtils::expr_type(x.m_args[0]))),
        "Second argument to set.add must be of same type as set's element type",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(x.m_type == nullptr,
        "Return type of set.add must be empty",
        x.base.base.loc, diagnostics);
}

static inline ASR::expr_t *eval_set_add(Allocator &/*al*/,
    const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/, diag::Diagnostics& /*diag*/) {
    // TODO: To be implemented for SetConstant expression
    return nullptr;
}

static inline ASR::asr_t* create_SetAdd(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    diag::Diagnostics& diag) {
    if (args.size() != 2) {
        append_error(diag, "Call to set.add must have exactly one argument", loc);
        return nullptr;
    }
    if (!ASRUtils::check_equal_type(ASRUtils::expr_type(args[1]),
        ASRUtils::get_contained_type(ASRUtils::expr_type(args[0])))) {
        append_error(diag, "Argument to set.add must be of same type as set's "
            "element type", loc);
        return nullptr;
    }

    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, args.size());
    for( size_t i = 0; i < args.size(); i++ ) {
        arg_values.push_back(al, ASRUtils::expr_value(args[i]));
    }
    ASR::expr_t* compile_time_value = eval_set_add(al, loc, nullptr, arg_values, diag);
    return ASR::make_Expr_t(al, loc,
            ASRUtils::EXPR(ASR::make_IntrinsicElementalFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicElementalFunctions::SetAdd),
            args.p, args.size(), 0, nullptr, compile_time_value)));
}

} // namespace SetAdd

namespace SetRemove {

static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
    ASRUtils::require_impl(x.n_args == 2, "Call to set.remove must have exactly one argument",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASR::is_a<ASR::Set_t>(*ASRUtils::expr_type(x.m_args[0])),
        "First argument to set.remove must be of set type",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASRUtils::check_equal_type(ASRUtils::expr_type(x.m_args[1]),
            ASRUtils::get_contained_type(ASRUtils::expr_type(x.m_args[0]))),
        "Second argument to set.remove must be of same type as set's element type",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(x.m_type == nullptr,
        "Return type of set.remove must be empty",
        x.base.base.loc, diagnostics);
}

static inline ASR::expr_t *eval_set_remove(Allocator &/*al*/,
    const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/, diag::Diagnostics& /*diag*/) {
    // TODO: To be implemented for SetConstant expression
    return nullptr;
}

static inline ASR::asr_t* create_SetRemove(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    diag::Diagnostics& diag) {
    if (args.size() != 2) {
        append_error(diag, "Call to set.remove must have exactly one argument", loc);
        return nullptr;
    }
    if (!ASRUtils::check_equal_type(ASRUtils::expr_type(args[1]),
        ASRUtils::get_contained_type(ASRUtils::expr_type(args[0])))) {
        append_error(diag, "Argument to set.remove must be of same type as set's "
            "element type", loc);
        return nullptr;
    }

    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, args.size());
    for( size_t i = 0; i < args.size(); i++ ) {
        arg_values.push_back(al, ASRUtils::expr_value(args[i]));
    }
    ASR::expr_t* compile_time_value = eval_set_remove(al, loc, nullptr, arg_values, diag);
    return ASR::make_Expr_t(al, loc,
            ASRUtils::EXPR(ASR::make_IntrinsicElementalFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicElementalFunctions::SetRemove),
            args.p, args.size(), 0, nullptr, compile_time_value)));
}

} // namespace SetRemove

namespace Max {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args > 1, "Call to max0 must have at least two arguments",
            x.base.base.loc, diagnostics);
        ASR::ttype_t* arg0_type = ASRUtils::extract_type(ASRUtils::expr_type(x.m_args[0]));
        ASRUtils::require_impl(ASR::is_a<ASR::Real_t>(*arg0_type) ||
            ASR::is_a<ASR::Integer_t>(*arg0_type) || ASR::is_a<ASR::String_t>(*arg0_type),
             "Arguments to max0 must be of real, integer or character type",
            x.base.base.loc, diagnostics);
        for(size_t i=0;i<x.n_args;i++){
            ASR::ttype_t* arg_type = ASRUtils::extract_type(ASRUtils::expr_type(x.m_args[i]));
            ASRUtils::require_impl((ASR::is_a<ASR::Real_t>(*arg_type) && ASR::is_a<ASR::Real_t>(*arg0_type)) ||
                                    (ASR::is_a<ASR::Integer_t>(*arg_type) && ASR::is_a<ASR::Integer_t>(*arg0_type)) ||
                                    (ASR::is_a<ASR::String_t>(*arg_type) && ASR::is_a<ASR::String_t>(*arg0_type) ),
            "All arguments must be of the same type",
            x.base.base.loc, diagnostics);
        }
    }

    static ASR::expr_t *eval_Max(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        if (ASR::is_a<ASR::Real_t>(*arg_type)) {
            double max_val = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            for (size_t i = 1; i < args.size(); i++) {
                double val = ASR::down_cast<ASR::RealConstant_t>(args[i])->m_r;
                max_val = std::fmax(max_val, val);
            }
            return ASR::down_cast<ASR::expr_t>(ASR::make_RealConstant_t(al, loc, max_val, arg_type));
        } else if (ASR::is_a<ASR::Integer_t>(*arg_type)) {
            int64_t max_val = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
            for (size_t i = 1; i < args.size(); i++) {
                int64_t val = ASR::down_cast<ASR::IntegerConstant_t>(args[i])->m_n;
                max_val = std::fmax(max_val, val);
            }
            return ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(al, loc, max_val, arg_type));
        } else if (ASR::is_a<ASR::String_t>(*arg_type)) {
            char* max_val = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
            for (size_t i = 1; i < args.size(); i++) {
                char* val = ASR::down_cast<ASR::StringConstant_t>(args[i])->m_s;
                if (strcmp(val, max_val) > 0) {
                    max_val = val;
                }
            }
            return ASR::down_cast<ASR::expr_t>(ASR::make_StringConstant_t(al, loc, max_val, arg_type));
        } else {
            return nullptr;
        }
    }

    static inline ASR::asr_t* create_Max(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
        diag::Diagnostics& diag) {
        bool is_compile_time = true;
        for(size_t i=0; i<100;i++){
            args.erase(nullptr);
        }
        if (args.size() < 2) {
            append_error(diag, "Intrinsic max0 must have 2 arguments", loc);
            return nullptr;
        }
        ASR::ttype_t *arg_type = ASRUtils::expr_type(args[0]);
        int kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
        bool all_args_same_kind = true;
        for(size_t i=1; i<args.size(); i++) {
            if (ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(args[i])) != kind) {
                diag.semantic_warning_label("Different kinds of args in max0 is a non-standard extension", {loc},
                "help: ensure all arguments have the same kind to make it standard");
                all_args_same_kind = false;
            }
            if (ASRUtils::extract_type(arg_type)->type != ASRUtils::extract_type(ASRUtils::expr_type(args[i]))->type) {
                append_error(diag, "All arguments to max0 must be of the same type", loc);
            return nullptr;
            }
        }

        if (!all_args_same_kind){
            promote_arguments_kinds(al, loc, args, diag);
        }

        Vec<ASR::expr_t*> arg_values;
        arg_values.reserve(al, args.size());
        ASR::expr_t *arg_value;
        for(size_t i=0;i<args.size();i++){
            arg_value = ASRUtils::expr_value(args[i]);
            if (!arg_value) {
                is_compile_time = false;
            }
            arg_values.push_back(al, arg_value);
        }
        if (is_compile_time) {
            ASR::expr_t *value = eval_Max(al, loc, expr_type(args[0]), arg_values, diag);
            return ASRUtils::make_IntrinsicElementalFunction_t_util(al, loc,
                static_cast<int64_t>(IntrinsicElementalFunctions::Max),
                args.p, args.n, 0, ASRUtils::expr_type(args[0]), value);
        } else {
            return ASRUtils::make_IntrinsicElementalFunction_t_util(al, loc,
                static_cast<int64_t>(IntrinsicElementalFunctions::Max),
                args.p, args.n, 0, ASRUtils::expr_type(args[0]), nullptr);
        }
    }

    static inline ASR::expr_t* instantiate_Max(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_max0_" + type_to_str_python(arg_types[0]));
        int64_t kind = extract_kind_from_ttype_t(arg_types[0]);
        if (ASR::is_a<ASR::String_t>(*arg_types[0])) {
            for (size_t i = 0; i < new_args.size(); i++) {
                fill_func_arg("x" + std::to_string(i), ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
            }
            return_type = TYPE(ASR::make_String_t(al, loc, 1, -3, EXPR(ASR::make_StringLen_t(al, loc, args[0], int32, nullptr)), ASR::string_physical_typeType::PointerString));
        } else if (ASR::is_a<ASR::Real_t>(*arg_types[0])) {
            for (size_t i = 0; i < new_args.size(); i++) {
                fill_func_arg("x" + std::to_string(i), ASRUtils::TYPE(ASR::make_Real_t(al, loc, kind)));
            }
        } else if (ASR::is_a<ASR::Integer_t>(*arg_types[0])) {
            for (size_t i = 0; i < new_args.size(); i++) {
                fill_func_arg("x" + std::to_string(i), ASRUtils::TYPE(ASR::make_Integer_t(al, loc, kind)));
            }
        } else {
            throw LCompilersException("Arguments to max0 must be of real, integer or character type");
        }
        return_type = ASRUtils::extract_type(return_type);
        auto result = declare(fn_name, return_type, ReturnVar);
        body.push_back(al, b.Assignment(result, args[0]));
        if (ASR::is_a<ASR::Integer_t>(*return_type)) {
            for (size_t i = 1; i < args.size(); i++) {
                body.push_back(al, b.If(b.Gt(args[i], result), {
                    b.Assignment(result, args[i])
                }, {}));
            }
        } else if (ASR::is_a<ASR::Real_t>(*return_type)) {
            for (size_t i = 1; i < args.size(); i++) {
                body.push_back(al, b.If(b.Gt(args[i], result), {
                    b.Assignment(result, args[i])
                }, {}));
            }
        } else if (ASR::is_a<ASR::String_t>(*return_type)) {
            for (size_t i = 1; i < args.size(); i++) {
                body.push_back(al, b.If(b.Gt(args[i], result), {
                    b.Assignment(result, args[i])
                }, {}));
            }
            return_type = TYPE(ASR::make_String_t(al, loc, 1, -3, EXPR(ASR::make_StringLen_t(al, loc, new_args[0].m_value, int32, nullptr)), ASR::string_physical_typeType::PointerString));
        } else {
            throw LCompilersException("Arguments to max0 must be of real, integer or character type");
        }

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

}  // namespace Max

namespace Min {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args > 1, "Call to min0 must have at least two arguments",
            x.base.base.loc, diagnostics);
        ASR::ttype_t* arg0_type = ASRUtils::extract_type(ASRUtils::expr_type(x.m_args[0]));
        ASRUtils::require_impl(ASR::is_a<ASR::Real_t>(*arg0_type) ||
            ASR::is_a<ASR::Integer_t>(*arg0_type) || ASR::is_a<ASR::String_t>(*arg0_type),
             "Arguments to min0 must be of real, integer or character type",
            x.base.base.loc, diagnostics);
        for(size_t i=0;i<x.n_args;i++){
            ASR::ttype_t* arg_type = ASRUtils::extract_type(ASRUtils::expr_type(x.m_args[i]));
            ASRUtils::require_impl((ASR::is_a<ASR::Real_t>(*arg_type) && ASR::is_a<ASR::Real_t>(*arg0_type)) ||
                                    (ASR::is_a<ASR::Integer_t>(*arg_type) && ASR::is_a<ASR::Integer_t>(*arg0_type)) ||
                                    (ASR::is_a<ASR::String_t>(*arg_type) && ASR::is_a<ASR::String_t>(*arg0_type) ),
            "All arguments must be of the same type",
            x.base.base.loc, diagnostics);
        }
    }

    static ASR::expr_t *eval_Min(Allocator &al, const Location &loc,
            ASR::ttype_t *arg_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        if (ASR::is_a<ASR::Real_t>(*arg_type)) {
            double min_val = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            for (size_t i = 1; i < args.size(); i++) {
                double val = ASR::down_cast<ASR::RealConstant_t>(args[i])->m_r;
                min_val = std::fmin(min_val, val);
            }
            return ASR::down_cast<ASR::expr_t>(ASR::make_RealConstant_t(al, loc, min_val, arg_type));
        } else if (ASR::is_a<ASR::Integer_t>(*arg_type)) {
            int64_t min_val = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
            for (size_t i = 1; i < args.size(); i++) {
                int64_t val = ASR::down_cast<ASR::IntegerConstant_t>(args[i])->m_n;
                min_val = std::fmin(min_val, val);
            }
            return ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(al, loc, min_val, arg_type));
        } else if (ASR::is_a<ASR::String_t>(*arg_type)) {
            char* min_val = ASR::down_cast<ASR::StringConstant_t>(args[0])->m_s;
            for (size_t i = 1; i < args.size(); i++) {
                char* val = ASR::down_cast<ASR::StringConstant_t>(args[i])->m_s;
                if (strcmp(val, min_val) < 0) {
                    min_val = val;
                }
            }
            return ASR::down_cast<ASR::expr_t>(ASR::make_StringConstant_t(al, loc, min_val, arg_type));
        } else {
            return nullptr;
        }
    }

    static inline ASR::asr_t* create_Min(
        Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
        diag::Diagnostics& diag) {
        bool is_compile_time = true;
        for(size_t i=0; i<100;i++){
            args.erase(nullptr);
        }
        if (args.size() < 2) {
            append_error(diag, "Intrinsic min0 must have 2 arguments", loc);
            return nullptr;
        }
        ASR::ttype_t *arg_type = ASRUtils::expr_type(args[0]);
        int kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
        bool all_args_same_kind = true;
        for(size_t i=1; i<args.size(); i++){
            if (ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(args[i])) != kind) {
                diag.semantic_warning_label("Different kinds of args in max0 is a non-standard extension", {loc},
                "help: ensure all arguments have the same kind to make it standard");
                all_args_same_kind = false;
            }
            if (ASRUtils::extract_type(arg_type)->type != ASRUtils::extract_type(ASRUtils::expr_type(args[i]))->type) {
                append_error(diag, "All arguments to min0 must be of the same type", loc);
                return nullptr;
            }
        }

        if (!all_args_same_kind){
            promote_arguments_kinds(al, loc, args, diag);
        }

        Vec<ASR::expr_t*> arg_values;
        arg_values.reserve(al, args.size());
        ASR::expr_t *arg_value;
        for(size_t i=0;i<args.size();i++){
            arg_value = ASRUtils::expr_value(args[i]);
            if (!arg_value) {
                is_compile_time = false;
            }
            arg_values.push_back(al, arg_value);
        }
        if (is_compile_time) {
            ASR::expr_t *value = eval_Min(al, loc, expr_type(args[0]), arg_values, diag);
            return ASRUtils::make_IntrinsicElementalFunction_t_util(al, loc,
                static_cast<int64_t>(IntrinsicElementalFunctions::Min),
                args.p, args.n, 0, ASRUtils::expr_type(args[0]), value);
        } else {
            return ASRUtils::make_IntrinsicElementalFunction_t_util(al, loc,
                static_cast<int64_t>(IntrinsicElementalFunctions::Min),
                args.p, args.n, 0, ASRUtils::expr_type(args[0]), nullptr);
        }
    }

    static inline ASR::expr_t* instantiate_Min(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_min0_" + type_to_str_python(arg_types[0]));
        int64_t kind = extract_kind_from_ttype_t(arg_types[0]);
        if (ASR::is_a<ASR::String_t>(*arg_types[0])) {
            for (size_t i = 0; i < new_args.size(); i++) {
                fill_func_arg("x" + std::to_string(i), ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, -1, nullptr, ASR::string_physical_typeType::PointerString)));
            }
            return_type = TYPE(ASR::make_String_t(al, loc, 1, -3, EXPR(ASR::make_StringLen_t(al, loc, args[0], int32, nullptr)), ASR::string_physical_typeType::PointerString));
        } else if (ASR::is_a<ASR::Real_t>(*arg_types[0])) {
            for (size_t i = 0; i < new_args.size(); i++) {
                fill_func_arg("x" + std::to_string(i), ASRUtils::TYPE(ASR::make_Real_t(al, loc, kind)));
            }
        } else if (ASR::is_a<ASR::Integer_t>(*arg_types[0])) {
            for (size_t i = 0; i < new_args.size(); i++) {
                fill_func_arg("x" + std::to_string(i), ASRUtils::TYPE(ASR::make_Integer_t(al, loc, kind)));
            }
        } else {
            throw LCompilersException("Arguments to min0 must be of real, integer or character type");
        }
        return_type = ASRUtils::extract_type(return_type);
        auto result = declare(fn_name, return_type, ReturnVar);
        body.push_back(al, b.Assignment(result, args[0]));
        if (ASR::is_a<ASR::Integer_t>(*return_type)) {
            for (size_t i = 1; i < args.size(); i++) {
                body.push_back(al, b.If(b.Lt(args[i], result), {
                    b.Assignment(result, args[i])
                }, {}));
            }
        } else if (ASR::is_a<ASR::Real_t>(*return_type)) {
            for (size_t i = 1; i < args.size(); i++) {
                body.push_back(al, b.If(b.Lt(args[i], result), {
                    b.Assignment(result, args[i])
                }, {}));
            }
        } else if (ASR::is_a<ASR::String_t>(*return_type)) {
            for (size_t i = 1; i < args.size(); i++) {
                body.push_back(al, b.If(b.Lt(args[i], result), {
                    b.Assignment(result, args[i])
                }, {}));
            }
            return_type = TYPE(ASR::make_String_t(al, loc, 1, -3, EXPR(ASR::make_StringLen_t(al, loc, new_args[0].m_value, int32, nullptr)), ASR::string_physical_typeType::PointerString));
        } else {
            throw LCompilersException("Arguments to min0 must be of real, integer or character type");
        }

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Min

namespace Partition {

    static inline ASR::expr_t* eval_Partition(Allocator &al, const Location &loc,
            std::string &s_var, std::string &sep) {
        /*
            using KMP algorithm to find separator inside string
            res_tuple: stores the resulting 3-tuple expression --->
            (if separator exist)           tuple:   (left of separator, separator, right of separator)
            (if separator does not exist)  tuple:   (string, "", "")
            res_tuple_type: stores the type of each expression present in resulting 3-tuple
        */
        ASRBuilder b(al, loc);
        int sep_pos = ASRUtils::KMP_string_match(s_var, sep);
        std::string first_res, second_res, third_res;
        if(sep_pos == -1) {
            /* seperator does not exist */
            first_res = s_var;
            second_res = "";
            third_res = "";
        } else {
            first_res = s_var.substr(0, sep_pos);
            second_res = sep;
            third_res = s_var.substr(sep_pos + sep.size());
        }

        Vec<ASR::expr_t *> res_tuple; res_tuple.reserve(al, 3);
        ASR::ttype_t *first_res_type = character(first_res.size());
        ASR::ttype_t *second_res_type = character(second_res.size());
        ASR::ttype_t *third_res_type = character(third_res.size());
        return b.TupleConstant({ b.StringConstant(first_res, first_res_type),
            b.StringConstant(second_res, second_res_type),
            b.StringConstant(third_res, third_res_type) },
            b.Tuple({first_res_type, second_res_type, third_res_type}));
    }

    static inline ASR::asr_t *create_partition(Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, ASR::expr_t *s_var,
            diag::Diagnostics& diag) {
        ASRBuilder b(al, loc);
        if (args.size() != 1) {
            append_error(diag, "str.partition() takes exactly one argument", loc);
            return nullptr;
        }
        ASR::expr_t *arg = args[0];
        if (!ASRUtils::is_character(*expr_type(arg))) {
            append_error(diag, "str.partition() takes one arguments of type: str", arg->base.loc);
            return nullptr;
        }

        Vec<ASR::expr_t *> e_args; e_args.reserve(al, 2);
        e_args.push_back(al, s_var);
        e_args.push_back(al, arg);

        ASR::ttype_t *return_type = b.Tuple({character(-2), character(-2), character(-2)});
        ASR::expr_t *value = nullptr;
        if (ASR::is_a<ASR::StringConstant_t>(*s_var)
         && ASR::is_a<ASR::StringConstant_t>(*arg)) {
            std::string s_sep = ASR::down_cast<ASR::StringConstant_t>(arg)->m_s;
            std::string s_str = ASR::down_cast<ASR::StringConstant_t>(s_var)->m_s;
            if (s_sep.size() == 0) {
                append_error(diag, "Separator cannot be an empty string", arg->base.loc);
                return nullptr;
            }
            value = eval_Partition(al, loc, s_str, s_sep);
        }

        return ASR::make_IntrinsicElementalFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicElementalFunctions::Partition),
            e_args.p, e_args.n, 0, return_type, value);
    }

    static inline ASR::expr_t *instantiate_Partition(Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*>& /*arg_types*/, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        // TODO: show runtime error for empty separator or pattern
        declare_basic_variables("_lpython_str_partition");
        fill_func_arg("target_string", character(-2));
        fill_func_arg("pattern", character(-2));

        auto result = declare("result", return_type, ReturnVar);
        auto index = declare("index", int32, Local);
        body.push_back(al, b.Assignment(index, b.Call(UnaryIntrinsicFunction::
            create_KMP_function(al, loc, scope), args, int32)));
        body.push_back(al, b.If(b.Eq(index, b.i_neg(b.i32(-1), int32)), {
                b.Assignment(result, b.TupleConstant({ args[0],
                    b.StringConstant("", character(0)),
                    b.StringConstant("", character(0)) },
                b.Tuple({character(-2), character(0), character(0)})))
            }, {
                b.Assignment(result, b.TupleConstant({
                    b.StringSection(args[0], b.i32(0), index), args[1],
                    b.StringSection(args[0], b.Add(index, b.StringLen(args[1])),
                        b.StringLen(args[0]))}, return_type))
            }));
        body.push_back(al, b.Return());
        ASR::symbol_t *fn_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, fn_sym);
        return b.Call(fn_sym, new_args, return_type, nullptr);
    }

} // namespace Partition

namespace Epsilon {

    static ASR::expr_t *eval_Epsilon(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& /*diag*/) {
        double epsilon_val = -1;
        ASRUtils::ASRBuilder b(al, loc);
        int32_t kind = extract_kind_from_ttype_t(arg_type);
        switch ( kind ) {
            case 4: {
                epsilon_val = std::numeric_limits<float>::epsilon(); break;
            } case 8: {
                epsilon_val = std::numeric_limits<double>::epsilon(); break;
            } default: {
                break;
            }
        }
        return b.f_t(epsilon_val, arg_type);
    }

}  // namespace Epsilon

namespace Precision {

    static ASR::expr_t *eval_Precision(Allocator &al, const Location &loc,
            ASR::ttype_t* /*return_type*/, Vec<ASR::expr_t*> &args, diag::Diagnostics& diag) {
        int64_t precision_val = -1;
        ASRUtils::ASRBuilder b(al, loc);
        ASR::ttype_t *arg_type = expr_type(args[0]);
        int32_t kind = extract_kind_from_ttype_t(arg_type);
        switch ( kind ) {
            case 4: {
                precision_val = 6; break;
            } case 8: {
                precision_val = 15; break;
            } default: {
                append_error(diag, "Kind " + std::to_string(kind) + " is not supported yet", loc);
                return nullptr;
            }
        }
        return b.i32(precision_val);
    }

}  // namespace Precision

namespace Tiny {

    static ASR::expr_t *eval_Tiny(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& diag) {
        double tiny_value = -1;
        ASRUtils::ASRBuilder b(al, loc);
        int32_t kind = extract_kind_from_ttype_t(arg_type);
        switch ( kind ) {
            case 4: {
                tiny_value = std::numeric_limits<float>::min(); break;
            } case 8: {
                tiny_value = std::numeric_limits<double>::min(); break;
            } default: {
                append_error(diag, "Kind " + std::to_string(kind) + " is not supported yet", loc);
                    return nullptr;
            }
        }
        return b.f_t(tiny_value, arg_type);
    }

}  // namespace Tiny

namespace Conjg {

    static ASR::expr_t *eval_Conjg(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args, diag::Diagnostics& /*diag*/) {
        std::complex<double> crv;
        if( extract_value(args[0], crv) ) {
            std::complex<double> val = std::conj(crv);
            return EXPR(ASR::make_ComplexConstant_t(
                al, loc, val.real(), val.imag(), arg_type));
        } else {
            return nullptr;
        }
    }

    static inline ASR::expr_t* instantiate_Conjg(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        std::string func_name = "_lcompilers_conjg_" + type_to_str_python(arg_types[0]);
        declare_basic_variables(func_name);
        if (scope->get_symbol(func_name)) {
            ASR::symbol_t *s = scope->get_symbol(func_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var), nullptr);
        }
        fill_func_arg("x", arg_types[0]);
        auto result = declare(fn_name, arg_types[0], ReturnVar);
        // * r = real(x) - aimag(x)*(0,1)

        body.push_back(al, b.Assignment(result, b.Sub(
            EXPR(ASR::make_Cast_t(al, loc, EXPR(ASR::make_ComplexRe_t(al, loc,
            args[0], TYPE(ASR::make_Real_t(al, loc, extract_kind_from_ttype_t(arg_types[0]))), nullptr)),
            ASR::cast_kindType::RealToComplex, arg_types[0], nullptr)),
            b.Mul(EXPR(ASR::make_Cast_t(al, loc, EXPR(ASR::make_ComplexIm_t(al, loc,
            args[0], TYPE(ASR::make_Real_t(al, loc, extract_kind_from_ttype_t(arg_types[0]))), nullptr)),
            ASR::cast_kindType::RealToComplex, arg_types[0], nullptr)), EXPR(ASR::make_ComplexConstant_t(al, loc,
            0.0, 1.0, arg_types[0]))))));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, ASRUtils::extract_type(return_type), nullptr);
    }

} // namespace Conjg

namespace Huge {

    static ASR::expr_t *eval_Huge(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& diag) {
        ASRUtils::ASRBuilder b(al, loc);
        int32_t kind = extract_kind_from_ttype_t(arg_type);
        if (ASR::is_a<ASR::Integer_t>(*arg_type)) {
            int64_t huge_value = -1;
            switch ( kind ) {
                case 1: {
                    huge_value = std::numeric_limits<int8_t>::max(); break;
                } case 2: {
                    huge_value = std::numeric_limits<int16_t>::max(); break;
                } case 4: {
                    huge_value = std::numeric_limits<int32_t>::max(); break;
                } case 8: {
                    huge_value = std::numeric_limits<int64_t>::max(); break;
                } default: {
                    append_error(diag, "Kind " + std::to_string(kind) + " is not supported yet", loc);
                    return nullptr;
                }
            }
            return b.i_t(huge_value, arg_type);
        } else {
            double huge_value = -1;
            switch ( kind ) {
                case 4: {
                    huge_value = std::numeric_limits<float>::max(); break;
                } case 8: {
                    huge_value = std::numeric_limits<double>::max(); break;
                } default: {
                    append_error(diag, "Kind " + std::to_string(kind) + " is not supported yet", loc);
                    return nullptr;
                }
            }
            return b.f_t(huge_value, arg_type);
        }
    }

}  // namespace Huge

namespace SymbolicSymbol {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        const Location& loc = x.base.base.loc;
        ASRUtils::require_impl(x.n_args == 1,
            "SymbolicSymbol intrinsic must have exactly 1 input argument",
            loc, diagnostics);

        ASR::ttype_t* input_type = ASRUtils::expr_type(x.m_args[0]);
        ASRUtils::require_impl(ASR::is_a<ASR::String_t>(*input_type),
            "SymbolicSymbol intrinsic expects a character input argument",
            loc, diagnostics);
    }

    static inline ASR::expr_t *eval_SymbolicSymbol(Allocator &/*al*/,
    const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/, diag::Diagnostics& /*diag*/) {
        // TODO
        return nullptr;
    }

    static inline ASR::asr_t* create_SymbolicSymbol(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            diag::Diagnostics& diag) {
        if (args.size() != 1) {
            append_error(diag, "Intrinsic Symbol function accepts exactly 1 argument", loc);
            return nullptr;
        }

        ASR::ttype_t *type = ASRUtils::expr_type(args[0]);
        if (!ASRUtils::is_character(*type)) {
            append_error(diag, "Argument of the Symbol function must be a String",
                args[0]->base.loc);
            return nullptr;
        }

        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args, eval_SymbolicSymbol,
            static_cast<int64_t>(IntrinsicElementalFunctions::SymbolicSymbol), 0, to_type, diag);
    }

} // namespace SymbolicSymbol

#define create_symbolic_binary_macro(X)                                                    \
namespace X{                                                                               \
    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x,             \
            diag::Diagnostics& diagnostics) {                                              \
        ASRUtils::require_impl(x.n_args == 2, "Intrinsic function `"#X"` accepts"          \
            "exactly 2 arguments", x.base.base.loc, diagnostics);                          \
                                                                                           \
        ASR::ttype_t* left_type = ASRUtils::expr_type(x.m_args[0]);                        \
        ASR::ttype_t* right_type = ASRUtils::expr_type(x.m_args[1]);                       \
                                                                                           \
        ASRUtils::require_impl(ASR::is_a<ASR::SymbolicExpression_t>(*left_type) &&         \
            ASR::is_a<ASR::SymbolicExpression_t>(*right_type),                             \
            "Both arguments of `"#X"` must be of type SymbolicExpression",                 \
            x.base.base.loc, diagnostics);                                                 \
    }                                                                                      \
                                                                                           \
    static inline ASR::expr_t* eval_##X(Allocator &/*al*/, const Location &/*loc*/,        \
            ASR::ttype_t *, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& /*diag*/) {    \
        /*TODO*/                                                                           \
        return nullptr;                                                                    \
    }                                                                                      \
                                                                                           \
    static inline ASR::asr_t* create_##X(Allocator& al, const Location& loc,               \
            Vec<ASR::expr_t*>& args,                                                       \
            diag::Diagnostics& diag) {                                                     \
        if (args.size() != 2) {                                                            \
            append_error(diag, "Intrinsic function `"#X"` accepts exactly 2 arguments",    \
                loc);                                                                      \
            return nullptr;                                                                \
        }                                                                                  \
                                                                                           \
        for (size_t i = 0; i < args.size(); i++) {                                         \
            ASR::ttype_t* argtype = ASRUtils::expr_type(args[i]);                          \
            if(!ASR::is_a<ASR::SymbolicExpression_t>(*argtype)) {                          \
                append_error(diag,                                                         \
                    "Arguments of `"#X"` function must be of type SymbolicExpression",     \
                    args[i]->base.loc);                                                    \
                return nullptr;                                                            \
            }                                                                              \
        }                                                                                  \
                                                                                           \
        Vec<ASR::expr_t*> arg_values;                                                      \
        arg_values.reserve(al, args.size());                                               \
        for( size_t i = 0; i < args.size(); i++ ) {                                        \
            arg_values.push_back(al, ASRUtils::expr_value(args[i]));                       \
        }                                                                                  \
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));   \
        ASR::expr_t* compile_time_value = eval_##X(al, loc, to_type, arg_values, diag);    \
        return ASR::make_IntrinsicElementalFunction_t(al, loc,                             \
                static_cast<int64_t>(IntrinsicElementalFunctions::X),                      \
                args.p, args.size(), 0, to_type, compile_time_value);                      \
    }                                                                                      \
} // namespace X

create_symbolic_binary_macro(SymbolicAdd)
create_symbolic_binary_macro(SymbolicSub)
create_symbolic_binary_macro(SymbolicMul)
create_symbolic_binary_macro(SymbolicDiv)
create_symbolic_binary_macro(SymbolicPow)
create_symbolic_binary_macro(SymbolicDiff)

#define create_symbolic_constants_macro(X)                                                \
namespace X {                                                                             \
    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x,            \
            diag::Diagnostics& diagnostics) {                                             \
        const Location& loc = x.base.base.loc;                                            \
        ASRUtils::require_impl(x.n_args == 0,                                             \
            #X " does not take arguments", loc, diagnostics);                             \
    }                                                                                     \
                                                                                          \
    static inline ASR::expr_t* eval_##X(Allocator &/*al*/, const Location &/*loc*/,       \
            ASR::ttype_t *, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& /*diag*/) {   \
        /*TODO*/                                                                          \
        return nullptr;                                                                   \
    }                                                                                     \
                                                                                          \
    static inline ASR::asr_t* create_##X(Allocator& al, const Location& loc,              \
            Vec<ASR::expr_t*>& args,                                                      \
            diag::Diagnostics& diag) {                                                    \
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));  \
        ASR::expr_t* compile_time_value = eval_##X(al, loc, to_type, args, diag);         \
        return ASR::make_IntrinsicElementalFunction_t(al, loc,                            \
                static_cast<int64_t>(IntrinsicElementalFunctions::X),                     \
                nullptr, 0, 0, to_type, compile_time_value);                              \
    }                                                                                     \
} // namespace X

create_symbolic_constants_macro(SymbolicPi)
create_symbolic_constants_macro(SymbolicE)

namespace SymbolicInteger {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 1,
            "SymbolicInteger intrinsic must have exactly 1 input argument",
            x.base.base.loc, diagnostics);

        ASR::ttype_t* input_type = ASRUtils::expr_type(x.m_args[0]);
        ASRUtils::require_impl(ASR::is_a<ASR::Integer_t>(*input_type),
            "SymbolicInteger intrinsic expects an integer input argument",
            x.base.base.loc, diagnostics);
    }

    static inline ASR::expr_t* eval_SymbolicInteger(Allocator &/*al*/,
    const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/, diag::Diagnostics& /*diag*/) {
        // TODO
        return nullptr;
    }

    static inline ASR::asr_t* create_SymbolicInteger(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            diag::Diagnostics& diag) {
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args, eval_SymbolicInteger,
            static_cast<int64_t>(IntrinsicElementalFunctions::SymbolicInteger), 0, to_type, diag);
    }

} // namespace SymbolicInteger

namespace SymbolicHasSymbolQ {
    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x,
        diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 2, "Intrinsic function SymbolicHasSymbolQ"
            "accepts exactly 2 arguments", x.base.base.loc, diagnostics);

        ASR::ttype_t* left_type = ASRUtils::expr_type(x.m_args[0]);
        ASR::ttype_t* right_type = ASRUtils::expr_type(x.m_args[1]);

        ASRUtils::require_impl(ASR::is_a<ASR::SymbolicExpression_t>(*left_type) &&
            ASR::is_a<ASR::SymbolicExpression_t>(*right_type),
            "Both arguments of SymbolicHasSymbolQ must be of type SymbolicExpression",
                x.base.base.loc, diagnostics);
    }

    static inline ASR::expr_t* eval_SymbolicHasSymbolQ(Allocator &/*al*/,
        const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& /*diag*/) {
        /*TODO*/
        return nullptr;
    }

    static inline ASR::asr_t* create_SymbolicHasSymbolQ(Allocator& al,
        const Location& loc, Vec<ASR::expr_t*>& args,
        diag::Diagnostics& diag) {

        if (args.size() != 2) {
            append_error(diag, "Intrinsic function SymbolicHasSymbolQ accepts exactly 2 arguments", loc);
            return nullptr;
        }

        for (size_t i = 0; i < args.size(); i++) {
            ASR::ttype_t* argtype = ASRUtils::expr_type(args[i]);
            if(!ASR::is_a<ASR::SymbolicExpression_t>(*argtype)) {
                append_error(diag, "Arguments of SymbolicHasSymbolQ function must be of type SymbolicExpression",
                    args[i]->base.loc);
                return nullptr;
            }
        }

        Vec<ASR::expr_t*> arg_values;
        arg_values.reserve(al, args.size());
        for( size_t i = 0; i < args.size(); i++ ) {
            arg_values.push_back(al, ASRUtils::expr_value(args[i]));
        }

        ASR::expr_t* compile_time_value = eval_SymbolicHasSymbolQ(al, loc, logical, arg_values, diag);
        return ASR::make_IntrinsicElementalFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicElementalFunctions::SymbolicHasSymbolQ),
            args.p, args.size(), 0, logical, compile_time_value);
    }
} // namespace SymbolicHasSymbolQ

namespace SymbolicGetArgument {
    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x,
        diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 2, "Intrinsic function SymbolicGetArgument"
            "accepts exactly 2 argument", x.base.base.loc, diagnostics);

        ASR::ttype_t* arg1_type = ASRUtils::expr_type(x.m_args[0]);
        ASR::ttype_t* arg2_type = ASRUtils::expr_type(x.m_args[1]);
        ASRUtils::require_impl(ASR::is_a<ASR::SymbolicExpression_t>(*arg1_type),
            "SymbolicGetArgument expects the first argument to be of type SymbolicExpression",
                x.base.base.loc, diagnostics);
        ASRUtils::require_impl(ASR::is_a<ASR::Integer_t>(*arg2_type),
            "SymbolicGetArgument expects the second argument to be of type Integer",
                x.base.base.loc, diagnostics);
    }

    static inline ASR::expr_t* eval_SymbolicGetArgument(Allocator &/*al*/,
        const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& /*diag*/) {
        /*TODO*/
        return nullptr;
    }

    static inline ASR::asr_t* create_SymbolicGetArgument(Allocator& al,
        const Location& loc, Vec<ASR::expr_t*>& args,
        diag::Diagnostics& diag) {

        if (args.size() != 2) {
            append_error(diag, "Intrinsic function SymbolicGetArguments accepts exactly 2 argument", loc);
            return nullptr;
        }

        ASR::ttype_t* arg1_type = ASRUtils::expr_type(args[0]);
        ASR::ttype_t* arg2_type = ASRUtils::expr_type(args[1]);
        if (!ASR::is_a<ASR::SymbolicExpression_t>(*arg1_type)) {
            append_error(diag, "The first argument of SymbolicGetArgument function must be of type SymbolicExpression",
                    args[0]->base.loc);
                return nullptr;
        }
        if (!ASR::is_a<ASR::Integer_t>(*arg2_type)) {
            append_error(diag, "The second argument of SymbolicGetArgument function must be of type Integer",
                    args[1]->base.loc);
                return nullptr;
        }

        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args, eval_SymbolicGetArgument,
            static_cast<int64_t>(IntrinsicElementalFunctions::SymbolicGetArgument),
            0, to_type, diag);
    }
} // namespace SymbolicGetArgument

#define create_symbolic_query_macro(X)                                                    \
namespace X {                                                                             \
    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x,            \
            diag::Diagnostics& diagnostics) {                                             \
        const Location& loc = x.base.base.loc;                                            \
        ASRUtils::require_impl(x.n_args == 1,                                             \
            #X " must have exactly 1 input argument", loc, diagnostics);                  \
                                                                                          \
        ASR::ttype_t* input_type = ASRUtils::expr_type(x.m_args[0]);                      \
        ASRUtils::require_impl(ASR::is_a<ASR::SymbolicExpression_t>(*input_type),         \
            #X " expects an argument of type SymbolicExpression", loc, diagnostics);      \
    }                                                                                     \
                                                                                          \
    static inline ASR::expr_t* eval_##X(Allocator &/*al*/, const Location &/*loc*/,       \
            ASR::ttype_t *, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& /*diag*/) {   \
        /*TODO*/                                                                          \
        return nullptr;                                                                   \
    }                                                                                     \
                                                                                          \
    static inline ASR::asr_t* create_##X(Allocator& al, const Location& loc,              \
            Vec<ASR::expr_t*>& args,                                                      \
            diag::Diagnostics& diag) {                                                    \
        if (args.size() != 1) {                                                           \
            append_error(diag, "Intrinsic " #X " function accepts exactly 1 argument",    \
                loc);                                                                     \
            return nullptr;                                                               \
        }                                                                                 \
                                                                                          \
        ASR::ttype_t* argtype = ASRUtils::expr_type(args[0]);                             \
        if (!ASR::is_a<ASR::SymbolicExpression_t>(*argtype)) {                            \
            append_error(diag,                                                            \
                "Argument of " #X " function must be of type SymbolicExpression",         \
                args[0]->base.loc);                                                       \
            return nullptr;                                                               \
        }                                                                                 \
                                                                                          \
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args, eval_##X,      \
            static_cast<int64_t>(IntrinsicElementalFunctions::X), 0, logical, diag);      \
    }                                                                                     \
} // namespace X

create_symbolic_query_macro(SymbolicAddQ)
create_symbolic_query_macro(SymbolicMulQ)
create_symbolic_query_macro(SymbolicPowQ)
create_symbolic_query_macro(SymbolicLogQ)
create_symbolic_query_macro(SymbolicSinQ)

#define create_symbolic_unary_macro(X)                                                    \
namespace X {                                                                             \
    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x,            \
            diag::Diagnostics& diagnostics) {                                             \
        const Location& loc = x.base.base.loc;                                            \
        ASRUtils::require_impl(x.n_args == 1,                                             \
            #X " must have exactly 1 input argument", loc, diagnostics);                  \
                                                                                          \
        ASR::ttype_t* input_type = ASRUtils::expr_type(x.m_args[0]);                      \
        ASRUtils::require_impl(ASR::is_a<ASR::SymbolicExpression_t>(*input_type),         \
            #X " expects an argument of type SymbolicExpression", loc, diagnostics);      \
    }                                                                                     \
                                                                                          \
    static inline ASR::expr_t* eval_##X(Allocator &/*al*/, const Location &/*loc*/,       \
            ASR::ttype_t *, Vec<ASR::expr_t*> &/*args*/, diag::Diagnostics& /*diag*/) {   \
        /*TODO*/                                                                          \
        return nullptr;                                                                   \
    }                                                                                     \
                                                                                          \
    static inline ASR::asr_t* create_##X(Allocator& al, const Location& loc,              \
            Vec<ASR::expr_t*>& args,                                                      \
            diag::Diagnostics& diag) {                                                    \
        if (args.size() != 1) {                                                           \
            append_error(diag, "Intrinsic " #X " function accepts exactly 1 argument",    \
                loc);                                                                     \
            return nullptr;                                                               \
        }                                                                                 \
                                                                                          \
        ASR::ttype_t* argtype = ASRUtils::expr_type(args[0]);                             \
        if (!ASR::is_a<ASR::SymbolicExpression_t>(*argtype)) {                            \
            append_error(diag,                                                            \
                "Argument of " #X " function must be of type SymbolicExpression",         \
                args[0]->base.loc);                                                       \
            return nullptr;                                                               \
        }                                                                                 \
                                                                                          \
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));  \
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args, eval_##X,      \
            static_cast<int64_t>(IntrinsicElementalFunctions::X), 0, to_type, diag);      \
    }                                                                                     \
} // namespace X

create_symbolic_unary_macro(SymbolicSin)
create_symbolic_unary_macro(SymbolicCos)
create_symbolic_unary_macro(SymbolicLog)
create_symbolic_unary_macro(SymbolicExp)
create_symbolic_unary_macro(SymbolicAbs)
create_symbolic_unary_macro(SymbolicExpand)

} // namespace LCompilers::ASRUtils

#endif // LIBASR_PASS_INTRINSIC_FUNCTIONS_H
