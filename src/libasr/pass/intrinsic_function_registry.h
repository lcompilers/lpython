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
    ListPop,
    Sum,
    Product,
    Max,
    MaxVal,
    Min,
    MinVal,
    Merge,
    SymbolicSymbol,
    SymbolicAdd,
    SymbolicSub,
    SymbolicMul,
    SymbolicDiv,
    SymbolicPow,
    SymbolicPi,
    SymbolicInteger,
    SymbolicDiff,
    SymbolicExpand,
    SymbolicSin,
    SymbolicCos,
    SymbolicLog,
    SymbolicExp,
    SymbolicAbs,
    // ...
};

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
        INTRINSIC_NAME_CASE(ListPop)
        INTRINSIC_NAME_CASE(Sum)
        INTRINSIC_NAME_CASE(Max)
        INTRINSIC_NAME_CASE(Min)
        INTRINSIC_NAME_CASE(Product)
        INTRINSIC_NAME_CASE(MaxVal)
        INTRINSIC_NAME_CASE(MinVal)
        INTRINSIC_NAME_CASE(Merge)
        INTRINSIC_NAME_CASE(SymbolicSymbol)
        INTRINSIC_NAME_CASE(SymbolicAdd)
        INTRINSIC_NAME_CASE(SymbolicSub)
        INTRINSIC_NAME_CASE(SymbolicMul)
        INTRINSIC_NAME_CASE(SymbolicDiv)
        INTRINSIC_NAME_CASE(SymbolicPow)
        INTRINSIC_NAME_CASE(SymbolicPi)
        INTRINSIC_NAME_CASE(SymbolicInteger)
        INTRINSIC_NAME_CASE(SymbolicDiff)
        INTRINSIC_NAME_CASE(SymbolicExpand)
        INTRINSIC_NAME_CASE(SymbolicSin)
        INTRINSIC_NAME_CASE(SymbolicCos)
        INTRINSIC_NAME_CASE(SymbolicLog)
        INTRINSIC_NAME_CASE(SymbolicExp)
        INTRINSIC_NAME_CASE(SymbolicAbs)
        default : {
            throw LCompilersException("pickle: intrinsic_id not implemented");
        }
    }
}

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

typedef void (*verify_array_func)(ASR::expr_t*, ASR::ttype_t*,
    const Location&, diag::Diagnostics&,
    ASRUtils::IntrinsicFunctions);

typedef ASR::expr_t* (*get_initial_value_func)(Allocator&, ASR::ttype_t*);


class ASRBuilder {
    private:

    Allocator& al;
    // TODO: use the location to point C++ code in `intrinsic_function_registry`
    const Location &loc;

    public:

    ASRBuilder(Allocator& al_, const Location& loc_): al(al_), loc(loc_) {}

