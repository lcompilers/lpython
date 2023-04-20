#ifndef LFORTRAN_PASS_INTRINSIC_FUNCTION_REGISTRY_H
#define LFORTRAN_PASS_INTRINSIC_FUNCTION_REGISTRY_H

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/asr_utils.h>
#include <libasr/pass/pass_utils.h>

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
    Vec<ASR::call_arg_t>&, int64_t, ASR::expr_t*);

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
    Tan,
    Asin,
    Acos,
    Atan,
    Gamma,
    LogGamma,
    Abs,
    Any,
    ListIndex,
    Partition,
    // ...
};

// macro definitions
// Symbols
#define create_variable(var, name, intent, abi, value_attr, symtab, type)       \
    ASR::symbol_t* sym_##var = ASR::down_cast<ASR::symbol_t>(                   \
        ASR::make_Variable_t(al, loc, symtab, s2c(al, name), nullptr, 0,        \
        intent, nullptr, nullptr, ASR::storage_typeType::Default, type, abi,    \
        ASR::Public, ASR::presenceType::Required, value_attr));                 \
    symtab->add_symbol(s2c(al, name), sym_##var);                               \
    ASR::expr_t* var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, sym_##var));

#define Variable(var_name, type)                                                \
    create_variable(var_name, #var_name, ASR::intentType::Local,                \
        ASR::abiType::Source, false, fn_symtab, type)                           \

#define make_Function_t(name, symtab, dep, args, body, return_var, abi, deftype,\
        bindc_name)                                                             \
    ASR::down_cast<ASR::symbol_t>( ASRUtils::make_Function_t_util(al, loc,      \
    symtab, s2c(al, name), dep.p, dep.n, args.p, args.n, body.p, body.n,        \
    return_var, ASR::abiType::abi, ASR::accessType::Public,                     \
    ASR::deftypeType::deftype, bindc_name, false, false, false, false, false,   \
    nullptr, 0, nullptr, 0, false, false, false));

#define make_Function_Without_ReturnVar_t(name, symtab, dep, args, body, abi,   \
        deftype, bindc_name)                                                    \
    ASR::down_cast<ASR::symbol_t>( ASRUtils::make_Function_t_util(al, loc,      \
    symtab, s2c(al, name), dep.p, dep.n, args.p, args.n, body.p, body.n,        \
    nullptr, ASR::abiType::abi, ASR::accessType::Public,                        \
    ASR::deftypeType::deftype, bindc_name, false, false, false, false, false,   \
    nullptr, 0, nullptr, 0, false, false, false));

// Types
#define i_32   TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0))
#define str    TYPE(ASR::make_Character_t(al, loc, 1, -2, nullptr, nullptr, 0))

// Expressions
#define i32(x) EXPR(ASR::make_IntegerConstant_t(al, loc, x, i_32))
// macro definitions

namespace ControlFlowConstructorAPI {
    template <typename LOOP_BODY>
    ASR::stmt_t* create_do_loop(
        Allocator& al, const Location& loc,
        int rank, ASR::expr_t* array,
        SymbolTable* scope, Vec<ASR::expr_t*>& idx_vars,
        Vec<ASR::stmt_t*>& doloop_body, LOOP_BODY loop_body) {
        PassUtils::create_idx_vars(idx_vars, rank, loc, al, scope, "_i");

        ASR::stmt_t* doloop = nullptr;
        for( int i = (int) idx_vars.size() - 1; i >= 0; i-- ) {
            ASR::do_loop_head_t head;
            head.m_v = idx_vars[i];
            head.m_start = PassUtils::get_bound(array, i + 1, "lbound", al);
            head.m_end = PassUtils::get_bound(array, i + 1, "ubound", al);
            head.m_increment = nullptr;

            head.loc = head.m_v->base.loc;
            doloop_body.reserve(al, 1);
            if( doloop == nullptr ) {
                loop_body();
            } else {
                doloop_body.push_back(al, doloop);
            }
            doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr,
                        head, doloop_body.p, doloop_body.size()));
        }
        return doloop;
    }

    template <typename LOOP_BODY>
    ASR::stmt_t* create_do_loop(
        Allocator& al, const Location& loc,
        ASR::expr_t* array, ASR::expr_t* result,
        Vec<ASR::expr_t*>& loop_vars, std::vector<int>& loop_dims,
        Vec<ASR::stmt_t*>& doloop_body, LOOP_BODY loop_body) {

        if( result ) {

        }

        ASR::stmt_t* doloop = nullptr;
        for( int i = (int) loop_vars.size() - 1; i >= 0; i-- ) {
            ASR::do_loop_head_t head;
            head.m_v = loop_vars[i];
            head.m_start = PassUtils::get_bound(array, loop_dims[i], "lbound", al);
            head.m_end = PassUtils::get_bound(array, loop_dims[i], "ubound", al);
            head.m_increment = nullptr;

            head.loc = head.m_v->base.loc;
            doloop_body.reserve(al, 1);
            if( doloop == nullptr ) {
                loop_body();
            } else {
                doloop_body.push_back(al, doloop);
            }
            doloop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr,
                        head, doloop_body.p, doloop_body.size()));
        }
        return doloop;
    }
}

namespace UnaryIntrinsicFunction {

static inline ASR::expr_t* instantiate_functions(Allocator &al,
        const Location &loc, SymbolTable *global_scope, std::string new_name,
        ASR::ttype_t *arg_type, Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/,
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
    new_name = "_lcompilers_" + new_name + "_" + type_to_str_python(arg_type);

    if (global_scope->get_symbol(new_name)) {
        ASR::symbol_t *s = global_scope->get_symbol(new_name);
        ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
        return ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc, s, s,
            new_args.p, new_args.size(), expr_type(f->m_return_var),
            value, nullptr));
    }
    new_name = global_scope->get_unique_name(new_name);
    SymbolTable *fn_symtab = al.make_new<SymbolTable>(global_scope);

    Vec<ASR::expr_t*> args;
    {
        args.reserve(al, 1);
        create_variable(arg, "x", ASR::intentType::In, ASR::abiType::Source,
            false, fn_symtab, arg_type);
        args.push_back(al, arg);
    }

    create_variable(return_var, new_name, ASRUtils::intent_return_var,
        ASR::abiType::Source, false, fn_symtab, arg_type);

    Vec<ASR::stmt_t*> body;
    body.reserve(al, 1);

    SetChar dep;
    dep.reserve(al, 1);

    {
        SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);
        Vec<ASR::expr_t*> args_1;
        {
            args_1.reserve(al, 1);
            create_variable(arg, "x", ASR::intentType::In, ASR::abiType::BindC,
                true, fn_symtab_1, arg_type);
            args_1.push_back(al, arg);
        }

        create_variable(return_var_1, c_func_name, ASRUtils::intent_return_var,
            ASR::abiType::BindC, false, fn_symtab_1, arg_type);

        SetChar dep_1; dep_1.reserve(al, 1);
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
            return_var, ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc, s, s,
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

