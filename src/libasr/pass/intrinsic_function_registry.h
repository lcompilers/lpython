#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/asr_utils.h>

#include <cmath>
#include <string>

namespace LCompilers {

namespace ASRUtils {

/*
To add a new function implementation,

1. Create a new namespace like, `Sin`, `LogGamma` in this file.
2. In the above created namespace add `eval_*`, `instantiate_*`, and `create_*`.
3. Then register in the maps present in `IntrinsicFunctionRegistry`.

You can use helper macros and define your own helper macros to reduce
the code size.
*/

typedef ASR::expr_t* (*impl_function)(
    Allocator&, const Location &,
    SymbolTable*, Vec<ASR::ttype_t*>&,
    Vec<ASR::call_arg_t>&, ASR::expr_t*);

typedef ASR::expr_t* (*eval_intrinsic_function)(
    Allocator&, const Location &,
    Vec<ASR::expr_t*>&);

typedef ASR::asr_t* (*create_intrinsic_function)(
    Allocator&, const Location&,
    Vec<ASR::expr_t*>&,
    const std::function<void (const std::string &, const Location &)>);

enum class IntrinsicFunctions : int64_t {
    Sin,
    Cos,
    Gamma,
    LogGamma,
    // ...
};

namespace UnaryIntrinsicFunction {

#define create_variable(var_sym, name, intent, abi, value_attr, symtab) \
    ASR::symbol_t *var_sym = ASR::down_cast<ASR::symbol_t>(ASR::make_Variable_t( \
        al, loc, symtab, s2c(al, name), nullptr, 0, intent, nullptr, nullptr, \
        ASR::storage_typeType::Default, arg_type, abi, ASR::Public, \
        ASR::presenceType::Required, value_attr)); \
    symtab->add_symbol(s2c(al, name), var_sym);

#define make_Function_t(name, symtab, dep, args, body, return_var, abi, deftype, bindc_name) \
    ASR::down_cast<ASR::symbol_t>( ASRUtils::make_Function_t_util(al, loc, \
    symtab, s2c(al, name), dep.p, dep.n, args.p, args.n, body.p, body.n, \
    ASRUtils::EXPR(ASR::make_Var_t(al, loc, return_var)), ASR::abiType::abi, \
    ASR::accessType::Public, ASR::deftypeType::deftype, bindc_name, false, \
    false, false, false, false, nullptr, 0, nullptr, 0, false, false, false));

static inline ASR::expr_t* instantiate_functions(Allocator &al,
        const Location &loc, SymbolTable *global_scope, std::string new_name,
        ASR::ttype_t *arg_type, Vec<ASR::call_arg_t>& new_args,
        ASR::expr_t *value) {
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
    new_name = "_lcompilers_" + new_name;
    // Check if Function is already defined.
    {
        std::string new_func_name = new_name;
        int i = 1;
        while (global_scope->get_symbol(new_func_name) != nullptr) {
            ASR::symbol_t *s = global_scope->get_symbol(new_func_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            if (ASRUtils::types_equal(ASRUtils::expr_type(f->m_return_var),
                    arg_type)) {
                return ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc, s,
                    s, new_args.p, new_args.size(), arg_type, value, nullptr));
            } else {
                new_func_name += std::to_string(i);
                i++;
            }
        }
    }
    new_name = global_scope->get_unique_name(new_name);
    SymbolTable *fn_symtab = al.make_new<SymbolTable>(global_scope);