    #define make_ConstantWithKind(Constructor, TypeConstructor, value, kind, loc) ASRUtils::EXPR( \
        ASR::Constructor( al, loc, value, \
            ASRUtils::TYPE(ASR::TypeConstructor(al, loc, 4)))) \

    #define make_ConstantWithType(Constructor, value, type, loc) ASRUtils::EXPR( \
        ASR::Constructor(al, loc, value, type)) \

    #define declare_basic_variables(name)                                       \
        std::string fn_name = scope->get_unique_name(name, false);                     \
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
    #define int32        TYPE(ASR::make_Integer_t(al, loc, 4))
    #define logical      TYPE(ASR::make_Logical_t(al, loc, 4))
    #define character(x) TYPE(ASR::make_Character_t(al, loc, 1, x, nullptr))
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

    #define make_Compare(Constructor, left, op, right) ASRUtils::EXPR(ASR::Constructor( \
        al, loc, left, ASR::cmpopType::op, right, \
        ASRUtils::TYPE(ASR::make_Logical_t( \
            al, loc, 4)), nullptr)); \

    #define create_ElementalBinOp(OpType, BinOpName, OpName, value) case ASR::ttypeType::OpType: { \
        return ASRUtils::EXPR(ASR::BinOpName(al, loc, \
                left, ASR::binopType::OpName, right, \
                ASRUtils::expr_type(left), value)); \
    } \

    ASR::expr_t* ElementalAdd(ASR::expr_t* left, ASR::expr_t* right,
        const Location& loc, ASR::expr_t* value=nullptr) {
        ASR::ttype_t *left_type = ASRUtils::expr_type(left);
        left_type = ASRUtils::type_get_past_pointer(left_type);
        switch (left_type->type) {
            create_ElementalBinOp(Real, make_RealBinOp_t, Add, value)
            create_ElementalBinOp(Integer, make_IntegerBinOp_t, Add, value)
            create_ElementalBinOp(Complex, make_ComplexBinOp_t, Add, value)
            default: {
                throw LCompilersException("Expression type, " +
                                          std::to_string(left_type->type) +
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

    ASR::expr_t* ElementalMul(ASR::expr_t* left, ASR::expr_t* right,
        const Location& loc, ASR::expr_t* value=nullptr) {
        switch (ASRUtils::expr_type(left)->type) {
            create_ElementalBinOp(Real, make_RealBinOp_t, Mul, value)
            create_ElementalBinOp(Integer, make_IntegerBinOp_t, Mul, value)
            create_ElementalBinOp(Complex, make_ComplexBinOp_t, Mul, value)
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

    ASR::expr_t* ElementalMax(ASR::expr_t* left, ASR::expr_t* right,
        const Location& loc, ASR::expr_t* value=nullptr) {
        ASR::expr_t* test_condition = nullptr;
        switch (ASRUtils::expr_type(left)->type) {
            case ASR::ttypeType::Integer: {
                test_condition = make_Compare(make_IntegerCompare_t, left, Gt, right);
                break;
            }
            case ASR::ttypeType::Real: {
                test_condition = make_Compare(make_RealCompare_t, left, Gt, right);
                break;
            }
            default: {
                throw LCompilersException("Expression type, " +
                    std::to_string(left->type) + " not yet supported");
            }
        }
        return ASRUtils::EXPR(ASR::make_IfExp_t(al, loc, test_condition, left, right, ASRUtils::expr_type(left), value));
    }

    ASR::expr_t* ElementalMin(ASR::expr_t* left, ASR::expr_t* right,
        const Location& loc, ASR::expr_t* value=nullptr) {
        ASR::expr_t* test_condition = nullptr;
        switch (ASRUtils::expr_type(left)->type) {
            case ASR::ttypeType::Integer: {
                test_condition = make_Compare(make_IntegerCompare_t, left, Lt, right);
                break;
            }
            case ASR::ttypeType::Real: {
                test_condition = make_Compare(make_RealCompare_t, left, Lt, right);
                break;
            }
            default: {
                throw LCompilersException("Expression type, " +
                    std::to_string(left->type) + " not yet supported");
            }
        }
        return ASRUtils::EXPR(ASR::make_IfExp_t(al, loc, test_condition, left, right, ASRUtils::expr_type(left), value));
    }

    ASR::expr_t* ElementalOr(ASR::expr_t* left, ASR::expr_t* right,
        const Location& loc) {
        return ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al, loc,
            left, ASR::Or, right,
            ASRUtils::TYPE(ASR::make_Logical_t( al, loc, 4)), nullptr));
    }

    ASR::expr_t* Or(ASR::expr_t* left, ASR::expr_t* right,
        const Location& loc) {
        return ASRUtils::EXPR(ASR::make_LogicalBinOp_t(al, loc,
            left, ASR::Or, right, ASRUtils::expr_type(left),
            nullptr));
    }

    ASR::expr_t* Call(ASR::symbol_t* s, Vec<ASR::call_arg_t>& args,
                      ASR::ttype_t* return_type) {
        return ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, loc,
                s, s, args.p, args.size(), return_type, nullptr, nullptr));
    }

    ASR::expr_t* Call(ASR::symbol_t* s, Vec<ASR::expr_t *>& args,
                      ASR::ttype_t* return_type) {
        Vec<ASR::call_arg_t> args_; args_.reserve(al, 2);
        visit_expr_list(al, args, args_);
        return ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, loc,
                s, s, args_.p, args_.size(), return_type, nullptr, nullptr));
    }

    ASR::expr_t* Call(ASR::symbol_t* s, Vec<ASR::call_arg_t>& args,
                      ASR::ttype_t* return_type, ASR::expr_t* value) {
        return ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, loc,
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
                                        Eq, current_dim);

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

    return ASRUtils::make_IntrinsicFunction_t_util(al, loc, intrinsic_id,
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
                ASRUtils::extract_kind_from_ttype_t(type)));
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
                test = make_Compare(make_IntegerCompare_t, args[0], GtE, zero);
                negative_x = EXPR(ASR::make_IntegerUnaryMinus_t(al, loc, args[0],
                    arg_types[0], nullptr));
            } else {
                ASR::expr_t* zero = make_ConstantWithType(make_RealConstant_t, 0.0, arg_types[0], loc);
                test = make_Compare(make_RealCompare_t, args[0], GtE, zero);
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
                                        ASRUtils::extract_kind_from_ttype_t(arg_types[0])));
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
                    arg.loc = args[0]->base.loc;
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
    ASRUtils::require_impl(x.n_args <= 4, "Call to list.index must have at most four arguments",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASR::is_a<ASR::List_t>(*ASRUtils::expr_type(x.m_args[0])) &&
        ASRUtils::check_equal_type(ASRUtils::expr_type(x.m_args[1]),
            ASRUtils::get_contained_type(ASRUtils::expr_type(x.m_args[0]))),
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
    const Location &/*loc*/, Vec<ASR::expr_t*>& /*args*/) {
    // TODO: To be implemented for ListConstant expression
    return nullptr;
}


static inline ASR::asr_t* create_ListIndex(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    int64_t overload_id = 0;
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
    if (args.size() >= 3) {
        overload_id = 1;
        if(!ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(args[2]))) {
            err("Third argument to list.index must be an integer", loc);
        }
    }
    if (args.size() == 4) {
        overload_id = 2;
        if(!ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(args[3]))) {
            err("Fourth argument to list.index must be an integer", loc);
        }
    }
    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, args.size());
    for( size_t i = 0; i < args.size(); i++ ) {
        arg_values.push_back(al, ASRUtils::expr_value(args[i]));
    }
    ASR::expr_t* compile_time_value = eval_list_index(al, loc, arg_values);
    ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
    return ASR::make_IntrinsicFunction_t(al, loc,
            static_cast<int64_t>(ASRUtils::IntrinsicFunctions::ListIndex),
            args.p, args.size(), overload_id, to_type, compile_time_value);
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
            ASRUtils::EXPR(ASRUtils::make_IntrinsicFunction_t_util(al, loc,
            static_cast<int64_t>(ASRUtils::IntrinsicFunctions::ListReverse),
            args.p, args.size(), 0, nullptr, compile_time_value)));
}

} // namespace ListReverse

namespace ListPop {

static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
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
    const Location &/*loc*/, Vec<ASR::expr_t*>& /*args*/) {
    // TODO: To be implemented for ListConstant expression
    return nullptr;
}

