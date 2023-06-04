#ifndef LFORTRAN_PASS_INTRINSIC_FUNCTION_REGISTRY_H
#define LFORTRAN_PASS_INTRINSIC_FUNCTION_REGISTRY_H

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/asr_utils.h>
#include <libasr/pass/pass_utils.h>

#include <cmath>
#include <string>
#include <tuple>

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

typedef void (*verify_function)(
    const ASR::IntrinsicFunction_t&,
    diag::Diagnostics&);

enum class IntrinsicFunctions : int64_t {
    Sin,
    Cos,
    Tan,
    Asin,
    Acos,
    Atan,
    Sinh,
    Cosh,
    Tanh,
    Gamma,
    LogGamma,
    Abs,
    Exp,
    Exp2,
    Expm1,
    Any,
    ListIndex,
    Partition,
    ListReverse,
    Sum,
    // ...
};


class ASRBuilder {
    private:

    Allocator& al;
    // TODO: use the location to point C++ code in `intrinsic_function_registry`
    const Location &loc;

    public:

    ASRBuilder(Allocator& al_, const Location& loc_): al(al_), loc(loc_) {}

    #define make_ConstantWithKind(Constructor, TypeConstructor, value, kind, loc) ASRUtils::EXPR( \
        ASR::Constructor( al, loc, value, \
            ASRUtils::TYPE(ASR::TypeConstructor(al, loc, 4, nullptr, 0)))) \

    #define make_ConstantWithType(Constructor, value, type, loc) ASRUtils::EXPR( \
        ASR::Constructor(al, loc, value, type)) \

    #define declare_basic_variables(name)                                       \
        std::string fn_name = scope->get_unique_name(name);                     \
        SymbolTable *fn_symtab = al.make_new<SymbolTable>(scope);               \
        ASRBuilder b(al, loc);                                                  \
        Vec<ASR::expr_t*> args; args.reserve(al, 1);                            \
        Vec<ASR::stmt_t*> body; body.reserve(al, 1);                            \
        SetChar dep; dep.reserve(al, 1);

    // Symbols -----------------------------------------------------------------
    ASR::expr_t *Variable(SymbolTable *symtab, std::string var_name,
            ASR::ttype_t *type, ASR::intentType intent,
            ASR::abiType abi=ASR::abiType::Source, bool a_value_attr=false) {
        ASR::symbol_t* sym = ASR::down_cast<ASR::symbol_t>(
            ASR::make_Variable_t(al, loc, symtab, s2c(al, var_name), nullptr, 0,
            intent, nullptr, nullptr, ASR::storage_typeType::Default, type, nullptr, abi,
            ASR::Public, ASR::presenceType::Required, a_value_attr));
        symtab->add_symbol(s2c(al, var_name), sym);
        return ASRUtils::EXPR(ASR::make_Var_t(al, loc, sym));
    }

    #define declare(var_name, type, intent)                                     \
        b.Variable(fn_symtab, var_name, type, ASR::intentType::intent)

    #define fill_func_arg(arg_name, type) {                              \
        auto arg = declare(arg_name, type, In);                                 \
        args.push_back(al, arg); }

    #define make_Function_t(name, symtab, dep, args, body, return_var, abi,     \
            deftype, bindc_name)                                                \
        ASR::down_cast<ASR::symbol_t>( ASRUtils::make_Function_t_util(al, loc,  \
        symtab, s2c(al, name), dep.p, dep.n, args.p, args.n, body.p, body.n,    \
        return_var, ASR::abiType::abi, ASR::accessType::Public,                 \
        ASR::deftypeType::deftype, bindc_name, false, false, false, false,      \
        false, nullptr, 0, nullptr, 0, false, false, false));

    #define make_Function_Without_ReturnVar_t(name, symtab, dep, args, body,    \
            abi, deftype, bindc_name)                                           \
        ASR::down_cast<ASR::symbol_t>( ASRUtils::make_Function_t_util(al, loc,  \
        symtab, s2c(al, name), dep.p, dep.n, args.p, args.n, body.p, body.n,    \
        nullptr, ASR::abiType::abi, ASR::accessType::Public,                    \
        ASR::deftypeType::deftype, bindc_name, false, false, false, false,      \
        false, nullptr, 0, nullptr, 0, false, false, false));

    // Types -------------------------------------------------------------------
    #define int32        TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0))
    #define logical      TYPE(ASR::make_Logical_t(al, loc, 4, nullptr, 0))
    #define character(x) TYPE(ASR::make_Character_t(al, loc, 1, x, nullptr,    \
        nullptr, 0))
    #define List(x)      TYPE(ASR::make_List_t(al, loc, x))
    ASR::ttype_t *Tuple(std::vector<ASR::ttype_t*> tuple_type) {
        Vec<ASR::ttype_t*> m_tuple_type; m_tuple_type.reserve(al, 3);
        for (auto &x: tuple_type) {
            m_tuple_type.push_back(al, x);
        }
        return TYPE(ASR::make_Tuple_t(al, loc, m_tuple_type.p, m_tuple_type.n));
    }

    // Expressions -------------------------------------------------------------
    #define i32(x)   EXPR(ASR::make_IntegerConstant_t(al, loc, x, int32))
    #define i32_n(x) EXPR(ASR::make_IntegerUnaryMinus_t(al, loc, i32(abs(x)),   \
        int32, i32(x)))
    #define bool32(x)  EXPR(ASR::make_LogicalConstant_t(al, loc, x, logical))

    #define ListItem(x, pos, type) EXPR(ASR::make_ListItem_t(al, loc, x, pos,   \
        type, nullptr))
    #define ListAppend(x, val) STMT(ASR::make_ListAppend_t(al, loc, x, val))

    #define StringSection(s, start, end) EXPR(ASR::make_StringSection_t(al, loc,\
        s, start, end, nullptr, character(-2), nullptr))
    #define StringItem(x, idx) EXPR(ASR::make_StringItem_t(al, loc, x, idx,     \
        character(-2), nullptr))
    #define StringConstant(s, type) EXPR(ASR::make_StringConstant_t(al, loc,    \
        s2c(al, s), type))
    #define StringLen(s) EXPR(ASR::make_StringLen_t(al, loc, s, int32, nullptr))

    #define iAdd(left, right) EXPR(ASR::make_IntegerBinOp_t(al, loc, left,      \
        ASR::binopType::Add, right, int32, nullptr))
    #define iSub(left, right) EXPR(ASR::make_IntegerBinOp_t(al, loc, left,      \
        ASR::binopType::Sub, right, int32, nullptr))

    #define And(x, y) EXPR(ASR::make_LogicalBinOp_t(al, loc, x,                 \
        ASR::logicalbinopType::And, y, logical, nullptr))
    #define Not(x)    EXPR(ASR::make_LogicalNot_t(al, loc, x, logical, nullptr))

    #define iEq(x, y) EXPR(ASR::make_IntegerCompare_t(al, loc, x,               \
        ASR::cmpopType::Eq, y, logical, nullptr))
    #define sEq(x, y) EXPR(ASR::make_StringCompare_t(al, loc, x,                \
        ASR::cmpopType::Eq, y, logical, nullptr))
    #define iNotEq(x, y) EXPR(ASR::make_IntegerCompare_t(al, loc, x,            \
        ASR::cmpopType::NotEq, y, logical, nullptr))
    #define sNotEq(x, y) EXPR(ASR::make_StringCompare_t(al, loc, x,             \
        ASR::cmpopType::NotEq, y, logical, nullptr))
    #define iLt(x, y) EXPR(ASR::make_IntegerCompare_t(al, loc, x,               \
        ASR::cmpopType::Lt, y, logical, nullptr))
    #define iLtE(x, y) EXPR(ASR::make_IntegerCompare_t(al, loc, x,              \
        ASR::cmpopType::LtE, y, logical, nullptr))
    #define iGtE(x, y) EXPR(ASR::make_IntegerCompare_t(al, loc, x,              \
        ASR::cmpopType::GtE, y, logical, nullptr))

    ASR::stmt_t *If(ASR::expr_t *a_test, std::vector<ASR::stmt_t*> if_body,
            std::vector<ASR::stmt_t*> else_body) {
        Vec<ASR::stmt_t*> m_if_body; m_if_body.reserve(al, 1);
        for (auto &x: if_body) m_if_body.push_back(al, x);

        Vec<ASR::stmt_t*> m_else_body; m_else_body.reserve(al, 1);
        for (auto &x: else_body) m_else_body.push_back(al, x);

        return STMT(ASR::make_If_t(al, loc, a_test, m_if_body.p, m_if_body.n,
            m_else_body.p, m_else_body.n));
    }

    ASR::stmt_t *While(ASR::expr_t *a_test, std::vector<ASR::stmt_t*> body) {
        Vec<ASR::stmt_t*> m_body; m_body.reserve(al, 1);
        for (auto &x: body) m_body.push_back(al, x);

        return STMT(ASR::make_WhileLoop_t(al, loc, nullptr, a_test,
            m_body.p, m_body.n));
    }

    ASR::expr_t *TupleConstant(std::vector<ASR::expr_t*> ele, ASR::ttype_t *type) {
        Vec<ASR::expr_t*> m_ele; m_ele.reserve(al, 3);
        for (auto &x: ele) m_ele.push_back(al, x);
        return EXPR(ASR::make_TupleConstant_t(al, loc, m_ele.p, m_ele.n, type));
    }

    #define make_Compare(Constructor, left, op, right, loc) ASRUtils::EXPR(ASR::Constructor( \
        al, loc, left, op, right, \
        ASRUtils::TYPE(ASR::make_Logical_t( \
            al, loc, 4, nullptr, 0)), nullptr)); \

    #define create_ElementalBinOp(OpType, BinOpName, OpName, value) case ASR::ttypeType::OpType: { \
        return ASRUtils::EXPR(ASR::BinOpName(al, loc, \
                left, ASR::binopType::OpName, right, \
                ASRUtils::expr_type(left), value)); \
    } \

    ASR::expr_t* ElementalAdd(ASR::expr_t* left, ASR::expr_t* right,
        const Location& loc, ASR::expr_t* value=nullptr) {
        switch (ASRUtils::expr_type(left)->type) {
            create_ElementalBinOp(Real, make_RealBinOp_t, Add, value)
            create_ElementalBinOp(Integer, make_IntegerBinOp_t, Add, value)
            create_ElementalBinOp(Complex, make_ComplexBinOp_t, Add, value)
            default: {
                throw LCompilersException("Expression type, " +
                                          std::to_string(left->type) +
                                          " not yet supported");
            }
        }
    }

    ASR::expr_t* ElementalSub(ASR::expr_t* left, ASR::expr_t* right,
        const Location& loc, ASR::expr_t* value=nullptr) {
        switch (ASRUtils::expr_type(left)->type) {
            create_ElementalBinOp(Real, make_RealBinOp_t, Sub, value)
            create_ElementalBinOp(Integer, make_IntegerBinOp_t, Sub, value)
            create_ElementalBinOp(Complex, make_ComplexBinOp_t, Sub, value)
            default: {
                throw LCompilersException("Expression type, " +
                                          std::to_string(left->type) +
                                          " not yet supported");
            }
        }
    }

    ASR::expr_t* ElementalDiv(ASR::expr_t* left, ASR::expr_t* right,
        const Location& loc, ASR::expr_t* value=nullptr) {
        switch (ASRUtils::expr_type(left)->type) {
            create_ElementalBinOp(Real, make_RealBinOp_t, Div, value)
            create_ElementalBinOp(Integer, make_IntegerBinOp_t, Div, value)
            create_ElementalBinOp(Complex, make_ComplexBinOp_t, Div, value)
            default: {
                throw LCompilersException("Expression type, " +
                                          std::to_string(left->type) +
                                          " not yet supported");
            }
        }
    }

    ASR::expr_t* ElementalPow(ASR::expr_t* left, ASR::expr_t* right,
        const Location& loc, ASR::expr_t* value=nullptr) {
        switch (ASRUtils::expr_type(left)->type) {
            create_ElementalBinOp(Real, make_RealBinOp_t, Pow, value)
            create_ElementalBinOp(Integer, make_IntegerBinOp_t, Pow, value)
            create_ElementalBinOp(Complex, make_ComplexBinOp_t, Pow, value)
            default: {
                throw LCompilersException("Expression type, " +
                                          std::to_string(left->type) +
                                          " not yet supported");
            }
        }
    }

    ASR::expr_t* ElementalOr(ASR::expr_t* left, ASR::expr_t* right,
        const Location& loc) {
        return ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al, loc,
            left, ASR::Or, right,
            ASRUtils::TYPE(ASR::make_Logical_t( al, loc, 4,
                nullptr, 0)), nullptr));
    }

    ASR::expr_t* Or(ASR::expr_t* left, ASR::expr_t* right,
        const Location& loc) {
        return ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al, loc,
            left, ASR::Or, right, ASRUtils::expr_type(left),
            nullptr));
    }

    ASR::expr_t* Call(ASR::symbol_t* s, Vec<ASR::call_arg_t>& args,
                      ASR::ttype_t* return_type) {
        return ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc,
                s, s, args.p, args.size(), return_type, nullptr, nullptr));
    }

    ASR::expr_t* Call(ASR::symbol_t* s, Vec<ASR::expr_t *>& args,
                      ASR::ttype_t* return_type) {
        Vec<ASR::call_arg_t> args_; args_.reserve(al, 2);
        visit_expr_list(al, args, args_);
        return ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc,
                s, s, args_.p, args_.size(), return_type, nullptr, nullptr));
    }

    ASR::expr_t* Call(ASR::symbol_t* s, Vec<ASR::call_arg_t>& args,
                      ASR::ttype_t* return_type, ASR::expr_t* value) {
        return ASRUtils::EXPR(ASR::make_FunctionCall_t(al, loc,
                s, s, args.p, args.size(), return_type, value, nullptr));
    }

    // Statements --------------------------------------------------------------
    #define Return() STMT(ASR::make_Return_t(al, loc))
    #define Assignment(lhs, rhs) ASRUtils::STMT(ASR::make_Assignment_t(al, loc, \
        lhs, rhs, nullptr))

    template <typename LOOP_BODY>
    ASR::stmt_t* create_do_loop(
        const Location& loc, int rank, ASR::expr_t* array,
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
        const Location& loc, ASR::expr_t* array,
        Vec<ASR::expr_t*>& loop_vars, std::vector<int>& loop_dims,
        Vec<ASR::stmt_t*>& doloop_body, LOOP_BODY loop_body) {

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

    template <typename INIT, typename LOOP_BODY>
    void generate_reduction_intrinsic_stmts_for_scalar_output(const Location& loc,
    ASR::expr_t* array, SymbolTable* fn_scope,
    Vec<ASR::stmt_t*>& fn_body, Vec<ASR::expr_t*>& idx_vars,
    Vec<ASR::stmt_t*>& doloop_body, INIT init_stmts, LOOP_BODY loop_body) {
        init_stmts();
        int rank = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(array));
        ASR::stmt_t* doloop = create_do_loop(loc,
            rank, array, fn_scope, idx_vars, doloop_body,
            loop_body);
        fn_body.push_back(al, doloop);
    }

    template <typename INIT, typename LOOP_BODY>
    void generate_reduction_intrinsic_stmts_for_array_output(const Location& loc,
        ASR::expr_t* array, ASR::expr_t* dim, SymbolTable* fn_scope,
        Vec<ASR::stmt_t*>& fn_body, Vec<ASR::expr_t*>& idx_vars,
        Vec<ASR::expr_t*>& target_idx_vars, Vec<ASR::stmt_t*>& doloop_body,
        INIT init_stmts, LOOP_BODY loop_body) {
        init_stmts();
        int n_dims = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(array));
        ASR::stmt_t** else_ = nullptr;
        size_t else_n = 0;
        idx_vars.reserve(al, n_dims);
        PassUtils::create_idx_vars(idx_vars, n_dims, loc, al, fn_scope, "_j");
        for( int i = 1; i <= n_dims; i++ ) {
            ASR::expr_t* current_dim = i32(i);
            ASR::expr_t* test_expr = make_Compare(make_IntegerCompare_t, dim,
                                        ASR::cmpopType::Eq, current_dim, loc);

            Vec<ASR::expr_t*> loop_vars;
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

            ASR::stmt_t* doloop = create_do_loop(loc,
            array, loop_vars, loop_dims, doloop_body,
            loop_body);
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

};