static inline ASR::symbol_t *create_KMP_function(Allocator &al,
        const Location &loc, SymbolTable *global_scope)
{
    /*
     * Knuth-Morris-Pratt (KMP) string-matching
     * This function takes two parameters:
     *     the sub-string or pattern string and the target string,
     * then returns the position of the first occurrence of the
     * string in the pattern.
     */
    std::string fn_name = global_scope->get_unique_name("KMP_string_matching");
    SymbolTable *fn_symtab = al.make_new<SymbolTable>(global_scope);

    Vec<ASR::expr_t*> args;
    {
        args.reserve(al, 2);
        create_variable(arg_1, "target_string", ASR::intentType::In,
            ASR::abiType::Source, false, fn_symtab, str);
        args.push_back(al, arg_1);
        create_variable(arg_2, "pattern", ASR::intentType::In,
            ASR::abiType::Source, false, fn_symtab, str);
        args.push_back(al, arg_2);
    }
    create_variable(return_var, "result", ASRUtils::intent_return_var,
        ASR::abiType::Source, false, fn_symtab, i_32);

    Vec<ASR::stmt_t*> body;
    body.reserve(al, 1);

    SetChar dep;
    dep.reserve(al, 1);
    {
        Variable(pi_len, i_32)
        Variable(i, i_32)
        Variable(j, i_32)
        Variable(s_len, i_32)
        Variable(pat_len, i_32)
        ASR::ttype_t *logical_type = TYPE(ASR::make_Logical_t(al, loc, 4,
            nullptr, 0));
        Variable(flag, logical_type)
        Variable(lps, TYPE(ASR::make_List_t(al, loc, i_32)))

        ASR::expr_t *result = EXPR(ASR::make_Var_t(al, loc, sym_return_var));

        body.push_back(al, STMT(ASR::make_Assignment_t(al, loc, s_len, EXPR(
            ASR::make_StringLen_t(al, loc, args[0], i_32, nullptr)), nullptr)));
        body.push_back(al, STMT(ASR::make_Assignment_t(al, loc, pat_len, EXPR(
            ASR::make_StringLen_t(al, loc, args[1], i_32, nullptr)), nullptr)));
        body.push_back(al, STMT(ASR::make_Assignment_t(al, loc, result,
            EXPR(ASR::make_IntegerUnaryMinus_t(al, loc, i32(-1), i_32,
            nullptr)), nullptr)));

        {
            Vec<ASR::stmt_t *> if_body_1; if_body_1.reserve(al, 1);
            Vec<ASR::stmt_t *> else_body; else_body.reserve(al, 1);
            ASR::expr_t *a_test_1 = EXPR(ASR::make_IntegerCompare_t(al, loc,
                pat_len, ASR::cmpopType::Eq, i32(0), logical_type, nullptr));
            if_body_1.push_back(al, STMT(ASR::make_Assignment_t(al, loc, result,
                i32(0), nullptr)));
            if_body_1.push_back(al, STMT(ASR::make_Return_t(al, loc)));
            ASR::expr_t *a_test_2 = EXPR(ASR::make_IntegerCompare_t(al, loc,
                s_len, ASR::cmpopType::Eq, i32(0), logical_type, nullptr));
            Vec<ASR::stmt_t *> if_body_2; if_body_2.reserve(al, 1);
            if_body_2.push_back(al, STMT(ASR::make_Return_t(al, loc)));
            else_body.push_back(al, STMT(ASR::make_If_t(al, loc, a_test_2,
                if_body_2.p, if_body_2.n, nullptr, 0)));
            body.push_back(al, STMT(ASR::make_If_t(al, loc, a_test_1,
                if_body_1.p, if_body_1.n, else_body.p, else_body.n)));
        }
        body.push_back(al, STMT(ASR::make_Assignment_t(al, loc, lps,
            EXPR(ASR::make_ListConstant_t(al, loc, nullptr, 0,
            TYPE(ASR::make_List_t(al, loc, i_32)))), nullptr)));
        {
            ASR::do_loop_head_t head;
            head.loc = loc;
            head.m_v = i;
            head.m_start = i32(0);
            head.m_end = EXPR(ASR::make_IntegerBinOp_t(al, loc, pat_len,
                ASR::binopType::Sub, i32(1), i_32, nullptr));
            head.m_increment = i32(1);
            Vec<ASR::stmt_t*> doloop_body; doloop_body.reserve(al, 1);
            doloop_body.push_back(al, STMT(ASR::make_ListAppend_t(al, loc, lps,
                i32(0))));
            body.push_back(al, STMT(ASR::make_DoLoop_t(al, loc, nullptr, head,
                doloop_body.p, doloop_body.n)));
        }
        body.push_back(al, STMT(ASR::make_Assignment_t(al, loc, flag,
            EXPR(ASR::make_LogicalConstant_t(al, loc, false,
            TYPE(ASR::make_Logical_t(al, loc, 4, nullptr, 0)))), nullptr)));
        body.push_back(al, STMT(ASR::make_Assignment_t(al, loc, i, i32(1), nullptr)));
        body.push_back(al, STMT(ASR::make_Assignment_t(al, loc, pi_len, i32(0), nullptr)));
        {
            ASR::expr_t *a_test = EXPR(ASR::make_IntegerCompare_t(al, loc, i,
                ASR::cmpopType::Lt, pat_len, logical_type, nullptr));
            Vec<ASR::stmt_t *>loop_body; loop_body.reserve(al, 1);
            {
                ASR::expr_t *a_test = EXPR(ASR::make_StringCompare_t(al, loc,
                    EXPR(ASR::make_StringItem_t(al, loc, args[1],
                        EXPR(ASR::make_IntegerBinOp_t(al, loc, i,
                            ASR::binopType::Add,
                            i32(1),
                            i_32, nullptr)),
                        str, nullptr)),
                        ASR::cmpopType::Eq,
                        EXPR(ASR::make_StringItem_t(al, loc, args[1],
                            EXPR(ASR::make_IntegerBinOp_t(al, loc, pi_len,
                                ASR::binopType::Add,
                                i32(1),
                            i_32, nullptr)),
                        str, nullptr)),
                    logical_type, nullptr));
                Vec<ASR::stmt_t *> if_body_1; if_body_1.reserve(al, 1);
                if_body_1.push_back(al, STMT(ASR::make_Assignment_t(al, loc,
                    pi_len, EXPR(ASR::make_IntegerBinOp_t(al, loc, pi_len,
                    ASR::binopType::Add, i32(1), i_32, nullptr)), nullptr)));
                if_body_1.push_back(al, STMT(ASR::make_Assignment_t(al, loc,
                    EXPR(ASR::make_ListItem_t(al, loc, lps, i, i_32, nullptr)),
                    pi_len, nullptr)));
                if_body_1.push_back(al, STMT(ASR::make_Assignment_t(al, loc, i,
                    EXPR(ASR::make_IntegerBinOp_t(al, loc, i, ASR::binopType::Add,
                    i32(1),
                    i_32, nullptr)), nullptr)));
                Vec<ASR::stmt_t *> else_body_1; else_body_1.reserve(al, 1);
                {
                    Vec<ASR::stmt_t *> if_body_2; if_body_2.reserve(al, 1);
                    Vec<ASR::stmt_t *> else_body_2; else_body_2.reserve(al, 1);
                    ASR::expr_t *a_test = EXPR(ASR::make_IntegerCompare_t(al, loc,
                        pi_len, ASR::cmpopType::NotEq,
                        i32(0),
                        logical_type, nullptr));
                    if_body_2.push_back(al, STMT(ASR::make_Assignment_t(al, loc,
                        pi_len, EXPR(ASR::make_ListItem_t(al, loc, lps,
                            EXPR(ASR::make_IntegerBinOp_t(al, loc, pi_len,
                                ASR::binopType::Sub,
                                i32(1),
                                i_32, nullptr)),
                            i_32, nullptr)), nullptr)));
                    else_body_2.push_back(al, STMT(ASR::make_Assignment_t(al, loc,
                        i, EXPR(ASR::make_IntegerBinOp_t(al, loc, i,
                            ASR::binopType::Add,
                            i32(1),
                            i_32, nullptr)), nullptr)));
                    else_body_1.push_back(al, STMT(ASR::make_If_t(al, loc, a_test,
                        if_body_2.p, if_body_2.n, else_body_2.p, else_body_2.n)));
                }
                loop_body.push_back(al, STMT(ASR::make_If_t(al, loc, a_test,
                    if_body_1.p, if_body_1.n, else_body_1.p, else_body_1.n)));
            }

            body.push_back(al, STMT(ASR::make_WhileLoop_t(al, loc, nullptr, a_test,
                loop_body.p, loop_body.n)));
        }
        body.push_back(al, STMT(ASR::make_Assignment_t(al, loc, j, i32(0), nullptr)));
        body.push_back(al, STMT(ASR::make_Assignment_t(al, loc, i, i32(0), nullptr)));
        {
            ASR::expr_t *a_test = EXPR(ASR::make_LogicalBinOp_t(al, loc,
                EXPR(ASR::make_IntegerCompare_t(al, loc,
                    EXPR(ASR::make_IntegerBinOp_t(al, loc, s_len,
                        ASR::binopType::Sub, i, i_32, nullptr)),
                    ASR::cmpopType::GtE,
                    EXPR(ASR::make_IntegerBinOp_t(al, loc, pat_len,
                        ASR::binopType::Sub, j, i_32, nullptr)),
                    logical_type, nullptr)),
                ASR::logicalbinopType::And,
                EXPR(ASR::make_LogicalNot_t(al, loc, flag, logical_type, nullptr)),
                logical_type, nullptr));
            Vec<ASR::stmt_t *>loop_body; loop_body.reserve(al, 1);
            {
                ASR::expr_t *a_test = EXPR(ASR::make_StringCompare_t(al, loc,
                    EXPR(ASR::make_StringItem_t(al, loc, args[1],
                        EXPR(ASR::make_IntegerBinOp_t(al, loc, j,
                            ASR::binopType::Add,
                            i32(1),
                            i_32, nullptr)),
                        str, nullptr)),
                        ASR::cmpopType::Eq,
                        EXPR(ASR::make_StringItem_t(al, loc, args[0],
                            EXPR(ASR::make_IntegerBinOp_t(al, loc, i,
                                ASR::binopType::Add,
                                i32(1),
                            i_32, nullptr)),
                        str, nullptr)),
                    logical_type, nullptr));
                Vec<ASR::stmt_t *> if_body_1; if_body_1.reserve(al, 1);
                if_body_1.push_back(al, STMT(ASR::make_Assignment_t(al, loc,
                    i, EXPR(ASR::make_IntegerBinOp_t(al, loc, i,
                    ASR::binopType::Add, i32(1), i_32, nullptr)), nullptr)));
                if_body_1.push_back(al, STMT(ASR::make_Assignment_t(al, loc, j,
                    EXPR(ASR::make_IntegerBinOp_t(al, loc, j, ASR::binopType::Add,
                    i32(1),
                    i_32, nullptr)), nullptr)));
                loop_body.push_back(al, STMT(ASR::make_If_t(al, loc, a_test,
                    if_body_1.p, if_body_1.n, nullptr, 0)));

                a_test = EXPR(ASR::make_IntegerCompare_t(al, loc, j,
                    ASR::cmpopType::Eq, pat_len, logical_type, nullptr));
                if_body_1.p = nullptr; if_body_1.n = 0;
                if_body_1.reserve(al, 1);
                if_body_1.push_back(al, STMT(ASR::make_Assignment_t(al, loc,
                    result, EXPR(ASR::make_IntegerBinOp_t(al, loc, i,
                    ASR::binopType::Sub, j, i_32, nullptr)), nullptr)));
                if_body_1.push_back(al, STMT(ASR::make_Assignment_t(al, loc,
                    flag, EXPR(ASR::make_LogicalConstant_t(al, loc, true,
                    logical_type)), nullptr)));
                if_body_1.push_back(al, STMT(ASR::make_Assignment_t(al, loc, j,
                    EXPR(ASR::make_ListItem_t(al, loc, lps,
                        EXPR(ASR::make_IntegerBinOp_t(al, loc, j,
                            ASR::binopType::Sub,
                            i32(1),
                        i_32, nullptr)),
                    i_32, nullptr)), nullptr)));
                Vec<ASR::stmt_t *> else_body_1; else_body_1.reserve(al, 1);
                {
                    Vec<ASR::stmt_t *> if_body_2; if_body_2.reserve(al, 1);
                    ASR::expr_t *a_test = EXPR(ASR::make_LogicalBinOp_t(al, loc,
                        EXPR(ASR::make_IntegerCompare_t(al, loc, i,
                            ASR::cmpopType::Lt, s_len, logical_type, nullptr)),
                        ASR::logicalbinopType::And,
                        EXPR(ASR::make_StringCompare_t(al, loc,
                            EXPR(ASR::make_StringItem_t(al, loc, args[1],
                                EXPR(ASR::make_IntegerBinOp_t(al, loc, j,
                                ASR::binopType::Add,
                                i32(1),
                                i_32, nullptr)),
                            str, nullptr)),
                            ASR::cmpopType::NotEq,
                            EXPR(ASR::make_StringItem_t(al, loc, args[0],
                                EXPR(ASR::make_IntegerBinOp_t(al, loc, i,
                                    ASR::binopType::Add,
                                    i32(1),
                                    i_32, nullptr)),
                                str, nullptr)),
                            logical_type, nullptr)),
                        logical_type, nullptr));
                    {
                        Vec<ASR::stmt_t *> if_body_3; if_body_3.reserve(al, 1);
                        Vec<ASR::stmt_t *> else_body_3; else_body_3.reserve(al, 1);
                        ASR::expr_t *a_test = EXPR(ASR::make_IntegerCompare_t(al,
                            loc, j, ASR::cmpopType::NotEq,
                            i32(0),
                            logical_type, nullptr));
                        if_body_3.push_back(al, STMT(ASR::make_Assignment_t(al,
                            loc, j, EXPR(ASR::make_ListItem_t(al, loc, lps,
                                EXPR(ASR::make_IntegerBinOp_t(al, loc, j,
                                    ASR::binopType::Sub,
                                    i32(1),
                                    i_32, nullptr)),
                                i_32, nullptr)), nullptr)));
                        else_body_3.push_back(al, STMT(ASR::make_Assignment_t(al,
                            loc, i, EXPR(ASR::make_IntegerBinOp_t(al, loc, i,
                                ASR::binopType::Add,
                                i32(1),
                                i_32, nullptr)), nullptr)));
                        if_body_2.push_back(al, STMT(ASR::make_If_t(al, loc, a_test,
                            if_body_3.p, if_body_3.n, else_body_3.p, else_body_3.n)));
                    }
                    else_body_1.push_back(al, STMT(ASR::make_If_t(al, loc, a_test,
                        if_body_2.p, if_body_2.n, nullptr, 0)));
                }
                loop_body.push_back(al, STMT(ASR::make_If_t(al, loc, a_test,
                    if_body_1.p, if_body_1.n, else_body_1.p, else_body_1.n)));
            }

            body.push_back(al, STMT(ASR::make_WhileLoop_t(al, loc, nullptr, a_test,
                loop_body.p, loop_body.n)));
        }
        body.push_back(al, STMT(ASR::make_Return_t(al, loc)));

    }
    ASR::symbol_t *fn_sym = make_Function_t(fn_name, fn_symtab, dep, args,
        body, return_var, Source, Implementation, nullptr);
    global_scope->add_symbol(fn_name, fn_sym);
    return fn_sym;
}

} // namespace UnaryIntrinsicFunction