static inline ASR::asr_t* create_ListPop(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    if (args.size() > 2) {
        err("Call to list.pop must have at most one argument", loc);
    }
    if (args.size() == 2 &&
        !ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(args[1]))) {
        err("Argument to list.pop must be an integer", loc);
    }

    ASR::expr_t* list_expr = args[0];
    ASR::ttype_t *type = ASRUtils::expr_type(list_expr);
    ASR::ttype_t *list_type = ASR::down_cast<ASR::List_t>(type)->m_type;

    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, args.size());
    for( size_t i = 0; i < args.size(); i++ ) {
        arg_values.push_back(al, ASRUtils::expr_value(args[i]));
    }
    ASR::expr_t* compile_time_value = eval_list_pop(al, loc, arg_values);
    ASR::ttype_t *to_type = list_type;
    int64_t overload_id = (args.size() == 2);
    return ASR::make_IntrinsicFunction_t(al, loc,
            static_cast<int64_t>(ASRUtils::IntrinsicFunctions::ListPop),
            args.p, args.size(), overload_id, to_type, compile_time_value);
}

} // namespace ListPop

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

    any_args.push_back(al, array);
    if( axis ) {
        any_args.push_back(al, axis);
    }

    return ASRUtils::make_IntrinsicFunction_t_util(al, loc,
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

    new_name = scope->get_unique_name(new_name, false);
    SymbolTable *fn_symtab = al.make_new<SymbolTable>(scope);

    ASR::ttype_t* logical_return_type = ASRUtils::TYPE(ASR::make_Logical_t(
                                            al, loc, 4));
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
                logical_return_type = ASRUtils::make_Array_t_util(al, loc,
                    ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)),
                        dims.p, dims.size());
            } else {
                logical_return_type = ASRUtils::TYPE(ASR::make_Logical_t(
                                        al, loc, 4));
            }
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

namespace Max {
    static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args > 1, "ASR Verify: Call to max0 must have at least two arguments",
            x.base.base.loc, diagnostics);
        ASRUtils::require_impl(ASR::is_a<ASR::Real_t>(*ASRUtils::expr_type(x.m_args[0])) ||
            ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(x.m_args[0])),
             "ASR Verify: Arguments to max0 must be of real or integer type",
            x.base.base.loc, diagnostics);
        for(size_t i=0;i<x.n_args;i++){
            ASRUtils::require_impl((ASR::is_a<ASR::Real_t>(*ASRUtils::expr_type(x.m_args[i])) &&
                                            ASR::is_a<ASR::Real_t>(*ASRUtils::expr_type(x.m_args[0]))) ||
                                        (ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(x.m_args[i])) &&
                                         ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(x.m_args[0]))),
            "ASR Verify: All arguments must be of the same type",
            x.base.base.loc, diagnostics);
        }
    }

    static ASR::expr_t *eval_Max(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::ttype_t* arg_type = ASRUtils::expr_type(args[0]);
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
        } else {
            return nullptr;
        }
    }

    static inline ASR::asr_t* create_Max(
        Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
        const std::function<void (const std::string &, const Location &)> err) {
        bool is_compile_time = true;
        for(size_t i=0; i<100;i++){
            args.erase(nullptr);
        }
        if (args.size() < 2) {
            err("Intrinsic max0 must have 2 arguments", loc);
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
            ASR::expr_t *value = eval_Max(al, loc, arg_values);
            return ASR::make_IntrinsicFunction_t(al, loc,
                static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Max),
                args.p, args.n, 0, ASRUtils::expr_type(args[0]), value);
        } else {
            return ASR::make_IntrinsicFunction_t(al, loc,
                static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Max),
                args.p, args.n, 0, ASRUtils::expr_type(args[0]), nullptr);
        }
    }

    static inline ASR::expr_t* instantiate_Max(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/, ASR::expr_t* compile_time_value) {
        std::string func_name = "_lcompilers_max0_" + type_to_str_python(arg_types[0]);
        ASR::ttype_t *return_type = arg_types[0];
        std::string fn_name = scope->get_unique_name(func_name, false);
        SymbolTable *fn_symtab = al.make_new<SymbolTable>(scope);
        Vec<ASR::expr_t*> args;
        args.reserve(al, new_args.size());
        ASRBuilder b(al, loc);
        Vec<ASR::stmt_t*> body; body.reserve(al, args.size());
        SetChar dep; dep.reserve(al, 1);
        if (scope->get_symbol(fn_name)) {
            ASR::symbol_t *s = scope->get_symbol(fn_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var),
                                compile_time_value);
        }
        for (size_t i = 0; i < new_args.size(); i++) {
            fill_func_arg("x" + std::to_string(i), arg_types[0]);
        }

        auto result = declare(fn_name, return_type, ReturnVar);

        ASR::expr_t* test;
        body.push_back(al, Assignment(result, args[0]));
        for (size_t i = 1; i < args.size(); i++) {
            test = make_Compare(make_IntegerCompare_t, args[i], Gt, result);
            Vec<ASR::stmt_t *> if_body; if_body.reserve(al, 1);
            if_body.push_back(al, Assignment(result, args[i]));
            body.push_back(al, STMT(ASR::make_If_t(al, loc, test,
                if_body.p, if_body.n, nullptr, 0)));
        }
        ASR::symbol_t *f_sym = make_Function_t(fn_name, fn_symtab, dep, args,
            body, result, Source, Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, compile_time_value);
    }

}  // namespace max0