namespace UnaryIntrinsicFunction {

static inline ASR::expr_t* instantiate_functions(Allocator &al,
        const Location &loc, SymbolTable *scope, std::string new_name,
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

    declare_basic_variables(new_name);
    if (scope->get_symbol(new_name)) {
        ASR::symbol_t *s = scope->get_symbol(new_name);
        ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
        return b.Call(s, new_args, expr_type(f->m_return_var), value);
    }
    fill_func_arg("x", arg_type);
    auto result = declare(new_name, arg_type, ReturnVar);

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
            arg_type, ASRUtils::intent_return_var, ASR::abiType::BindC, false);

        SetChar dep_1; dep_1.reserve(al, 1);
        Vec<ASR::stmt_t*> body_1; body_1.reserve(al, 1);
        ASR::symbol_t *s = make_Function_t(c_func_name, fn_symtab_1, dep_1, args_1,
            body_1, return_var_1, BindC, Interface, s2c(al, c_func_name));
        fn_symtab->add_symbol(c_func_name, s);
        dep.push_back(al, s2c(al, c_func_name));
        body.push_back(al, Assignment(result, b.Call(s, args, arg_type)));
    }

    ASR::symbol_t *new_symbol = make_Function_t(fn_name, fn_symtab, dep, args,
        body, result, Source, Implementation, nullptr);
    scope->add_symbol(fn_name, new_symbol);
    return b.Call(new_symbol, new_args, arg_type, value);
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

    body.push_back(al, Assignment(s_len, StringLen(args[0])));
    body.push_back(al, Assignment(pat_len, StringLen(args[1])));
    body.push_back(al, Assignment(result, i32_n(-1)));
    body.push_back(al, b.If(iEq(pat_len, i32(0)), {
            Assignment(result, i32(0)), Return()
        }, {
            b.If(iEq(s_len, i32(0)), { Return() }, {})
        }));
    body.push_back(al, Assignment(lps,
        EXPR(ASR::make_ListConstant_t(al, loc, nullptr, 0, List(int32)))));
    body.push_back(al, Assignment(i, i32(0)));
    body.push_back(al, b.While(iLtE(i, iSub(pat_len, i32(1))), {
        Assignment(i, iAdd(i, i32(1))),
        ListAppend(lps, i32(0))
    }));
    body.push_back(al, Assignment(flag, bool32(false)));
    body.push_back(al, Assignment(i, i32(1)));
    body.push_back(al, Assignment(pi_len, i32(0)));
    body.push_back(al, b.While(iLt(i, pat_len), {
        b.If(sEq(StringItem(args[1], iAdd(i, i32(1))),
                 StringItem(args[1], iAdd(pi_len, i32(1)))), {
            Assignment(pi_len, iAdd(pi_len, i32(1))),
            Assignment(ListItem(lps, i, int32), pi_len),
            Assignment(i, iAdd(i, i32(1)))
        }, {
            b.If(iNotEq(pi_len, i32(0)), {
                Assignment(pi_len, ListItem(lps, iSub(pi_len, i32(1)), int32))
            }, {
                Assignment(i, iAdd(i, i32(1)))
            })
        })
    }));
    body.push_back(al, Assignment(j, i32(0)));
    body.push_back(al, Assignment(i, i32(0)));
    body.push_back(al, b.While(And(iGtE(iSub(s_len, i),
            iSub(pat_len, j)), Not(flag)), {
        b.If(sEq(StringItem(args[1], iAdd(j, i32(1))),
                StringItem(args[0], iAdd(i, i32(1)))), {
            Assignment(i, iAdd(i, i32(1))),
            Assignment(j, iAdd(j, i32(1)))
        }, {}),
        b.If(iEq(j, pat_len), {
            Assignment(result, iSub(i, j)),
            Assignment(flag, bool32(true)),
            Assignment(j, ListItem(lps, iSub(j, i32(1)), int32))
        }, {
            b.If(And(iLt(i, s_len), sNotEq(StringItem(args[1], iAdd(j, i32(1))),
                    StringItem(args[0], iAdd(i, i32(1))))), {
                b.If(iNotEq(j, i32(0)), {
                    Assignment(j, ListItem(lps, iSub(j, i32(1)), int32))
                }, {
                    Assignment(i, iAdd(i, i32(1)))
                })
            }, {})
        })
    }));
    body.push_back(al, Return());
    ASR::symbol_t *fn_sym = make_Function_t(fn_name, fn_symtab, dep, args,
        body, result, Source, Implementation, nullptr);
    scope->add_symbol(fn_name, fn_sym);
    return fn_sym;
}

