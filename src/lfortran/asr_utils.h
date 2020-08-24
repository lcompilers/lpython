#ifndef LFORTRAN_ASR_UTILS_H
#define LFORTRAN_ASR_UTILS_H

#include <lfortran/asr.h>

namespace LFortran  {

static inline ASR::expr_t* EXPR(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::expr);
    return (ASR::expr_t*)f;
}

static inline ASR::var_t* VAR(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::var);
    return (ASR::var_t*)f;
}

static inline ASR::Variable_t* VARIABLE(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::var);
    ASR::var_t *t = (ASR::var_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::varType::Variable);
    return (ASR::Variable_t*)t;
}

static inline ASR::Subroutine_t* SUBROUTINE(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::sub);
    ASR::sub_t *t = (ASR::sub_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::subType::Subroutine);
    return (ASR::Subroutine_t*)t;
}

static inline ASR::Function_t* FUNCTION(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::fn);
    ASR::fn_t *t = (ASR::fn_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::fnType::Function);
    return (ASR::Function_t*)t;
}

static inline ASR::Program_t* PROGRAM(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::prog);
    ASR::prog_t *t = (ASR::prog_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::progType::Program);
    return (ASR::Program_t*)t;
}

static inline ASR::TranslationUnit_t* TRANSLATION_UNIT(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::unit);
    ASR::unit_t *t = (ASR::unit_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::unitType::TranslationUnit);
    return (ASR::TranslationUnit_t*)t;
}

static inline ASR::stmt_t* STMT(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::stmt);
    return (ASR::stmt_t*)f;
}

static inline ASR::ttype_t* TYPE(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::ttype);
    return (ASR::ttype_t*)f;
}

static inline ASR::Real_t* TYPE_REAL(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::ttype);
    ASR::ttype_t *t = (ASR::ttype_t*)f;
    LFORTRAN_ASSERT(t->type == ASR::ttypeType::Real);
    return (ASR::Real_t*)t;
}

static inline ASR::Integer_t* TYPE_INTEGER(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::ttype);
    ASR::ttype_t *t = (ASR::ttype_t*)f;
    LFORTRAN_ASSERT(t->type == ASR::ttypeType::Integer);
    return (ASR::Integer_t*)t;
}

static inline ASR::Logical_t* TYPE_LOGICAL(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::ttype);
    ASR::ttype_t *t = (ASR::ttype_t*)f;
    LFORTRAN_ASSERT(t->type == ASR::ttypeType::Logical);
    return (ASR::Logical_t*)t;
}

static inline ASR::Var_t* EXPR_VAR(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::expr);
    ASR::expr_t *t = (ASR::expr_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::exprType::Var);
    return (ASR::Var_t*)t;
}

static inline ASR::Num_t* EXPR_NUM(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::expr);
    ASR::expr_t *t = (ASR::expr_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::exprType::Num);
    return (ASR::Num_t*)t;
}

static inline ASR::UnaryOp_t* EXPR_UNARYOP(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::expr);
    ASR::expr_t *t = (ASR::expr_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::exprType::UnaryOp);
    return (ASR::UnaryOp_t*)t;
}


static inline ASR::ttype_t* expr_type(const ASR::expr_t *f)
{
    switch (f->type) {
        case ASR::exprType::BoolOp: { return ((ASR::BoolOp_t*)f)->m_type; }
        case ASR::exprType::BinOp: { return ((ASR::BinOp_t*)f)->m_type; }
        case ASR::exprType::UnaryOp: { return ((ASR::UnaryOp_t*)f)->m_type; }
        case ASR::exprType::Compare: { return ((ASR::Compare_t*)f)->m_type; }
        case ASR::exprType::FuncCall: { return ((ASR::FuncCall_t*)f)->m_type; }
        case ASR::exprType::ArrayRef: { return ((ASR::ArrayRef_t*)f)->m_type; }
        case ASR::exprType::ArrayInitializer: { return ((ASR::ArrayInitializer_t*)f)->m_type; }
        case ASR::exprType::Num: { return ((ASR::Num_t*)f)->m_type; }
        case ASR::exprType::ConstantReal: { return ((ASR::ConstantReal_t*)f)->m_type; }
        case ASR::exprType::Str: { return ((ASR::Str_t*)f)->m_type; }
        case ASR::exprType::ImplicitCast: { return ((ASR::ImplicitCast_t*)f)->m_type; }
        case ASR::exprType::ExplicitCast: { return ((ASR::ExplicitCast_t*)f)->m_type; }
        case ASR::exprType::VariableOld: { return ((ASR::VariableOld_t*)f)->m_type; }
        case ASR::exprType::Var: { return VARIABLE((ASR::asr_t*)((ASR::Var_t*)f)->m_v)->m_type; }
        case ASR::exprType::Constant: { return ((ASR::Constant_t*)f)->m_type; }
        default : throw LFortranException("Not implemented");
    }
}

const ASR::intentType intent_local=ASR::intentType::Local; // local variable (not a dummy argument)
const ASR::intentType intent_in   =ASR::intentType::In; // dummy argument, intent(in)
const ASR::intentType intent_out  =ASR::intentType::Out; // dummy argument, intent(out)
const ASR::intentType intent_inout=ASR::intentType::InOut; // dummy argument, intent(inout)
const ASR::intentType intent_return_var=ASR::intentType::ReturnVar; // return variable of a function
const ASR::intentType intent_external=ASR::intentType::External; // external variable

static inline bool is_arg_dummy(int intent) {
    return intent == intent_in || intent == intent_out
        || intent == intent_inout;
}


} // namespace LFortran

#endif // LFORTRAN_ASR_UTILS_H
