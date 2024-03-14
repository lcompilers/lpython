#ifndef LFORTRAN_PASS_INTRINSIC_FUNCTION_REGISTRY_H
#define LFORTRAN_PASS_INTRINSIC_FUNCTION_REGISTRY_H

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/asr_utils.h>
#include <libasr/casting_utils.h>
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
3. Then register in the maps present in `IntrinsicScalarFunctionRegistry`.

You can use helper macros and define your own helper macros to reduce
the code size.
*/

enum class IntrinsicScalarFunctions : int64_t {
    ObjectType,
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
    Gamma,
    LogGamma,
    Trunc,
    Fix,
    Abs,
    Exp,
    Exp2,
    Expm1,
    FMA,
    FlipSign,
    Mod,
    Trailz,
    FloorDiv,
    ListIndex,
    Partition,
    ListReverse,
    ListPop,
    Reserve,
    DictKeys,
    DictValues,
    SetAdd,
    SetRemove,
    Max,
    Min,
    Radix,
    Sign,
    SignFromValue,
    Aint,
    Sqrt,
    Sngl,
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
    // ...
};

#define INTRINSIC_NAME_CASE(X)                                                  \
    case (static_cast<int64_t>(ASRUtils::IntrinsicScalarFunctions::X)) : {      \
        return #X;                                                              \
    }

inline std::string get_intrinsic_name(int x) {
    switch (x) {
        INTRINSIC_NAME_CASE(ObjectType)
        INTRINSIC_NAME_CASE(Sin)
        INTRINSIC_NAME_CASE(Cos)
        INTRINSIC_NAME_CASE(Tan)
        INTRINSIC_NAME_CASE(Asin)
        INTRINSIC_NAME_CASE(Acos)
        INTRINSIC_NAME_CASE(Atan)
        INTRINSIC_NAME_CASE(Sinh)
        INTRINSIC_NAME_CASE(Cosh)
        INTRINSIC_NAME_CASE(Tanh)
        INTRINSIC_NAME_CASE(Atan2)
        INTRINSIC_NAME_CASE(Gamma)
        INTRINSIC_NAME_CASE(LogGamma)
        INTRINSIC_NAME_CASE(Trunc)
        INTRINSIC_NAME_CASE(Fix)
        INTRINSIC_NAME_CASE(Abs)
        INTRINSIC_NAME_CASE(Exp)
        INTRINSIC_NAME_CASE(Exp2)
        INTRINSIC_NAME_CASE(Expm1)
        INTRINSIC_NAME_CASE(FMA)
        INTRINSIC_NAME_CASE(FlipSign)
        INTRINSIC_NAME_CASE(FloorDiv)
        INTRINSIC_NAME_CASE(Mod)
        INTRINSIC_NAME_CASE(Trailz)
        INTRINSIC_NAME_CASE(ListIndex)
        INTRINSIC_NAME_CASE(Partition)
        INTRINSIC_NAME_CASE(ListReverse)
        INTRINSIC_NAME_CASE(ListPop)
        INTRINSIC_NAME_CASE(Reserve)
        INTRINSIC_NAME_CASE(DictKeys)
        INTRINSIC_NAME_CASE(DictValues)
        INTRINSIC_NAME_CASE(SetAdd)
        INTRINSIC_NAME_CASE(SetRemove)
        INTRINSIC_NAME_CASE(Max)
        INTRINSIC_NAME_CASE(Min)
        INTRINSIC_NAME_CASE(Sign)
        INTRINSIC_NAME_CASE(SignFromValue)
        INTRINSIC_NAME_CASE(Aint)
        INTRINSIC_NAME_CASE(Sqrt)
        INTRINSIC_NAME_CASE(Sngl)
        INTRINSIC_NAME_CASE(SymbolicSymbol)
        INTRINSIC_NAME_CASE(SymbolicAdd)
        INTRINSIC_NAME_CASE(SymbolicSub)
        INTRINSIC_NAME_CASE(SymbolicMul)
        INTRINSIC_NAME_CASE(SymbolicDiv)
        INTRINSIC_NAME_CASE(SymbolicPow)
        INTRINSIC_NAME_CASE(SymbolicPi)
        INTRINSIC_NAME_CASE(SymbolicE)
        INTRINSIC_NAME_CASE(SymbolicInteger)
        INTRINSIC_NAME_CASE(SymbolicDiff)
        INTRINSIC_NAME_CASE(SymbolicExpand)
        INTRINSIC_NAME_CASE(SymbolicSin)
        INTRINSIC_NAME_CASE(SymbolicCos)
        INTRINSIC_NAME_CASE(SymbolicLog)
        INTRINSIC_NAME_CASE(SymbolicExp)
        INTRINSIC_NAME_CASE(SymbolicAbs)
        INTRINSIC_NAME_CASE(SymbolicHasSymbolQ)
        INTRINSIC_NAME_CASE(SymbolicAddQ)
        INTRINSIC_NAME_CASE(SymbolicMulQ)
        INTRINSIC_NAME_CASE(SymbolicPowQ)
        INTRINSIC_NAME_CASE(SymbolicLogQ)
        INTRINSIC_NAME_CASE(SymbolicSinQ)
        INTRINSIC_NAME_CASE(SymbolicGetArgument)
        default : {
            throw LCompilersException("pickle: intrinsic_id not implemented");
        }
    }
}

typedef ASR::expr_t* (*impl_function)(
    Allocator&, const Location &,
    SymbolTable*, Vec<ASR::ttype_t*>&, ASR::ttype_t *,
    Vec<ASR::call_arg_t>&, int64_t);

typedef ASR::expr_t* (*eval_intrinsic_function)(
    Allocator&, const Location &, ASR::ttype_t *,
    Vec<ASR::expr_t*>&);

typedef ASR::asr_t* (*create_intrinsic_function)(
    Allocator&, const Location&,
    Vec<ASR::expr_t*>&,
    const std::function<void (const std::string &, const Location &)>);