static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
    const Location& loc = x.base.base.loc;
    ASRUtils::require_impl(x.n_args == 1,
        "Elemental intrinsics must have only 1 input argument",
        loc, diagnostics);

    ASR::ttype_t* input_type = ASRUtils::expr_type(x.m_args[0]);
    ASR::ttype_t* output_type = x.m_type;
    ASRUtils::require_impl(ASRUtils::check_equal_type(input_type, output_type, true),
        "The input and output type of elemental intrinsics must exactly match, input type: " +
        ASRUtils::get_type_code(input_type) + " output type: " + ASRUtils::get_type_code(output_type),
        loc, diagnostics);
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
    return make_ConstantWithType(make_RealConstant_t, val, t, loc);
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
            return make_ConstantWithType(make_RealConstant_t, val, t, loc);  \
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
create_trig(Sinh, sinh, sinh)
create_trig(Cosh, cosh, cosh)
create_trig(Tanh, tanh, tanh)

namespace Abs {

    static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
        const Location& loc = x.base.base.loc;
        ASRUtils::require_impl(x.n_args == 1,
            "Elemental intrinsics must have only 1 input argument",
            loc, diagnostics);

        ASR::ttype_t* input_type = ASRUtils::expr_type(x.m_args[0]);
        ASR::ttype_t* output_type = x.m_type;
        std::string input_type_str = ASRUtils::get_type_code(input_type);
        std::string output_type_str = ASRUtils::get_type_code(output_type);
        if( ASR::is_a<ASR::Complex_t>(*ASRUtils::type_get_past_pointer(input_type)) ) {
            ASRUtils::require_impl(ASR::is_a<ASR::Real_t>(*output_type),
                "Abs intrinsic must return output of real for complex input, found: " + output_type_str,
                loc, diagnostics);
            int input_kind = ASRUtils::extract_kind_from_ttype_t(input_type);
            int output_kind = ASRUtils::extract_kind_from_ttype_t(output_type);
            ASRUtils::require_impl(input_kind == output_kind,
                "The input and output type of Abs intrinsic must be of same kind, input kind: " +
                std::to_string(input_kind) + " output kind: " + std::to_string(output_kind),
                loc, diagnostics);
            ASR::dimension_t *input_dims, *output_dims;
            size_t input_n_dims = ASRUtils::extract_dimensions_from_ttype(input_type, input_dims);
            size_t output_n_dims = ASRUtils::extract_dimensions_from_ttype(output_type, output_dims);
            ASRUtils::require_impl(ASRUtils::dimensions_equal(input_dims, input_n_dims, output_dims, output_n_dims),
                "The dimensions of input and output arguments of Abs intrinsic must be same, input: " +
                input_type_str + " output: " + output_type_str, loc, diagnostics);
        } else {
            ASRUtils::require_impl(ASRUtils::check_equal_type(input_type, output_type, true),
                "The input and output type of elemental intrinsics must exactly match, input type: " +
                input_type_str + " output type: " + output_type_str, loc, diagnostics);
        }
    }

    static ASR::expr_t *eval_Abs(Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::expr_t* arg = args[0];
        ASR::ttype_t* t = ASRUtils::expr_type(args[0]);
        if (ASRUtils::is_real(*t)) {
            double rv = ASR::down_cast<ASR::RealConstant_t>(arg)->m_r;
            double val = std::abs(rv);
            return make_ConstantWithType(make_RealConstant_t, val, t, loc);
        } else if (ASRUtils::is_integer(*t)) {
            int64_t rv = ASR::down_cast<ASR::IntegerConstant_t>(arg)->m_n;
            int64_t val = std::abs(rv);
            return make_ConstantWithType(make_IntegerConstant_t, val, t, loc);
        } else if (ASRUtils::is_complex(*t)) {
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
        declare_basic_variables(func_name);
        if (scope->get_symbol(func_name)) {
            ASR::symbol_t *s = scope->get_symbol(func_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var),
                                compile_time_value);
        }
        fill_func_arg("x", arg_types[0]);

        auto result = declare(func_name, return_type, ReturnVar);

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
                ASR::expr_t* zero = make_ConstantWithType(make_IntegerConstant_t, 0, arg_types[0], loc);
                test = make_Compare(make_IntegerCompare_t, args[0],
                                    ASR::cmpopType::GtE, zero, loc);
                negative_x = EXPR(ASR::make_IntegerUnaryMinus_t(al, loc, args[0],
                    arg_types[0], nullptr));
            } else {
                ASR::expr_t* zero = make_ConstantWithType(make_RealConstant_t, 0.0, arg_types[0], loc);
                test = make_Compare(make_RealCompare_t, args[0],
                                    ASR::cmpopType::GtE, zero, loc);
                negative_x = EXPR(ASR::make_RealUnaryMinus_t(al, loc, args[0],
                    arg_types[0], nullptr));
            }

            Vec<ASR::stmt_t *> if_body; if_body.reserve(al, 1);
            if_body.push_back(al, Assignment(result, args[0]));
            Vec<ASR::stmt_t *> else_body; else_body.reserve(al, 1);
            else_body.push_back(al, Assignment(result, negative_x));
            body.push_back(al, STMT(ASR::make_If_t(al, loc, test,
                if_body.p, if_body.n, else_body.p, else_body.n)));
        } else {
            // * Complex type: `r = (real(x)**2 + aimag(x)**2)**0.5`
            ASR::ttype_t *real_type = TYPE(ASR::make_Real_t(al, loc,
                                        ASRUtils::extract_kind_from_ttype_t(arg_types[0]),
                                        nullptr, 0));
            ASR::symbol_t *sym_result = ASR::down_cast<ASR::Var_t>(result)->m_v;
            ASR::Variable_t *r_var = ASR::down_cast<ASR::Variable_t>(sym_result);
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
                    auto arg = b.Variable(fn_symtab_1, "x", arg_types[0],
                        ASR::intentType::In, ASR::abiType::BindC, true);
                    args_1.push_back(al, arg);
                }

                auto return_var_1 = b.Variable(fn_symtab_1, c_func_name, real_type,
                    ASR::intentType::ReturnVar, ASR::abiType::BindC, false);

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
                aimag_of_x = b.Call(s, call_args, real_type);
            }
            ASR::expr_t *constant_two = make_ConstantWithType(make_RealConstant_t, 2.0, real_type, loc);
            ASR::expr_t *constant_point_five = make_ConstantWithType(make_RealConstant_t, 0.5, real_type, loc);
            ASR::expr_t *real_of_x = EXPR(ASR::make_Cast_t(al, loc, args[0],
                ASR::cast_kindType::ComplexToReal, real_type, nullptr));

            ASR::expr_t *bin_op_1 = b.ElementalPow(real_of_x, constant_two, loc);
            ASR::expr_t *bin_op_2 = b.ElementalPow(aimag_of_x, constant_two, loc);

            bin_op_1 = b.ElementalAdd(bin_op_1, bin_op_2, loc);

            body.push_back(al, Assignment(result,
                b.ElementalPow(bin_op_1, constant_point_five, loc)));
        }

        ASR::symbol_t *f_sym = make_Function_t(func_name, fn_symtab, dep, args,
            body, result, Source, Implementation, nullptr);
        scope->add_symbol(func_name, f_sym);
        return b.Call(f_sym, new_args, return_type, compile_time_value);
    }

} // namespace Abs