namespace Min {
    static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args > 1, "ASR Verify: Call to min0 must have at least two arguments",
            x.base.base.loc, diagnostics);
        ASRUtils::require_impl(ASR::is_a<ASR::Real_t>(*ASRUtils::expr_type(x.m_args[0])) ||
            ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(x.m_args[0])),
             "ASR Verify: Arguments to min0 must be of real or integer type",
            x.base.base.loc, diagnostics);
        for(size_t i=0;i<x.n_args;i++){
            ASRUtils::require_impl((ASR::is_a<ASR::Real_t>(*ASRUtils::expr_type(x.m_args[i])) &&
                                            ASR::is_a<ASR::Real_t>(*ASRUtils::expr_type(x.m_args[0]))) ||
                                        (ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(x.m_args[i])) &&
                                         ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(x.m_args[0]))),
            "ASR Verify: All arguments must be of the same type",
            x.base.base.loc, diagnostics);
        }
    }

    static ASR::expr_t *eval_Min(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::ttype_t* arg_type = ASRUtils::expr_type(args[0]);
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
        } else {
            return nullptr;
        }
    }

    static inline ASR::asr_t* create_Min(
        Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
        const std::function<void (const std::string &, const Location &)> err) {
        bool is_compile_time = true;
        for(size_t i=0; i<100;i++){
            args.erase(nullptr);
        }
        if (args.size() < 2) {
            err("Intrinsic min0 must have 2 arguments", loc);
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
            ASR::expr_t *value = eval_Min(al, loc, arg_values);
            return ASR::make_IntrinsicFunction_t(al, loc,
                static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Min),
                args.p, args.n, 0, ASRUtils::expr_type(args[0]), value);
        } else {
            return ASR::make_IntrinsicFunction_t(al, loc,
                static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Min),
                args.p, args.n, 0, ASRUtils::expr_type(args[0]), nullptr);
        }
    }

    static inline ASR::expr_t* instantiate_Min(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/, ASR::expr_t* compile_time_value) {
        std::string func_name = "_lcompilers_min0_" + type_to_str_python(arg_types[0]);
        ASR::ttype_t *return_type = arg_types[0];
        std::string fn_name = scope->get_unique_name(func_name, false);
        SymbolTable *fn_symtab = al.make_new<SymbolTable>(scope);
        Vec<ASR::expr_t*> args;
        args.reserve(al, new_args.size());
        ASRBuilder b(al, loc);
        Vec<ASR::stmt_t*> body; body.reserve(al, args.size());
        SetChar dep; dep.reserve(al, 1);
        if (scope->get_symbol(fn_name)) {
            ASR::symbol_t *s = scope->get_symbol(fn_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var),
                                compile_time_value);
        }
        for (size_t i = 0; i < new_args.size(); i++) {
            fill_func_arg("x" + std::to_string(i), arg_types[0]);
        }

        auto result = declare(fn_name, return_type, ReturnVar);

        ASR::expr_t* test;
        body.push_back(al, Assignment(result, args[0]));
        if (return_type->type == ASR::ttypeType::Integer) {
            for (size_t i = 1; i < args.size(); i++) {
                test = make_Compare(make_IntegerCompare_t, args[i], Lt, result);
                Vec<ASR::stmt_t *> if_body; if_body.reserve(al, 1);
                if_body.push_back(al, Assignment(result, args[i]));
                body.push_back(al, STMT(ASR::make_If_t(al, loc, test,
                    if_body.p, if_body.n, nullptr, 0)));
            }
        } else if (return_type->type == ASR::ttypeType::Real) {
            for (size_t i = 1; i < args.size(); i++) {
                test = make_Compare(make_RealCompare_t, args[i], Lt, result);
                Vec<ASR::stmt_t *> if_body; if_body.reserve(al, 1);
                if_body.push_back(al, Assignment(result, args[i]));
                body.push_back(al, STMT(ASR::make_If_t(al, loc, test,
                    if_body.p, if_body.n, nullptr, 0)));
            }
        } else {
            throw LCompilersException("Arguments to min0 must be of real or integer type");
        }
        ASR::symbol_t *f_sym = make_Function_t(fn_name, fn_symtab, dep, args,
            body, result, Source, Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, compile_time_value);
    }

}  // namespace min0

typedef ASR::expr_t* (ASRBuilder::*elemental_operation_func)(ASR::expr_t*, ASR::expr_t*, const Location&, ASR::expr_t*);