typedef void (*verify_function)(
    const ASR::IntrinsicScalarFunction_t&,
    diag::Diagnostics&);

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
        std::string fn_name = scope->get_unique_name(name, false);              \
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

    #define fill_func_arg(arg_name, type) {                                     \
        auto arg = declare(arg_name, type, In);                                 \
        args.push_back(al, arg); }

    #define make_ASR_Function_t(name, symtab, dep, args, body, return_var, abi,     \
            deftype, bindc_name)                                                \
        ASR::down_cast<ASR::symbol_t>( ASRUtils::make_Function_t_util(al, loc,  \
        symtab, s2c(al, name), dep.p, dep.n, args.p, args.n, body.p, body.n,    \
        return_var, abi, ASR::accessType::Public,                 \
        deftype, bindc_name, false, false, false, false,      \
        false, nullptr, 0, false, false, false));

    #define make_Function_Without_ReturnVar_t(name, symtab, dep, args, body,    \
            abi, deftype, bindc_name)                                           \
        ASR::down_cast<ASR::symbol_t>( ASRUtils::make_Function_t_util(al, loc,  \
        symtab, s2c(al, name), dep.p, dep.n, args.p, args.n, body.p, body.n,    \
        nullptr, abi, ASR::accessType::Public,                    \
        deftype, bindc_name, false, false, false, false,      \
        false, nullptr, 0, false, false, false));

    // Types -------------------------------------------------------------------
    #define int32        ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4))
    #define int64        TYPE(ASR::make_Integer_t(al, loc, 8))
    #define real32       TYPE(ASR::make_Real_t(al, loc, 4))
    #define real64       TYPE(ASR::make_Real_t(al, loc, 8))
    #define logical      ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4))
    #define character(x) TYPE(ASR::make_Character_t(al, loc, 1, x, nullptr))
    #define List(x)      TYPE(ASR::make_List_t(al, loc, x))

    ASR::ttype_t *Tuple(std::vector<ASR::ttype_t*> tuple_type) {
        Vec<ASR::ttype_t*> m_tuple_type; m_tuple_type.reserve(al, 3);
        for (auto &x: tuple_type) {
            m_tuple_type.push_back(al, x);
        }
        return TYPE(ASR::make_Tuple_t(al, loc, m_tuple_type.p, m_tuple_type.n));
    }
    ASR::ttype_t *Array(std::vector<int64_t> dims, ASR::ttype_t *type) {
        Vec<ASR::dimension_t> m_dims; m_dims.reserve(al, 1);
        for (auto &x: dims) {
            ASR::dimension_t dim;
            dim.loc = loc;
            if (x == -1) {
                dim.m_start = nullptr;
                dim.m_length = nullptr;
            } else {
                dim.m_start = EXPR(ASR::make_IntegerConstant_t(al, loc, 1, int32));
                dim.m_length = EXPR(ASR::make_IntegerConstant_t(al, loc, x, int32));
            }
            m_dims.push_back(al, dim);
        }
        return make_Array_t_util(al, loc, type, m_dims.p, m_dims.n);
    }

    // Expressions -------------------------------------------------------------
    #define i(x, t)   EXPR(ASR::make_IntegerConstant_t(al, loc, x, t))
    #define i32(x)   ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, x, int32))
    #define i32_n(x) EXPR(ASR::make_IntegerUnaryMinus_t(al, loc, i32(abs(x)),   \
        int32, i32(x)))
    #define i32_neg(x, t) EXPR(ASR::make_IntegerUnaryMinus_t(al, loc, x, t, nullptr))

    #define f(x, t)   EXPR(ASR::make_RealConstant_t(al, loc, x, t))
    #define f32(x) EXPR(ASR::make_RealConstant_t(al, loc, x, real32))
    #define f32_neg(x, t) EXPR(ASR::make_RealUnaryMinus_t(al, loc, x, t, nullptr))

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

    // Cast --------------------------------------------------------------------
    #define r2i32(x) EXPR(ASR::make_Cast_t(al, loc, x,                          \
        ASR::cast_kindType::RealToInteger, int32, nullptr))
    #define r2i64(x) EXPR(ASR::make_Cast_t(al, loc, x,                          \
        ASR::cast_kindType::RealToInteger, int64, nullptr))
    #define i2r32(x) EXPR(ASR::make_Cast_t(al, loc, x,                          \
        ASR::cast_kindType::IntegerToReal, real32, nullptr))
    #define i2r64(x) EXPR(ASR::make_Cast_t(al, loc, x,                          \
        ASR::cast_kindType::IntegerToReal, real64, nullptr))
    #define i2i(x, t) EXPR(ASR::make_Cast_t(al, loc, x,                         \
        ASR::cast_kindType::IntegerToInteger, t, nullptr))
    #define i2i64(x) EXPR(ASR::make_Cast_t(al, loc, x,                          \
        ASR::cast_kindType::IntegerToInteger, int64, nullptr))
    #define i2i32(x) EXPR(ASR::make_Cast_t(al, loc, x,                          \
        ASR::cast_kindType::IntegerToInteger, int32, nullptr))
    #define r2r32(x) EXPR(ASR::make_Cast_t(al, loc, x,                          \
        ASR::cast_kindType::RealToReal, real32, nullptr))
    #define r2r64(x) EXPR(ASR::make_Cast_t(al, loc, x,                          \
        ASR::cast_kindType::RealToReal, real64, nullptr))
    #define r2r(x, t) EXPR(ASR::make_Cast_t(al, loc, x,                         \
        ASR::cast_kindType::RealToReal, t, nullptr))
    #define i2r(x, t) EXPR(ASR::make_Cast_t(al, loc, x,                         \
        ASR::cast_kindType::IntegerToReal, t, nullptr))

    // Binop -------------------------------------------------------------------
    #define iAdd(left, right) EXPR(ASR::make_IntegerBinOp_t(al, loc, left,      \
            ASR::binopType::Add, right, int32, nullptr))

    #define iMul(left, right) EXPR(ASR::make_IntegerBinOp_t(al, loc, left,      \
            ASR::binopType::Mul, right, int32, nullptr))
    #define iSub(left, right) EXPR(ASR::make_IntegerBinOp_t(al, loc, left,      \
            ASR::binopType::Sub, right, int32, nullptr))
    #define iDiv(left, right) r2i32(EXPR(ASR::make_RealBinOp_t(al, loc, \
                i2r32(left), ASR::binopType::Div, i2r32(right), real32, nullptr)))
    #define i64Sub(left, right) EXPR(ASR::make_IntegerBinOp_t(al, loc, left,      \
        ASR::binopType::Sub, right, int64, nullptr))
    #define iAdd64(left, right) EXPR(ASR::make_IntegerBinOp_t(al, loc, left,      \
            ASR::binopType::Add, right, int64, nullptr))
    #define iDiv64(left, right) r2i64(EXPR(ASR::make_RealBinOp_t(al, loc, \
                i2r32(left), ASR::binopType::Div, i2r32(right), real32, nullptr)))
    #define r32Div(left, right) EXPR(ASR::make_RealBinOp_t(al, loc, \
                left, ASR::binopType::Div, right, real32, nullptr))
    #define r64Div(left, right) EXPR(ASR::make_RealBinOp_t(al, loc, \
                left, ASR::binopType::Div, right, real64, nullptr))

    #define r32Sub(left, right) EXPR(ASR::make_RealBinOp_t(al, loc, left,      \
            ASR::binopType::Sub, right, real32, nullptr))
    #define r64Sub(left, right) EXPR(ASR::make_RealBinOp_t(al, loc, left,      \
            ASR::binopType::Sub, right, real64, nullptr))
    #define r32Mul(left, right) EXPR(ASR::make_RealBinOp_t(al, loc, left,      \
            ASR::binopType::Mul, right, real32, nullptr))
    #define r64Mul(left, right) EXPR(ASR::make_RealBinOp_t(al, loc, left,      \
            ASR::binopType::Mul, right, real64, nullptr))
    #define rDiv(left, right) ASRUtils::make_ArrayBroadcast_t_util(al, loc, left, right); \
        EXPR(ASR::make_RealBinOp_t(al, loc, left, \
            ASR::binopType::Div, right, real32, nullptr)) \

    #define And(x, y) EXPR(ASR::make_LogicalBinOp_t(al, loc, x,                 \
            ASR::logicalbinopType::And, y, logical, nullptr))
    #define Not(x)    EXPR(ASR::make_LogicalNot_t(al, loc, x, logical, nullptr))

    ASR::expr_t *Add(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        ASRUtils::make_ArrayBroadcast_t_util(al, loc, left, right);
        switch (type->type) {
            case ASR::ttypeType::Integer : {
                return EXPR(ASR::make_IntegerBinOp_t(al, loc, left,
                    ASR::binopType::Add, right, type, nullptr));
                break;
            }
            case ASR::ttypeType::Real : {
                return EXPR(ASR::make_RealBinOp_t(al, loc, left,
                    ASR::binopType::Add, right, type, nullptr));
                break;
            }
            default: {
                LCOMPILERS_ASSERT(false);
                return nullptr;
            }
        }
    }

    ASR::expr_t *Mul(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        ASRUtils::make_ArrayBroadcast_t_util(al, loc, left, right);
        switch (type->type) {
            case ASR::ttypeType::Integer : {
                return EXPR(ASR::make_IntegerBinOp_t(al, loc, left,
                    ASR::binopType::Mul, right, type, nullptr));
                break;
            }
            case ASR::ttypeType::Real : {
                return EXPR(ASR::make_RealBinOp_t(al, loc, left,
                    ASR::binopType::Mul, right, type, nullptr));
                break;
            }
            default: {
                LCOMPILERS_ASSERT(false);
                return nullptr;
            }
        }
    }

    // Compare -----------------------------------------------------------------
    #define iEq(x, y) ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc, x,     \
        ASR::cmpopType::Eq, y, logical, nullptr))
    #define iNotEq(x, y) EXPR(ASR::make_IntegerCompare_t(al, loc, x,            \
        ASR::cmpopType::NotEq, y, logical, nullptr))
    #define iLt(x, y) EXPR(ASR::make_IntegerCompare_t(al, loc, x,               \
        ASR::cmpopType::Lt, y, logical, nullptr))
    #define iLtE(x, y) EXPR(ASR::make_IntegerCompare_t(al, loc, x,              \
        ASR::cmpopType::LtE, y, logical, nullptr))
    #define iGtE(x, y) EXPR(ASR::make_IntegerCompare_t(al, loc, x,              \
        ASR::cmpopType::GtE, y, logical, nullptr))
    #define iGt(x, y) EXPR(ASR::make_IntegerCompare_t(al, loc, x,               \
        ASR::cmpopType::Gt, y, logical, nullptr))

    #define ArraySize_1(x, dim) EXPR(make_ArraySize_t_util(al, loc, x, dim,     \
        int32, nullptr))
    #define ArraySize_2(x, dim, t) EXPR(make_ArraySize_t_util(al, loc, x, dim,  \
        t, nullptr))

    #define fGtE(x, y) EXPR(ASR::make_RealCompare_t(al, loc, x,                 \
        ASR::cmpopType::GtE, y, logical, nullptr))
    #define fLt(x, y) EXPR(ASR::make_RealCompare_t(al, loc, x,                  \
        ASR::cmpopType::Lt, y, logical, nullptr))
    #define fGt(x, y) EXPR(ASR::make_RealCompare_t(al, loc, x,                  \
        ASR::cmpopType::Gt, y, logical, nullptr))
    #define fNotEq(x, y) EXPR(ASR::make_RealCompare_t(al, loc, x,                 \
        ASR::cmpopType::NotEq, y, logical, nullptr))

    #define sEq(x, y) EXPR(ASR::make_StringCompare_t(al, loc, x,                \
        ASR::cmpopType::Eq, y, logical, nullptr))
    #define sNotEq(x, y) EXPR(ASR::make_StringCompare_t(al, loc, x,             \
        ASR::cmpopType::NotEq, y, logical, nullptr))

    ASR::expr_t *Gt(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        if (is_real(*expr_type(left))) {
            return fGt(left, right);
        } else if (is_integer(*expr_type(left))) {
            return iGt(left, right);
        } else {
            LCOMPILERS_ASSERT(false);
            return nullptr;
        }
    }

    ASR::expr_t *Lt(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        if (is_real(*expr_type(left))) {
            return fLt(left, right);
        } else if (is_integer(*expr_type(left))) {
            return iLt(left, right);
        } else {
            LCOMPILERS_ASSERT(false);
            return nullptr;
        }
    }

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
                                          std::to_string(expr_type(left)->type) +
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
                                          std::to_string(expr_type(left)->type) +
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
                                          std::to_string(expr_type(left)->type) +
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
                                          std::to_string(expr_type(left)->type) +
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
                    std::to_string(expr_type(left)->type) + " not yet supported");
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
                    std::to_string(expr_type(left)->type) + " not yet supported");
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

    ASR::expr_t *ArrayItem_01(ASR::expr_t *arr, std::vector<ASR::expr_t*> idx) {
        Vec<ASR::expr_t*> idx_vars; idx_vars.reserve(al, 1);
        for (auto &x: idx) idx_vars.push_back(al, x);
        return PassUtils::create_array_ref(arr, idx_vars, al);
    }

    #define ArrayItem_02(arr, idx_vars) PassUtils::create_array_ref(arr,        \
        idx_vars, al)

    ASR::expr_t *ArrayConstant(std::vector<ASR::expr_t *> elements,
            ASR::ttype_t *base_type, bool cast2descriptor=true) {
        // This function only creates array with rank one
        // TODO: Support other dimensions
        Vec<ASR::expr_t *> m_eles; m_eles.reserve(al, 1);
        for (auto &x: elements) m_eles.push_back(al, x);

        ASR::ttype_t *fixed_size_type = Array({(int64_t) elements.size()}, base_type);
        ASR::expr_t *arr_constant = EXPR(ASR::make_ArrayConstant_t(al, loc,
            m_eles.p, m_eles.n, fixed_size_type, ASR::arraystorageType::ColMajor));

        if (cast2descriptor) {
            return cast_to_descriptor(al, arr_constant);
        } else {
            return arr_constant;
        }
    }

    ASR::dimension_t set_dim(ASR::expr_t *start, ASR::expr_t *length) {
        ASR::dimension_t dim;
        dim.loc = loc;
        dim.m_start = start;
        dim.m_length = length;
        return dim;
    }

    // Statements --------------------------------------------------------------
    #define Return() STMT(ASR::make_Return_t(al, loc))

    ASR::stmt_t *Assignment(ASR::expr_t *lhs, ASR::expr_t *rhs) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(lhs), expr_type(rhs)));
        return STMT(ASR::make_Assignment_t(al, loc, lhs, rhs, nullptr));
    }

    template <typename T>
    ASR::stmt_t *Assign_Constant(ASR::expr_t *lhs, T init_value) {
        ASR::ttype_t *type = expr_type(lhs);
        switch(type->type) {
            case ASR::ttypeType::Integer : {
                return Assignment(lhs, i(init_value, type));
            }
            case ASR::ttypeType::Real : {
                return Assignment(lhs, f(init_value, type));
            }
            default : {
                LCOMPILERS_ASSERT(false);
                return nullptr;
            }
        }
    }

    ASR::stmt_t *Allocate(ASR::expr_t *m_a, Vec<ASR::dimension_t> dims) {
        Vec<ASR::alloc_arg_t> alloc_args; alloc_args.reserve(al, 1);
        ASR::alloc_arg_t alloc_arg;
        alloc_arg.loc = loc;
        alloc_arg.m_a = m_a;
        alloc_arg.m_dims = dims.p;
        alloc_arg.n_dims = dims.n;
        alloc_arg.m_type = nullptr;
        alloc_arg.m_len_expr = nullptr;
        alloc_args.push_back(al, alloc_arg);
        return STMT(ASR::make_Allocate_t(al, loc, alloc_args.p, 1,
            nullptr, nullptr, nullptr));
    }

    #define UBound(arr, dim) PassUtils::get_bound(arr, dim, "ubound", al)
    #define LBound(arr, dim) PassUtils::get_bound(arr, dim, "lbound", al)

    ASR::stmt_t *DoLoop(ASR::expr_t *m_v, ASR::expr_t *start, ASR::expr_t *end,
            std::vector<ASR::stmt_t*> loop_body, ASR::expr_t *step=nullptr) {
        ASR::do_loop_head_t head;
        head.loc = m_v->base.loc;
        head.m_v = m_v;
        head.m_start = start;
        head.m_end = end;
        head.m_increment = step;
        Vec<ASR::stmt_t *> body;
        body.from_pointer_n_copy(al, &loop_body[0], loop_body.size());
        return STMT(ASR::make_DoLoop_t(al, loc, nullptr, head, body.p, body.n));
    }

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

    ASR::stmt_t *Print(std::vector<ASR::expr_t *> items) {
        // Used for debugging
        Vec<ASR::expr_t *> x_exprs;
        x_exprs.from_pointer_n_copy(al, &items[0], items.size());
        return STMT(ASR::make_Print_t(al, loc, x_exprs.p, x_exprs.n,
            nullptr, nullptr));
    }

};

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
    auto result = declare(new_name, return_type, ReturnVar);

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
        ASR::symbol_t *s = make_ASR_Function_t(c_func_name, fn_symtab_1, dep_1, args_1,
            body_1, return_var_1, ASR::abiType::BindC, ASR::deftypeType::Interface, s2c(al, c_func_name));
        fn_symtab->add_symbol(c_func_name, s);
        dep.push_back(al, s2c(al, c_func_name));
        body.push_back(al, b.Assignment(result, b.Call(s, args, arg_type)));
    }

    ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
        body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
    scope->add_symbol(fn_name, new_symbol);
    return b.Call(new_symbol, new_args, return_type);
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
        value = eval_function(al, loc, type, arg_values);
    }

    return ASRUtils::make_IntrinsicScalarFunction_t_util(al, loc, intrinsic_id,
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

    body.push_back(al, b.Assignment(s_len, StringLen(args[0])));
    body.push_back(al, b.Assignment(pat_len, StringLen(args[1])));
    body.push_back(al, b.Assignment(result, i32_n(-1)));
    body.push_back(al, b.If(iEq(pat_len, i32(0)), {
            b.Assignment(result, i32(0)), Return()
        }, {
            b.If(iEq(s_len, i32(0)), { Return() }, {})
        }));
    body.push_back(al, b.Assignment(lps,
        EXPR(ASR::make_ListConstant_t(al, loc, nullptr, 0, List(int32)))));
    body.push_back(al, b.Assignment(i, i32(0)));
    body.push_back(al, b.While(iLtE(i, iSub(pat_len, i32(1))), {
        b.Assignment(i, iAdd(i, i32(1))),
        ListAppend(lps, i32(0))
    }));
    body.push_back(al, b.Assignment(flag, bool32(false)));
    body.push_back(al, b.Assignment(i, i32(1)));
    body.push_back(al, b.Assignment(pi_len, i32(0)));
    body.push_back(al, b.While(iLt(i, pat_len), {
        b.If(sEq(StringItem(args[1], iAdd(i, i32(1))),
                 StringItem(args[1], iAdd(pi_len, i32(1)))), {
            b.Assignment(pi_len, iAdd(pi_len, i32(1))),
            b.Assignment(ListItem(lps, i, int32), pi_len),
            b.Assignment(i, iAdd(i, i32(1)))
        }, {
            b.If(iNotEq(pi_len, i32(0)), {
                b.Assignment(pi_len, ListItem(lps, iSub(pi_len, i32(1)), int32))
            }, {
                b.Assignment(i, iAdd(i, i32(1)))
            })
        })
    }));
    body.push_back(al, b.Assignment(j, i32(0)));
    body.push_back(al, b.Assignment(i, i32(0)));
    body.push_back(al, b.While(And(iGtE(iSub(s_len, i),
            iSub(pat_len, j)), Not(flag)), {
        b.If(sEq(StringItem(args[1], iAdd(j, i32(1))),
                StringItem(args[0], iAdd(i, i32(1)))), {
            b.Assignment(i, iAdd(i, i32(1))),
            b.Assignment(j, iAdd(j, i32(1)))
        }, {}),
        b.If(iEq(j, pat_len), {
            b.Assignment(result, iSub(i, j)),
            b.Assignment(flag, bool32(true)),
            b.Assignment(j, ListItem(lps, iSub(j, i32(1)), int32))
        }, {
            b.If(And(iLt(i, s_len), sNotEq(StringItem(args[1], iAdd(j, i32(1))),
                    StringItem(args[0], iAdd(i, i32(1))))), {
                b.If(iNotEq(j, i32(0)), {
                    b.Assignment(j, ListItem(lps, iSub(j, i32(1)), int32))
                }, {
                    b.Assignment(i, iAdd(i, i32(1)))
                })
            }, {})
        })
    }));
    body.push_back(al, Return());
    ASR::symbol_t *fn_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
        body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
    scope->add_symbol(fn_name, fn_sym);
    return fn_sym;
}