#define instantiate_UnaryFunctionArgs Allocator &al, const Location &loc,    \
    SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,    \
    Vec<ASR::call_arg_t>& new_args, int64_t overload_id, ASR::expr_t* compile_time_value    \

#define instantiate_UnaryFunctionBody(Y)    \
    LCOMPILERS_ASSERT(arg_types.size() == 1);    \
    ASR::ttype_t* arg_type = arg_types[0];    \
    return UnaryIntrinsicFunction::instantiate_functions(    \
        al, loc, scope, #Y, arg_type, new_args, overload_id,    \
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
#define create_trig(X, stdeval, lcompilers_name)                                \
namespace X {                                                                   \
    static inline ASR::expr_t *eval_##X(Allocator &al,                          \
            const Location &loc, Vec<ASR::expr_t*>& args) {                     \
        LCOMPILERS_ASSERT(args.size() == 1);                                    \
        double rv;                                                              \
        ASR::ttype_t *t = ASRUtils::expr_type(args[0]);                         \
        if( ASRUtils::extract_value(args[0], rv) ) {                            \
            double val = std::stdeval(rv);                                      \
            return ASRUtils::EXPR(ASR::make_RealConstant_t(al, loc, val, t));   \
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
    static inline ASR::asr_t* create_##X(Allocator& al, const Location& loc,    \
        Vec<ASR::expr_t*>& args,                                                \
        const std::function<void (const std::string &, const Location &)> err)  \
    {                                                                           \
        ASR::ttype_t *type = ASRUtils::expr_type(args[0]);                      \
        if (!ASRUtils::is_real(*type) && !ASRUtils::is_complex(*type)) {        \
            err("`x` argument of `"#X"` must be real or complex",               \
                args[0]->base.loc);                                             \
        }                                                                       \
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args,      \
                eval_##X, static_cast<int64_t>(ASRUtils::IntrinsicFunctions::X),\
                0, type);                                                       \
    }                                                                           \
    static inline ASR::expr_t* instantiate_##X (Allocator &al,                  \
        const Location &loc, SymbolTable *scope,                                \
        Vec<ASR::ttype_t*>& arg_types, Vec<ASR::call_arg_t>& new_args,          \
         int64_t overload_id, ASR::expr_t* compile_time_value)                  \
    {                                                                           \
        LCOMPILERS_ASSERT(arg_types.size() == 1);                               \
        ASR::ttype_t* arg_type = arg_types[0];                                  \
        return UnaryIntrinsicFunction::instantiate_functions(al, loc, scope,    \
            #lcompilers_name, arg_type, new_args, overload_id,                  \
            compile_time_value);                                                \
    }                                                                           \
} // namespace X

