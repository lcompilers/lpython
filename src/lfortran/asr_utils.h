#ifndef LFORTRAN_ASR_UTILS_H
#define LFORTRAN_ASR_UTILS_H

#include <lfortran/asr.h>

namespace LFortran  {

static inline ASR::expr_t* EXPR(const ASR::asr_t *f)
{
    return ASR::down_cast<ASR::expr_t>(f);
}

static inline ASR::var_t* VAR(const ASR::asr_t *f)
{
    return ASR::down_cast<ASR::var_t>(f);
}

static inline ASR::stmt_t* STMT(const ASR::asr_t *f)
{
    return ASR::down_cast<ASR::stmt_t>(f);
}

static inline ASR::ttype_t* TYPE(const ASR::asr_t *f)
{
    return ASR::down_cast<ASR::ttype_t>(f);
}

static inline ASR::Variable_t* EXPR2VAR(const ASR::expr_t *f)
{
    return ASR::down_cast<ASR::Variable_t>(ASR::down_cast<ASR::Var_t>(f)->m_v);
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
        case ASR::exprType::Var: { return EXPR2VAR(f)->m_type; }
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