    Vec<ASR::expr_t*> args;
    {
        args.reserve(al, 1);
        create_variable(arg, "x", ASR::intentType::In, ASR::abiType::Source,
            false, fn_symtab);
        args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg)));
    }

    create_variable(return_var, new_name, ASRUtils::intent_return_var,
        ASR::abiType::Source, false, fn_symtab);

    Vec<ASR::stmt_t*> body;
    body.reserve(al, 1);

    Vec<char *> dep;
    dep.reserve(al, 1);

    {
        SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);
        Vec<ASR::expr_t*> args_1;
        {
            args_1.reserve(al, 1);
            create_variable(arg, "x", ASR::intentType::In, ASR::abiType::BindC,
                true, fn_symtab_1);
            args_1.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc, arg)));
        }

        create_variable(return_var_1, c_func_name, ASRUtils::intent_return_var,
            ASR::abiType::BindC, false, fn_symtab_1);

        Vec<char *> dep_1; dep_1.reserve(al, 1);
        Vec<ASR::stmt_t*> body_1; body_1.reserve(al, 1);
        ASR::symbol_t *s = make_Function_t(c_func_name, fn_symtab_1, dep_1, args_1,
            body_1, return_var_1, BindC, Interface, s2c(al, c_func_name));
        fn_symtab->add_symbol(c_func_name, s);
        dep.push_back(al, s2c(al, c_func_name));
        Vec<ASR::call_arg_t> call_args;
        {
            call_args.reserve(al, 1);
            ASR::call_arg_t arg;
            arg.m_value = args[0];
            call_args.push_back(al, arg);
        }
        body.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, loc,
            ASRUtils::EXPR(ASR::make_Var_t(al, loc, return_var)),
            ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc, s, s,
            call_args.p, call_args.n, arg_type, nullptr, nullptr)), nullptr)));
    }

    ASR::symbol_t *new_symbol = make_Function_t(new_name, fn_symtab, dep, args,
        body, return_var, Source, Implementation, nullptr);
    global_scope->add_symbol(new_name, new_symbol);
    return ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc, new_symbol,
        new_symbol, new_args.p, new_args.size(), arg_type, value, nullptr));
}

static inline ASR::asr_t* create_UnaryFunction(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args, eval_intrinsic_function eval_function,
    int64_t intrinsic_id, int64_t overload_id, ASR::ttype_t* type) {
    ASR::expr_t *value = nullptr;
    ASR::expr_t *arg_value = ASRUtils::expr_value(args[0]);
    if (arg_value) {
        Vec<ASR::expr_t*> arg_values;
        arg_values.reserve(al, 1);
        arg_values.push_back(al, arg_value);
        value = eval_function(al, loc, arg_values);
    }

    return ASR::make_IntrinsicFunction_t(al, loc, intrinsic_id,
        args.p, args.n, overload_id, type, value);
}

} // namespace UnaryIntrinsicFunction

#define instantiate_UnaryFunctionArgs Allocator &al, const Location &loc,    \
    SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,    \
    Vec<ASR::call_arg_t>& new_args, ASR::expr_t* compile_time_value    \

#define instantiate_UnaryFunctionBody(Y)    \
    LCOMPILERS_ASSERT(arg_types.size() == 1);    \
    ASR::ttype_t* arg_type = arg_types[0];    \
    return UnaryIntrinsicFunction::instantiate_functions(    \
        al, loc, scope, #Y, arg_type, new_args,    \
        compile_time_value);    \

namespace LogGamma {

static inline ASR::expr_t *eval_log_gamma(Allocator &al, const Location &loc, Vec<ASR::expr_t*>& args) {
    double rv = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
    double val = lgamma(rv);
    ASR::ttype_t *t = ASRUtils::expr_type(args[0]);
    return ASRUtils::EXPR(ASR::make_RealConstant_t(al, loc, val, t));
}

static inline ASR::asr_t* create_LogGamma(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    ASR::ttype_t *type = ASRUtils::expr_type(args[0]);

    if (!ASRUtils::is_real(*type)) {
        err("`x` argument of `log_gamma` must be real",
            args[0]->base.loc);
    }

    return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args,
            eval_log_gamma, static_cast<int64_t>(ASRUtils::IntrinsicFunctions::LogGamma),
            0, type);
}

static inline ASR::expr_t* instantiate_LogGamma (instantiate_UnaryFunctionArgs) {
    instantiate_UnaryFunctionBody(log_gamma)
}

} // namespace LogGamma