static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x,
        diag::Diagnostics& diagnostics) {
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

namespace BinaryIntrinsicFunction {

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
    fill_func_arg("y", arg_type)
    auto result = declare(new_name, return_type, ReturnVar);

    {
        SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);
        Vec<ASR::expr_t*> args_1;
        {
            args_1.reserve(al, 2);
            ASR::expr_t *arg_1 = b.Variable(fn_symtab_1, "x", arg_type,
                ASR::intentType::In, ASR::abiType::BindC, true);
            ASR::expr_t *arg_2 = b.Variable(fn_symtab_1, "y", arg_type,
                ASR::intentType::In, ASR::abiType::BindC, true);
            args_1.push_back(al, arg_1);
            args_1.push_back(al, arg_2);
        }

        ASR::expr_t *return_var_1 = b.Variable(fn_symtab_1, c_func_name,
            arg_type, ASRUtils::intent_return_var, ASR::abiType::BindC, false);

        SetChar dep_1; dep_1.reserve(al, 1);
        Vec<ASR::stmt_t*> body_1; body_1.reserve(al, 1);
        ASR::symbol_t *s = make_ASR_Function_t(c_func_name, fn_symtab_1, dep_1, args_1,
            body_1, return_var_1, ASR::abiType::BindC, ASR::deftypeType::Interface, s2c(al, c_func_name));
        fn_symtab->add_symbol(c_func_name, s);
        dep.push_back(al, s2c(al, c_func_name));
        body.push_back(al, b.Assignment(result, b.Call(s, args, arg_type)));
    }

    ASR::symbol_t *new_symbol = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
        body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
    scope->add_symbol(fn_name, new_symbol);
    return b.Call(new_symbol, new_args, return_type);
}

static inline ASR::asr_t* create_BinaryFunction(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args, eval_intrinsic_function eval_function,
    int64_t intrinsic_id, int64_t overload_id, ASR::ttype_t* type) {
    ASR::expr_t *value = nullptr;
    ASR::expr_t *arg_value_1 = ASRUtils::expr_value(args[0]);
    ASR::expr_t *arg_value_2 = ASRUtils::expr_value(args[1]);
    if (arg_value_1 && arg_value_2) {
        Vec<ASR::expr_t*> arg_values;
        arg_values.reserve(al, 2);
        arg_values.push_back(al, arg_value_1);
        arg_values.push_back(al, arg_value_2);
        value = eval_function(al, loc, type, arg_values);
    }

    return ASRUtils::make_IntrinsicScalarFunction_t_util(al, loc, intrinsic_id,
        args.p, args.n, overload_id, type, value);
}

static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x,
        diag::Diagnostics& diagnostics) {
    const Location& loc = x.base.base.loc;
    ASRUtils::require_impl(x.n_args == 2,
        "Binary intrinsics must have only 2 input arguments",
        loc, diagnostics);

    ASR::ttype_t* input_type = ASRUtils::expr_type(x.m_args[0]);
    ASR::ttype_t* input_type_2 = ASRUtils::expr_type(x.m_args[1]);
    ASR::ttype_t* output_type = x.m_type;
    ASRUtils::require_impl(ASRUtils::check_equal_type(input_type, input_type_2, true),
        "The types of both the arguments of binary intrinsics must exactly match, argument 1 type: " +
        ASRUtils::get_type_code(input_type) + " argument 2 type: " + ASRUtils::get_type_code(input_type_2),
        loc, diagnostics);
    ASRUtils::require_impl(ASRUtils::check_equal_type(input_type, output_type, true),
        "The input and output type of elemental intrinsics must exactly match, input type: " +
        ASRUtils::get_type_code(input_type) + " output type: " + ASRUtils::get_type_code(output_type),
        loc, diagnostics);
}

} // namespace BinaryIntrinsicFunction


namespace ObjectType {

     static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 1,
            "ASR Verify: type() takes only 1 argument `object`",
            x.base.base.loc, diagnostics);
    }

    static ASR::expr_t *eval_ObjectType(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*>& /*args*/) {
        std::string object_type = "<class '";
        switch (t1->type) {
            case ASR::ttypeType::Integer : {
                object_type += "int"; break;
            } case ASR::ttypeType::Real : {
                object_type += "float"; break;
            } case ASR::ttypeType::Character : {
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
        return StringConstant(object_type, character(object_type.length()));
    }

    static inline ASR::asr_t* create_ObjectType(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        if (args.size() != 1) {
            err("type() takes exactly 1 argument `object` for now", loc);
        }
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t *> arg_values;

        m_value = eval_ObjectType(al, loc, expr_type(args[0]), arg_values);

        return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::ObjectType),
            args.p, args.n, 0, ASRUtils::expr_type(m_value), m_value);
    }


} // namespace ObjectType


namespace LogGamma {

static inline ASR::expr_t *eval_log_gamma(Allocator &al, const Location &loc,
        ASR::ttype_t *t, Vec<ASR::expr_t*>& args) {
    double rv = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
    double val = lgamma(rv);
    return make_ConstantWithType(make_RealConstant_t, val, t, loc);
}

static inline ASR::asr_t* create_LogGamma(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    ASR::ttype_t *type = ASRUtils::expr_type(args[0]);

    if (args.n != 1) {
            err("Intrinsic `log_gamma` accepts exactly one argument", loc);
    } else if (!ASRUtils::is_real(*type)) {
        err("`x` argument of `log_gamma` must be real",
            args[0]->base.loc);
    }

    return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args,
            eval_log_gamma, static_cast<int64_t>(IntrinsicScalarFunctions::LogGamma),
            0, type);
}

static inline ASR::expr_t* instantiate_LogGamma (Allocator &al,
        const Location &loc, SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types,
        ASR::ttype_t *return_type, Vec<ASR::call_arg_t>& new_args,
        int64_t overload_id) {
    ASR::ttype_t* arg_type = arg_types[0];
    return UnaryIntrinsicFunction::instantiate_functions(al, loc, scope,
        "log_gamma", arg_type, return_type, new_args, overload_id);
}

} // namespace LogGamma

#define create_trunc_macro(X, stdeval)                                              \
namespace X {                                                                       \
    static inline ASR::expr_t *eval_##X(Allocator &al, const Location &loc,         \
            ASR::ttype_t *t, Vec<ASR::expr_t*>& args) {                             \
        LCOMPILERS_ASSERT(args.size() == 1);                                        \
        double rv = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;              \
        if (ASRUtils::extract_value(args[0], rv)) {                                 \
            double val = std::stdeval(rv);                                          \
            return make_ConstantWithType(make_RealConstant_t, val, t, loc);         \
        }                                                                           \
        return nullptr;                                                             \
    }                                                                               \
    static inline ASR::asr_t* create_##X(Allocator& al, const Location& loc,        \
        Vec<ASR::expr_t*>& args,                                                    \
        const std::function<void (const std::string &, const Location &)> err) {    \
        ASR::ttype_t *type = ASRUtils::expr_type(args[0]);                          \
        if (args.n != 1) {                                                          \
            err("Intrinsic `#X` accepts exactly one argument", loc);                \
        } else if (!ASRUtils::is_real(*type)) {                                     \
            err("`x` argument of `#X` must be real",                                \
                args[0]->base.loc);                                                 \
        }                                                                           \
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args,          \
                eval_##X, static_cast<int64_t>(IntrinsicScalarFunctions::Trunc),    \
                0, type);                                                           \
    }                                                                               \
    static inline ASR::expr_t* instantiate_##X (Allocator &al,                      \
            const Location &loc, SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, \
            ASR::ttype_t *return_type, Vec<ASR::call_arg_t>& new_args,              \
            int64_t overload_id) {                                                  \
        ASR::ttype_t* arg_type = arg_types[0];                                      \
        return UnaryIntrinsicFunction::instantiate_functions(al, loc, scope,        \
            "#X", arg_type, return_type, new_args, overload_id);                    \
    }                                                                               \
} // namespace X

create_trunc_macro(Trunc, trunc)

namespace Fix {
    static inline ASR::expr_t *eval_Fix(Allocator &al, const Location &loc,
            ASR::ttype_t *t, Vec<ASR::expr_t*>& args) {
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

    static inline ASR::asr_t* create_Fix(Allocator& al, const Location& loc,
        Vec<ASR::expr_t*>& args,
        const std::function<void (const std::string &, const Location &)> err) {
        ASR::ttype_t *type = ASRUtils::expr_type(args[0]);
        if (args.n != 1) {
            err("Intrinsic `fix` accepts exactly one argument", loc);
        } else if (!ASRUtils::is_real(*type)) {
            err("`fix` argument of `fix` must be real",
                args[0]->base.loc);
        }
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args,
                eval_Fix, static_cast<int64_t>(IntrinsicScalarFunctions::Fix),
                0, type);
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

// `X` is the name of the function in the IntrinsicScalarFunctions enum and
// we use the same name for `create_X` and other places
// `stdeval` is the name of the function in the `std` namespace for compile
//  numerical time evaluation
// `lcompilers_name` is the name that we use in the C runtime library
#define create_trig(X, stdeval, lcompilers_name)                                \
namespace X {                                                                   \
    static inline ASR::expr_t *eval_##X(Allocator &al, const Location &loc,     \
            ASR::ttype_t *t, Vec<ASR::expr_t*>& args) {                         \
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
    static inline ASR::asr_t* create_##X(Allocator& al, const Location& loc,    \
        Vec<ASR::expr_t*>& args,                                                \
        const std::function<void (const std::string &, const Location &)> err)  \
    {                                                                           \
        ASR::ttype_t *type = ASRUtils::expr_type(args[0]);                      \
        if (args.n != 1) {                                                      \
            err("Intrinsic `"#X"` accepts exactly one argument", loc);          \
        } else if (!ASRUtils::is_real(*type) && !ASRUtils::is_complex(*type)) { \
            err("`x` argument of `"#X"` must be real or complex",               \
                args[0]->base.loc);                                             \
        }                                                                       \
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args,      \
                eval_##X, static_cast<int64_t>(IntrinsicScalarFunctions::X),    \
                0, type);                                                       \
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

namespace Atan2 {
    static inline ASR::expr_t *eval_Atan2(Allocator &al, const Location &loc,
            ASR::ttype_t *t, Vec<ASR::expr_t*>& args) {
        LCOMPILERS_ASSERT(args.size() == 2);
        double rv = -1, rv2 = -1;
        if( ASRUtils::extract_value(args[0], rv) && ASRUtils::extract_value(args[1], rv2) ) {
            double val = std::atan2(rv,rv2);
            return make_ConstantWithType(make_RealConstant_t, val, t, loc);
        }
        return nullptr;
    }
    static inline ASR::asr_t* create_Atan2(Allocator& al, const Location& loc,
        Vec<ASR::expr_t*>& args,
        const std::function<void (const std::string &, const Location &)> err)
    {
        ASR::ttype_t *type_1 = ASRUtils::expr_type(args[0]);
        ASR::ttype_t *type_2 = ASRUtils::expr_type(args[1]);
        if (!ASRUtils::is_real(*type_1)) {
            err("`x` argument of \"atan2\" must be real",args[0]->base.loc);
        } else if (!ASRUtils::is_real(*type_2)) {
            err("`y` argument of \"atan2\" must be real",args[1]->base.loc);
        }
        return BinaryIntrinsicFunction::create_BinaryFunction(al, loc, args,
                eval_Atan2, static_cast<int64_t>(IntrinsicScalarFunctions::Atan2),
                0, type_1);
    }
    static inline ASR::expr_t* instantiate_Atan2 (Allocator &al,
            const Location &loc, SymbolTable *scope,
            Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args,int64_t overload_id) {
        ASR::ttype_t* arg_type = arg_types[0];
        return BinaryIntrinsicFunction::instantiate_functions(al, loc, scope,
            "atan2", arg_type, return_type, new_args, overload_id);
    }
}

namespace Abs {

    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
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
            ASR::ttype_t *t, Vec<ASR::expr_t*> &args) {
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
            static_cast<int64_t>(IntrinsicScalarFunctions::Abs), 0, type);
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
            if_body.push_back(al, b.Assignment(result, args[0]));
            Vec<ASR::stmt_t *> else_body; else_body.reserve(al, 1);
            else_body.push_back(al, b.Assignment(result, negative_x));
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
                ASR::symbol_t *s = make_ASR_Function_t(c_func_name, fn_symtab_1, dep_1, args_1,
                    body_1, return_var_1, ASR::abiType::BindC, ASR::deftypeType::Interface, s2c(al, c_func_name));
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

            body.push_back(al, b.Assignment(result,
                b.ElementalPow(bin_op_1, constant_point_five, loc)));
        }

        ASR::symbol_t *f_sym = make_ASR_Function_t(func_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(func_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Abs

namespace Radix {

    // Helper function to verify arguments
    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x,
            diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.m_args[0], "Argument of the `radix` "
            "can be a nullptr", x.base.base.loc, diagnostics);
    }

    // Function to create an instance of the 'radix' intrinsic function
    static inline ASR::asr_t* create_Radix(Allocator& al, const Location& loc,
        Vec<ASR::expr_t*>& args,
        const std::function<void (const std::string &, const Location &)> err) {
        if ( args.n != 1 ) {
            err("Intrinsic `radix` accepts exactly one argument", loc);
        } else if ( !is_real(*expr_type(args[0]))
                 && !is_integer(*expr_type(args[0])) ) {
            err("Argument of the `radix` must be Integer or Real", loc);
        }

        return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::Radix),
            args.p, args.n, 0, int32, i32(2));
    }

}  // namespace Radix

