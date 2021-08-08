#ifndef LFORTRAN_ASR_UTILS_H
#define LFORTRAN_ASR_UTILS_H

#include <lfortran/assert.h>
#include <lfortran/asr.h>

namespace LFortran  {

    namespace ASRUtils  {

static inline  double extract_real(const char *s) {
        return std::atof(s);
    }

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

static inline const ASR::symbol_t *symbol_get_past_external(const ASR::symbol_t *f)
{
    if (f->type == ASR::symbolType::ExternalSymbol) {
        ASR::ExternalSymbol_t *e = ASR::down_cast<ASR::ExternalSymbol_t>(f);
        LFORTRAN_ASSERT(!ASR::is_a<ASR::ExternalSymbol_t>(*e->m_external));
        return e->m_external;
    } else {
        return f;
    }
}

static inline ASR::Variable_t* EXPR2VAR(const ASR::expr_t *f)
{
    return ASR::down_cast<ASR::Variable_t>(symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(f)->m_v));
}

static inline ASR::Function_t* EXPR2FUN(const ASR::expr_t *f)
{
    return ASR::down_cast<ASR::Function_t>(symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(f)->m_v));
}

static inline ASR::Subroutine_t* EXPR2SUB(const ASR::expr_t *f)
{
    return ASR::down_cast<ASR::Subroutine_t>(symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(f)->m_v));
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
        case ASR::exprType::DerivedRef: { return ((ASR::DerivedRef_t*)f)->m_type; }
        case ASR::exprType::ConstantArray: { return ((ASR::ConstantArray_t*)f)->m_type; }
        case ASR::exprType::ConstantInteger: { return ((ASR::ConstantInteger_t*)f)->m_type; }
        case ASR::exprType::ConstantReal: { return ((ASR::ConstantReal_t*)f)->m_type; }
        case ASR::exprType::ConstantComplex: { return ((ASR::ConstantComplex_t*)f)->m_type; }
        case ASR::exprType::ConstantString: { return ((ASR::ConstantString_t*)f)->m_type; }
        case ASR::exprType::ImplicitCast: { return ((ASR::ImplicitCast_t*)f)->m_type; }
        case ASR::exprType::ExplicitCast: { return ((ASR::ExplicitCast_t*)f)->m_type; }
        case ASR::exprType::Var: { return EXPR2VAR(f)->m_type; }
        case ASR::exprType::ConstantLogical: { return ((ASR::ConstantLogical_t*)f)->m_type; }
        case ASR::exprType::StrOp: { return ((ASR::StrOp_t*)f)->m_type; }
        case ASR::exprType::ImpliedDoLoop: { return ((ASR::ImpliedDoLoop_t*)f)->m_type; }
        default : throw LFortranException("Not implemented");
    }
}