// `X` is the name of the function in the IntrinsicFunctions enum and we use
// the same name for `create_X` and other places
// `stdeval` is the name of the function in the `std` namespace for compile
//  numerical time evaluation
// `lcompilers_name` is the name that we use in the C runtime library
#define create_trig(X, stdeval, lcompilers_name)                                 \
namespace X {                                                                    \
    static inline ASR::expr_t *eval_##X(Allocator &al,                           \
            const Location &loc, Vec<ASR::expr_t*>& args) {                      \
        LCOMPILERS_ASSERT(args.size() == 1);                                     \
        double rv;                                                               \
        ASR::ttype_t *t = ASRUtils::expr_type(args[0]);                          \
        if( ASRUtils::extract_value(args[0], rv) ) {                             \
            double val = std::stdeval(rv);                                       \
            return ASRUtils::EXPR(ASR::make_RealConstant_t(al, loc, val, t));    \
        } else {                                                                 \
            std::complex<double> crv;                                            \
            if( ASRUtils::extract_value(args[0], crv) ) {                        \
                std::complex<double> val = std::stdeval(crv);                    \
                return ASRUtils::EXPR(ASR::make_ComplexConstant_t(               \
                    al, loc, val.real(), val.imag(), t));                        \
            }                                                                    \
        }                                                                        \
        return nullptr;                                                          \
    }                                                                            \
    static inline ASR::asr_t* create_##X(Allocator& al, const Location& loc,     \
        Vec<ASR::expr_t*>& args,                                                 \
        const std::function<void (const std::string &, const Location &)> err)   \
    {                                                                            \
        ASR::ttype_t *type = ASRUtils::expr_type(args[0]);                       \
        if (!ASRUtils::is_real(*type) && !ASRUtils::is_complex(*type)) {         \
            err("`x` argument of `"#X"` must be real or complex",                \
                args[0]->base.loc);                                              \
        }                                                                        \
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args,       \
                eval_##X, static_cast<int64_t>(ASRUtils::IntrinsicFunctions::X), \
                0, type);                                                        \
    }                                                                            \
    static inline ASR::expr_t* instantiate_##X (Allocator &al,                   \
        const Location &loc, SymbolTable *scope,                                 \
        Vec<ASR::ttype_t*>& arg_types, Vec<ASR::call_arg_t>& new_args,           \
        ASR::expr_t* compile_time_value)                                         \
    {                                                                            \
        LCOMPILERS_ASSERT(arg_types.size() == 1);                                \
        ASR::ttype_t* arg_type = arg_types[0];                                   \
        return UnaryIntrinsicFunction::instantiate_functions(al, loc, scope,     \
            #lcompilers_name, arg_type, new_args, compile_time_value);           \
    }                                                                            \
} // namespace X

create_trig(Sin, sin, sin)
create_trig(Cos, cos, cos)

namespace IntrinsicFunctionRegistry {

    static const std::map<int64_t, impl_function>& intrinsic_function_by_id_db = {
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::LogGamma),
            &LogGamma::instantiate_LogGamma},

        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Sin),
            &Sin::instantiate_Sin},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Cos),
            &Cos::instantiate_Cos}
    };

    static const std::map<std::string,
        std::pair<create_intrinsic_function,
                    eval_intrinsic_function>>& intrinsic_function_by_name_db = {
                {"log_gamma", {&LogGamma::create_LogGamma, &LogGamma::eval_log_gamma}},
                {"sin", {&Sin::create_Sin, &Sin::eval_Sin}},
                {"cos", {&Cos::create_Cos, &Cos::eval_Cos}}
    };

    static inline bool is_intrinsic_function(const std::string& name) {
        return intrinsic_function_by_name_db.find(name) != intrinsic_function_by_name_db.end();
    }

    static inline bool is_intrinsic_function(int64_t id) {
        return intrinsic_function_by_id_db.find(id) != intrinsic_function_by_id_db.end();
    }

    static inline create_intrinsic_function get_create_function(const std::string& name) {
        return intrinsic_function_by_name_db.at(name).first;
    }

    static inline impl_function get_instantiate_function(int64_t id) {
        return intrinsic_function_by_id_db.at(id);
    }

} // namespace IntrinsicFunctionRegistry

#define INTRINSIC_NAME_CASE(X)                                         \
    case (static_cast<int64_t>(ASRUtils::IntrinsicFunctions::X)) : {   \
        return #X;                                                     \
    }

inline std::string get_intrinsic_name(int x) {
    switch (x) {
        INTRINSIC_NAME_CASE(Sin)
        INTRINSIC_NAME_CASE(Cos)
        INTRINSIC_NAME_CASE(Gamma)
        INTRINSIC_NAME_CASE(LogGamma)
        default : {
            throw LCompilersException("pickle: intrinsic_id not implemented");
        }
    }
}

} // namespace ASRUtils

} // namespace LCompilers