create_trig(Sin, sin, sin)
create_trig(Cos, cos, cos)
create_trig(Tan, tan, tan)
create_trig(Asin, asin, asin)
create_trig(Acos, acos, acos)
create_trig(Atan, atan, atan)

namespace Abs {

    static inline ASR::expr_t *eval_Abs(Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::expr_t* arg = args[0];
        ASR::ttype_t* t = ASRUtils::expr_type(args[0]);
        if (ASRUtils::is_real(*t)) {
            double rv = ASR::down_cast<ASR::RealConstant_t>(arg)->m_r;
            double val = std::abs(rv);
            return ASR::down_cast<ASR::expr_t>(ASR::make_RealConstant_t(
                al, loc, val, t));
        } else if (ASRUtils::is_integer(*t)) {
            int64_t rv = ASR::down_cast<ASR::IntegerConstant_t>(arg)->m_n;
            int64_t val = std::abs(rv);
            return ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                al, loc, val, t));
        } else if (ASRUtils::is_complex(*t)) {
            double re = ASR::down_cast<ASR::ComplexConstant_t>(arg)->m_re;
            double im = ASR::down_cast<ASR::ComplexConstant_t>(arg)->m_im;
            std::complex<double> x(re, im);
            double result = std::abs(x);
            ASR::ttype_t *real_type = TYPE(ASR::make_Real_t(al, t->base.loc,
                ASRUtils::extract_kind_from_ttype_t(t), nullptr, 0));
            return ASR::down_cast<ASR::expr_t>(ASR::make_RealConstant_t(
                al, loc, result, real_type));
        } else {
            return nullptr;
        }
    }

    static inline ASR::asr_t* create_Abs(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        if (args.size() != 1) {
            err("Intrinsic abs function accepts exactly 1 argument", loc);
        }
        ASR::ttype_t *type = ASRUtils::expr_type(args[0]);
        if (!ASRUtils::is_integer(*type) && !ASRUtils::is_real(*type)
                && !ASRUtils::is_complex(*type)) {
            err("Argument of the abs function must be Integer, Real or Complex",
                args[0]->base.loc);
        }
        if (is_complex(*type)) {
            type = TYPE(ASR::make_Real_t(al, type->base.loc,
                ASRUtils::extract_kind_from_ttype_t(type), nullptr, 0));
        }
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args, eval_Abs,
            static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Abs), 0, type);
    }

    static inline ASR::expr_t* instantiate_Abs(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/, ASR::expr_t* compile_time_value) {
        std::string func_name = "_lcompilers_abs_" + type_to_str_python(arg_types[0]);
        ASR::ttype_t *return_type = arg_types[0];
        if (scope->get_symbol(func_name)) {
            ASR::symbol_t *s = scope->get_symbol(func_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc, s, s,
                new_args.p, new_args.size(), expr_type(f->m_return_var),
                compile_time_value, nullptr));
        }

        func_name = scope->get_unique_name(func_name);
        SymbolTable *fn_symtab = al.make_new<SymbolTable>(scope);

        Vec<ASR::expr_t*> args;
        {
            args.reserve(al, 1);
            create_variable(arg, "x", ASR::intentType::In, ASR::abiType::Source,
                false, fn_symtab, arg_types[0]);
            args.push_back(al, arg);
        }

        create_variable(return_var, func_name, ASRUtils::intent_return_var,
            ASR::abiType::Source, false, fn_symtab, return_type);

        Vec<ASR::stmt_t*> body;
        body.reserve(al, 1);
        SetChar dep;
        dep.reserve(al, 1);

        if (is_integer(*arg_types[0]) || is_real(*arg_types[0])) {
            /*
             * if (x >= 0) then
             *     r = x
             * else
             *     r = -x
             * end if
             */
            ASR::expr_t *test;
            ASR::expr_t *negative_x;
            if (is_integer(*arg_types[0])) {
                test = EXPR(ASR::make_IntegerCompare_t(al, loc, args[0],
                    ASR::cmpopType::GtE, EXPR(ASR::make_IntegerConstant_t(al,
                    loc, 0, arg_types[0])), arg_types[0], nullptr));
                negative_x = EXPR(ASR::make_IntegerUnaryMinus_t(al, loc, args[0],
                    arg_types[0], nullptr));
            } else {
                test = EXPR(ASR::make_RealCompare_t(al, loc, args[0],
                    ASR::cmpopType::GtE, EXPR(ASR::make_RealConstant_t(al,
                    loc, 0.0, arg_types[0])), arg_types[0], nullptr));
                negative_x = EXPR(ASR::make_RealUnaryMinus_t(al, loc, args[0],
                    arg_types[0], nullptr));
            }

            Vec<ASR::stmt_t *> if_body; if_body.reserve(al, 1);
            if_body.push_back(al, STMT(ASR::make_Assignment_t(al, loc,
                return_var, args[0], nullptr)));
            Vec<ASR::stmt_t *> else_body; else_body.reserve(al, 1);
            else_body.push_back(al, STMT(ASR::make_Assignment_t(al, loc,
                return_var, negative_x, nullptr)));
            body.push_back(al, STMT(ASR::make_If_t(al, loc, test,
                if_body.p, if_body.n, else_body.p, else_body.n)));
        } else {
            // * Complex type: `r = (real(x)**2 + aimag(x)**2)**0.5`
            ASR::ttype_t *real_type = TYPE(ASR::make_Real_t(al, loc,
                extract_kind_from_ttype_t(arg_types[0]), nullptr, 0));
            ASR::Variable_t *r_var = ASR::down_cast<ASR::Variable_t>(sym_return_var);
            r_var->m_type = return_type = real_type;
            ASR::expr_t *aimag_of_x;
            {
                std::string c_func_name;
                if (ASRUtils::extract_kind_from_ttype_t(arg_types[0]) == 4) {
                    c_func_name = "_lfortran_caimag";
                } else {
                    c_func_name = "_lfortran_zaimag";
                }
                SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);
                Vec<ASR::expr_t*> args_1;
                {
                    args_1.reserve(al, 1);
                    create_variable(arg, "x", ASR::intentType::In, ASR::abiType::BindC,
                        true, fn_symtab_1, arg_types[0]);
                    args_1.push_back(al, arg);
                }

                create_variable(return_var_1, c_func_name, ASRUtils::intent_return_var,
                    ASR::abiType::BindC, false, fn_symtab_1, real_type);

                SetChar dep_1; dep_1.reserve(al, 1);
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
                aimag_of_x = EXPR(ASR::make_FunctionCall_t(al, loc, s,
                    s, call_args.p, call_args.n, real_type, nullptr, nullptr));
            }
            ASR::expr_t *constant_two = EXPR(ASR::make_RealConstant_t(al, loc,
                2.0, real_type));
            ASR::expr_t *constant_point_five = EXPR(ASR::make_RealConstant_t(
                al, loc, 0.5, real_type));
            ASR::expr_t *real_of_x = EXPR(ASR::make_Cast_t(al, loc, args[0],
                ASR::cast_kindType::ComplexToReal, real_type, nullptr));

            ASR::expr_t *bin_op_1 = EXPR(ASR::make_RealBinOp_t(al, loc, real_of_x,
                ASR::binopType::Pow, constant_two, real_type, nullptr));
            ASR::expr_t *bin_op_2 = EXPR(ASR::make_RealBinOp_t(al, loc, aimag_of_x,
                ASR::binopType::Pow, constant_two, real_type, nullptr));

            bin_op_1 = EXPR(ASR::make_RealBinOp_t(al, loc, bin_op_1,
                ASR::binopType::Add, bin_op_2, real_type, nullptr));

            body.push_back(al, STMT(ASR::make_Assignment_t(al, loc,
                return_var, EXPR(ASR::make_RealBinOp_t(al, loc, bin_op_1,
                    ASR::binopType::Pow, constant_point_five, real_type, nullptr)),
                nullptr)));
        }

        ASR::symbol_t *f_sym = make_Function_t(func_name, fn_symtab, dep, args,
            body, return_var, Source, Implementation, nullptr);
        scope->add_symbol(func_name, f_sym);
        return ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc, f_sym, f_sym,
            new_args.p, new_args.size(), return_type, compile_time_value, nullptr));
    }

} // namespace Abs