namespace Sign {

     static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 2,
            "ASR Verify: Call to sign must have exactly two arguments",
            x.base.base.loc, diagnostics);
        ASR::ttype_t *type1 = ASRUtils::expr_type(x.m_args[0]);
        ASR::ttype_t *type2 = ASRUtils::expr_type(x.m_args[1]);
        ASRUtils::require_impl((is_real(*type1) || is_integer(*type2)),
            "ASR Verify: Arguments to sign must be of real or integer type",
            x.base.base.loc, diagnostics);
        ASRUtils::require_impl((ASRUtils::check_equal_type(type1, type2)),
            "ASR Verify: All arguments must be of the same type",
            x.base.base.loc, diagnostics);
    }

    static ASR::expr_t *eval_Sign(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args) {
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

    static inline ASR::asr_t* create_Sign(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        if (args.size() != 2) {
            err("Intrinsic sign function accepts exactly 2 arguments", loc);
        }
        ASR::ttype_t *type1 = ASRUtils::expr_type(args[0]);
        ASR::ttype_t *type2 = ASRUtils::expr_type(args[1]);
        if (!ASRUtils::is_integer(*type1) && !ASRUtils::is_real(*type1)) {
            err("Argument of the sign function must be Integer or Real",
                args[0]->base.loc);
        }
        if (!ASRUtils::check_equal_type(type1, type2)) {
            err("Type mismatch in statement function: "
                "the second argument must have the same type "
                "and kind as the first argument.", args[1]->base.loc);
        }
        ASR::expr_t *m_value = nullptr;
        if (all_args_evaluated(args)) {
            Vec<ASR::expr_t*> arg_values; arg_values.reserve(al, 2);
            arg_values.push_back(al, expr_value(args[0]));
            arg_values.push_back(al, expr_value(args[1]));
            m_value = eval_Sign(al, loc, expr_type(args[0]), arg_values);
        }
        return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::Sign),
            args.p, args.n, 0, ASRUtils::expr_type(args[0]), m_value);
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
            ASR::expr_t* real_copy_sign = ASRUtils::EXPR(ASR::make_RealCopySign_t(al, loc, args[0], args[1], arg_types[0], nullptr));
            return real_copy_sign;
        } else {
            /*
            * r = abs(x)
            * if (y < 0) then
            *     r = -r
            * end if
            */
            ASR::expr_t *zero = i(0, arg_types[0]);
            body.push_back(al, b.If(iGtE(args[0], zero), {
                b.Assignment(result, args[0])
            }, /* else */  {
                b.Assignment(result, i32_neg(args[0], arg_types[0]))
            }));
            body.push_back(al, b.If(iLt(args[1], zero), {
                b.Assignment(result, i32_neg(result, arg_types[0]))
            }, {}));

            ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
                body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
            scope->add_symbol(fn_name, f_sym);
            return b.Call(f_sym, new_args, return_type, nullptr);
        }
    }

} // namespace Sign

namespace Aint {

    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x,
            diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args > 0 && x.n_args < 3,
            "ASR Verify: Call to aint must have one or two arguments",
            x.base.base.loc, diagnostics);
        ASR::ttype_t *type = ASRUtils::expr_type(x.m_args[0]);
        ASRUtils::require_impl(ASRUtils::is_real(*type),
            "ASR Verify: Arguments to aint must be of real type",
            x.base.base.loc, diagnostics);
        if (x.n_args == 2) {
            ASR::ttype_t *type2 = ASRUtils::expr_type(x.m_args[1]);
            ASRUtils::require_impl(ASRUtils::is_integer(*type2),
                "ASR Verify: Second Argument to aint must be of integer type",
                x.base.base.loc, diagnostics);
        }
    }

    static ASR::expr_t *eval_Aint(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args) {
        double rv = ASR::down_cast<ASR::RealConstant_t>(expr_value(args[0]))->m_r;
        return f(std::trunc(rv), arg_type);
    }

    static inline ASR::asr_t* create_Aint(
            Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        ASR::ttype_t* return_type = expr_type(args[0]);
        if (!(args.size() == 1 || args.size() == 2)) {
            err("Intrinsic `aint` function accepts exactly 1 or 2 arguments", loc);
        } else if (!ASRUtils::is_real(*return_type)) {
            err("Argument of the `aint` function must be Real", args[0]->base.loc);
        }
        Vec<ASR::expr_t *> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if ( args[1] ) {
            int kind = -1;
            if (!ASR::is_a<ASR::Integer_t>(*expr_type(args[1])) ||
                    !extract_value(args[1], kind)) {
                err("`kind` argument of the `aint` function must be an "
                    "scalar Integer constant", args[1]->base.loc);
            }
            return_type = TYPE(ASR::make_Real_t(al, return_type->base.loc, kind));
        }
        ASR::expr_t *m_value = nullptr;
        if (all_args_evaluated(m_args)) {
            m_value = eval_Aint(al, loc, return_type, m_args);
        }
        return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::Aint),
            m_args.p, m_args.n, 0, return_type, m_value);
    }

    static inline ASR::expr_t* instantiate_Aint(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        std::string func_name = "_lcompilers_aint_" + type_to_str_python(arg_types[0]);
        std::string fn_name = scope->get_unique_name(func_name);
        SymbolTable *fn_symtab = al.make_new<SymbolTable>(scope);
        Vec<ASR::expr_t*> args;
        args.reserve(al, new_args.size());
        ASRBuilder b(al, loc);
        Vec<ASR::stmt_t*> body; body.reserve(al, 1);
        SetChar dep; dep.reserve(al, 1);
        if (scope->get_symbol(fn_name)) {
            ASR::symbol_t *s = scope->get_symbol(fn_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var), nullptr);
        }
        fill_func_arg("a", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);

        // Cast: Real -> Integer -> Real
        // TODO: this approach doesn't work for numbers > i64_max
        body.push_back(al, b.Assignment(result, i2r(r2i64(args[0]), return_type)));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

}  // namespace Aint

namespace Sqrt {

    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x,
            diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 1,
            "ASR Verify: Call `sqrt` must have exactly one argument",
            x.base.base.loc, diagnostics);
        ASR::ttype_t *type = ASRUtils::expr_type(x.m_args[0]);
        ASRUtils::require_impl(ASRUtils::is_real(*type) || ASRUtils::is_complex(*type),
            "ASR Verify: Arguments to `sqrt` must be of real or complex type",
            x.base.base.loc, diagnostics);
    }

    static ASR::expr_t *eval_Sqrt(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args) {
        if (is_real(*arg_type)) {
            double val = ASR::down_cast<ASR::RealConstant_t>(expr_value(args[0]))->m_r;
            return f(std::sqrt(val), arg_type);
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

    static inline ASR::asr_t* create_Sqrt(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        ASR::ttype_t* return_type = expr_type(args[0]);
        if ( args.n != 1 ) {
            err("Intrinsic `sqrt` accepts exactly one argument", loc);
        } else if ( !(is_real(*return_type) || is_complex(*return_type)) ) {
            err("Argument of the `sqrt` must be Real or Complex", loc);
        }
        ASR::expr_t *m_value = nullptr;
        if (all_args_evaluated(args)) {
            m_value = eval_Sqrt(al, loc, return_type, args);
        }
        return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::Sqrt),
            args.p, args.n, 0, return_type, m_value);
    }

    static inline ASR::expr_t* instantiate_Sqrt(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t overload_id) {
        ASR::ttype_t* arg_type = arg_types[0];
        if (is_real(*arg_type)) {
            return EXPR(ASR::make_IntrinsicFunctionSqrt_t(al, loc,
                new_args[0].m_value, return_type, nullptr));
        } else {
            return UnaryIntrinsicFunction::instantiate_functions(al, loc, scope,
                "sqrt", arg_type, return_type, new_args, overload_id);
        }
    }

}  // namespace Sqrt

namespace Sngl {

    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x,
            diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 1,
            "ASR Verify: Call `sngl` must have exactly one argument",
            x.base.base.loc, diagnostics);
        ASR::ttype_t *type = ASRUtils::expr_type(x.m_args[0]);
        ASRUtils::require_impl(ASRUtils::is_real(*type),
            "ASR Verify: Arguments to `sngl` must be of real type",
            x.base.base.loc, diagnostics);
    }

    static ASR::expr_t *eval_Sngl(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args) {
        double val = ASR::down_cast<ASR::RealConstant_t>(expr_value(args[0]))->m_r;
        return f(val, arg_type);
    }

    static inline ASR::asr_t* create_Sngl(
            Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        ASR::ttype_t* return_type = real32;
        if ( args.n != 1 ) {
            err("Intrinsic `sngl` accepts exactly one argument", loc);
        } else if ( !is_real(*expr_type(args[0])) ) {
            err("Argument of the `sngl` must be Real", loc);
        }
        Vec<ASR::expr_t *> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        ASR::expr_t *m_value = nullptr;
        if (all_args_evaluated(m_args)) {
            m_value = eval_Sngl(al, loc, return_type, m_args);
        }
        return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::Sngl),
            m_args.p, m_args.n, 0, return_type, m_value);
    }

    static inline ASR::expr_t* instantiate_Sngl(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        std::string func_name = "_lcompilers_sngl_" + type_to_str_python(arg_types[0]);
        std::string fn_name = scope->get_unique_name(func_name);
        SymbolTable *fn_symtab = al.make_new<SymbolTable>(scope);
        Vec<ASR::expr_t*> args;
        args.reserve(al, new_args.size());
        ASRBuilder b(al, loc);
        Vec<ASR::stmt_t*> body; body.reserve(al, 1);
        SetChar dep; dep.reserve(al, 1);
        if (scope->get_symbol(fn_name)) {
            ASR::symbol_t *s = scope->get_symbol(fn_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var), nullptr);
        }
        fill_func_arg("a", arg_types[0]);
        auto result = declare(fn_name, return_type, ReturnVar);
        body.push_back(al, b.Assignment(result, r2r32(args[0])));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

}  // namespace Sngl

namespace FMA {

     static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 3,
            "ASR Verify: Call to FMA must have exactly 3 arguments",
            x.base.base.loc, diagnostics);
        ASR::ttype_t *type1 = ASRUtils::expr_type(x.m_args[0]);
        ASR::ttype_t *type2 = ASRUtils::expr_type(x.m_args[1]);
        ASR::ttype_t *type3 = ASRUtils::expr_type(x.m_args[2]);
        ASRUtils::require_impl((is_real(*type1) && is_real(*type2) && is_real(*type3)),
            "ASR Verify: Arguments to FMA must be of real type",
            x.base.base.loc, diagnostics);
    }

    static ASR::expr_t *eval_FMA(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args) {
        double a = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
        double b = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
        double c = ASR::down_cast<ASR::RealConstant_t>(args[2])->m_r;
        return make_ConstantWithType(make_RealConstant_t, a + b*c, t1, loc);
    }

    static inline ASR::asr_t* create_FMA(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        if (args.size() != 3) {
            err("Intrinsic FMA function accepts exactly 3 arguments", loc);
        }
        ASR::ttype_t *type1 = ASRUtils::expr_type(args[0]);
        ASR::ttype_t *type2 = ASRUtils::expr_type(args[1]);
        ASR::ttype_t *type3 = ASRUtils::expr_type(args[2]);
        if (!ASRUtils::is_real(*type1) || !ASRUtils::is_real(*type2) || !ASRUtils::is_real(*type3)) {
            err("Argument of the FMA function must be Real",
                args[0]->base.loc);
        }
        ASR::expr_t *m_value = nullptr;
        if (all_args_evaluated(args)) {
            Vec<ASR::expr_t*> arg_values; arg_values.reserve(al, 3);
            arg_values.push_back(al, expr_value(args[0]));
            arg_values.push_back(al, expr_value(args[1]));
            arg_values.push_back(al, expr_value(args[2]));
            m_value = eval_FMA(al, loc, expr_type(args[0]), arg_values);
        }
        return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::FMA),
            args.p, args.n, 0, ASRUtils::expr_type(args[0]), m_value);
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

        ASR::expr_t *op1 = b.ElementalMul(args[1], args[2], loc);
        body.push_back(al, b.Assignment(result,
        b.ElementalAdd(args[0], op1, loc)));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace FMA