#define create_exp_macro(X, stdeval)                                                      \
namespace X {                                                                             \
    static inline ASR::expr_t* eval_##X(Allocator &al, const Location &loc,               \
            Vec<ASR::expr_t*> &args) {                                                    \
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));                            \
        double rv;                                                                        \
        ASR::ttype_t* t = ASRUtils::expr_type(args[0]);                                   \
        if( ASRUtils::extract_value(args[0], rv) ) {                                      \
            double val = std::stdeval(rv);                                                \
            return ASRUtils::EXPR(ASR::make_RealConstant_t(al, loc, val, t));             \
        }                                                                                 \
        return nullptr;                                                                   \
    }                                                                                     \
    static inline ASR::asr_t* create_##X(Allocator& al, const Location& loc,              \
            Vec<ASR::expr_t*>& args,                                                      \
            const std::function<void (const std::string &, const Location &)> err) {      \
        if (args.size() != 1) {                                                           \
            err("Intrinsic function `"#X"` accepts exactly 1 argument", loc);             \
        }                                                                                 \
        ASR::ttype_t *type = ASRUtils::expr_type(args[0]);                                \
        if (!ASRUtils::is_real(*type)) {                                                  \
            err("Argument of the `"#X"` function must be either Real",                    \
                args[0]->base.loc);                                                       \
        }                                                                                 \
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args, eval_##X,      \
            static_cast<int64_t>(ASRUtils::IntrinsicFunctions::X), 0, type);              \
    }                                                                                     \
} // namespace X

create_exp_macro(Exp, exp)
create_exp_macro(Exp2, exp2)
create_exp_macro(Expm1, expm1)

namespace ListIndex {

static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
    ASRUtils::require_impl(x.n_args == 2, "Call to list.index must have exactly two arguments",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASR::is_a<ASR::List_t>(*ASRUtils::expr_type(x.m_args[0])) &&
        ASRUtils::check_equal_type(ASRUtils::expr_type(x.m_args[1]),
            ASRUtils::get_contained_type(ASRUtils::expr_type(x.m_args[0]))),
        "First argument to list.index must be of list type and "
        "second argument must be of same type as list elemental type",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASR::is_a<ASR::Integer_t>(*x.m_type),
        "Return type of list.index must be an integer",
        x.base.base.loc, diagnostics);
}

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