namespace ListIndex {

static inline ASR::expr_t *eval_list_index(Allocator &/*al*/,
    const Location &/*loc*/, Vec<ASR::expr_t*>& /*args*/) {
    // TODO: To be implemented for ListConstant expression
    return nullptr;
}

static inline ASR::asr_t* create_ListIndex(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    if (args.size() != 2) {
        // Support start and end arguments by overloading ListIndex
        // intrinsic. We need 3 overload IDs,
        // 0 - only list and element
        // 1 - list, element and start
        // 2 - list, element, start and end
        // list, element and end case is not possible as list.index
        // doesn't accept keyword arguments
        err("For now index() takes exactly one argument", loc);
    }

    ASR::expr_t* list_expr = args[0];
    ASR::ttype_t *type = ASRUtils::expr_type(list_expr);
    ASR::ttype_t *list_type = ASR::down_cast<ASR::List_t>(type)->m_type;
    ASR::ttype_t *ele_type = ASRUtils::expr_type(args[1]);
    if (!ASRUtils::check_equal_type(ele_type, list_type)) {
        std::string fnd = ASRUtils::get_type_code(ele_type);
        std::string org = ASRUtils::get_type_code(list_type);
        err(
            "Type mismatch in 'index', the types must be compatible "
            "(found: '" + fnd + "', expected: '" + org + "')", loc);
    }
    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, args.size());
    for( size_t i = 0; i < args.size(); i++ ) {
        arg_values.push_back(al, ASRUtils::expr_value(args[i]));
    }
    ASR::expr_t* compile_time_value = eval_list_index(al, loc, arg_values);
    ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
                            4, nullptr, 0));
    return ASR::make_IntrinsicFunction_t(al, loc,
            static_cast<int64_t>(ASRUtils::IntrinsicFunctions::ListIndex),
            args.p, args.size(), 0, to_type, compile_time_value);
}

} // namespace ListIndex

namespace Any {

static inline ASR::expr_t *eval_Any(Allocator & /*al*/,
    const Location & /*loc*/, Vec<ASR::expr_t*>& /*args*/) {
    return nullptr;
}

static inline ASR::asr_t* create_Any(
    Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    int64_t overload_id = 0;
    if(args.size() != 1 && args.size() != 2) {
        err("any intrinsic accepts a maximum of two arguments, found "
                    + std::to_string(args.size()), loc);
    }

    ASR::expr_t* array = args[0];
    if( ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(array)) == 0 ) {
        err("mask argument to any must be an array and must not be a scalar",
            args[0]->base.loc);
    }

    // TODO: Add a check for range of values axis can take
    // if axis is available at compile time

    ASR::expr_t *value = nullptr;
    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, 2);
    ASR::expr_t *array_value = ASRUtils::expr_value(array);
    arg_values.push_back(al, array_value);
    if( args.size() == 2 ) {
        ASR::expr_t* axis = args[1];
        ASR::expr_t *axis_value = ASRUtils::expr_value(axis);
        arg_values.push_back(al, axis_value);
    }
    value = eval_Any(al, loc, arg_values);

    ASR::ttype_t* logical_return_type = nullptr;
    if( args.size() == 1 ) {
        overload_id = 0;
        logical_return_type = ASRUtils::TYPE(ASR::make_Logical_t(
                                al, loc, 4, nullptr, 0));
    } else if( args.size() == 2 ) {
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
        logical_return_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4, dims.p, dims.size()));
    }

    return ASR::make_IntrinsicFunction_t(al, loc,
        static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Any),
        args.p, args.n, overload_id, logical_return_type, value);
}