namespace SignFromValue {

     static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 2,
            "ASR Verify: Call to SignFromValue must have exactly 2 arguments",
            x.base.base.loc, diagnostics);
        ASR::ttype_t *type1 = ASRUtils::expr_type(x.m_args[0]);
        ASR::ttype_t *type2 = ASRUtils::expr_type(x.m_args[1]);
        bool eq_type = ASRUtils::types_equal(type1, type2);
        ASRUtils::require_impl(((is_real(*type1) || is_integer(*type1)) &&
                                (is_real(*type2) || is_integer(*type2)) && eq_type),
            "ASR Verify: Arguments to SignFromValue must be of equal type and "
            "should be either real or integer",
            x.base.base.loc, diagnostics);
    }

    static ASR::expr_t *eval_SignFromValue(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args) {
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

    static inline ASR::asr_t* create_SignFromValue(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        if (args.size() != 2) {
            err("Intrinsic SignFromValue function accepts exactly 2 arguments", loc);
        }
        ASR::ttype_t *type1 = ASRUtils::expr_type(args[0]);
        ASR::ttype_t *type2 = ASRUtils::expr_type(args[1]);
        bool eq_type = ASRUtils::types_equal(type1, type2);
        if (!((is_real(*type1) || is_integer(*type1)) &&
                                (is_real(*type2) || is_integer(*type2)) && eq_type)) {
            err("Argument of the SignFromValue function must be either Real or Integer "
                "and must be of equal type",
                args[0]->base.loc);
        }
        ASR::expr_t *m_value = nullptr;
        if (all_args_evaluated(args)) {
            Vec<ASR::expr_t*> arg_values; arg_values.reserve(al, 2);
            arg_values.push_back(al, expr_value(args[0]));
            arg_values.push_back(al, expr_value(args[1]));
            m_value = eval_SignFromValue(al, loc, expr_type(args[0]), arg_values);
        }
        return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::SignFromValue),
            args.p, args.n, 0, ASRUtils::expr_type(args[0]), m_value);
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
            ASR::expr_t *zero = f(0.0, arg_types[1]);
            body.push_back(al, b.If(fLt(args[1], zero), {
                b.Assignment(result, f32_neg(args[0], arg_types[0]))
            }, {
                b.Assignment(result, args[0])
            }));
        } else {
            ASR::expr_t *zero = i(0, arg_types[1]);
            body.push_back(al, b.If(iLt(args[1], zero), {
                b.Assignment(result, i32_neg(args[0], arg_types[0]))
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

     static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 2,
            "ASR Verify: Call to FlipSign must have exactly 2 arguments",
            x.base.base.loc, diagnostics);
        ASR::ttype_t *type1 = ASRUtils::expr_type(x.m_args[0]);
        ASR::ttype_t *type2 = ASRUtils::expr_type(x.m_args[1]);
        ASRUtils::require_impl((is_integer(*type1) && is_real(*type2)),
            "ASR Verify: Arguments to FlipSign must be of int and real type respectively",
            x.base.base.loc, diagnostics);
    }

    static ASR::expr_t *eval_FlipSign(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args) {
        int a = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        double b = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
        if (a % 2 == 1) b = -b;
        return make_ConstantWithType(make_RealConstant_t, b, t1, loc);
    }

    static inline ASR::asr_t* create_FlipSign(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        if (args.size() != 2) {
            err("Intrinsic FlipSign function accepts exactly 2 arguments", loc);
        }
        ASR::ttype_t *type1 = ASRUtils::expr_type(args[0]);
        ASR::ttype_t *type2 = ASRUtils::expr_type(args[1]);
        if (!ASRUtils::is_integer(*type1) || !ASRUtils::is_real(*type2)) {
            err("Argument of the FlipSign function must be int and real respectively",
                args[0]->base.loc);
        }
        ASR::expr_t *m_value = nullptr;
        if (all_args_evaluated(args)) {
            Vec<ASR::expr_t*> arg_values; arg_values.reserve(al, 2);
            arg_values.push_back(al, expr_value(args[0]));
            arg_values.push_back(al, expr_value(args[1]));
            m_value = eval_FlipSign(al, loc, expr_type(args[1]), arg_values);
        }
        return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::FlipSign),
            args.p, args.n, 0, ASRUtils::expr_type(args[1]), m_value);
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

        ASR::expr_t *two = i(2, arg_types[0]);
        ASR::expr_t *q = iDiv(args[0], two);
        ASR::expr_t *cond = iSub(args[0], iMul(two, q));
        body.push_back(al, b.If(iEq(cond, i(1, arg_types[0])), {
            b.Assignment(result, f32_neg(args[1], arg_types[1]))
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


     static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 2,
            "ASR Verify: Call to FloorDiv must have exactly 2 arguments",
            x.base.base.loc, diagnostics);
        ASR::ttype_t *type1 = ASRUtils::expr_type(x.m_args[0]);
        ASR::ttype_t *type2 = ASRUtils::expr_type(x.m_args[1]);
        type1 = ASRUtils::type_get_past_const(type1);
        type2 = ASRUtils::type_get_past_const(type2);
        ASRUtils::require_impl((is_integer(*type1) && is_integer(*type2)) ||
                                (is_unsigned_integer(*type1) && is_unsigned_integer(*type2)) ||
                                (is_real(*type1) && is_real(*type2)) ||
                                (is_logical(*type1) && is_logical(*type2)),
            "ASR Verify: Arguments to FloorDiv must be of real, integer, unsigned integer or logical type",
            x.base.base.loc, diagnostics);
    }


    static ASR::expr_t *eval_FloorDiv(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args) {
        ASR::ttype_t *type1 = ASRUtils::expr_type(args[0]);
        ASR::ttype_t *type2 = ASRUtils::expr_type(args[1]);
        type1 = ASRUtils::type_get_past_const(type1);
        type2 = ASRUtils::type_get_past_const(type2);
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
            return make_ConstantWithType(make_IntegerConstant_t, a / b, t1, loc);
        } else if (is_unsigned_int1 && is_unsigned_int2) {
            int64_t a = ASR::down_cast<ASR::UnsignedIntegerConstant_t>(args[0])->m_n;
            int64_t b = ASR::down_cast<ASR::UnsignedIntegerConstant_t>(args[1])->m_n;
            return make_ConstantWithType(make_UnsignedIntegerConstant_t, a / b, t1, loc);
        } else if (is_logical1 && is_logical2) {
            bool a = ASR::down_cast<ASR::LogicalConstant_t>(args[0])->m_value;
            bool b = ASR::down_cast<ASR::LogicalConstant_t>(args[1])->m_value;
            return make_ConstantWithType(make_LogicalConstant_t, a / b, t1, loc);
        } else if (is_real1 && is_real2) {
            double a = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            double b = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
            double r = a / b;
            int64_t result = (int64_t)r;
            if ( r >= 0.0 || (double)result == r) {
                return make_ConstantWithType(make_RealConstant_t, (double)result, t1, loc);
            }
            return make_ConstantWithType(make_RealConstant_t, (double)(result - 1), t1, loc);
        }
        return nullptr;
    }



    static inline ASR::asr_t* create_FloorDiv(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        if (args.size() != 2) {
            err("Intrinsic FloorDiv function accepts exactly 2 arguments", loc);
        }
        ASR::ttype_t *type1 = ASRUtils::expr_type(args[0]);
        ASR::ttype_t *type2 = ASRUtils::expr_type(args[1]);
        type1 = ASRUtils::type_get_past_const(type1);
        type2 = ASRUtils::type_get_past_const(type2);
        if (!((ASRUtils::is_integer(*type1) && ASRUtils::is_integer(*type2)) ||
            (ASRUtils::is_unsigned_integer(*type1) && ASRUtils::is_unsigned_integer(*type2)) ||
            (ASRUtils::is_real(*type1) && ASRUtils::is_real(*type2)) ||
            (ASRUtils::is_logical(*type1) && ASRUtils::is_logical(*type2)))) {
            err("Argument of the FloorDiv function must be either Real, Integer, Unsigned Integer or Logical",
                args[0]->base.loc);
        }
        ASR::expr_t *m_value = nullptr;
        double compile_time_arg2_val;
        if (ASRUtils::extract_value(expr_value(args[1]), compile_time_arg2_val)) {
            if (compile_time_arg2_val == 0.0) {
                err("Division by 0 is not allowed", args[1]->base.loc);
            }
        }
        if (all_args_evaluated(args)) {
            Vec<ASR::expr_t*> arg_values; arg_values.reserve(al, 2);
            arg_values.push_back(al, expr_value(args[0]));
            arg_values.push_back(al, expr_value(args[1]));
            m_value = eval_FloorDiv(al, loc, expr_type(args[1]), arg_values);
        }
        return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::FloorDiv),
            args.p, args.n, 0, ASRUtils::expr_type(args[0]), m_value);
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


        ASR::expr_t *op1 = r64Div(CastingUtil::perform_casting(args[0], arg_types[0], real64, al, loc),
            CastingUtil::perform_casting(args[1], arg_types[1], real64, al, loc));
        body.push_back(al, b.Assignment(r, op1));
        body.push_back(al, b.Assignment(tmp, r2i64(r)));
        body.push_back(al, b.If(And(fLt(r, f(0.0, real64)), fNotEq(i2r64(tmp), r)), {
                b.Assignment(tmp, i64Sub(tmp, i(1, int64)))
            }, {}));
        body.push_back(al, b.Assignment(result, CastingUtil::perform_casting(tmp, int64, return_type, al, loc)));
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace FloorDiv

namespace Mod {

     static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 2,
            "ASR Verify: Call to Mod must have exactly 2 arguments",
            x.base.base.loc, diagnostics);
        ASR::ttype_t *type1 = ASRUtils::expr_type(x.m_args[0]);
        ASR::ttype_t *type2 = ASRUtils::expr_type(x.m_args[1]);
        ASRUtils::require_impl((is_integer(*type1) && is_integer(*type2)) ||
                                (is_real(*type1) && is_real(*type2)),
            "ASR Verify: Arguments to Mod must be of real or integer type",
            x.base.base.loc, diagnostics);
    }

    static ASR::expr_t *eval_Mod(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args) {
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

    static inline ASR::asr_t* create_Mod(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        if (args.size() != 2) {
            err("Intrinsic Mod function accepts exactly 2 arguments", loc);
        }
        ASR::ttype_t *type1 = ASRUtils::expr_type(args[0]);
        ASR::ttype_t *type2 = ASRUtils::expr_type(args[1]);
        if (!((ASRUtils::is_integer(*type1) && ASRUtils::is_integer(*type2)) ||
            (ASRUtils::is_real(*type1) && ASRUtils::is_real(*type2)))) {
            err("Argument of the Mod function must be either Real or Integer",
                args[0]->base.loc);
        }
        ASR::expr_t *m_value = nullptr;
        if (all_args_evaluated(args)) {
            Vec<ASR::expr_t*> arg_values; arg_values.reserve(al, 2);
            arg_values.push_back(al, expr_value(args[0]));
            arg_values.push_back(al, expr_value(args[1]));
            m_value = eval_Mod(al, loc, expr_type(args[1]), arg_values);
        }
        return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::Mod),
            args.p, args.n, 0, ASRUtils::expr_type(args[0]), m_value);
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

        ASR::expr_t *q = nullptr, *op1 = nullptr, *op2 = nullptr;
        if (is_real(*arg_types[1])) {
            int kind = ASRUtils::extract_kind_from_ttype_t(arg_types[1]);
            if (kind == 4) {
                q = r2i32(r32Div(args[0], args[1]));
                op1 = r32Mul(args[1], i2r32(q));
                op2 = r32Sub(args[0], op1);
            } else {
                q = r2i64(r64Div(args[0], args[1]));
                op1 = r64Mul(args[1], i2r64(q));
                op2 = r64Sub(args[0], op1);
            }
        } else {
            q = iDiv(args[0], args[1]);
            op1 = iMul(args[1], q);
            op2 = iSub(args[0], op1);
        }
        body.push_back(al, b.Assignment(result, op2));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Mod

namespace Trailz {