namespace ArrIntrinsic {

static inline void verify_array_int_real_cmplx(ASR::expr_t* array, ASR::ttype_t* return_type,
    const Location& loc, diag::Diagnostics& diagnostics, ASRUtils::IntrinsicFunctions intrinsic_func_id) {
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
    const Location& loc, diag::Diagnostics& diagnostics, ASRUtils::IntrinsicFunctions intrinsic_func_id) {
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
    ASR::ttype_t* return_type, const Location& loc, diag::Diagnostics& diagnostics, ASRUtils::IntrinsicFunctions intrinsic_func_id) {
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

static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics,
    ASRUtils::IntrinsicFunctions intrinsic_func_id, verify_array_func verify_array) {
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
    const Location & /*loc*/, Vec<ASR::expr_t*>& /*args*/) {
    return nullptr;
}

static inline ASR::asr_t* create_ArrIntrinsic(
    Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err,
    ASRUtils::IntrinsicFunctions intrinsic_func_id) {
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
    value = eval_ArrIntrinsic(al, loc, arg_values);

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

    Vec<ASR::expr_t*> arr_intrinsic_args;
    arr_intrinsic_args.reserve(al, 3);
    arr_intrinsic_args.push_back(al, array);
    if( arg2 ) {
        arr_intrinsic_args.push_back(al, arg2);
    }
    if( arg3 ) {
        arr_intrinsic_args.push_back(al, arg3);
    }

    return ASRUtils::make_IntrinsicFunction_t_util(al, loc,
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
        [=, &al, &fn_body] {
            ASR::ttype_t* array_type = ASRUtils::expr_type(array);
            ASR::ttype_t* element_type = ASRUtils::duplicate_type_without_dims(al, array_type, loc);
            ASR::expr_t* initial_val = get_initial_value(al, element_type);
            ASR::stmt_t* return_var_init = Assignment(return_var, initial_val);
            fn_body.push_back(al, return_var_init);
        },
        [=, &al, &idx_vars, &doloop_body, &builder] () {
            ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
            ASR::expr_t* elemental_operation_val = (builder.*elemental_operation)(return_var, array_ref, loc, nullptr);
            ASR::stmt_t* loop_invariant = Assignment(return_var, elemental_operation_val);
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
        [=, &al, &fn_body] {
            ASR::ttype_t* array_type = ASRUtils::expr_type(array);
            ASR::ttype_t* element_type = ASRUtils::duplicate_type_without_dims(al, array_type, loc);
            ASR::expr_t* initial_val = get_initial_value(al, element_type);
            ASR::stmt_t* return_var_init = Assignment(return_var, initial_val);
            fn_body.push_back(al, return_var_init);
        },
        [=, &al, &idx_vars, &doloop_body, &builder] () {
            ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
            ASR::expr_t* mask_ref = PassUtils::create_array_ref(mask, idx_vars, al);
            ASR::expr_t* elemental_operation_val = (builder.*elemental_operation)(return_var, array_ref, loc, nullptr);
            ASR::stmt_t* loop_invariant = Assignment(return_var, elemental_operation_val);
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
        [=, &al, &fn_body] () {
            ASR::ttype_t* array_type = ASRUtils::expr_type(array);
            ASR::expr_t* initial_val = get_initial_value(al, array_type);
            ASR::stmt_t* result_init = Assignment(result, initial_val);
            fn_body.push_back(al, result_init);
        },
        [=, &al, &idx_vars, &target_idx_vars, &doloop_body, &builder, &result] () {
            ASR::expr_t* result_ref = PassUtils::create_array_ref(result, target_idx_vars, al);
            ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
            ASR::expr_t* elemental_operation_val = (builder.*elemental_operation)(result_ref, array_ref, loc, nullptr);
            ASR::stmt_t* loop_invariant = Assignment(result_ref, elemental_operation_val);
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
        [=, &al, &fn_body] () {
            ASR::ttype_t* array_type = ASRUtils::expr_type(array);
            ASR::expr_t* initial_val = get_initial_value(al, array_type);
            ASR::stmt_t* result_init = Assignment(result, initial_val);
            fn_body.push_back(al, result_init);
        },
        [=, &al, &idx_vars, &target_idx_vars, &doloop_body, &builder, &result] () {
            ASR::expr_t* result_ref = PassUtils::create_array_ref(result, target_idx_vars, al);
            ASR::expr_t* array_ref = PassUtils::create_array_ref(array, idx_vars, al);
            ASR::expr_t* mask_ref = PassUtils::create_array_ref(mask, idx_vars, al);
            ASR::expr_t* elemental_operation_val = (builder.*elemental_operation)(result_ref, array_ref, loc, nullptr);
            ASR::stmt_t* loop_invariant = Assignment(result_ref, elemental_operation_val);
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

static inline ASR::expr_t* instantiate_ArrIntrinsic(Allocator &al, const Location &loc,
    SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
    Vec<ASR::call_arg_t>& new_args, int64_t overload_id,
    ASR::expr_t* compile_time_value, ASRUtils::IntrinsicFunctions intrinsic_func_id,
    get_initial_value_func get_initial_value, elemental_operation_func elemental_operation) {
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
    return builder.Call(new_symbol, new_args, return_type,
                        compile_time_value);
}

} // namespace ArrIntrinsic

namespace Sum {

static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
    ArrIntrinsic::verify_args(x, diagnostics, ASRUtils::IntrinsicFunctions::Sum, &ArrIntrinsic::verify_array_int_real_cmplx);
}

static inline ASR::expr_t *eval_Sum(Allocator & /*al*/,
    const Location & /*loc*/, Vec<ASR::expr_t*>& /*args*/) {
    return nullptr;
}

static inline ASR::asr_t* create_Sum(
    Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    return ArrIntrinsic::create_ArrIntrinsic(al, loc, args, err, ASRUtils::IntrinsicFunctions::Sum);
}

static inline ASR::expr_t* instantiate_Sum(Allocator &al, const Location &loc,
    SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
    Vec<ASR::call_arg_t>& new_args, int64_t overload_id,
    ASR::expr_t* compile_time_value) {
    return ArrIntrinsic::instantiate_ArrIntrinsic(al, loc, scope, arg_types, new_args,
        overload_id, compile_time_value, ASRUtils::IntrinsicFunctions::Sum,
        &ASRUtils::get_constant_zero_with_given_type, &ASRUtils::ASRBuilder::ElementalAdd);
}

} // namespace Sum

namespace Product {

static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
    ArrIntrinsic::verify_args(x, diagnostics, ASRUtils::IntrinsicFunctions::Product, &ArrIntrinsic::verify_array_int_real_cmplx);
}

static inline ASR::expr_t *eval_Product(Allocator & /*al*/,
    const Location & /*loc*/, Vec<ASR::expr_t*>& /*args*/) {
    return nullptr;
}

static inline ASR::asr_t* create_Product(
    Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    return ArrIntrinsic::create_ArrIntrinsic(al, loc, args, err, ASRUtils::IntrinsicFunctions::Product);
}

static inline ASR::expr_t* instantiate_Product(Allocator &al, const Location &loc,
    SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
    Vec<ASR::call_arg_t>& new_args, int64_t overload_id,
    ASR::expr_t* compile_time_value) {
    return ArrIntrinsic::instantiate_ArrIntrinsic(al, loc, scope, arg_types, new_args,
        overload_id, compile_time_value, ASRUtils::IntrinsicFunctions::Product,
        &ASRUtils::get_constant_one_with_given_type, &ASRUtils::ASRBuilder::ElementalMul);
}

} // namespace Product

namespace MaxVal {

static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
    ArrIntrinsic::verify_args(x, diagnostics, ASRUtils::IntrinsicFunctions::MaxVal, &ArrIntrinsic::verify_array_int_real);
}

static inline ASR::expr_t *eval_MaxVal(Allocator & /*al*/,
    const Location & /*loc*/, Vec<ASR::expr_t*>& /*args*/) {
    return nullptr;
}

static inline ASR::asr_t* create_MaxVal(
    Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    return ArrIntrinsic::create_ArrIntrinsic(al, loc, args, err, ASRUtils::IntrinsicFunctions::MaxVal);
}

static inline ASR::expr_t* instantiate_MaxVal(Allocator &al, const Location &loc,
    SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
    Vec<ASR::call_arg_t>& new_args, int64_t overload_id,
    ASR::expr_t* compile_time_value) {
    return ArrIntrinsic::instantiate_ArrIntrinsic(al, loc, scope, arg_types, new_args,
        overload_id, compile_time_value, ASRUtils::IntrinsicFunctions::MaxVal,
        &ASRUtils::get_minimum_value_with_given_type, &ASRUtils::ASRBuilder::ElementalMax);
}

} // namespace MaxVal

namespace MinVal {

static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
    ArrIntrinsic::verify_args(x, diagnostics, ASRUtils::IntrinsicFunctions::MinVal, &ArrIntrinsic::verify_array_int_real);
}

static inline ASR::expr_t *eval_MinVal(Allocator & /*al*/,
    const Location & /*loc*/, Vec<ASR::expr_t*>& /*args*/) {
    return nullptr;
}

static inline ASR::asr_t* create_MinVal(
    Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    return ArrIntrinsic::create_ArrIntrinsic(al, loc, args, err, ASRUtils::IntrinsicFunctions::MinVal);
}

static inline ASR::expr_t* instantiate_MinVal(Allocator &al, const Location &loc,
    SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
    Vec<ASR::call_arg_t>& new_args, int64_t overload_id,
    ASR::expr_t* compile_time_value) {
    return ArrIntrinsic::instantiate_ArrIntrinsic(al, loc, scope, arg_types, new_args,
        overload_id, compile_time_value, ASRUtils::IntrinsicFunctions::MinVal,
        &ASRUtils::get_maximum_value_with_given_type, &ASRUtils::ASRBuilder::ElementalMin);
}

} // namespace MinVal

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

        return ASRUtils::make_IntrinsicFunction_t_util(al, loc,
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

namespace Merge {

    static inline ASR::expr_t* eval_Merge(
        Allocator &/*al*/, const Location &/*loc*/, Vec<ASR::expr_t*>& args) {
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

    static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
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

        return ASR::make_IntrinsicFunction_t(al, loc,
            static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Merge),
            args.p, args.size(), 0, result_type, nullptr);
    }

    static inline ASR::expr_t* instantiate_Merge(Allocator &al,
        const Location &loc, SymbolTable *scope,
        Vec<ASR::ttype_t*>& arg_types, Vec<ASR::call_arg_t>& new_args,
        int64_t /*overload_id*/, ASR::expr_t* compile_time_value) {
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
            return b.Call(s, new_args, expr_type(f->m_return_var), compile_time_value);
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
            if_body.push_back(al, Assignment(result, tsource_arg));
            Vec<ASR::stmt_t *> else_body; else_body.reserve(al, 1);
            else_body.push_back(al, Assignment(result, fsource_arg));
            body.push_back(al, STMT(ASR::make_If_t(al, loc, mask_arg,
                if_body.p, if_body.n, else_body.p, else_body.n)));
        }

        ASR::symbol_t *new_symbol = make_Function_t(fn_name, fn_symtab, dep, args,
            body, result, Source, Implementation, nullptr);
        scope->add_symbol(fn_name, new_symbol);
        return b.Call(new_symbol, new_args, tsource_type, compile_time_value);
    }
}

namespace SymbolicSymbol {