static inline void generate_body_for_scalar_output(Allocator& al, const Location& loc,
    ASR::expr_t* array, ASR::expr_t* return_var, SymbolTable* fn_scope,
    Vec<ASR::stmt_t*>& fn_body) {
    ASR::expr_t* logical_false = ASRUtils::EXPR(ASR::make_LogicalConstant_t(
                                    al, loc, false, ASRUtils::expr_type(array)));
    ASR::stmt_t* return_var_init = ASRUtils::STMT(ASR::make_Assignment_t(al, loc,
                                        return_var, logical_false, nullptr));
    fn_body.push_back(al, return_var_init);
    int rank = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(array));
    Vec<ASR::expr_t*> idx_vars;
    idx_vars.reserve(al, 1);
    Vec<ASR::stmt_t*> doloop_body;
    doloop_body.reserve(al, 1);
    ASR::stmt_t* doloop = ControlFlowConstructorAPI::create_do_loop(al, loc,
        rank, array, fn_scope, idx_vars, doloop_body,
    [=, &al, &idx_vars, &doloop_body] () {
        ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
        ASR::expr_t* logical_or = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al, loc,
            return_var, ASR::Or, array_ref, ASRUtils::expr_type(return_var), nullptr));
        ASR::stmt_t* loop_invariant = ASRUtils::STMT(ASR::make_Assignment_t(al, loc,
                                        return_var, logical_or, nullptr));
        doloop_body.push_back(al, loop_invariant);
    });
    fn_body.push_back(al, doloop);
}

static inline void generate_body_for_array_output(Allocator& al, const Location& loc,
    ASR::expr_t* array, ASR::expr_t* dim, ASR::expr_t* result,
    SymbolTable* fn_scope, Vec<ASR::stmt_t*>& fn_body) {
    ASR::expr_t* logical_false = ASRUtils::EXPR(ASR::make_LogicalConstant_t(
                                    al, loc, false, ASRUtils::TYPE(ASR::make_Logical_t(
                                        al, loc, 4, nullptr, 0))));
    ASR::stmt_t* result_init = ASRUtils::STMT(ASR::make_Assignment_t(al, loc,
                                        result, logical_false, nullptr));
    fn_body.push_back(al, result_init);

    int n_dims = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(array));
    ASR::stmt_t** else_ = nullptr;
    size_t else_n = 0;
    Vec<ASR::expr_t*> idx_vars;
    idx_vars.reserve(al, n_dims);
    PassUtils::create_idx_vars(idx_vars, n_dims, loc, al, fn_scope, "_j");
    for( int i = 1; i <= n_dims; i++ ) {
        ASR::expr_t* current_dim = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                                    al, loc, i, ASRUtils::expr_type(dim)));
        ASR::expr_t* test_expr = ASRUtils::EXPR(ASR::make_IntegerCompare_t(
                                    al, loc, dim, ASR::cmpopType::Eq,
                                    current_dim, ASRUtils::TYPE(ASR::make_Logical_t(
                                        al, loc, 4, nullptr, 0)), nullptr));

        Vec<ASR::expr_t*> target_idx_vars, loop_vars;
        std::vector<int> loop_dims;
        loop_dims.reserve(n_dims);
        loop_vars.reserve(al, n_dims);
        target_idx_vars.reserve(al, n_dims - 1);
        for( int j = 1; j <= n_dims; j++ ) {
            if( j == i ) {
                continue ;
            }
            target_idx_vars.push_back(al, idx_vars[j - 1]);
            loop_dims.push_back(j);
            loop_vars.push_back(al, idx_vars[j - 1]);
        }
        loop_dims.push_back(i);
        loop_vars.push_back(al, idx_vars[i - 1]);

        Vec<ASR::stmt_t*> doloop_body;
        ASR::stmt_t* doloop = ControlFlowConstructorAPI::create_do_loop(al, loc,
        array, result, loop_vars, loop_dims, doloop_body,
        [=, &al, &idx_vars, &target_idx_vars, &doloop_body] () {
                ASR::expr_t* result_ref = PassUtils::create_array_ref(result, target_idx_vars, al);
                ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
                ASR::expr_t* logical_or = ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al, loc,
                                            result_ref, ASR::Or, array_ref,
                                            ASRUtils::TYPE(ASR::make_Logical_t( al, loc, 4,
                                                nullptr, 0)), nullptr));
                ASR::stmt_t* loop_invariant = ASRUtils::STMT(ASR::make_Assignment_t(al, loc,
                                                result_ref, logical_or, nullptr));
                doloop_body.push_back(al, loop_invariant);
        });
        Vec<ASR::stmt_t*> if_body;
        if_body.reserve(al, 1);
        if_body.push_back(al, doloop);
        ASR::stmt_t* if_ = ASRUtils::STMT(ASR::make_If_t(al, loc, test_expr,
                            if_body.p, if_body.size(), else_, else_n));
        Vec<ASR::stmt_t*> if_else_if;
        if_else_if.reserve(al, 1);
        if_else_if.push_back(al, if_);
        else_ = if_else_if.p;
        else_n = if_else_if.size();
    }
    fn_body.push_back(al, else_[0]);
}