     static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 1,
            "ASR Verify: Call to Trailz must have exactly 1 argument",
            x.base.base.loc, diagnostics);
        ASR::ttype_t *type1 = ASRUtils::expr_type(x.m_args[0]);
        ASRUtils::require_impl(is_integer(*type1),
            "ASR Verify: Arguments to Trailz must be of integer type",
            x.base.base.loc, diagnostics);
    }

    static ASR::expr_t *eval_Trailz(Allocator &al, const Location &loc,
            ASR::ttype_t* t1, Vec<ASR::expr_t*> &args) {
        int64_t a = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
        int64_t trailing_zeros = ASRUtils::compute_trailing_zeros(a);
        return make_ConstantWithType(make_IntegerConstant_t, trailing_zeros, t1, loc);
    }

    static inline ASR::asr_t* create_Trailz(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> err) {
        if (args.size() != 1) {
            err("Intrinsic Trailz function accepts exactly 1 arguments", loc);
        }
        ASR::ttype_t *type1 = ASRUtils::expr_type(args[0]);
        if (!(ASRUtils::is_integer(*type1))) {
            err("Argument of the Trailz function must be Integer",
                args[0]->base.loc);
        }
        ASR::expr_t *m_value = nullptr;
        if (all_args_evaluated(args)) {
            Vec<ASR::expr_t*> arg_values; arg_values.reserve(al, 1);
            arg_values.push_back(al, expr_value(args[0]));
            m_value = eval_Trailz(al, loc, expr_type(args[0]), arg_values);
        }
        return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::Trailz),
            args.p, args.n, 0, ASRUtils::expr_type(args[0]), m_value);
    }

    static inline ASR::expr_t* instantiate_Trailz(Allocator &al, const Location &loc,
            SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
            Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        declare_basic_variables("_lcompilers_optimization_trailz_" + type_to_str_python(arg_types[0]));
        fill_func_arg("n", arg_types[0]);
        auto result = declare(fn_name, arg_types[0], ReturnVar);
        // This is not the most efficient way to do this, but it works for now.
        /*
        function trailz(n) result(result)
            integer :: n
            integer :: result
            result = 0
            if (n == 0) then
                result = 32
            else
                do while (mod(n,2) == 0)
                    n = n/2
                    result = result + 1
                end do
            end if
        end function
        */

        body.push_back(al, b.Assignment(result, i(0, arg_types[0])));
        ASR::expr_t *two = i(2, arg_types[0]);
        int arg_0_kind = ASRUtils::extract_kind_from_ttype_t(arg_types[0]);

        Vec<ASR::ttype_t*> arg_types_mod; arg_types_mod.reserve(al, 2);
        arg_types_mod.push_back(al, arg_types[0]); arg_types_mod.push_back(al, ASRUtils::expr_type(two));

        Vec<ASR::call_arg_t> new_args_mod; new_args_mod.reserve(al, 2);
        ASR::call_arg_t arg1; arg1.loc = loc; arg1.m_value = args[0];
        ASR::call_arg_t arg2; arg2.loc = loc; arg2.m_value = two;
        new_args_mod.push_back(al, arg1); new_args_mod.push_back(al, arg2);

        ASR::expr_t* func_call_mod = Mod::instantiate_Mod(al, loc, scope, arg_types_mod, return_type, new_args_mod, 0);
        ASR::expr_t *cond = iEq(func_call_mod, i(0, arg_types[0]));

        std::vector<ASR::stmt_t*> while_loop_body;
        if (arg_0_kind == 4) {
            while_loop_body.push_back(b.Assignment(args[0], iDiv(args[0], two)));
            while_loop_body.push_back(b.Assignment(result, iAdd(result, i(1, arg_types[0]))));
        } else {
            while_loop_body.push_back(b.Assignment(args[0], iDiv64(args[0], two)));
            while_loop_body.push_back(b.Assignment(result, iAdd64(result, i(1, arg_types[0]))));
        }

        ASR::expr_t* check_zero = iEq(args[0], i(0, arg_types[0]));
        std::vector<ASR::stmt_t*> if_body; if_body.push_back(b.Assignment(result, i(32, arg_types[0])));
        std::vector<ASR::stmt_t*> else_body; else_body.push_back(b.While(cond, while_loop_body));
        body.push_back(al, b.If(check_zero, if_body, else_body));

        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Trailz

#define create_exp_macro(X, stdeval)                                                      \
namespace X {                                                                             \
    static inline ASR::expr_t* eval_##X(Allocator &al, const Location &loc,               \
            ASR::ttype_t *t, Vec<ASR::expr_t*> &args) {                                   \
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));                            \
        double rv = -1;                                                                    \
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
            static_cast<int64_t>(IntrinsicScalarFunctions::X), 0, type);                  \
    }                                                                                     \
} // namespace X

create_exp_macro(Exp, exp)
create_exp_macro(Exp2, exp2)
create_exp_macro(Expm1, expm1)

namespace ListIndex {

static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
    ASRUtils::require_impl(x.n_args <= 4, "Call to list.index must have at most four arguments",
        x.base.base.loc, diagnostics);
    ASR::ttype_t *list_type = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
    ASRUtils::require_impl(ASR::is_a<ASR::List_t>(*list_type) &&
        ASRUtils::check_equal_type(ASRUtils::expr_type(x.m_args[1]),
            ASRUtils::get_contained_type(list_type)),
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
    const Location &/*loc*/, ASR::ttype_t */*t*/, Vec<ASR::expr_t*>& /*args*/) {
    // TODO: To be implemented for ListConstant expression
    return nullptr;
}


static inline ASR::asr_t* create_ListIndex(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    int64_t overload_id = 0;
    ASR::expr_t* list_expr = args[0];
    ASR::ttype_t *type = ASRUtils::type_get_past_const(ASRUtils::expr_type(list_expr));
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
    ASR::ttype_t *to_type = int32;
    ASR::expr_t* compile_time_value = eval_list_index(al, loc, to_type, arg_values);
    return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::ListIndex),
            args.p, args.size(), overload_id, to_type, compile_time_value);
}

} // namespace ListIndex

namespace ListReverse {

static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
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
    const Location &/*loc*/, ASR::ttype_t */*t*/, Vec<ASR::expr_t*>& /*args*/) {
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
    ASR::expr_t* compile_time_value = eval_list_reverse(al, loc, nullptr, arg_values);
    return ASR::make_Expr_t(al, loc,
            ASRUtils::EXPR(ASRUtils::make_IntrinsicScalarFunction_t_util(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::ListReverse),
            args.p, args.size(), 0, nullptr, compile_time_value)));
}

} // namespace ListReverse

namespace ListPop {

static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
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
    const Location &/*loc*/, ASR::ttype_t */*t*/, Vec<ASR::expr_t*>& /*args*/) {
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
    ASR::ttype_t *to_type = list_type;
    ASR::expr_t* compile_time_value = eval_list_pop(al, loc, to_type, arg_values);
    int64_t overload_id = (args.size() == 2);
    return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::ListPop),
            args.p, args.size(), overload_id, to_type, compile_time_value);
}

} // namespace ListPop

namespace Reserve {

static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
    ASRUtils::require_impl(x.n_args == 2, "Call to reserve must have exactly one argument",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASR::is_a<ASR::List_t>(*ASRUtils::expr_type(x.m_args[0])),
        "First argument to reserve must be of list type",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(x.m_args[1])),
        "Second argument to reserve must be an integer",
        x.base.base.loc, diagnostics);
    ASRUtils::require_impl(x.m_type == nullptr,
        "Return type of reserve must be empty",
        x.base.base.loc, diagnostics);
}

static inline ASR::expr_t *eval_reserve(Allocator &/*al*/,
    const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/) {
    // TODO: To be implemented for ListConstant expression
    return nullptr;
}

static inline ASR::asr_t* create_Reserve(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    if (args.size() != 2) {
        err("Call to reserve must have exactly two argument", loc);
    }
    if (!ASR::is_a<ASR::List_t>(*ASRUtils::expr_type(args[0]))) {
        err("First argument to reserve must be of list type", loc);
    }
    if (!ASR::is_a<ASR::Integer_t>(*ASRUtils::expr_type(args[1]))) {
        err("Second argument to reserve must be an integer", loc);
    }

    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, args.size());
    for( size_t i = 0; i < args.size(); i++ ) {
        arg_values.push_back(al, ASRUtils::expr_value(args[i]));
    }
    ASR::expr_t* compile_time_value = eval_reserve(al, loc, nullptr, arg_values);
    return ASR::make_Expr_t(al, loc,
            ASRUtils::EXPR(ASRUtils::make_IntrinsicScalarFunction_t_util(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::Reserve),
            args.p, args.size(), 0, nullptr, compile_time_value)));
}

} // namespace Reserve

namespace DictKeys {

static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
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
    const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/) {
    // TODO: To be implemented for DictConstant expression
    return nullptr;
}

static inline ASR::asr_t* create_DictKeys(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    if (args.size() != 1) {
        err("Call to dict.keys must have no argument", loc);
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
    ASR::expr_t* compile_time_value = eval_dict_keys(al, loc, to_type, arg_values);
    return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::DictKeys),
            args.p, args.size(), 0, to_type, compile_time_value);
}

} // namespace DictKeys

namespace DictValues {

static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
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
    const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/) {
    // TODO: To be implemented for DictConstant expression
    return nullptr;
}

static inline ASR::asr_t* create_DictValues(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    if (args.size() != 1) {
        err("Call to dict.values must have no argument", loc);
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
    ASR::expr_t* compile_time_value = eval_dict_values(al, loc, to_type, arg_values);
    return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::DictValues),
            args.p, args.size(), 0, to_type, compile_time_value);
}

} // namespace DictValues

namespace SetAdd {

static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
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
    const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/) {
    // TODO: To be implemented for SetConstant expression
    return nullptr;
}

static inline ASR::asr_t* create_SetAdd(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    if (args.size() != 2) {
        err("Call to set.add must have exactly one argument", loc);
    }
    if (!ASRUtils::check_equal_type(ASRUtils::expr_type(args[1]),
        ASRUtils::get_contained_type(ASRUtils::expr_type(args[0])))) {
        err("Argument to set.add must be of same type as set's "
            "element type", loc);
    }

    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, args.size());
    for( size_t i = 0; i < args.size(); i++ ) {
        arg_values.push_back(al, ASRUtils::expr_value(args[i]));
    }
    ASR::expr_t* compile_time_value = eval_set_add(al, loc, nullptr, arg_values);
    return ASR::make_Expr_t(al, loc,
            ASRUtils::EXPR(ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::SetAdd),
            args.p, args.size(), 0, nullptr, compile_time_value)));
}

} // namespace SetAdd

namespace SetRemove {

static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
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
    const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/) {
    // TODO: To be implemented for SetConstant expression
    return nullptr;
}

static inline ASR::asr_t* create_SetRemove(Allocator& al, const Location& loc,
    Vec<ASR::expr_t*>& args,
    const std::function<void (const std::string &, const Location &)> err) {
    if (args.size() != 2) {
        err("Call to set.remove must have exactly one argument", loc);
    }
    if (!ASRUtils::check_equal_type(ASRUtils::expr_type(args[1]),
        ASRUtils::get_contained_type(ASRUtils::expr_type(args[0])))) {
        err("Argument to set.remove must be of same type as set's "
            "element type", loc);
    }

    Vec<ASR::expr_t*> arg_values;
    arg_values.reserve(al, args.size());
    for( size_t i = 0; i < args.size(); i++ ) {
        arg_values.push_back(al, ASRUtils::expr_value(args[i]));
    }
    ASR::expr_t* compile_time_value = eval_set_remove(al, loc, nullptr, arg_values);
    return ASR::make_Expr_t(al, loc,
            ASRUtils::EXPR(ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::SetRemove),
            args.p, args.size(), 0, nullptr, compile_time_value)));
}

} // namespace SetRemove

namespace Max {

    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
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

    static ASR::expr_t *eval_Max(Allocator &al, const Location &loc,
            ASR::ttype_t* arg_type, Vec<ASR::expr_t*> &args) {
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
            ASR::expr_t *value = eval_Max(al, loc, expr_type(args[0]), arg_values);
            return ASR::make_IntrinsicScalarFunction_t(al, loc,
                static_cast<int64_t>(IntrinsicScalarFunctions::Max),
                args.p, args.n, 0, ASRUtils::expr_type(args[0]), value);
        } else {
            return ASR::make_IntrinsicScalarFunction_t(al, loc,
                static_cast<int64_t>(IntrinsicScalarFunctions::Max),
                args.p, args.n, 0, ASRUtils::expr_type(args[0]), nullptr);
        }
    }

    static inline ASR::expr_t* instantiate_Max(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        std::string func_name = "_lcompilers_max0_" + type_to_str_python(arg_types[0]);
        std::string fn_name = scope->get_unique_name(func_name);
        SymbolTable *fn_symtab = al.make_new<SymbolTable>(scope);
        Vec<ASR::expr_t*> args;
        args.reserve(al, new_args.size());
        ASRBuilder b(al, loc);
        Vec<ASR::stmt_t*> body; body.reserve(al, args.size());
        SetChar dep; dep.reserve(al, 1);
        if (scope->get_symbol(fn_name)) {
            ASR::symbol_t *s = scope->get_symbol(fn_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var), nullptr);
        }
        for (size_t i = 0; i < new_args.size(); i++) {
            fill_func_arg("x" + std::to_string(i), arg_types[0]);
        }

        auto result = declare(fn_name, return_type, ReturnVar);

        ASR::expr_t* test;
        body.push_back(al, b.Assignment(result, args[0]));
        for (size_t i = 1; i < args.size(); i++) {
            test = make_Compare(make_IntegerCompare_t, args[i], Gt, result);
            Vec<ASR::stmt_t *> if_body; if_body.reserve(al, 1);
            if_body.push_back(al, b.Assignment(result, args[i]));
            body.push_back(al, STMT(ASR::make_If_t(al, loc, test,
                if_body.p, if_body.n, nullptr, 0)));
        }
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

}  // namespace Max

namespace Min {

    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
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