namespace ListReverse {

static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
    ASRUtils::require_impl(x.n_args == 1, "Call to list.reverse must have exactly one argument",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASR::is_a<ASR::List_t>(*ASRUtils::expr_type(x.m_args[0])),
        "Argument to list.reverse must be of list type",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(x.m_type == nullptr,
        "Return type of list.reverse must be empty",
        x.base.base.loc, diagnostics);
}

static inline ASR::expr_t *eval_list_reverse(Allocator &/*al*/,
    const Location &/*loc*/, Vec<ASR::expr_t*>& /*args*/) {
    // TODO: To be implemented for ListConstant expression
    return nullptr;
}

static inline ASR::asr_t* create_ListReverse(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    if (args.size() != 1) {
        err("list.reverse() takes no arguments", loc);
    }

    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, args.size());
    for( size_t i = 0; i < args.size(); i++ ) {
        arg_values.push_back(al, ASRUtils::expr_value(args[i]));
    }
    ASR::expr_t* compile_time_value = eval_list_reverse(al, loc, arg_values);
    return ASR::make_Expr_t(al, loc,
            ASRUtils::EXPR(ASR::make_IntrinsicFunction_t(al, loc,
            static_cast<int64_t>(ASRUtils::IntrinsicFunctions::ListReverse),
            args.p, args.size(), 0, nullptr, compile_time_value)));
}

} // namespace ListReverse

namespace Any {

static inline void verify_array(ASR::expr_t* array, ASR::ttype_t* return_type,
    const Location& loc, diag::Diagnostics& diagnostics) {
    ASR::ttype_t* array_type = ASRUtils::expr_type(array);
    ASRUtils::require_impl(ASR::is_a<ASR::Logical_t>(*ASRUtils::type_get_past_pointer(array_type)),
        "Input to Any intrinsic must be of logical type, found: " + ASRUtils::get_type_code(array_type),
        loc, diagnostics);
    int array_n_dims = ASRUtils::extract_n_dims_from_ttype(array_type);
    ASRUtils::require_impl(array_n_dims > 0, "Input to Any intrinsic must always be an array",
        loc, diagnostics);
    ASRUtils::require_impl(ASR::is_a<ASR::Logical_t>(*return_type),
        "Any intrinsic must return a logical output", loc, diagnostics);
    int return_n_dims = ASRUtils::extract_n_dims_from_ttype(return_type);
    ASRUtils::require_impl(return_n_dims == 0,
    "Any intrinsic output for array only input should be a scalar",
    loc, diagnostics);
}

static inline void verify_array_dim(ASR::expr_t* array, ASR::expr_t* dim,
    ASR::ttype_t* return_type, const Location& loc, diag::Diagnostics& diagnostics) {
    ASR::ttype_t* array_type = ASRUtils::expr_type(array);
    ASRUtils::require_impl(ASR::is_a<ASR::Logical_t>(*ASRUtils::type_get_past_pointer(array_type)),
        "Input to Any intrinsic must be of logical type, found: " + ASRUtils::get_type_code(array_type),
        loc, diagnostics);
    int array_n_dims = ASRUtils::extract_n_dims_from_ttype(array_type);
    ASRUtils::require_impl(array_n_dims > 0, "Input to Any intrinsic must always be an array",
        loc, diagnostics);

    ASRUtils::require_impl(ASR::is_a<ASR::Integer_t>(*ASRUtils::type_get_past_pointer(ASRUtils::expr_type(dim))),
        "dim argument must be an integer", loc, diagnostics);

    ASRUtils::require_impl(ASR::is_a<ASR::Logical_t>(*return_type),
        "Any intrinsic must return a logical output", loc, diagnostics);
    int return_n_dims = ASRUtils::extract_n_dims_from_ttype(return_type);
    ASRUtils::require_impl(array_n_dims == return_n_dims + 1,
        "Any intrinsic output must return a logical array with dimension "
        "only 1 less than that of input array",
        loc, diagnostics);
}

static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
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
    const Location & /*loc*/, Vec<ASR::expr_t*>& /*args*/) {
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
    value = eval_Any(al, loc, arg_values);

    ASR::ttype_t* logical_return_type = nullptr;
    if( axis == nullptr ) {
        overload_id = 0;
        logical_return_type = ASRUtils::TYPE(ASR::make_Logical_t(
                                al, loc, 4, nullptr, 0));
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
        logical_return_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4, dims.p, dims.size()));
    }

    any_args.push_back(al, array);
    if( axis ) {
        any_args.push_back(al, axis);
    }

    return ASR::make_IntrinsicFunction_t(al, loc,
        static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Any),
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
        [=, &al, &fn_body] () {
            ASR::expr_t* logical_false = make_ConstantWithKind(
                make_LogicalConstant_t, make_Logical_t, false, 4, loc);
            ASR::stmt_t* return_var_init = Assignment(return_var, logical_false);
            fn_body.push_back(al, return_var_init);
        },
        [=, &al, &idx_vars, &doloop_body, &builder] () {
            ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
            ASR::expr_t* logical_or = builder.Or(return_var, array_ref, loc);
            ASR::stmt_t* loop_invariant = Assignment(return_var, logical_or);
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
        [=, &al, &fn_body] {
            ASR::expr_t* logical_false = make_ConstantWithKind(
                make_LogicalConstant_t, make_Logical_t, false, 4, loc);
            ASR::stmt_t* result_init = Assignment(result, logical_false);
            fn_body.push_back(al, result_init);
        },
        [=, &al, &idx_vars, &target_idx_vars, &doloop_body, &result, &builder] () {
            ASR::expr_t* result_ref = PassUtils::create_array_ref(result, target_idx_vars, al);
            ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
            ASR::expr_t* logical_or = builder.ElementalOr(result_ref, array_ref, loc);
            ASR::stmt_t* loop_invariant = Assignment(result_ref, logical_or);
            doloop_body.push_back(al, loop_invariant);
        });
}

static inline ASR::expr_t* instantiate_Any(Allocator &al, const Location &loc,
    SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
    Vec<ASR::call_arg_t>& new_args, int64_t overload_id,
    ASR::expr_t* compile_time_value) {
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
                ASR::ttype_t* return_type = nullptr;
                if( f->m_return_var ) {
                    return_type = ASRUtils::expr_type(f->m_return_var);
                } else {
                    return_type = ASRUtils::expr_type(f->m_args[(int) f->n_args - 1]);
                }
                return builder.Call(s, new_args, return_type, compile_time_value);
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
        fill_func_arg("mask", mask_type);
        if( overload_id == 1 ) {
            ASR::ttype_t* dim_type = ASRUtils::expr_type(new_args[1].m_value);
            LCOMPILERS_ASSERT(ASR::is_a<ASR::Integer_t>(*dim_type));
            int kind = ASRUtils::extract_kind_from_ttype_t(dim_type);
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
            logical_return_type = ASRUtils::TYPE(ASR::make_Logical_t(
                                    al, loc, 4, dims.p, dims.size()));
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
    return builder.Call(new_symbol, new_args, logical_return_type,
                        compile_time_value);
}

} // namespace Any