    static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
        const Location& loc = x.base.base.loc;
        ASRUtils::require_impl(x.n_args == 1,
            "SymbolicSymbol intrinsic must have exactly 1 input argument",
            loc, diagnostics);

        ASR::ttype_t* input_type = ASRUtils::expr_type(x.m_args[0]);
        ASRUtils::require_impl(ASR::is_a<ASR::Character_t>(*input_type),
            "SymbolicSymbol intrinsic expects a character input argument",
            loc, diagnostics);
    }

    static inline ASR::expr_t *eval_SymbolicSymbol(Allocator &/*al*/,
    const Location &/*loc*/, Vec<ASR::expr_t*>& /*args*/) {
        // TODO
        return nullptr;
    }

    static inline ASR::asr_t* create_SymbolicSymbol(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        if (args.size() != 1) {
            err("Intrinsic Symbol function accepts exactly 1 argument", loc);
        }

        ASR::ttype_t *type = ASRUtils::expr_type(args[0]);
        if (!ASRUtils::is_character(*type)) {
            err("Argument of the Symbol function must be a Character",
                args[0]->base.loc);
        }

        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args, eval_SymbolicSymbol,
            static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicSymbol), 0, to_type);
    }

} // namespace SymbolicSymbol

#define create_symbolic_binary_macro(X)                                                     \
namespace X{                                                                               \
                                                                                           \
    static inline void verify_args(const ASR::IntrinsicFunction_t& x,                      \
            diag::Diagnostics& diagnostics) {                                              \
        ASRUtils::require_impl(x.n_args == 2, "Intrinsic function `"#X"` accepts           \
            exactly 2 arguments", x.base.base.loc, diagnostics);                           \
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
            Vec<ASR::expr_t*> &/*args*/) {                                                 \
        /*TODO*/                                                                           \
        return nullptr;                                                                    \
    }                                                                                      \
                                                                                           \
    static inline ASR::asr_t* create_##X(Allocator& al, const Location& loc,               \
            Vec<ASR::expr_t*>& args,                                                       \
            const std::function<void (const std::string &, const Location &)> err) {       \
        if (args.size() != 2) {                                                            \
            err("Intrinsic function `"#X"` accepts exactly 2 arguments", loc);             \
        }                                                                                  \
                                                                                           \
        for (size_t i = 0; i < args.size(); i++) {                                         \
            ASR::ttype_t* argtype = ASRUtils::expr_type(args[i]);                          \
            if(!ASR::is_a<ASR::SymbolicExpression_t>(*argtype)) {                          \
                err("Arguments of `"#X"` function must be of type SymbolicExpression",     \
                args[i]->base.loc);                                                        \
            }                                                                              \
        }                                                                                  \
                                                                                           \
        Vec<ASR::expr_t*> arg_values;                                                      \
        arg_values.reserve(al, args.size());                                               \
        for( size_t i = 0; i < args.size(); i++ ) {                                        \
            arg_values.push_back(al, ASRUtils::expr_value(args[i]));                       \
        }                                                                                  \
        ASR::expr_t* compile_time_value = eval_##X(al, loc, arg_values);                   \
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));   \
        return ASR::make_IntrinsicFunction_t(al, loc,                                      \
                static_cast<int64_t>(ASRUtils::IntrinsicFunctions::X),                     \
                args.p, args.size(), 0, to_type, compile_time_value);                      \
    }                                                                                      \
} // namespace X

create_symbolic_binary_macro(SymbolicAdd)
create_symbolic_binary_macro(SymbolicSub)
create_symbolic_binary_macro(SymbolicMul)
create_symbolic_binary_macro(SymbolicDiv)
create_symbolic_binary_macro(SymbolicPow)
create_symbolic_binary_macro(SymbolicDiff)