    static ASR::expr_t *eval_Min(Allocator &al, const Location &loc,
            ASR::ttype_t *arg_type, Vec<ASR::expr_t*> &args) {
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
            ASR::expr_t *value = eval_Min(al, loc, expr_type(args[0]), arg_values);
            return ASR::make_IntrinsicScalarFunction_t(al, loc,
                static_cast<int64_t>(IntrinsicScalarFunctions::Min),
                args.p, args.n, 0, ASRUtils::expr_type(args[0]), value);
        } else {
            return ASR::make_IntrinsicScalarFunction_t(al, loc,
                static_cast<int64_t>(IntrinsicScalarFunctions::Min),
                args.p, args.n, 0, ASRUtils::expr_type(args[0]), nullptr);
        }
    }

    static inline ASR::expr_t* instantiate_Min(Allocator &al, const Location &loc,
        SymbolTable *scope, Vec<ASR::ttype_t*>& arg_types, ASR::ttype_t *return_type,
        Vec<ASR::call_arg_t>& new_args, int64_t /*overload_id*/) {
        std::string func_name = "_lcompilers_min0_" + type_to_str_python(arg_types[0]);
        std::string fn_name = scope->get_unique_name(func_name);
        SymbolTable *fn_symtab = al.make_new<SymbolTable>(scope);
        Vec<ASR::expr_t*> args;
        args.reserve(al, new_args.size());
        ASRBuilder b(al, loc);
        Vec<ASR::stmt_t*> body; body.reserve(al, args.size());
        SetChar dep; dep.reserve(al, 1);
        if (scope->get_symbol(fn_name)) {
            ASR::symbol_t *s = scope->get_symbol(fn_name);
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
            return b.Call(s, new_args, expr_type(f->m_return_var), nullptr);
        }
        for (size_t i = 0; i < new_args.size(); i++) {
            fill_func_arg("x" + std::to_string(i), arg_types[0]);
        }

        auto result = declare(fn_name, return_type, ReturnVar);

        ASR::expr_t* test;
        body.push_back(al, b.Assignment(result, args[0]));
        if (return_type->type == ASR::ttypeType::Integer) {
            for (size_t i = 1; i < args.size(); i++) {
                test = make_Compare(make_IntegerCompare_t, args[i], Lt, result);
                Vec<ASR::stmt_t *> if_body; if_body.reserve(al, 1);
                if_body.push_back(al, b.Assignment(result, args[i]));
                body.push_back(al, STMT(ASR::make_If_t(al, loc, test,
                    if_body.p, if_body.n, nullptr, 0)));
            }
        } else if (return_type->type == ASR::ttypeType::Real) {
            for (size_t i = 1; i < args.size(); i++) {
                test = make_Compare(make_RealCompare_t, args[i], Lt, result);
                Vec<ASR::stmt_t *> if_body; if_body.reserve(al, 1);
                if_body.push_back(al, b.Assignment(result, args[i]));
                body.push_back(al, STMT(ASR::make_If_t(al, loc, test,
                    if_body.p, if_body.n, nullptr, 0)));
            }
        } else {
            throw LCompilersException("Arguments to min0 must be of real or integer type");
        }
        ASR::symbol_t *f_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, f_sym);
        return b.Call(f_sym, new_args, return_type, nullptr);
    }

} // namespace Min

namespace Partition {

    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
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

        return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::Partition),
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
        body.push_back(al, b.If(iEq(index, i32_n(-1)), {
                b.Assignment(result, b.TupleConstant({ args[0],
                    StringConstant("", character(0)),
                    StringConstant("", character(0)) },
                b.Tuple({character(-2), character(0), character(0)})))
            }, {
                b.Assignment(result, b.TupleConstant({
                    StringSection(args[0], i32(0), index), args[1],
                    StringSection(args[0], iAdd(index, StringLen(args[1])),
                        StringLen(args[0]))}, return_type))
            }));
        body.push_back(al, Return());
        ASR::symbol_t *fn_sym = make_ASR_Function_t(fn_name, fn_symtab, dep, args,
            body, result, ASR::abiType::Source, ASR::deftypeType::Implementation, nullptr);
        scope->add_symbol(fn_name, fn_sym);
        return b.Call(fn_sym, new_args, return_type, nullptr);
    }

} // namespace Partition

namespace SymbolicSymbol {

    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
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
    const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/) {
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
            static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicSymbol), 0, to_type);
    }

} // namespace SymbolicSymbol

#define create_symbolic_binary_macro(X)                                                    \
namespace X{                                                                               \
    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x,                \
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
            ASR::ttype_t *, Vec<ASR::expr_t*> &/*args*/) {                                 \
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
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));   \
        ASR::expr_t* compile_time_value = eval_##X(al, loc, to_type, arg_values);          \
        return ASR::make_IntrinsicScalarFunction_t(al, loc,                                \
                static_cast<int64_t>(IntrinsicScalarFunctions::X),                         \
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
    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x,               \
            diag::Diagnostics& diagnostics) {                                             \
        const Location& loc = x.base.base.loc;                                            \
        ASRUtils::require_impl(x.n_args == 0,                                             \
            #X " does not take arguments", loc, diagnostics);                             \
    }                                                                                     \
                                                                                          \
    static inline ASR::expr_t* eval_##X(Allocator &/*al*/, const Location &/*loc*/,       \
            ASR::ttype_t *, Vec<ASR::expr_t*> &/*args*/) {                                \
        /*TODO*/                                                                          \
        return nullptr;                                                                   \
    }                                                                                     \
                                                                                          \
    static inline ASR::asr_t* create_##X(Allocator& al, const Location& loc,              \
            Vec<ASR::expr_t*>& args,                                                      \
            const std::function<void (const std::string &, const Location &)> /*err*/) {  \
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));  \
        ASR::expr_t* compile_time_value = eval_##X(al, loc, to_type, args);               \
        return ASR::make_IntrinsicScalarFunction_t(al, loc,                               \
                static_cast<int64_t>(IntrinsicScalarFunctions::X),                        \
                nullptr, 0, 0, to_type, compile_time_value);                              \
    }                                                                                     \
} // namespace X

create_symbolic_constants_macro(SymbolicPi)
create_symbolic_constants_macro(SymbolicE)

namespace SymbolicInteger {

    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x, diag::Diagnostics& diagnostics) {
        ASRUtils::require_impl(x.n_args == 1,
            "SymbolicInteger intrinsic must have exactly 1 input argument",
            x.base.base.loc, diagnostics);

        ASR::ttype_t* input_type = ASRUtils::expr_type(x.m_args[0]);
        ASRUtils::require_impl(ASR::is_a<ASR::Integer_t>(*input_type),
            "SymbolicInteger intrinsic expects an integer input argument",
            x.base.base.loc, diagnostics);
    }

    static inline ASR::expr_t* eval_SymbolicInteger(Allocator &/*al*/,
    const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*>& /*args*/) {
        // TODO
        return nullptr;
    }

    static inline ASR::asr_t* create_SymbolicInteger(Allocator& al, const Location& loc,
            Vec<ASR::expr_t*>& args,
            const std::function<void (const std::string &, const Location &)> /*err*/) {
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args, eval_SymbolicInteger,
            static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicInteger), 0, to_type);
    }

} // namespace SymbolicInteger

namespace SymbolicHasSymbolQ {
    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x,
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
        const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*> &/*args*/) {
        /*TODO*/
        return nullptr;
    }

    static inline ASR::asr_t* create_SymbolicHasSymbolQ(Allocator& al,
        const Location& loc, Vec<ASR::expr_t*>& args,
        const std::function<void (const std::string &, const Location &)> err) {

        if (args.size() != 2) {
            err("Intrinsic function SymbolicHasSymbolQ accepts exactly 2 arguments", loc);
        }

        for (size_t i = 0; i < args.size(); i++) {
            ASR::ttype_t* argtype = ASRUtils::expr_type(args[i]);
            if(!ASR::is_a<ASR::SymbolicExpression_t>(*argtype)) {
                err("Arguments of SymbolicHasSymbolQ function must be of type SymbolicExpression",
                    args[i]->base.loc);
            }
        }

        Vec<ASR::expr_t*> arg_values;
        arg_values.reserve(al, args.size());
        for( size_t i = 0; i < args.size(); i++ ) {
            arg_values.push_back(al, ASRUtils::expr_value(args[i]));
        }

        ASR::expr_t* compile_time_value = eval_SymbolicHasSymbolQ(al, loc, logical, arg_values);
        return ASR::make_IntrinsicScalarFunction_t(al, loc,
            static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicHasSymbolQ),
            args.p, args.size(), 0, logical, compile_time_value);
    }
} // namespace SymbolicHasSymbolQ

namespace SymbolicGetArgument {
    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x,
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
        const Location &/*loc*/, ASR::ttype_t *, Vec<ASR::expr_t*> &/*args*/) {
        /*TODO*/
        return nullptr;
    }

    static inline ASR::asr_t* create_SymbolicGetArgument(Allocator& al,
        const Location& loc, Vec<ASR::expr_t*>& args,
        const std::function<void (const std::string &, const Location &)> err) {

        if (args.size() != 2) {
            err("Intrinsic function SymbolicGetArguments accepts exactly 2 argument", loc);
        }

        ASR::ttype_t* arg1_type = ASRUtils::expr_type(args[0]);
        ASR::ttype_t* arg2_type = ASRUtils::expr_type(args[1]);
        if (!ASR::is_a<ASR::SymbolicExpression_t>(*arg1_type)) {
            err("The first argument of SymbolicGetArgument function must be of type SymbolicExpression",
                    args[0]->base.loc);
        }
        if (!ASR::is_a<ASR::Integer_t>(*arg2_type)) {
            err("The second argument of SymbolicGetArgument function must be of type Integer",
                    args[1]->base.loc);
        }

        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args, eval_SymbolicGetArgument,
            static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicGetArgument),
            0, to_type);
    }
} // namespace SymbolicGetArgument

#define create_symbolic_query_macro(X)                                                    \
namespace X {                                                                             \
    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x,               \
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
            ASR::ttype_t *, Vec<ASR::expr_t*> &/*args*/) {                                \
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
        return UnaryIntrinsicFunction::create_UnaryFunction(al, loc, args, eval_##X,      \
            static_cast<int64_t>(IntrinsicScalarFunctions::X), 0, logical);               \
    }                                                                                     \
} // namespace X

create_symbolic_query_macro(SymbolicAddQ)
create_symbolic_query_macro(SymbolicMulQ)
create_symbolic_query_macro(SymbolicPowQ)
create_symbolic_query_macro(SymbolicLogQ)
create_symbolic_query_macro(SymbolicSinQ)

#define create_symbolic_unary_macro(X)                                                    \
namespace X {                                                                             \
    static inline void verify_args(const ASR::IntrinsicScalarFunction_t& x,               \
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
            ASR::ttype_t *, Vec<ASR::expr_t*> &/*args*/) {                                \
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
            static_cast<int64_t>(IntrinsicScalarFunctions::X), 0, to_type);               \
    }                                                                                     \
} // namespace X

create_symbolic_unary_macro(SymbolicSin)
create_symbolic_unary_macro(SymbolicCos)
create_symbolic_unary_macro(SymbolicLog)
create_symbolic_unary_macro(SymbolicExp)
create_symbolic_unary_macro(SymbolicAbs)
create_symbolic_unary_macro(SymbolicExpand)


namespace IntrinsicScalarFunctionRegistry {