namespace Sum {

static inline void verify_array(ASR::expr_t* array, ASR::ttype_t* return_type,
    const Location& loc, diag::Diagnostics& diagnostics) {
    ASR::ttype_t* array_type = ASRUtils::expr_type(array);
    ASRUtils::require_impl(ASR::is_a<ASR::Integer_t>(*ASRUtils::type_get_past_pointer(array_type)) ||
        ASR::is_a<ASR::Real_t>(*ASRUtils::type_get_past_pointer(array_type)) ||
        ASR::is_a<ASR::Complex_t>(*ASRUtils::type_get_past_pointer(array_type)),
        "Input to Sum intrinsic must be of integer, real or complex type, found: " +
        ASRUtils::get_type_code(array_type), loc, diagnostics);
    int array_n_dims = ASRUtils::extract_n_dims_from_ttype(array_type);
    ASRUtils::require_impl(array_n_dims > 0, "Input to Sum intrinsic must always be an array",
        loc, diagnostics);
    ASRUtils::require_impl(ASRUtils::check_equal_type(
        return_type, ASRUtils::type_get_past_pointer(array_type), false),
        "Sum intrinsic must return an output of the same type as input", loc, diagnostics);
    int return_n_dims = ASRUtils::extract_n_dims_from_ttype(return_type);
    ASRUtils::require_impl(return_n_dims == 0,
    "Sum intrinsic output for array only input should be a scalar, found an array of " +
    std::to_string(return_n_dims), loc, diagnostics);
}

static inline void verify_array_dim(ASR::expr_t* array, ASR::expr_t* dim,
    ASR::ttype_t* return_type, const Location& loc, diag::Diagnostics& diagnostics) {
    ASR::ttype_t* array_type = ASRUtils::expr_type(array);
    ASRUtils::require_impl(ASR::is_a<ASR::Integer_t>(*ASRUtils::type_get_past_pointer(array_type)) ||
        ASR::is_a<ASR::Real_t>(*ASRUtils::type_get_past_pointer(array_type)) ||
        ASR::is_a<ASR::Complex_t>(*ASRUtils::type_get_past_pointer(array_type)),
        "Input to Sum intrinsic must be of integer, real or complex type, found: " +
        ASRUtils::get_type_code(array_type), loc, diagnostics);
    int array_n_dims = ASRUtils::extract_n_dims_from_ttype(array_type);
    ASRUtils::require_impl(array_n_dims > 0, "Input to Sum intrinsic must always be an array",
        loc, diagnostics);

    ASRUtils::require_impl(ASR::is_a<ASR::Integer_t>(*ASRUtils::type_get_past_pointer(ASRUtils::expr_type(dim))),
        "dim argument must be an integer", loc, diagnostics);

    ASRUtils::require_impl(ASRUtils::check_equal_type(
        return_type, ASRUtils::type_get_past_pointer(array_type), false),
        "Sum intrinsic must return an output of the same type as input", loc, diagnostics);
    int return_n_dims = ASRUtils::extract_n_dims_from_ttype(return_type);
    ASRUtils::require_impl(array_n_dims == return_n_dims + 1,
        "Sum intrinsic output must return an array with dimension "
        "only 1 less than that of input array",
        loc, diagnostics);
}

static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
    ASRUtils::require_impl(x.n_args >= 1, "Sum intrinsic must accept at least one argument",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(x.m_args[0] != nullptr, "Array argument to Sum intrinsic cannot be nullptr",
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
            verify_array(x.m_args[0], x.m_type, x.base.base.loc, diagnostics);
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
            verify_array_dim(x.m_args[0], x.m_args[1], x.m_type, x.base.base.loc, diagnostics);
            break;
        }
        default: {
            require_impl(false, "Unrecognised overload id in Sum intrinsic",
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
        ASR::ttype_t* array_type = ASRUtils::type_get_past_pointer(ASRUtils::expr_type(x.m_args[0]));
        ASR::ttype_t* mask_type = ASRUtils::type_get_past_pointer(ASRUtils::expr_type(mask));
        size_t array_n_dims = ASRUtils::extract_dimensions_from_ttype(array_type, array_dims);
        size_t mask_n_dims = ASRUtils::extract_dimensions_from_ttype(mask_type, mask_dims);
        ASRUtils::require_impl(ASRUtils::dimensions_equal(array_dims, array_n_dims, mask_dims, mask_n_dims),
            "The dimensions of array and mask arguments of Sum intrinsic must be same",
            x.base.base.loc, diagnostics);
    }
}

static inline ASR::expr_t *eval_Sum(Allocator & /*al*/,
    const Location & /*loc*/, Vec<ASR::expr_t*>& /*args*/) {
    return nullptr;
}

static inline ASR::asr_t* create_Sum(
    Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
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
            err("dim argument to sum must be a scalar and must not be an array",
                arg2->base.loc);
        }

        if( arg3_rank == 0 ) {
            err("mask argument to sum must be an array and must not be a scalar",
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
    value = eval_Sum(al, loc, arg_values);

    ASR::ttype_t* return_type = nullptr;
    if( overload_id == id_array ||
        overload_id == id_array_mask ) {
        return_type = ASRUtils::duplicate_type_without_dims(
                        al, array_type, loc);
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

    Vec<ASR::expr_t*> sum_args;
    sum_args.reserve(al, 3);
    sum_args.push_back(al, array);
    if( arg2 ) {
        sum_args.push_back(al, arg2);
    }
    if( arg3 ) {
        sum_args.push_back(al, arg3);
    }

    return ASR::make_IntrinsicFunction_t(al, loc,
        static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Sum),
        sum_args.p, sum_args.n, overload_id, return_type, value);
}

static inline void generate_body_for_array_input(Allocator& al, const Location& loc,
    ASR::expr_t* array, ASR::expr_t* return_var, SymbolTable* fn_scope,
    Vec<ASR::stmt_t*>& fn_body) {
    ASRBuilder builder(al, loc);
    Vec<ASR::expr_t*> idx_vars;
    Vec<ASR::stmt_t*> doloop_body;
    builder.generate_reduction_intrinsic_stmts_for_scalar_output(loc,
        array, fn_scope, fn_body, idx_vars, doloop_body,
        [=, &al, &fn_body] {
            ASR::ttype_t* array_type = ASRUtils::expr_type(array);
            ASR::ttype_t* element_type = ASRUtils::duplicate_type_without_dims(al, array_type, loc);
            ASR::expr_t* zero = ASRUtils::get_constant_expression_with_given_type(al, element_type, true);
            ASR::stmt_t* return_var_init = Assignment(return_var, zero);
            fn_body.push_back(al, return_var_init);
        },
        [=, &al, &idx_vars, &doloop_body, &builder] () {
            ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
            ASR::expr_t* add_expr = builder.ElementalAdd(return_var, array_ref, loc);
            ASR::stmt_t* loop_invariant = Assignment(return_var, add_expr);
            doloop_body.push_back(al, loop_invariant);
    });
}

static inline void generate_body_for_array_mask_input(Allocator& al, const Location& loc,
    ASR::expr_t* array, ASR::expr_t* mask, ASR::expr_t* return_var, SymbolTable* fn_scope,
    Vec<ASR::stmt_t*>& fn_body) {
    ASRBuilder builder(al, loc);
    Vec<ASR::expr_t*> idx_vars;
    Vec<ASR::stmt_t*> doloop_body;
    builder.generate_reduction_intrinsic_stmts_for_scalar_output(loc,
        array, fn_scope, fn_body, idx_vars, doloop_body,
        [=, &al, &fn_body] {
            ASR::ttype_t* array_type = ASRUtils::expr_type(array);
            ASR::ttype_t* element_type = ASRUtils::duplicate_type_without_dims(al, array_type, loc);
            ASR::expr_t* zero = ASRUtils::get_constant_expression_with_given_type(al, element_type, true);
            ASR::stmt_t* return_var_init = Assignment(return_var, zero);
            fn_body.push_back(al, return_var_init);
        },
        [=, &al, &idx_vars, &doloop_body, &builder] () {
            ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
            ASR::expr_t* mask_ref = PassUtils::create_array_ref(mask, idx_vars, al);
            ASR::expr_t* add_expr = builder.ElementalAdd(return_var, array_ref, loc);
            ASR::stmt_t* loop_invariant = Assignment(return_var, add_expr);
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
    SymbolTable* fn_scope, Vec<ASR::stmt_t*>& fn_body) {
    ASRBuilder builder(al, loc);
    Vec<ASR::expr_t*> idx_vars, target_idx_vars;
    Vec<ASR::stmt_t*> doloop_body;
    builder.generate_reduction_intrinsic_stmts_for_array_output(
        loc, array, dim, fn_scope, fn_body,
        idx_vars, target_idx_vars, doloop_body,
        [=, &al, &fn_body] () {
            ASR::ttype_t* array_type = ASRUtils::expr_type(array);
            ASR::expr_t* zero = ASRUtils::get_constant_expression_with_given_type(al, array_type, true);
            ASR::stmt_t* result_init = Assignment(result, zero);
            fn_body.push_back(al, result_init);
        },
        [=, &al, &idx_vars, &target_idx_vars, &doloop_body, &builder, &result] () {
            ASR::expr_t* result_ref = PassUtils::create_array_ref(result, target_idx_vars, al);
            ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
            ASR::expr_t* add_expr = builder.ElementalAdd(result_ref, array_ref, loc);
            ASR::stmt_t* loop_invariant = Assignment(result_ref, add_expr);
            doloop_body.push_back(al, loop_invariant);
        });
}

static inline void generate_body_for_array_dim_mask_input(
    Allocator& al, const Location& loc,
    ASR::expr_t* array, ASR::expr_t* dim,
    ASR::expr_t* mask, ASR::expr_t* result,
    SymbolTable* fn_scope, Vec<ASR::stmt_t*>& fn_body) {
    ASRBuilder builder(al, loc);
    Vec<ASR::expr_t*> idx_vars, target_idx_vars;
    Vec<ASR::stmt_t*> doloop_body;
    builder.generate_reduction_intrinsic_stmts_for_array_output(
        loc, array, dim, fn_scope, fn_body,
        idx_vars, target_idx_vars, doloop_body,
        [=, &al, &fn_body] () {
            ASR::ttype_t* array_type = ASRUtils::expr_type(array);
            ASR::expr_t* zero = ASRUtils::get_constant_expression_with_given_type(al, array_type, true);
            ASR::stmt_t* result_init = Assignment(result, zero);
            fn_body.push_back(al, result_init);
        },
        [=, &al, &idx_vars, &target_idx_vars, &doloop_body, &builder, &result] () {
            ASR::expr_t* result_ref = PassUtils::create_array_ref(result, target_idx_vars, al);
            ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
            ASR::expr_t* mask_ref = PassUtils::create_array_ref(mask, idx_vars, al);
            ASR::expr_t* add_expr = builder.ElementalAdd(result_ref, array_ref, loc);
            ASR::stmt_t* loop_invariant = Assignment(result_ref, add_expr);
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

static inline ASR::expr_t* instantiate_Sum(Allocator &al, const Location &loc,
    SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
    Vec<ASR::call_arg_t>& new_args, int64_t overload_id,
    ASR::expr_t* compile_time_value) {
    ASRBuilder builder(al, loc);
    ASRBuilder& b = builder;
    int64_t id_array = 0, id_array_dim = 1, id_array_mask = 2;
    int64_t id_array_dim_mask = 3;

    ASR::ttype_t* arg_type = arg_types[0];
    int kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
    int rank = ASRUtils::extract_n_dims_from_ttype(arg_type);
    std::string new_name = "sum_" + std::to_string(kind) +
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
                return builder.Call(s, new_args, return_type, compile_time_value);
            } else {
                new_func_name += std::to_string(i);
                i++;
            }
        }
    }

    new_name = scope->get_unique_name(new_name);
    SymbolTable *fn_symtab = al.make_new<SymbolTable>(scope);

    Vec<ASR::expr_t*> args;
    args.reserve(al, 1);

    ASR::ttype_t* array_type = ASRUtils::duplicate_type_with_empty_dims(al, arg_type);
    fill_func_arg("array", array_type)
    if( overload_id == id_array_dim ||
        overload_id == id_array_dim_mask ) {
        ASR::ttype_t* dim_type = ASRUtils::TYPE(ASR::make_Integer_t(
                                    al, arg_type->base.loc, 4, nullptr, 0));
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
                        al, arg_type->base.loc,
                        4, mask_dims.p, mask_dims.size()));
        fill_func_arg("mask", mask_type)
    }

    ASR::ttype_t* return_type = nullptr;
    int result_dims = 0;
    if( overload_id == id_array_mask ||
        overload_id == id_array ) {
        return_type = ASRUtils::duplicate_type_without_dims(al, arg_type, loc);
    } else if( overload_id == id_array_dim_mask ||
               overload_id == id_array_dim ) {
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
        return_type = ASRUtils::duplicate_type(al, arg_type, &dims);
    }
    LCOMPILERS_ASSERT(return_type != nullptr);

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
                                      fn_symtab, body);
    } else if( overload_id == id_array_dim ) {
        generate_body_for_array_dim_input(al, loc, args[0], args[1], output_var,
                                          fn_symtab, body);
    } else if( overload_id == id_array_dim_mask ) {
        generate_body_for_array_dim_mask_input(al, loc, args[0], args[1], args[2],
                                               output_var, fn_symtab, body);
    } else if( overload_id == id_array_mask ) {
        generate_body_for_array_mask_input(al, loc, args[0], args[1], output_var,
                                           fn_symtab, body);
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
    return builder.Call(new_symbol, new_args, return_type,
                        compile_time_value);
}

} // namespace Sum

namespace Partition {