namespace SymbolicPi {

    static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 0, "SymbolicPi does not take arguments",
            x.base.base.loc, diagnostics);
    }

    static inline ASR::expr_t *eval_SymbolicPi(Allocator &/*al*/,
    const Location &/*loc*/, Vec<ASR::expr_t*>& /*args*/) {
        // TODO
        return nullptr;
    }

    static inline ASR::asr_t* create_SymbolicPi(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> /*err*/) {
        ASR::expr_t* compile_time_value = eval_SymbolicPi(al, loc, args);
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));
        return ASR::make_IntrinsicFunction_t(al, loc,
                static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicPi),
                nullptr, 0, 0, to_type, compile_time_value);
    }

} // namespace SymbolicPi

namespace SymbolicInteger {

    static inline void verify_args(const ASR::IntrinsicFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 1,
            "SymbolicInteger intrinsic must have exactly 1 input argument",
            x.base.base.loc, diagnostics);

        ASR::ttype_t* input_type = ASRUtils::expr_type(x.m_args[0]);
        ASRUtils::require_impl(ASR::is_a<ASR::Integer_t>(*input_type),
            "SymbolicInteger intrinsic expects an integer input argument",
            x.base.base.loc, diagnostics);
    }

    static inline ASR::expr_t* eval_SymbolicInteger(Allocator &/*al*/,
    const Location &/*loc*/, Vec<ASR::expr_t*>& /*args*/) {
        // TODO
        return nullptr;
    }

    static inline ASR::asr_t* create_SymbolicInteger(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> /*err*/) {
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args, eval_SymbolicInteger,
            static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicInteger), 0, to_type);
    }
} // namespace SymbolicInteger

#define create_symbolic_unary_macro(X)                                                    \
namespace X {                                                                             \
                                                                                          \
    static inline void verify_args(const ASR::IntrinsicFunction_t& x,                     \
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
            Vec<ASR::expr_t*> &/*args*/) {                                                \
        /*TODO*/                                                                          \
        return nullptr;                                                                   \
    }                                                                                     \
                                                                                          \
    static inline ASR::asr_t* create_##X(Allocator& al, const Location& loc,              \
            Vec<ASR::expr_t*>& args,                                                      \
            const std::function<void (const std::string &, const Location &)> err) {      \
        if (args.size() != 1) {                                                           \
            err("Intrinsic " #X " function accepts exactly 1 argument", loc);             \
        }                                                                                 \
                                                                                          \
        ASR::ttype_t* argtype = ASRUtils::expr_type(args[0]);                             \
        if (!ASR::is_a<ASR::SymbolicExpression_t>(*argtype)) {                            \
            err("Argument of " #X " function must be of type SymbolicExpression",         \
                args[0]->base.loc);                                                       \
        }                                                                                 \
                                                                                          \
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));  \
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args, eval_##X,      \
            static_cast<int64_t>(ASRUtils::IntrinsicFunctions::X), 0, to_type);           \
    }                                                                                     \
                                                                                          \
} // namespace X