    static const std::map<int64_t,
        std::tuple<impl_function,
                   verify_function>>& intrinsic_function_by_id_db = {
        {static_cast<int64_t>(IntrinsicScalarFunctions::ObjectType),
            {nullptr, &ObjectType::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::LogGamma),
            {&LogGamma::instantiate_LogGamma, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Trunc),
            {&Trunc::instantiate_Trunc, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Fix),
            {&Fix::instantiate_Fix, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Sin),
            {&Sin::instantiate_Sin, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Cos),
            {&Cos::instantiate_Cos, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Tan),
            {&Tan::instantiate_Tan, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Asin),
            {&Asin::instantiate_Asin, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Acos),
            {&Acos::instantiate_Acos, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Atan),
            {&Atan::instantiate_Atan, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Sinh),
            {&Sinh::instantiate_Sinh, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Cosh),
            {&Cosh::instantiate_Cosh, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Tanh),
            {&Tanh::instantiate_Tanh, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Atan2),
            {&Atan2::instantiate_Atan2, &BinaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Exp),
            {nullptr, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Exp2),
            {nullptr, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Expm1),
            {nullptr, &UnaryIntrinsicFunction::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::FMA),
            {&FMA::instantiate_FMA, &FMA::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::FlipSign),
            {&FlipSign::instantiate_FlipSign, &FlipSign::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::FloorDiv),
            {&FloorDiv::instantiate_FloorDiv, &FloorDiv::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Mod),
            {&Mod::instantiate_Mod, &Mod::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Trailz),
            {&Trailz::instantiate_Trailz, &Trailz::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Abs),
            {&Abs::instantiate_Abs, &Abs::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Partition),
            {&Partition::instantiate_Partition, &Partition::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::ListIndex),
            {nullptr, &ListIndex::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::ListReverse),
            {nullptr, &ListReverse::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::DictKeys),
            {nullptr, &DictKeys::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::DictValues),
            {nullptr, &DictValues::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::ListPop),
            {nullptr, &ListPop::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Reserve),
            {nullptr, &Reserve::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SetAdd),
            {nullptr, &SetAdd::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SetRemove),
            {nullptr, &SetRemove::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Max),
            {&Max::instantiate_Max, &Max::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Min),
            {&Min::instantiate_Min, &Min::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Sign),
            {&Sign::instantiate_Sign, &Sign::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Radix),
            {nullptr, &Radix::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Aint),
            {&Aint::instantiate_Aint, &Aint::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Sqrt),
            {&Sqrt::instantiate_Sqrt, &Sqrt::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Sngl),
            {&Sngl::instantiate_Sngl, &Sngl::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SignFromValue),
            {&SignFromValue::instantiate_SignFromValue, &SignFromValue::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicSymbol),
            {nullptr, &SymbolicSymbol::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicAdd),
            {nullptr, &SymbolicAdd::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicSub),
            {nullptr, &SymbolicSub::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicMul),
            {nullptr, &SymbolicMul::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicDiv),
            {nullptr, &SymbolicDiv::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicPow),
            {nullptr, &SymbolicPow::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicPi),
            {nullptr, &SymbolicPi::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicE),
            {nullptr, &SymbolicE::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicInteger),
            {nullptr, &SymbolicInteger::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicDiff),
            {nullptr, &SymbolicDiff::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicExpand),
            {nullptr, &SymbolicExpand::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicSin),
            {nullptr, &SymbolicSin::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicCos),
            {nullptr, &SymbolicCos::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicLog),
            {nullptr, &SymbolicLog::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicExp),
            {nullptr, &SymbolicExp::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicAbs),
            {nullptr, &SymbolicAbs::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicHasSymbolQ),
            {nullptr, &SymbolicHasSymbolQ::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicAddQ),
            {nullptr, &SymbolicAddQ::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicMulQ),
            {nullptr, &SymbolicMulQ::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicPowQ),
            {nullptr, &SymbolicPowQ::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicLogQ),
            {nullptr, &SymbolicLogQ::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicSinQ),
            {nullptr, &SymbolicSinQ::verify_args}},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicGetArgument),
            {nullptr, &SymbolicGetArgument::verify_args}},
    };

    static const std::map<int64_t, std::string>& intrinsic_function_id_to_name = {
        {static_cast<int64_t>(IntrinsicScalarFunctions::ObjectType),
            "type"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::LogGamma),
            "log_gamma"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Trunc),
            "trunc"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Fix),
            "fix"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Sin),
            "sin"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Cos),
            "cos"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Tan),
            "tan"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Asin),
            "asin"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Acos),
            "acos"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Atan),
            "atan"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Sinh),
            "sinh"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Cosh),
            "cosh"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Tanh),
            "tanh"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Atan2),
            "atan2"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Abs),
            "abs"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Exp),
            "exp"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Exp2),
            "exp2"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::FMA),
            "fma"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::FlipSign),
            "flipsign"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::FloorDiv),
            "floordiv"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Mod),
            "mod"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Trailz),
            "trailz"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Expm1),
            "expm1"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::ListIndex),
            "list.index"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::ListReverse),
            "list.reverse"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::ListPop),
            "list.pop"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Reserve),
            "reserve"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::DictKeys),
            "dict.keys"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::DictValues),
            "dict.values"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SetAdd),
            "set.add"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SetRemove),
            "set.remove"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Max),
            "max"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Min),
            "min"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Radix),
            "radix"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Sign),
            "sign"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Aint),
            "aint"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Sqrt),
            "sqrt"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::Sngl),
            "sngl"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SignFromValue),
            "signfromvalue"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicSymbol),
            "Symbol"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicAdd),
            "SymbolicAdd"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicSub),
            "SymbolicSub"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicMul),
            "SymbolicMul"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicDiv),
            "SymbolicDiv"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicPow),
            "SymbolicPow"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicPi),
            "pi"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicE),
            "E"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicInteger),
            "SymbolicInteger"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicDiff),
            "SymbolicDiff"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicExpand),
            "SymbolicExpand"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicSin),
            "SymbolicSin"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicCos),
            "SymbolicCos"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicLog),
            "SymbolicLog"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicExp),
            "SymbolicExp"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicAbs),
            "SymbolicAbs"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicHasSymbolQ),
            "SymbolicHasSymbolQ"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicAddQ),
            "SymbolicAddQ"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicMulQ),
            "SymbolicMulQ"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicPowQ),
            "SymbolicPowQ"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicLogQ),
            "SymbolicLogQ"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicSinQ),
            "SymbolicSinQ"},
        {static_cast<int64_t>(IntrinsicScalarFunctions::SymbolicGetArgument),
            "SymbolicGetArgument"},
    };


    static const std::map<std::string,
        std::tuple<create_intrinsic_function,
                    eval_intrinsic_function>>& intrinsic_function_by_name_db = {
                {"type", {&ObjectType::create_ObjectType, &ObjectType::eval_ObjectType}},
                {"log_gamma", {&LogGamma::create_LogGamma, &LogGamma::eval_log_gamma}},
                {"trunc", {&Trunc::create_Trunc, &Trunc::eval_Trunc}},
                {"fix", {&Fix::create_Fix, &Fix::eval_Fix}},
                {"sin", {&Sin::create_Sin, &Sin::eval_Sin}},
                {"cos", {&Cos::create_Cos, &Cos::eval_Cos}},
                {"tan", {&Tan::create_Tan, &Tan::eval_Tan}},
                {"asin", {&Asin::create_Asin, &Asin::eval_Asin}},
                {"acos", {&Acos::create_Acos, &Acos::eval_Acos}},
                {"atan", {&Atan::create_Atan, &Atan::eval_Atan}},
                {"sinh", {&Sinh::create_Sinh, &Sinh::eval_Sinh}},
                {"cosh", {&Cosh::create_Cosh, &Cosh::eval_Cosh}},
                {"tanh", {&Tanh::create_Tanh, &Tanh::eval_Tanh}},
                {"atan2", {&Atan2::create_Atan2, &Atan2::eval_Atan2}},
                {"abs", {&Abs::create_Abs, &Abs::eval_Abs}},
                {"exp", {&Exp::create_Exp, &Exp::eval_Exp}},
                {"exp2", {&Exp2::create_Exp2, &Exp2::eval_Exp2}},
                {"expm1", {&Expm1::create_Expm1, &Expm1::eval_Expm1}},
                {"fma", {&FMA::create_FMA, &FMA::eval_FMA}},
                {"floordiv", {&FloorDiv::create_FloorDiv, &FloorDiv::eval_FloorDiv}},
                {"mod", {&Mod::create_Mod, &Mod::eval_Mod}},
                {"trailz", {&Trailz::create_Trailz, &Trailz::eval_Trailz}},
                {"list.index", {&ListIndex::create_ListIndex, &ListIndex::eval_list_index}},
                {"list.reverse", {&ListReverse::create_ListReverse, &ListReverse::eval_list_reverse}},
                {"list.pop", {&ListPop::create_ListPop, &ListPop::eval_list_pop}},
                {"reserve", {&Reserve::create_Reserve, &Reserve::eval_reserve}},
                {"dict.keys", {&DictKeys::create_DictKeys, &DictKeys::eval_dict_keys}},
                {"dict.values", {&DictValues::create_DictValues, &DictValues::eval_dict_values}},
                {"set.add", {&SetAdd::create_SetAdd, &SetAdd::eval_set_add}},
                {"set.remove", {&SetRemove::create_SetRemove, &SetRemove::eval_set_remove}},
                {"max0", {&Max::create_Max, &Max::eval_Max}},
                {"min0", {&Min::create_Min, &Min::eval_Min}},
                {"min", {&Min::create_Min, &Min::eval_Min}},
                {"radix", {&Radix::create_Radix, nullptr}},
                {"sign", {&Sign::create_Sign, &Sign::eval_Sign}},
                {"aint", {&Aint::create_Aint, &Aint::eval_Aint}},
                {"sqrt", {&Sqrt::create_Sqrt, &Sqrt::eval_Sqrt}},
                {"sngl", {&Sngl::create_Sngl, &Sngl::eval_Sngl}},
                {"Symbol", {&SymbolicSymbol::create_SymbolicSymbol, &SymbolicSymbol::eval_SymbolicSymbol}},
                {"SymbolicAdd", {&SymbolicAdd::create_SymbolicAdd, &SymbolicAdd::eval_SymbolicAdd}},
                {"SymbolicSub", {&SymbolicSub::create_SymbolicSub, &SymbolicSub::eval_SymbolicSub}},
                {"SymbolicMul", {&SymbolicMul::create_SymbolicMul, &SymbolicMul::eval_SymbolicMul}},
                {"SymbolicDiv", {&SymbolicDiv::create_SymbolicDiv, &SymbolicDiv::eval_SymbolicDiv}},
                {"SymbolicPow", {&SymbolicPow::create_SymbolicPow, &SymbolicPow::eval_SymbolicPow}},
                {"pi", {&SymbolicPi::create_SymbolicPi, &SymbolicPi::eval_SymbolicPi}},
                {"E", {&SymbolicE::create_SymbolicE, &SymbolicE::eval_SymbolicE}},
                {"SymbolicInteger", {&SymbolicInteger::create_SymbolicInteger, &SymbolicInteger::eval_SymbolicInteger}},
                {"diff", {&SymbolicDiff::create_SymbolicDiff, &SymbolicDiff::eval_SymbolicDiff}},
                {"expand", {&SymbolicExpand::create_SymbolicExpand, &SymbolicExpand::eval_SymbolicExpand}},
                {"SymbolicSin", {&SymbolicSin::create_SymbolicSin, &SymbolicSin::eval_SymbolicSin}},
                {"SymbolicCos", {&SymbolicCos::create_SymbolicCos, &SymbolicCos::eval_SymbolicCos}},
                {"SymbolicLog", {&SymbolicLog::create_SymbolicLog, &SymbolicLog::eval_SymbolicLog}},
                {"SymbolicExp", {&SymbolicExp::create_SymbolicExp, &SymbolicExp::eval_SymbolicExp}},
                {"SymbolicAbs", {&SymbolicAbs::create_SymbolicAbs, &SymbolicAbs::eval_SymbolicAbs}},
                {"has", {&SymbolicHasSymbolQ::create_SymbolicHasSymbolQ, &SymbolicHasSymbolQ::eval_SymbolicHasSymbolQ}},
                {"AddQ", {&SymbolicAddQ::create_SymbolicAddQ, &SymbolicAddQ::eval_SymbolicAddQ}},
                {"MulQ", {&SymbolicMulQ::create_SymbolicMulQ, &SymbolicMulQ::eval_SymbolicMulQ}},
                {"PowQ", {&SymbolicPowQ::create_SymbolicPowQ, &SymbolicPowQ::eval_SymbolicPowQ}},
                {"LogQ", {&SymbolicLogQ::create_SymbolicLogQ, &SymbolicLogQ::eval_SymbolicLogQ}},
                {"SinQ", {&SymbolicSinQ::create_SymbolicSinQ, &SymbolicSinQ::eval_SymbolicSinQ}},
                {"GetArgument", {&SymbolicGetArgument::create_SymbolicGetArgument, &SymbolicGetArgument::eval_SymbolicGetArgument}},
    };

    static inline bool is_intrinsic_function(const std::string& name) {
        return intrinsic_function_by_name_db.find(name) != intrinsic_function_by_name_db.end();
    }

    static inline bool is_intrinsic_function(int64_t id) {
        return intrinsic_function_by_id_db.find(id) != intrinsic_function_by_id_db.end();
    }

    static inline bool is_elemental(int64_t id) {
        IntrinsicScalarFunctions id_ = static_cast<IntrinsicScalarFunctions>(id);
        return ( id_ == IntrinsicScalarFunctions::Abs ||
                 id_ == IntrinsicScalarFunctions::Cos ||
                 id_ == IntrinsicScalarFunctions::Gamma ||
                 id_ == IntrinsicScalarFunctions::LogGamma ||
                 id_ == IntrinsicScalarFunctions::Trunc ||
                 id_ == IntrinsicScalarFunctions::Fix ||
                 id_ == IntrinsicScalarFunctions::Sin ||
                 id_ == IntrinsicScalarFunctions::Exp ||
                 id_ == IntrinsicScalarFunctions::Exp2 ||
                 id_ == IntrinsicScalarFunctions::Expm1 ||
                 id_ == IntrinsicScalarFunctions::Min ||
                 id_ == IntrinsicScalarFunctions::Max ||
                 id_ == IntrinsicScalarFunctions::Sqrt ||
                 id_ == IntrinsicScalarFunctions::SymbolicSymbol);
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

} // namespace IntrinsicScalarFunctionRegistry

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
        default : {
            throw LCompilersException("pickle: intrinsic_id not implemented");
        }
    }
}

} // namespace ASRUtils

} // namespace LCompilers

#endif // LFORTRAN_PASS_INTRINSIC_FUNCTION_REGISTRY_H