    static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 2, "Call to partition must have exactly two arguments",
            x.base.base.loc, diagnostics);
        ASRUtils::require_impl(ASR::is_a<ASR::Character_t>(*ASRUtils::expr_type(x.m_args[0])) &&
            ASR::is_a<ASR::Character_t>(*ASRUtils::expr_type(x.m_args[1])),
            "Both arguments to partition must be of character type",
            x.base.base.loc, diagnostics);
        ASRUtils::require_impl(ASR::is_a<ASR::Tuple_t>(*x.m_type),
            "Return type of partition must be a tuple",
            x.base.base.loc, diagnostics);
    }

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
        return b.TupleConstant({ StringConstant(first_res, first_res_type),
            StringConstant(second_res, second_res_type),
            StringConstant(third_res, third_res_type) },
            b.Tuple({first_res_type, second_res_type, third_res_type}));
    }

    static inline ASR::asr_t *create_partition(Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args, ASR::expr_t *s_var,
            const std::function<void (const std::string &, const Location &)> err) {
        ASRBuilder b(al, loc);
        if (args.size() != 1) {
            err("str.partition() takes exactly one argument", loc);
        }
        ASR::expr_t *arg = args[0];
        if (!ASRUtils::is_character(*expr_type(arg))) {
            err("str.partition() takes one arguments of type: str", arg->base.loc);
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
                err("Separator cannot be an empty string", arg->base.loc);
            }
            value = eval_Partition(al, loc, s_str, s_sep);
        }

        return ASR::make_IntrinsicFunction_t(al, loc,
            static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Partition),
            e_args.p, e_args.n, 0, return_type, value);
    }

    static inline ASR::expr_t *instantiate_Partition(Allocator &al,
        const Location &loc, SymbolTable *scope,
        Vec<ASR::ttype_t*>& /*arg_types*/, Vec<ASR::call_arg_t>& new_args,
        int64_t /*overload_id*/, ASR::expr_t* compile_time_value)
    {
        // TODO: show runtime error for empty separator or pattern
        declare_basic_variables("_lpython_str_partition");
        fill_func_arg("target_string", character(-2));
        fill_func_arg("pattern", character(-2));

        ASR::ttype_t *return_type = b.Tuple({character(-2), character(-2), character(-2)});
        auto result = declare("result", return_type, ReturnVar);
        auto index = declare("index", int32, Local);
        body.push_back(al, Assignment(index, b.Call(UnaryIntrinsicFunction::
            create_KMP_function(al, loc, scope), args, int32)));
        body.push_back(al, b.If(iEq(index, i32_n(-1)), {
                Assignment(result, b.TupleConstant({ args[0],
                    StringConstant("", character(0)),
                    StringConstant("", character(0)) },
                b.Tuple({character(-2), character(0), character(0)})))
            }, {
                Assignment(result, b.TupleConstant({
                    StringSection(args[0], i32(0), index), args[1],
                    StringSection(args[0], iAdd(index, StringLen(args[1])),
                        StringLen(args[0]))}, return_type))
            }));
        body.push_back(al, Return());
        ASR::symbol_t *fn_sym = make_Function_t(fn_name, fn_symtab, dep, args,
            body, result, Source, Implementation, nullptr);
        scope->add_symbol(fn_name, fn_sym);
        return b.Call(fn_sym, new_args, return_type, compile_time_value);
    }
} // namespace Partition