create_symbolic_unary_macro(SymbolicSin)
create_symbolic_unary_macro(SymbolicCos)
create_symbolic_unary_macro(SymbolicLog)
create_symbolic_unary_macro(SymbolicExp)
create_symbolic_unary_macro(SymbolicAbs)
create_symbolic_unary_macro(SymbolicExpand)


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
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Product),
            {&Product::instantiate_Product, &Product::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Partition),
            {&Partition::instantiate_Partition, &Partition::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::ListIndex),
            {nullptr, &ListIndex::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::ListReverse),
            {nullptr, &ListReverse::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::ListPop),
            {nullptr, &ListPop::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Max),
            {&Max::instantiate_Max, &Max::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::MaxVal),
            {&MaxVal::instantiate_MaxVal, &MaxVal::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Min),
            {&Min::instantiate_Min, &Min::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::MinVal),
            {&MinVal::instantiate_MinVal, &MinVal::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Merge),
            {&Merge::instantiate_Merge, &Merge::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicSymbol),
            {nullptr, &SymbolicSymbol::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicAdd),
            {nullptr, &SymbolicAdd::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicSub),
            {nullptr, &SymbolicSub::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicMul),
            {nullptr, &SymbolicMul::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicDiv),
            {nullptr, &SymbolicDiv::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicPow),
            {nullptr, &SymbolicPow::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicPi),
            {nullptr, &SymbolicPi::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicInteger),
            {nullptr, &SymbolicInteger::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicDiff),
            {nullptr, &SymbolicDiff::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicExpand),
            {nullptr, &SymbolicExpand::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicSin),
            {nullptr, &SymbolicSin::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicCos),
            {nullptr, &SymbolicCos::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicLog),
            {nullptr, &SymbolicLog::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicExp),
            {nullptr, &SymbolicExp::verify_args}},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicAbs),
            {nullptr, &SymbolicAbs::verify_args}},
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
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::ListPop),
            "list.pop"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Any),
            "any"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Sum),
            "sum"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Product),
            "product"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Max),
            "max"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::MaxVal),
            "maxval"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Min),
            "min"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::MinVal),
            "minval"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Merge),
            "merge"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicSymbol),
            "Symbol"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicAdd),
            "SymbolicAdd"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicSub),
            "SymbolicSub"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicMul),
            "SymbolicMul"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicDiv),
            "SymbolicDiv"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicPow),
            "SymbolicPow"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicPi),
            "pi"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicInteger),
            "SymbolicInteger"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicDiff),
            "SymbolicDiff"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicExpand),
            "SymbolicExpand"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicSin),
            "SymbolicSin"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicCos),
            "SymbolicCos"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicLog),
            "SymbolicLog"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicExp),
            "SymbolicExp"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::SymbolicAbs),
            "SymbolicAbs"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Any),
            "any"},
        {static_cast<int64_t>(ASRUtils::IntrinsicFunctions::Sum),
            "sum"}
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
                {"product", {&Product::create_Product, &Product::eval_Product}},
                {"list.index", {&ListIndex::create_ListIndex, &ListIndex::eval_list_index}},
                {"list.reverse", {&ListReverse::create_ListReverse, &ListReverse::eval_list_reverse}},
                {"list.pop", {&ListPop::create_ListPop, &ListPop::eval_list_pop}},
                {"max0", {&Max::create_Max, &Max::eval_Max}},
                {"maxval", {&MaxVal::create_MaxVal, &MaxVal::eval_MaxVal}},
                {"min0", {&Min::create_Min, &Min::eval_Min}},
                {"min", {&Min::create_Min, &Min::eval_Min}},
                {"minval", {&MinVal::create_MinVal, &MinVal::eval_MinVal}},
                {"merge", {&Merge::create_Merge, &Merge::eval_Merge}},
                {"Symbol", {&SymbolicSymbol::create_SymbolicSymbol, &SymbolicSymbol::eval_SymbolicSymbol}},
                {"SymbolicAdd", {&SymbolicAdd::create_SymbolicAdd, &SymbolicAdd::eval_SymbolicAdd}},
                {"SymbolicSub", {&SymbolicSub::create_SymbolicSub, &SymbolicSub::eval_SymbolicSub}},
                {"SymbolicMul", {&SymbolicMul::create_SymbolicMul, &SymbolicMul::eval_SymbolicMul}},
                {"SymbolicDiv", {&SymbolicDiv::create_SymbolicDiv, &SymbolicDiv::eval_SymbolicDiv}},
                {"SymbolicPow", {&SymbolicPow::create_SymbolicPow, &SymbolicPow::eval_SymbolicPow}},
                {"pi", {&SymbolicPi::create_SymbolicPi, &SymbolicPi::eval_SymbolicPi}},
                {"SymbolicInteger", {&SymbolicInteger::create_SymbolicInteger, &SymbolicInteger::eval_SymbolicInteger}},
                {"diff", {&SymbolicDiff::create_SymbolicDiff, &SymbolicDiff::eval_SymbolicDiff}},
                {"expand", {&SymbolicExpand::create_SymbolicExpand, &SymbolicExpand::eval_SymbolicExpand}},
                {"SymbolicSin", {&SymbolicSin::create_SymbolicSin, &SymbolicSin::eval_SymbolicSin}},
                {"SymbolicCos", {&SymbolicCos::create_SymbolicCos, &SymbolicCos::eval_SymbolicCos}},
                {"SymbolicLog", {&SymbolicLog::create_SymbolicLog, &SymbolicLog::eval_SymbolicLog}},
                {"SymbolicExp", {&SymbolicExp::create_SymbolicExp, &SymbolicExp::eval_SymbolicExp}},
                {"SymbolicAbs", {&SymbolicAbs::create_SymbolicAbs, &SymbolicAbs::eval_SymbolicAbs}},

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
                 id_ == ASRUtils::IntrinsicFunctions::Expm1 ||
                 id_ == ASRUtils::IntrinsicFunctions::Merge ||
                 id_ == ASRUtils::IntrinsicFunctions::SymbolicSymbol);
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
            id == ASRUtils::IntrinsicFunctions::Sum ||
            id == ASRUtils::IntrinsicFunctions::Product ||
            id == ASRUtils::IntrinsicFunctions::MaxVal ||
            id == ASRUtils::IntrinsicFunctions::MinVal) {
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

/************************* Intrinsic Impure Function **************************/
enum class IntrinsicImpureFunctions : int64_t {
    IsIostatEnd,
    IsIostatEor,
    // ...
};

namespace IsIostatEnd {

    static inline ASR::asr_t* create_IsIostatEnd(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> /*err*/) {
        // Compile time value cannot be computed
        return ASR::make_IntrinsicImpureFunction_t(al, loc,
                static_cast<int64_t>(ASRUtils::IntrinsicImpureFunctions::IsIostatEnd),
                args.p, args.n, 0, logical, nullptr);
    }

} // namespace IsIostatEnd

namespace IsIostatEor {

    static inline ASR::asr_t* create_IsIostatEor(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> /*err*/) {
        // Compile time value cannot be computed
        return ASR::make_IntrinsicImpureFunction_t(al, loc,
                static_cast<int64_t>(ASRUtils::IntrinsicImpureFunctions::IsIostatEor),
                args.p, args.n, 0, logical, nullptr);
    }

} // namespace IsIostatEor

namespace IntrinsicImpureFunctionRegistry {

    static const std::map<std::string, std::tuple<create_intrinsic_function,
            eval_intrinsic_function>>& function_by_name_db = {
        {"is_iostat_end", {&IsIostatEnd::create_IsIostatEnd, nullptr}},
        {"is_iostat_eor", {&IsIostatEor::create_IsIostatEor, nullptr}},
    };

    static inline bool is_intrinsic_function(const std::string& name) {
        return function_by_name_db.find(name) != function_by_name_db.end();
    }

    static inline create_intrinsic_function get_create_function(const std::string& name) {
        return  std::get<0>(function_by_name_db.at(name));
    }

} // namespace IntrinsicImpureFunctionRegistry


#define IMPURE_INTRINSIC_NAME_CASE(X)                                           \
    case (static_cast<int64_t>(ASRUtils::IntrinsicImpureFunctions::X)) : {      \
        return #X;                                                              \
    }

inline std::string get_impure_intrinsic_name(int x) {
    switch (x) {
        IMPURE_INTRINSIC_NAME_CASE(IsIostatEnd)
        IMPURE_INTRINSIC_NAME_CASE(IsIostatEor)
        INTRINSIC_NAME_CASE(Max)
        INTRINSIC_NAME_CASE(Min)
        default : {
            throw LCompilersException("pickle: intrinsic_id not implemented");
        }
    }
}

} // namespace ASRUtils

} // namespace LCompilers

#endif // LFORTRAN_PASS_INTRINSIC_FUNCTION_REGISTRY_H