static inline ASR::expr_t* instantiate_Any(Allocator &al, const Location &loc,
    SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
    Vec<ASR::call_arg_t>& new_args, int64_t overload_id,
    ASR::expr_t* compile_time_value) {

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
                ASR::ttype_t* return_type = nullptr;
                if( f->m_return_var ) {
                    return_type = ASRUtils::expr_type(f->m_return_var);
                } else {
                    return_type = ASRUtils::expr_type(f->m_args[(int) f->n_args - 1]);
                }
                return ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc, s,
                    s, new_args.p, new_args.size(), return_type, compile_time_value,
                    nullptr));
            } else {
                new_func_name += std::to_string(i);
                i++;
            }
        }
    }

    new_name = scope->get_unique_name(new_name);
    SymbolTable *fn_symtab = al.make_new<SymbolTable>(scope);

    ASR::ttype_t* logical_return_type = ASRUtils::TYPE(ASR::make_Logical_t(
                                            al, loc, 4, nullptr, 0));
    Vec<ASR::expr_t*> args;
    int result_dims = 0;
    {
        args.reserve(al, 1);
        ASR::ttype_t* mask_type = ASRUtils::duplicate_type_with_empty_dims(al, arg_type);
        create_variable(mask_arg, "mask", ASR::intentType::In, ASR::abiType::Source,
            false, fn_symtab, mask_type);
        args.push_back(al, mask_arg);
        if( overload_id == 1 ) {
            ASR::ttype_t* dim_type = ASRUtils::expr_type(new_args[1].m_value);
            LCOMPILERS_ASSERT(ASR::is_a<ASR::Integer_t>(*dim_type));
            int kind = ASRUtils::extract_kind_from_ttype_t(dim_type);
            LCOMPILERS_ASSERT(kind == 4);
            create_variable(dim_arg, "dim", ASR::intentType::In, ASR::abiType::Source,
                false, fn_symtab, dim_type);
            args.push_back(al, dim_arg);

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
            logical_return_type = ASRUtils::TYPE(ASR::make_Logical_t(
                                    al, loc, 4, dims.p, dims.size()));
            if( result_dims > 0 ) {
                create_variable(result_arg, "result", ASR::intentType::Out, ASR::abiType::Source,
                    false, fn_symtab, logical_return_type);
                args.push_back(al, result_arg);
            }
        }
    }

    ASR::expr_t* return_var = nullptr;
    if( result_dims == 0 ) {
        create_variable(return_var_, new_name, ASRUtils::intent_return_var,
            ASR::abiType::Source, false, fn_symtab, logical_return_type);
        return_var = return_var_;
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
    return ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc, new_symbol,
            new_symbol, new_args.p, new_args.size(), logical_return_type,
            compile_time_value, nullptr));
}

} // namespace Any

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
        Vec<ASR::ttype_t *> res_tuple_type; res_tuple_type.reserve(al, 3);
        ASR::ttype_t *first_res_type, *second_res_type, *third_res_type;

        first_res_type = TYPE(ASR::make_Character_t(al, loc, 1,
            first_res.size(), nullptr, nullptr, 0));
        res_tuple.push_back(al, ASRUtils::EXPR(ASR::make_StringConstant_t(
            al, loc, s2c(al, first_res), first_res_type)));
        second_res_type = TYPE(ASR::make_Character_t(al, loc, 1,
            second_res.size(), nullptr, nullptr, 0));
        res_tuple.push_back(al, ASRUtils::EXPR(ASR::make_StringConstant_t(
            al, loc, s2c(al, second_res), second_res_type)));
        third_res_type = TYPE(ASR::make_Character_t(al, loc, 1,
            third_res.size(), nullptr, nullptr, 0));
        res_tuple.push_back(al, ASRUtils::EXPR(ASR::make_StringConstant_t(
            al, loc, s2c(al, third_res), third_res_type)));
        res_tuple_type.push_back(al, first_res_type);
        res_tuple_type.push_back(al, second_res_type);
        res_tuple_type.push_back(al, third_res_type);

        ASR::ttype_t *tuple_type = ASRUtils::TYPE(ASR::make_Tuple_t(al, loc,
            res_tuple_type.p, res_tuple_type.n));
        return EXPR(ASR::make_TupleConstant_t(al, loc, res_tuple.p, res_tuple.n,
            tuple_type));
    }

    static inline ASR::asr_t*  create_partition(Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, ASR::expr_t *s_var,
            const std::function<void (const std::string &, const Location &)> err) {
        if (args.size() != 1) {
            err("str.partition() takes exactly one argument", loc);
        }
        ASR::expr_t *arg = args[0];
        if (!ASRUtils::is_character(*expr_type(arg))) {
            err("str.partition() takes one arguments of type: str", arg->base.loc);
        }

        Vec<ASR::expr_t *> e_args; e_args.reserve(al, 1);
        e_args.push_back(al, s_var);
        e_args.push_back(al, arg);

        Vec<ASR::ttype_t *> tuple_type; tuple_type.reserve(al, 3);
        tuple_type.push_back(al, str);
        tuple_type.push_back(al, str);
        tuple_type.push_back(al, str);
        ASR::ttype_t *return_type = ASRUtils::TYPE(ASR::make_Tuple_t(al, loc,
            tuple_type.p, tuple_type.n));
        ASR::expr_t *value = nullptr;
        if (ASR::is_a<ASR::StringConstant_t>(*s_var)
         && ASR::is_a<ASR::StringConstant_t>(*arg)) {
            ASR::StringConstant_t* sep = ASR::down_cast<ASR::StringConstant_t>(arg);
            std::string s_sep = sep->m_s;
            ASR::StringConstant_t* str = ASR::down_cast<ASR::StringConstant_t>(s_var);
            std::string s_str = str->m_s;
            if (s_sep.size() == 0) {
                err("Separator cannot be an empty string", sep->base.base.loc);
            }
            value = eval_Partition(al, loc, s_str, s_sep);
        }

        return ASR::make_IntrinsicFunction_t(al, loc,
            static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Partition),
            e_args.p, e_args.n, 0, return_type, value);
    }

    static inline ASR::expr_t* instantiate_Partition(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& /*arg_types*/,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/,
            ASR::expr_t* compile_time_value)
    {
        std::string fn_name = scope->get_unique_name("_lpython_str_partition");
        SymbolTable *fn_symtab = al.make_new<SymbolTable>(scope);

        Vec<ASR::expr_t*> args;
        {
            args.reserve(al, 2);
            create_variable(arg_1, "target_string", ASR::intentType::In,
                ASR::abiType::Source, false, fn_symtab, str);
            args.push_back(al, arg_1);
            create_variable(arg_2, "pattern", ASR::intentType::In,
                ASR::abiType::Source, false, fn_symtab, str);
            args.push_back(al, arg_2);
        }
        Vec<ASR::ttype_t *> tuple_type; tuple_type.reserve(al, 1);
        tuple_type.push_back(al, str);
        tuple_type.push_back(al, str);
        tuple_type.push_back(al, str);
        ASR::ttype_t *return_type = TYPE(ASR::make_Tuple_t(al, loc,
            tuple_type.p, tuple_type.n));
        create_variable(return_var, "result", ASRUtils::intent_return_var,
            ASR::abiType::Source, false, fn_symtab, return_type);
        ASR::expr_t *result = EXPR(ASR::make_Var_t(al, loc, sym_return_var));

        Vec<ASR::stmt_t*> body;
        body.reserve(al, 1);

        SetChar dep;
        dep.reserve(al, 1);
        {
            ASR::ttype_t *logical_type = TYPE(ASR::make_Logical_t(al, loc, 4, nullptr, 0));
            Variable(index, i_32)
            ASR::symbol_t *kmp_fn = UnaryIntrinsicFunction::create_KMP_function(
                al, loc, scope);
            Vec<ASR::call_arg_t> args_; args_.reserve(al, 2);
            {
                ASR::call_arg_t arg;
                arg.loc = loc;
                arg.m_value = args[0];
                args_.push_back(al, arg);
                arg.m_value = args[1];
                args_.push_back(al, arg);
            }
            body.push_back(al, STMT(ASR::make_Assignment_t(al, loc, index,
                EXPR(ASR::make_FunctionCall_t(al, loc, kmp_fn, kmp_fn,
                args_.p, args_.n, i_32, nullptr, nullptr)), nullptr)));

            ASR::expr_t *a_test = EXPR(ASR::make_IntegerCompare_t(al, loc, index,
                ASR::cmpopType::Eq, EXPR(ASR::make_IntegerUnaryMinus_t(al, loc,
                i32(-1), i_32, nullptr)), logical_type, nullptr));
            Vec<ASR::stmt_t *> if_body; if_body.reserve(al, 1);
            Vec<ASR::expr_t *> tuple_ele; tuple_ele.reserve(al, 3);
            {
                ASR::ttype_t *char_type_0 = TYPE(ASR::make_Character_t(al, loc,
                    1, 0, nullptr, nullptr, 0));
                tuple_ele.push_back(al, args[0]);
                tuple_ele.push_back(al, EXPR(ASR::make_StringConstant_t(al, loc,
                    s2c(al, ""), char_type_0)));
                tuple_ele.push_back(al, EXPR(ASR::make_StringConstant_t(al, loc,
                    s2c(al, ""), char_type_0)));
                Vec<ASR::ttype_t *> tuple_type_; tuple_type_.reserve(al, 1);
                tuple_type_.push_back(al, str);
                tuple_type_.push_back(al, char_type_0);
                tuple_type_.push_back(al, char_type_0);
                if_body.push_back(al, STMT(ASR::make_Assignment_t(al, loc,
                    result, EXPR(ASR::make_TupleConstant_t(al, loc,
                    tuple_ele.p, tuple_ele.n, TYPE(ASR::make_Tuple_t(al, loc,
                    tuple_type_.p, tuple_type_.n)))), nullptr)));
            }
            Vec<ASR::stmt_t *> else_body; else_body.reserve(al, 1);
            tuple_ele.p = nullptr; tuple_ele.n = 0;
            tuple_ele.reserve(al, 3);
            {
                tuple_ele.push_back(al, EXPR(ASR::make_StringSection_t(al, loc,
                    args[0], i32(0),
                    index, nullptr, str, nullptr)));
                tuple_ele.push_back(al, args[1]);
                tuple_ele.push_back(al, EXPR(ASR::make_StringSection_t(al, loc,
                    args[0], EXPR(ASR::make_IntegerBinOp_t(al, loc, index,
                        ASR::binopType::Add,
                        EXPR(ASR::make_StringLen_t(al, loc, args[1], i_32, nullptr)),
                        i_32, nullptr)),
                    EXPR(ASR::make_StringLen_t(al, loc, args[0], i_32, nullptr)),
                    nullptr, str, nullptr)));
                else_body.push_back(al, STMT(ASR::make_Assignment_t(al, loc,
                    result, EXPR(ASR::make_TupleConstant_t(al, loc,
                    tuple_ele.p, tuple_ele.n, return_type)), nullptr)));
            }
            body.push_back(al, STMT(ASR::make_If_t(al, loc, a_test,
                if_body.p, if_body.n, else_body.p, else_body.n)));
            body.push_back(al, STMT(ASR::make_Return_t(al, loc)));
        }
        ASR::symbol_t *fn_sym = make_Function_t(fn_name, fn_symtab, dep, args,
            body, return_var, Source, Implementation, nullptr);
        scope->add_symbol(fn_name, fn_sym);
        return EXPR(ASR::make_FunctionCall_t(al, loc, fn_sym, fn_sym,
            new_args.p, new_args.n, return_type, compile_time_value, nullptr));
    }
} // namespace Partition