namespace IntrinsicFunctionRegistry {

    static const std::map<int64_t,
        std::tuple<impl_function,
                   verify_function>>& intrinsic_function_by_id_db = {
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::LogGamma),
            {&LogGamma::instantiate_LogGamma, &UnaryIntrinsicFunction::verify_args}},

        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Sin),
            {&Sin::instantiate_Sin, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Cos),
            {&Cos::instantiate_Cos, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Tan),
            {&Tan::instantiate_Tan, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Asin),
            {&Asin::instantiate_Asin, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Acos),
            {&Acos::instantiate_Acos, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Atan),
            {&Atan::instantiate_Atan, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Sinh),
            {&Sinh::instantiate_Sinh, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Cosh),
            {&Cosh::instantiate_Cosh, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Tanh),
            {&Tanh::instantiate_Tanh, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Exp),
            {nullptr, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Exp2),
            {nullptr, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Expm1),
            {nullptr, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Abs),
            {&Abs::instantiate_Abs, &Abs::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Any),
            {&Any::instantiate_Any, &Any::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Sum),
            {&Sum::instantiate_Sum, &Sum::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Partition),
            {&Partition::instantiate_Partition, &Partition::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::ListIndex),
            {nullptr, &ListIndex::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::ListReverse),
            {nullptr, &ListReverse::verify_args}},
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
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Sinh),
            "sinh"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Cosh),
            "cosh"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Tanh),
            "tanh"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Abs),
            "abs"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Exp),
            "exp"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Exp2),
            "exp2"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Expm1),
            "expm1"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::ListIndex),
            "list.index"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::ListReverse),
            "list.reverse"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Any),
            "any"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Sum),
            "sum"},
    };


    static const std::map<std::string,
        std::tuple<create_intrinsic_function,
                    eval_intrinsic_function>>& intrinsic_function_by_name_db = {
                {"log_gamma", {&LogGamma::create_LogGamma, &LogGamma::eval_log_gamma}},
                {"sin", {&Sin::create_Sin, &Sin::eval_Sin}},
                {"cos", {&Cos::create_Cos, &Cos::eval_Cos}},
                {"tan", {&Tan::create_Tan, &Tan::eval_Tan}},
                {"asin", {&Asin::create_Asin, &Asin::eval_Asin}},
                {"acos", {&Acos::create_Acos, &Acos::eval_Acos}},
                {"atan", {&Atan::create_Atan, &Atan::eval_Atan}},
                {"sinh", {&Sinh::create_Sinh, &Sinh::eval_Sinh}},
                {"cosh", {&Cosh::create_Cosh, &Cosh::eval_Cosh}},
                {"tanh", {&Tanh::create_Tanh, &Tanh::eval_Tanh}},
                {"abs", {&Abs::create_Abs, &Abs::eval_Abs}},
                {"exp", {&Exp::create_Exp, &Exp::eval_Exp}},
                {"exp2", {&Exp2::create_Exp2, &Exp2::eval_Exp2}},
                {"expm1", {&Expm1::create_Expm1, &Expm1::eval_Expm1}},
                {"any", {&Any::create_Any, &Any::eval_Any}},
                {"sum", {&Sum::create_Sum, &Sum::eval_Sum}},
                {"list.index", {&ListIndex::create_ListIndex, &ListIndex::eval_list_index}},
                {"list.reverse", {&ListReverse::create_ListReverse, &ListReverse::eval_list_reverse}},
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
                 id_ == ASRUtils::IntrinsicFunctions::Sin ||
                 id_ == ASRUtils::IntrinsicFunctions::Exp ||
                 id_ == ASRUtils::IntrinsicFunctions::Exp2 ||
                 id_ == ASRUtils::IntrinsicFunctions::Expm1 );
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
        if( id == ASRUtils::IntrinsicFunctions::Any ||
            id == ASRUtils::IntrinsicFunctions::Sum ) {
            return 1;
        } else {
            LCOMPILERS_ASSERT(false);
        }
        return -1;
    }

    static inline create_intrinsic_function get_create_function(const std::string& name) {
        return  std::get<0>(intrinsic_function_by_name_db.at(name));
    }

    static inline verify_function get_verify_function(int64_t id) {
        return std::get<1>(intrinsic_function_by_id_db.at(id));
    }

    static inline impl_function get_instantiate_function(int64_t id) {
        if( intrinsic_function_by_id_db.find(id) == intrinsic_function_by_id_db.end() ) {
            return nullptr;
        }
        return std::get<0>(intrinsic_function_by_id_db.at(id));
    }

    static inline std::string get_intrinsic_function_name(int64_t id) {
        if( intrinsic_function_id_to_name.find(id) == intrinsic_function_id_to_name.end() ) {
            throw LCompilersException("IntrinsicFunction with ID " + std::to_string(id) +
                                      " has no name registered for it");
        }
        return intrinsic_function_id_to_name.at(id);
    }

    static inline bool is_input_type_supported(const std::string& name, Vec<ASR::expr_t*>& args) {
        if( name == "exp" ) {
            if( !ASRUtils::is_real(*ASRUtils::expr_type(args[0])) ) {
                return false;
            }
        }
        return true;
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
        INTRINSIC_NAME_CASE(Sinh)
        INTRINSIC_NAME_CASE(Cosh)
        INTRINSIC_NAME_CASE(Tanh)
        INTRINSIC_NAME_CASE(Gamma)
        INTRINSIC_NAME_CASE(LogGamma)
        INTRINSIC_NAME_CASE(Abs)
        INTRINSIC_NAME_CASE(Exp)
        INTRINSIC_NAME_CASE(Exp2)
        INTRINSIC_NAME_CASE(Expm1)
        INTRINSIC_NAME_CASE(Any)
        INTRINSIC_NAME_CASE(ListIndex)
        INTRINSIC_NAME_CASE(Partition)
        INTRINSIC_NAME_CASE(ListReverse)
        INTRINSIC_NAME_CASE(Sum)
        default : {
            throw LCompilersException("pickle: intrinsic_id not implemented");
        }
    }
}

} // namespace ASRUtils

} // namespace LCompilers

#endif // LFORTRAN_PASS_INTRINSIC_FUNCTION_REGISTRY_H