static inline ASR::expr_t* expr_value(ASR::expr_t *f)
{
    switch (f->type) {
        case ASR::exprType::BoolOp: { return ASR::down_cast<ASR::BoolOp_t>(f)->m_value; }
        case ASR::exprType::BinOp: { return ASR::down_cast<ASR::BinOp_t>(f)->m_value; }
        case ASR::exprType::UnaryOp: { return ASR::down_cast<ASR::UnaryOp_t>(f)->m_value; }
        case ASR::exprType::Compare: { return ASR::down_cast<ASR::Compare_t>(f)->m_value; }
        case ASR::exprType::FunctionCall: { return ASR::down_cast<ASR::FunctionCall_t>(f)->m_value; }
        case ASR::exprType::ArrayRef: { return ASR::down_cast<ASR::ArrayRef_t>(f)->m_value; }
        case ASR::exprType::DerivedRef: { return ASR::down_cast<ASR::DerivedRef_t>(f)->m_value; }
        case ASR::exprType::ImplicitCast: { return ASR::down_cast<ASR::ImplicitCast_t>(f)->m_value; }
        case ASR::exprType::ExplicitCast: { return ASR::down_cast<ASR::ExplicitCast_t>(f)->m_value; }
        case ASR::exprType::Var: { return EXPR2VAR(f)->m_value; }
        case ASR::exprType::StrOp: { return ASR::down_cast<ASR::StrOp_t>(f)->m_value; }
        case ASR::exprType::ImpliedDoLoop: { return ASR::down_cast<ASR::ImpliedDoLoop_t>(f)->m_value; }
        case ASR::exprType::ConstantArray: // Drop through
        case ASR::exprType::ConstantInteger: // Drop through
        case ASR::exprType::ConstantReal: // Drop through
        case ASR::exprType::ConstantComplex: // Drop through
        case ASR::exprType::ConstantLogical: // Drop through
        case ASR::exprType::ConstantString:{ // For all Constants
            return f;
        }
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
        case ASR::symbolType::ClassProcedure: {
            return ASR::down_cast<ASR::ClassProcedure_t>(f)->m_name;
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
        case ASR::symbolType::ClassProcedure: {
            return ASR::down_cast<ASR::ClassProcedure_t>(f)->m_parent_symtab;
        }
        default : throw LFortranException("Not implemented");
    }
}

const ASR::intentType intent_local=ASR::intentType::Local; // local variable (not a dummy argument)
const ASR::intentType intent_in   =ASR::intentType::In; // dummy argument, intent(in)
const ASR::intentType intent_out  =ASR::intentType::Out; // dummy argument, intent(out)
const ASR::intentType intent_inout=ASR::intentType::InOut; // dummy argument, intent(inout)
const ASR::intentType intent_return_var=ASR::intentType::ReturnVar; // return variable of a function
const ASR::intentType intent_unspecified=ASR::intentType::Unspecified; // dummy argument, ambiguous intent

static inline bool is_arg_dummy(int intent) {
    return intent == intent_in || intent == intent_out
        || intent == intent_inout || intent == intent_unspecified;
}

static inline bool main_program_present(const ASR::TranslationUnit_t &unit)
{
    for (auto &a : unit.m_global_scope->scope) {
        if (ASR::is_a<ASR::Program_t>(*a.second)) return true;
    }
    return false;
}

// Accepts dependencies in the form A -> [B, D, ...], B -> [C, D]
// Returns a list of dependencies in the order that they should be built:
// [D, C, B, A]
std::vector<int> order_deps(std::map<int, std::vector<int>> &deps);
std::vector<std::string> order_deps(std::map<std::string,
        std::vector<std::string>> &deps);

std::vector<std::string> determine_module_dependencies(
        const ASR::TranslationUnit_t &unit);

ASR::Module_t* extract_module(const ASR::TranslationUnit_t &m);

ASR::Module_t* load_module(Allocator &al, SymbolTable *symtab,
                            const std::string &module_name,
                            const Location &loc, bool intrinsic);

ASR::TranslationUnit_t* find_and_load_module(Allocator &al, const std::string &msym,
                                                SymbolTable &symtab, bool intrinsic);

void set_intrinsic(ASR::TranslationUnit_t* trans_unit);

void set_intrinsic(ASR::symbol_t* sym);

static inline int extract_kind_from_ttype_t(const ASR::ttype_t* curr_type) {
                if( curr_type == nullptr ) {
                    return -1;
                }
                switch (curr_type->type) {
                    case ASR::ttypeType::Real : {
                        return ((ASR::Real_t*)(&(curr_type->base)))->m_kind;
                    }
                    case ASR::ttypeType::RealPointer : {
                        return ((ASR::RealPointer_t*)(&(curr_type->base)))->m_kind;
                    }
                    case ASR::ttypeType::Integer : {
                        return ((ASR::Integer_t*)(&(curr_type->base)))->m_kind;
                    }
                    case ASR::ttypeType::IntegerPointer : {
                        return ((ASR::IntegerPointer_t*)(&(curr_type->base)))->m_kind;
                    }
                    case ASR::ttypeType::Complex: {
                        return ((ASR::Complex_t*)(&(curr_type->base)))->m_kind;
                    }
                    case ASR::ttypeType::ComplexPointer: {
                        return ((ASR::ComplexPointer_t*)(&(curr_type->base)))->m_kind;
                    }
                    default : {
                        return -1;
                    }
                }
            }

       static inline bool is_pointer(ASR::ttype_t* x) {
                switch( x->type ) {
                    case ASR::ttypeType::IntegerPointer:
                    case ASR::ttypeType::RealPointer:
                    case ASR::ttypeType::ComplexPointer:
                    case ASR::ttypeType::CharacterPointer:
                    case ASR::ttypeType::LogicalPointer:
                    case ASR::ttypeType::DerivedPointer:
                        return true;
                        break;
                    default:
                        break;
                }
                return false;
            }

            inline bool is_array(ASR::ttype_t* x) {
                int n_dims = 0;
                switch( x->type ) {
                    case ASR::ttypeType::IntegerPointer: {
                        ASR::IntegerPointer_t* _type = (ASR::IntegerPointer_t*)(&(x->base));
                        n_dims = _type->n_dims;
                        break;
                    }
                    case ASR::ttypeType::Integer: {
                        ASR::Integer_t* _type = (ASR::Integer_t*)(&(x->base));
                        n_dims = _type->n_dims;
                        break;
                    }
                    case ASR::ttypeType::Real: {
                        ASR::Real_t* _type = (ASR::Real_t*)(&(x->base));
                        n_dims = _type->n_dims > 0;
                        break;
                    }
                    case ASR::ttypeType::RealPointer: {
                        ASR::RealPointer_t* _type = (ASR::RealPointer_t*)(&(x->base));
                        n_dims = _type->n_dims;
                        break;
                    }
                    case ASR::ttypeType::Complex: {
                        ASR::Complex_t* _type = (ASR::Complex_t*)(&(x->base));
                        n_dims = _type->n_dims > 0;
                        break;
                    }
                    case ASR::ttypeType::Logical: {
                        ASR::Logical_t* _type = (ASR::Logical_t*)(&(x->base));
                        n_dims = _type->n_dims > 0;
                        break;
                    }
                    default:
                        break;
                }
                return n_dims > 0;
            }

            inline bool is_same_type_pointer(ASR::ttype_t* source, ASR::ttype_t* dest) {
                bool is_source_pointer = is_pointer(source), is_dest_pointer = is_pointer(dest);
                if( (!is_source_pointer && !is_dest_pointer) ||
                    (is_source_pointer && is_dest_pointer) ) {
                    return false;
                }
                if( is_source_pointer && !is_dest_pointer ) {
                    ASR::ttype_t* temp = source;
                    source = dest;
                    dest = temp;
                }
                bool res = false;
                switch( dest->type ) {
                    case ASR::ttypeType::IntegerPointer:
                        res = source->type == ASR::ttypeType::Integer;
                        break;
                    case ASR::ttypeType::RealPointer:
                        res = source->type == ASR::ttypeType::Real;
                        break;
                    case ASR::ttypeType::ComplexPointer:
                        res = source->type == ASR::ttypeType::Complex;
                        break;
                    case ASR::ttypeType::CharacterPointer:
                        res = source->type == ASR::ttypeType::Character;
                        break;
                    case ASR::ttypeType::LogicalPointer:
                        return source->type == ASR::ttypeType::Logical;
                        break;
                    case ASR::ttypeType::DerivedPointer:
                        res = source->type == ASR::ttypeType::Derived;
                        break;
                    default:
                        break;
                }
                return res;
            }

            inline int extract_kind(char* m_n) {
                char *p = m_n;
                while (*p != '\0') {
                    if (*p == '_') {
                        p++;
                        std::string kind = std::string(p);
                        int ikind = std::atoi(p);
                        if (ikind == 0) {
                            // Not an integer
                            return 4; // FIXME: we need to return a string
                        } else {
                            return ikind;
                        }
                    }
                    if (*p == 'd' || *p == 'D') {
                        // Double precision
                        return 8;
                    }
                    p++;
                }
                return 4;
            }

            inline int extract_kind(ASR::expr_t* kind_expr, const Location& loc) {
                int a_kind = 4;
                switch( kind_expr->type ) {
                    case ASR::exprType::ConstantInteger: {
                        a_kind = ASR::down_cast<ASR::ConstantInteger_t>
                                (kind_expr)->m_n;
                        break;
                    }
                    case ASR::exprType::Var: {
                        ASR::Var_t* kind_var =
                            ASR::down_cast<ASR::Var_t>(kind_expr);
                        ASR::Variable_t* kind_variable =
                            ASR::down_cast<ASR::Variable_t>(
                                symbol_get_past_external(kind_var->m_v));
                        if( kind_variable->m_storage == ASR::storage_typeType::Parameter ) {
                            if( kind_variable->m_type->type == ASR::ttypeType::Integer ) {
                                LFORTRAN_ASSERT( kind_variable->m_value != nullptr );
                                a_kind = ASR::down_cast<ASR::ConstantInteger_t>(kind_variable->m_value)->m_n;
                            } else {
                                std::string msg = "Integer variable required. " + std::string(kind_variable->m_name) +
                                                " is not an Integer variable.";
                                throw SemanticError(msg, loc);
                            }
                        } else {
                            std::string msg = "Parameter " + std::string(kind_variable->m_name) +
                                            " is a variable, which does not reduce to a constant expression";
                            throw SemanticError(msg, loc);
                        }
                        break;
                    }
                    default: {
                        throw SemanticError(R"""(Only Integer literals or expressions which reduce to constant Integer are accepted as kind parameters.)""",
                                            loc);
                    }
                }
                return a_kind;
            }

            inline bool check_equal_type(ASR::ttype_t* x, ASR::ttype_t* y) {
                if( x->type == y->type ) {
                    return true;
                }

                return ASRUtils::is_same_type_pointer(x, y);
            }
} // namespace ASRUtils

} // namespace LFortran

#endif // LFORTRAN_ASR_UTILS_H
