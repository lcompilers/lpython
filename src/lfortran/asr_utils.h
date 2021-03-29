#ifndef LFORTRAN_ASR_UTILS_H
#define LFORTRAN_ASR_UTILS_H

#include <lfortran/asr.h>

namespace LFortran  {

static inline ASR::expr_t* EXPR(const ASR::asr_t *f)
{
    return ASR::down_cast<ASR::expr_t>(f);
}

static inline ASR::stmt_t* STMT(const ASR::asr_t *f)
{
    return ASR::down_cast<ASR::stmt_t>(f);
}

static inline ASR::case_stmt_t* CASE_STMT(const ASR::asr_t *f)
{
    return ASR::down_cast<ASR::case_stmt_t>(f);
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
        case ASR::exprType::FunctionCall: { return ((ASR::FunctionCall_t*)f)->m_type; }
        case ASR::exprType::ArrayRef: { return ((ASR::ArrayRef_t*)f)->m_type; }
        case ASR::exprType::ArrayInitializer: { return ((ASR::ArrayInitializer_t*)f)->m_type; }
        case ASR::exprType::ConstantInteger: { return ((ASR::ConstantInteger_t*)f)->m_type; }
        case ASR::exprType::ConstantReal: { return ((ASR::ConstantReal_t*)f)->m_type; }
        case ASR::exprType::ConstantComplex: { return ((ASR::ConstantComplex_t*)f)->m_type; }
        case ASR::exprType::Str: { return ((ASR::Str_t*)f)->m_type; }
        case ASR::exprType::ImplicitCast: { return ((ASR::ImplicitCast_t*)f)->m_type; }
        case ASR::exprType::ExplicitCast: { return ((ASR::ExplicitCast_t*)f)->m_type; }
        case ASR::exprType::Var: { return EXPR2VAR(f)->m_type; }
        case ASR::exprType::ConstantLogical: { return ((ASR::ConstantLogical_t*)f)->m_type; }
        case ASR::exprType::StrOp: { return ((ASR::StrOp_t*)f)->m_type; }
        default : throw LFortranException("Not implemented");
    }
}

static inline char *symbol_name(const ASR::symbol_t *f)
{
    switch (f->type) {
        case ASR::symbolType::Program: {
            return ASR::down_cast<ASR::Program_t>(f)->m_name;
        }
        case ASR::symbolType::Module: {
            return ASR::down_cast<ASR::Module_t>(f)->m_name;
        }
        case ASR::symbolType::Subroutine: {
            return ASR::down_cast<ASR::Subroutine_t>(f)->m_name;
        }
        case ASR::symbolType::Function: {
            return ASR::down_cast<ASR::Function_t>(f)->m_name;
        }
        case ASR::symbolType::GenericProcedure: {
            return ASR::down_cast<ASR::GenericProcedure_t>(f)->m_name;
        }
        case ASR::symbolType::DerivedType: {
            return ASR::down_cast<ASR::DerivedType_t>(f)->m_name;
        }
        case ASR::symbolType::Variable: {
            return ASR::down_cast<ASR::Variable_t>(f)->m_name;
        }
        case ASR::symbolType::ExternalSymbol: {
            return ASR::down_cast<ASR::ExternalSymbol_t>(f)->m_name;
        }
        default : throw LFortranException("Not implemented");
    }
}

static inline SymbolTable *symbol_parent_symtab(const ASR::symbol_t *f)
{
    switch (f->type) {
        case ASR::symbolType::Program: {
            return ASR::down_cast<ASR::Program_t>(f)->m_symtab->parent;
        }
        case ASR::symbolType::Module: {
            return ASR::down_cast<ASR::Module_t>(f)->m_symtab->parent;
        }
        case ASR::symbolType::Subroutine: {
            return ASR::down_cast<ASR::Subroutine_t>(f)->m_symtab->parent;
        }
        case ASR::symbolType::Function: {
            return ASR::down_cast<ASR::Function_t>(f)->m_symtab->parent;
        }
        case ASR::symbolType::GenericProcedure: {
            return ASR::down_cast<ASR::GenericProcedure_t>(f)->m_parent_symtab;
        }
        case ASR::symbolType::DerivedType: {
            return ASR::down_cast<ASR::DerivedType_t>(f)->m_symtab->parent;
        }
        case ASR::symbolType::Variable: {
            return ASR::down_cast<ASR::Variable_t>(f)->m_parent_symtab;
        }
        case ASR::symbolType::ExternalSymbol: {
            return ASR::down_cast<ASR::ExternalSymbol_t>(f)->m_parent_symtab;
        }
        default : throw LFortranException("Not implemented");
    }
}

const ASR::intentType intent_local=ASR::intentType::Local; // local variable (not a dummy argument)
const ASR::intentType intent_in   =ASR::intentType::In; // dummy argument, intent(in)
const ASR::intentType intent_out  =ASR::intentType::Out; // dummy argument, intent(out)
const ASR::intentType intent_inout=ASR::intentType::InOut; // dummy argument, intent(inout)
const ASR::intentType intent_return_var=ASR::intentType::ReturnVar; // return variable of a function

static inline bool is_arg_dummy(int intent) {
    return intent == intent_in || intent == intent_out
        || intent == intent_inout;
}


} // namespace LFortran

#endif // LFORTRAN_ASR_UTILS_H