namespace IntrinsicFunctionRegistry {

    static const std::map<int64_t, impl_function>& intrinsic_function_by_id_db = {
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::LogGamma),
            &LogGamma::instantiate_LogGamma},

        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Sin),
            &Sin::instantiate_Sin},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Cos),
            &Cos::instantiate_Cos},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Tan),
            &Tan::instantiate_Tan},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Asin),
            &Asin::instantiate_Asin},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Acos),
            &Acos::instantiate_Acos},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Atan),
            &Atan::instantiate_Atan},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Abs),
            &Abs::instantiate_Abs},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Any),
            &Any::instantiate_Any},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Partition),
            &Partition::instantiate_Partition}
    };

    static const std::map<int64_t, std::string>& intrinsic_function_id_to_name = {
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::LogGamma),
            "log_gamma"},

        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Sin),
            "sin"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Cos),
            "cos"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Tan),
            "tan"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Asin),
            "asin"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Acos),
            "acos"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Atan),
            "atan"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Abs),
            "abs"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::ListIndex),
            "list.index"}
    };

    static const std::map<std::string,
        std::pair<create_intrinsic_function,
                    eval_intrinsic_function>>& intrinsic_function_by_name_db = {
                {"log_gamma", {&LogGamma::create_LogGamma, &LogGamma::eval_log_gamma}},
                {"sin", {&Sin::create_Sin, &Sin::eval_Sin}},
                {"cos", {&Cos::create_Cos, &Cos::eval_Cos}},
                {"tan", {&Tan::create_Tan, &Tan::eval_Tan}},
                {"asin", {&Asin::create_Asin, &Asin::eval_Asin}},
                {"acos", {&Acos::create_Acos, &Acos::eval_Acos}},
                {"atan", {&Atan::create_Atan, &Atan::eval_Atan}},
                {"abs", {&Abs::create_Abs, &Abs::eval_Abs}},
                {"any", {&Any::create_Any, &Any::eval_Any}},
                {"list.index", {&ListIndex::create_ListIndex, &ListIndex::eval_list_index}},
    };

    static inline bool is_intrinsic_function(const std::string& name) {
        return intrinsic_function_by_name_db.find(name) != intrinsic_function_by_name_db.end();
    }

    static inline bool is_intrinsic_function(int64_t id) {
        return intrinsic_function_by_id_db.find(id) != intrinsic_function_by_id_db.end();
    }

    static inline bool is_elemental(int64_t id) {
        ASRUtils::IntrinsicFunctions id_ = static_cast<ASRUtils::IntrinsicFunctions>(id);
        return ( id_ == ASRUtils::IntrinsicFunctions::Abs ||
                 id_ == ASRUtils::IntrinsicFunctions::Cos ||
                 id_ == ASRUtils::IntrinsicFunctions::Gamma ||
                 id_ == ASRUtils::IntrinsicFunctions::LogGamma ||
                 id_ == ASRUtils::IntrinsicFunctions::Sin );
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
    static inline int get_dim_index(ASRUtils::IntrinsicFunctions id) {
        if( id == ASRUtils::IntrinsicFunctions::Any ) {
            return 1;
        } else {
            LCOMPILERS_ASSERT(false);
        }
        return -1;
    }

    static inline create_intrinsic_function get_create_function(const std::string& name) {
        return intrinsic_function_by_name_db.at(name).first;
    }

    static inline impl_function get_instantiate_function(int64_t id) {
        if( intrinsic_function_by_id_db.find(id) == intrinsic_function_by_id_db.end() ) {
            return nullptr;
        }
        return intrinsic_function_by_id_db.at(id);
    }

    static inline std::string get_intrinsic_function_name(int64_t id) {
        if( intrinsic_function_id_to_name.find(id) == intrinsic_function_id_to_name.end() ) {
            throw LCompilersException("IntrinsicFunction with ID " + std::to_string(id) +
                                      " has no name registered for it");
        }
        return intrinsic_function_id_to_name.at(id);
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
        INTRINSIC_NAME_CASE(Tan)
        INTRINSIC_NAME_CASE(Asin)
        INTRINSIC_NAME_CASE(Acos)
        INTRINSIC_NAME_CASE(Atan)
        INTRINSIC_NAME_CASE(Gamma)
        INTRINSIC_NAME_CASE(LogGamma)
        INTRINSIC_NAME_CASE(Abs)
        INTRINSIC_NAME_CASE(Any)
        INTRINSIC_NAME_CASE(ListIndex)
        INTRINSIC_NAME_CASE(Partition)
        default : {
            throw LCompilersException("pickle: intrinsic_id not implemented");
        }
    }
}

} // namespace ASRUtils

} // namespace LCompilers

#endif // LFORTRAN_PASS_INTRINSIC_FUNCTION_REGISTRY_H
