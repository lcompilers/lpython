#ifndef LFORTRAN_ASR_UTILS_H
#define LFORTRAN_ASR_UTILS_H

#include <functional>
#include <map>

#include <libasr/assert.h>
#include <libasr/asr.h>
#include <libasr/string_utils.h>

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

static inline ASR::symbol_t *symbol_get_past_external(ASR::symbol_t *f)
{
    if (f->type == ASR::symbolType::ExternalSymbol) {
        ASR::ExternalSymbol_t *e = ASR::down_cast<ASR::ExternalSymbol_t>(f);
        LFORTRAN_ASSERT(!ASR::is_a<ASR::ExternalSymbol_t>(*e->m_external));
        return e->m_external;
    } else {
        return f;
    }
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

static inline ASR::ttype_t *type_get_past_pointer(ASR::ttype_t *f)
{
    if (ASR::is_a<ASR::Pointer_t>(*f)) {
        ASR::Pointer_t *e = ASR::down_cast<ASR::Pointer_t>(f);
        LFORTRAN_ASSERT(!ASR::is_a<ASR::Pointer_t>(*e->m_type));
        return e->m_type;
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
    LFORTRAN_ASSERT(f != nullptr);
    switch (f->type) {
        case ASR::exprType::BoolOp: { return ((ASR::BoolOp_t*)f)->m_type; }
        case ASR::exprType::BinOp: { return ((ASR::BinOp_t*)f)->m_type; }
        case ASR::exprType::StrOp: { return ((ASR::StrOp_t*)f)->m_type; }
        case ASR::exprType::UnaryOp: { return ((ASR::UnaryOp_t*)f)->m_type; }
        case ASR::exprType::ComplexConstructor: { return ((ASR::ComplexConstructor_t*)f)->m_type; }
        case ASR::exprType::NamedExpr: { return ((ASR::NamedExpr_t*)f)->m_type; }
        case ASR::exprType::Compare: { return ((ASR::Compare_t*)f)->m_type; }
        case ASR::exprType::IfExp: { return ((ASR::IfExp_t*)f)->m_type; }
        case ASR::exprType::FunctionCall: { return ((ASR::FunctionCall_t*)f)->m_type; }
        case ASR::exprType::DerivedTypeConstructor: { return ((ASR::DerivedTypeConstructor_t*)f)->m_type; }
        case ASR::exprType::ConstantArray: { return ((ASR::ConstantArray_t*)f)->m_type; }
        case ASR::exprType::ImpliedDoLoop: { return ((ASR::ImpliedDoLoop_t*)f)->m_type; }
        case ASR::exprType::ConstantInteger: { return ((ASR::ConstantInteger_t*)f)->m_type; }
        case ASR::exprType::ConstantReal: { return ((ASR::ConstantReal_t*)f)->m_type; }
        case ASR::exprType::ConstantComplex: { return ((ASR::ConstantComplex_t*)f)->m_type; }
        case ASR::exprType::ConstantSet: { return ((ASR::ConstantSet_t*)f)->m_type; }
        case ASR::exprType::ConstantTuple: { return ((ASR::ConstantTuple_t*)f)->m_type; }
        case ASR::exprType::ConstantLogical: { return ((ASR::ConstantLogical_t*)f)->m_type; }
        case ASR::exprType::ConstantString: { return ((ASR::ConstantString_t*)f)->m_type; }
        case ASR::exprType::ConstantDictionary: { return ((ASR::ConstantDictionary_t*)f)->m_type; }
        case ASR::exprType::BOZ: { return ((ASR::BOZ_t*)f)->m_type; }
        case ASR::exprType::Var: { return EXPR2VAR(f)->m_type; }
        case ASR::exprType::ArrayRef: { return ((ASR::ArrayRef_t*)f)->m_type; }
        case ASR::exprType::DerivedRef: { return ((ASR::DerivedRef_t*)f)->m_type; }
        case ASR::exprType::ImplicitCast: { return ((ASR::ImplicitCast_t*)f)->m_type; }
        case ASR::exprType::ExplicitCast: { return ((ASR::ExplicitCast_t*)f)->m_type; }
        default : throw LFortranException("Not implemented");
    }
}

static inline std::string type_to_str(const ASR::ttype_t *t)
{
    switch (t->type) {
        case ASR::ttypeType::Integer: {
            return "integer";
        }
        case ASR::ttypeType::Real: {
            return "real";
        }
        case ASR::ttypeType::Complex: {
            return "complex";
        }
        case ASR::ttypeType::Logical: {
            return "logical";
        }
        case ASR::ttypeType::Character: {
            return "character";
        }
        case ASR::ttypeType::Tuple: {
            return "tuple";
        }
        case ASR::ttypeType::Set: {
            return "set";
        }
        case ASR::ttypeType::Dict: {
            return "dict";
        }
        default : throw LFortranException("Not implemented");
    }
}

static inline std::string unop_to_str(const ASR::unaryopType t) {
    switch (t) {
        case (ASR::unaryopType::Not): { return "!"; }
        case (ASR::unaryopType::USub): { return "-"; }
        case (ASR::unaryopType::UAdd): { return "+"; }
        case (ASR::unaryopType::Invert): {return "~"; }
        default : throw LFortranException("Not implemented");
    }
}

static inline std::string binop_to_str(const ASR::binopType t) {
    switch (t) {
        case (ASR::binopType::Add): { return " + "; }
        case (ASR::binopType::Sub): { return " - "; }
        case (ASR::binopType::Mul): { return "*"; }
        case (ASR::binopType::Div): { return "/"; }
        default : throw LFortranException("Cannot represent the binary operator as a string");
    }
}

static inline std::string cmpop_to_str(const ASR::cmpopType t) {
    switch (t) {
        case (ASR::cmpopType::Eq): { return " == "; }
        case (ASR::cmpopType::NotEq): { return " != "; }
        case (ASR::cmpopType::Lt): { return " < "; }
        case (ASR::cmpopType::LtE): { return " <= "; }
        case (ASR::cmpopType::Gt): { return " > "; }
        case (ASR::cmpopType::GtE): { return " >= "; }
        default : throw LFortranException("Cannot represent the comparison as a string");
    }
}

static inline std::string boolop_to_str(const ASR::boolopType t) {
    switch (t) {
        case (ASR::boolopType::And): { return " && "; }
        case (ASR::boolopType::Or): { return " || "; }
        case (ASR::boolopType::Eqv): { return " == "; }
        case (ASR::boolopType::NEqv): { return " != "; }
        default : throw LFortranException("Cannot represent the boolean operator as a string");
    }
}

static inline ASR::expr_t* expr_value(ASR::expr_t *f)
{
    switch (f->type) {
        case ASR::exprType::BoolOp: { return ASR::down_cast<ASR::BoolOp_t>(f)->m_value; }
        case ASR::exprType::BinOp: { return ASR::down_cast<ASR::BinOp_t>(f)->m_value; }
        case ASR::exprType::UnaryOp: { return ASR::down_cast<ASR::UnaryOp_t>(f)->m_value; }
        case ASR::exprType::ComplexConstructor: { return ASR::down_cast<ASR::ComplexConstructor_t>(f)->m_value; }
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
        case ASR::exprType::ConstantTuple: // Drop through
        case ASR::exprType::ConstantDictionary: // Drop through
        case ASR::exprType::ConstantSet: // Drop through
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
        case ASR::symbolType::CustomOperator: {
            return ASR::down_cast<ASR::CustomOperator_t>(f)->m_name;
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
        case ASR::symbolType::CustomOperator: {
            return ASR::down_cast<ASR::CustomOperator_t>(f)->m_parent_symtab;
        }
        default : throw LFortranException("Not implemented");
    }
}

// Returns the `symbol`'s symtab, or nullptr if the symbol has no symtab
static inline SymbolTable *symbol_symtab(const ASR::symbol_t *f)
{
    switch (f->type) {
        case ASR::symbolType::Program: {
            return ASR::down_cast<ASR::Program_t>(f)->m_symtab;
        }
        case ASR::symbolType::Module: {
            return ASR::down_cast<ASR::Module_t>(f)->m_symtab;
        }
        case ASR::symbolType::Subroutine: {
            return ASR::down_cast<ASR::Subroutine_t>(f)->m_symtab;
        }
        case ASR::symbolType::Function: {
            return ASR::down_cast<ASR::Function_t>(f)->m_symtab;
        }
        case ASR::symbolType::GenericProcedure: {
            return nullptr;
            //throw LFortranException("GenericProcedure does not have a symtab");
        }
        case ASR::symbolType::DerivedType: {
            return ASR::down_cast<ASR::DerivedType_t>(f)->m_symtab;
        }
        case ASR::symbolType::Variable: {
            return nullptr;
            //throw LFortranException("Variable does not have a symtab");
        }
        case ASR::symbolType::ExternalSymbol: {
            return nullptr;
            //throw LFortranException("ExternalSymbol does not have a symtab");
        }
        case ASR::symbolType::ClassProcedure: {
            return nullptr;
            //throw LFortranException("ClassProcedure does not have a symtab");
        }
        default : throw LFortranException("Not implemented");
    }
}

// Returns the Module_t the symbol is in, or nullptr if not in a module
static inline ASR::Module_t *get_sym_module(const ASR::symbol_t *sym) {
    const SymbolTable *s = symbol_parent_symtab(sym);
    while (s->parent != nullptr) {
        ASR::symbol_t *asr_owner = ASR::down_cast<ASR::symbol_t>(s->asr_owner);
        if (ASR::is_a<ASR::Module_t>(*asr_owner)) {
            return ASR::down_cast<ASR::Module_t>(asr_owner);
        }
        s = s->parent;
    }
    return nullptr;
}

// Returns the Module_t the symbol is in, or nullptr if not in a module
// or no asr_owner yet
static inline ASR::Module_t *get_sym_module0(const ASR::symbol_t *sym) {
    const SymbolTable *s = symbol_parent_symtab(sym);
    while (s->parent != nullptr) {
        if (s->asr_owner != nullptr) {
            ASR::symbol_t *asr_owner = ASR::down_cast<ASR::symbol_t>(s->asr_owner);
            if (ASR::is_a<ASR::Module_t>(*asr_owner)) {
                return ASR::down_cast<ASR::Module_t>(asr_owner);
            }
        }
        s = s->parent;
    }
    return nullptr;
}

// Returns true if the Function is intrinsic, otherwise false
static inline bool is_intrinsic_function(const ASR::Function_t *fn) {
    ASR::symbol_t *sym = (ASR::symbol_t*)fn;
    ASR::Module_t *m = get_sym_module0(sym);
    if (m != nullptr) {
        if (startswith(m->m_name, "lfortran_intrinsic")) return true;
    }
    return false;
}

// Returns true if the Function is intrinsic, otherwise false
// This version uses the `intrinsic` member of `Module`, so it
// should be used instead of is_intrinsic_function
static inline bool is_intrinsic_function2(const ASR::Function_t *fn) {
    ASR::symbol_t *sym = (ASR::symbol_t*)fn;
    ASR::Module_t *m = get_sym_module0(sym);
    if (m != nullptr) {
        if (m->m_intrinsic ||
            fn->m_abi == ASR::abiType::Intrinsic) {
                return true;
        }
    }
    return false;
}

// Returns true if the Function is intrinsic, otherwise false
template <typename T>
static inline bool is_intrinsic_optimization(const T *routine) {
    ASR::symbol_t *sym = (ASR::symbol_t*)routine;
    if( ASR::is_a<ASR::ExternalSymbol_t>(*sym) ) {
        ASR::ExternalSymbol_t* ext_sym = ASR::down_cast<ASR::ExternalSymbol_t>(sym);
        return (std::string(ext_sym->m_module_name).find("lfortran_intrinsic_optimization") != std::string::npos);
    }
    ASR::Module_t *m = get_sym_module0(sym);
    if (m != nullptr) {
        return (std::string(m->m_name).find("lfortran_intrinsic_optimization") != std::string::npos);
    }
    return false;
}

// Returns true if all arguments have a `value`
static inline bool all_args_have_value(const Vec<ASR::expr_t*> &args) {
    for (auto &a : args) {
        ASR::expr_t *v = expr_value(a);
        if (v == nullptr) return false;
    }
    return true;
}

static inline bool is_value_constant(ASR::expr_t *a_value) {
    if( a_value == nullptr ) {
        return false;
    }
    if (ASR::is_a<ASR::ConstantInteger_t>(*a_value)) {
        // OK
    } else if (ASR::is_a<ASR::ConstantReal_t>(*a_value)) {
        // OK
    } else if (ASR::is_a<ASR::ConstantComplex_t>(*a_value)) {
        // OK
    } else if (ASR::is_a<ASR::ConstantLogical_t>(*a_value)) {
        // OK
    } else if (ASR::is_a<ASR::ConstantString_t>(*a_value)) {
        // OK
    } else {
        return false;
    }
    return true;
}

template <typename T>
static inline bool is_value_constant(ASR::expr_t *a_value, T& const_value) {
    if( a_value == nullptr ) {
        return false;
    }
    if (ASR::is_a<ASR::ConstantInteger_t>(*a_value)) {
        ASR::ConstantInteger_t* const_int = ASR::down_cast<ASR::ConstantInteger_t>(a_value);
        const_value = const_int->m_n;
    } else if (ASR::is_a<ASR::ConstantReal_t>(*a_value)) {
        ASR::ConstantReal_t* const_real = ASR::down_cast<ASR::ConstantReal_t>(a_value);
        const_value = const_real->m_r;
    } else if (ASR::is_a<ASR::ConstantLogical_t>(*a_value)) {
        ASR::ConstantLogical_t* const_logical = ASR::down_cast<ASR::ConstantLogical_t>(a_value);
        const_value = const_logical->m_value;
    } else {
        return false;
    }
    return true;
}

// Returns true if all arguments are evaluated
static inline bool all_args_evaluated(const Vec<ASR::expr_t*> &args) {
    for (auto &a : args) {
        ASR::expr_t* a_value = ASRUtils::expr_value(a);
        if( !is_value_constant(a_value) ) {
            return false;
        }
    }
    return true;
}

// Returns true if all arguments are evaluated
// Overload for array
static inline bool all_args_evaluated(const Vec<ASR::array_index_t> &args) {
    for (auto &a : args) {
        bool is_m_left_const, is_m_right_const, is_m_step_const;
        is_m_left_const = is_m_right_const = is_m_step_const = false;
        if( a.m_left != nullptr ) {
            ASR::expr_t *m_left_value = ASRUtils::expr_value(a.m_left);
            is_m_left_const = is_value_constant(m_left_value);
        } else {
            is_m_left_const = true;
        }
        if( a.m_right != nullptr ) {
            ASR::expr_t *m_right_value = ASRUtils::expr_value(a.m_right);
            is_m_right_const = is_value_constant(m_right_value);
        } else {
            is_m_right_const = true;
        }
        if( a.m_step != nullptr ) {
            ASR::expr_t *m_step_value = ASRUtils::expr_value(a.m_step);
            is_m_step_const = is_value_constant(m_step_value);
        } else {
            is_m_step_const = true;
        }
        if( !(is_m_left_const && is_m_right_const && is_m_step_const) ) {
            return false;
        }
    }
    return true;
}

template <typename T>
static inline bool extract_value(ASR::expr_t* value_expr, T& value) {
    if( !is_value_constant(value_expr) ) {
        return false;
    }

    switch( value_expr->type ) {
        case ASR::exprType::ConstantReal: {
            ASR::ConstantReal_t* const_real = ASR::down_cast<ASR::ConstantReal_t>(value_expr);
            value = (T) const_real->m_r;
            break;
        }
        default:
            return false;
    }
    return true;
}

// Returns a list of values
static inline Vec<ASR::call_arg_t> get_arg_values(Allocator &al, const Vec<ASR::call_arg_t>& args) {
    Vec<ASR::call_arg_t> values;
    values.reserve(al, args.size());
    for (auto &a : args) {
        ASR::expr_t *v = expr_value(a.m_value);
        if (v == nullptr) return values;
        ASR::call_arg_t v_arg;
        v_arg.loc = v->base.loc, v_arg.m_value = v;
        values.push_back(al, v_arg);
    }
    return values;
}

// Converts a vector of call_arg to a vector of expr
// It skips missing call_args
static inline Vec<ASR::expr_t*> call_arg2expr(Allocator &al, const Vec<ASR::call_arg_t>& call_args) {
    Vec<ASR::expr_t*> args;
    args.reserve(al, call_args.size());
    for (auto &a : call_args) {
        if (a.m_value != nullptr) {
            args.push_back(al, a.m_value);
        }
    }
    return args;
}

// Returns the TranslationUnit_t's symbol table by going via parents
static inline SymbolTable *get_tu_symtab(SymbolTable *symtab) {
    SymbolTable *s = symtab;
    while (s->parent != nullptr) {
        s = s->parent;
    }
    LFORTRAN_ASSERT(ASR::is_a<ASR::unit_t>(*s->asr_owner))
    return s;
}

// Returns the name of scopes in reverse order (local scope first, function second, module last)
static inline Vec<char*> get_scope_names(Allocator &al, const SymbolTable *symtab) {
    Vec<char*> scope_names;
    scope_names.reserve(al, 4);
    const SymbolTable *s = symtab;
    while (s->parent != nullptr) {
        char *owner_name = symbol_name(ASR::down_cast<ASR::symbol_t>(s->asr_owner));
        scope_names.push_back(al, owner_name);
        s = s->parent;
    }
    return scope_names;
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
std::vector<std::string> order_deps(std::map<std::string,
        std::vector<std::string>> const &deps);

std::vector<std::string> determine_module_dependencies(
        const ASR::TranslationUnit_t &unit);

ASR::Module_t* extract_module(const ASR::TranslationUnit_t &m);

ASR::Module_t* load_module(Allocator &al, SymbolTable *symtab,
                            const std::string &module_name,
                            const Location &loc, bool intrinsic,
                            const std::string &rl_path,
                            bool run_verify,
                            const std::function<void (const std::string &, const Location &)> err);

ASR::TranslationUnit_t* find_and_load_module(Allocator &al, const std::string &msym,
                                                SymbolTable &symtab, bool intrinsic,
                                                const std::string &rl_path);

void set_intrinsic(ASR::TranslationUnit_t* trans_unit);

ASR::asr_t* getDerivedRef_t(Allocator& al, const Location& loc,
                            ASR::asr_t* v_var, ASR::symbol_t* member,
                            SymbolTable* current_scope);

bool use_overloaded(ASR::expr_t* left, ASR::expr_t* right,
                    ASR::binopType op, std::string& intrinsic_op_name,
                    SymbolTable* curr_scope, ASR::asr_t*& asr,
                    Allocator &al, const Location& loc,
                    const std::function<void (const std::string &, const Location &)> err);

bool is_op_overloaded(ASR::binopType op, std::string& intrinsic_op_name,
                      SymbolTable* curr_scope);

bool use_overloaded(ASR::expr_t* left, ASR::expr_t* right,
                    ASR::cmpopType op, std::string& intrinsic_op_name,
                    SymbolTable* curr_scope, ASR::asr_t*& asr,
                    Allocator &al, const Location& loc,
                    const std::function<void (const std::string &, const Location &)> err);

bool is_op_overloaded(ASR::cmpopType op, std::string& intrinsic_op_name,
                      SymbolTable* curr_scope);

bool use_overloaded_assignment(ASR::expr_t* target, ASR::expr_t* value,
                               SymbolTable* curr_scope, ASR::asr_t*& asr,
                               Allocator &al, const Location& loc);

void set_intrinsic(ASR::symbol_t* sym);

static inline int extract_kind_from_ttype_t(const ASR::ttype_t* type) {
    if (type == nullptr) {
        return -1;
    }
    switch (type->type) {
        case ASR::ttypeType::Integer : {
            return ASR::down_cast<ASR::Integer_t>(type)->m_kind;
        }
        case ASR::ttypeType::Real : {
            return ASR::down_cast<ASR::Real_t>(type)->m_kind;
        }
        case ASR::ttypeType::Complex: {
            return ASR::down_cast<ASR::Complex_t>(type)->m_kind;
        }
        case ASR::ttypeType::Character: {
            return ASR::down_cast<ASR::Character_t>(type)->m_kind;
        }
        case ASR::ttypeType::Logical: {
            return ASR::down_cast<ASR::Logical_t>(type)->m_kind;
        }
        case ASR::ttypeType::Pointer: {
            return extract_kind_from_ttype_t(ASR::down_cast<ASR::Pointer_t>(type)->m_type);
        }
        default : {
            return -1;
        }
    }
}

static inline bool is_pointer(ASR::ttype_t *x) {
    return ASR::is_a<ASR::Pointer_t>(*x);
}

static inline bool is_integer(ASR::ttype_t &x) {
    return ASR::is_a<ASR::Integer_t>(*type_get_past_pointer(&x));
}

static inline bool is_real(ASR::ttype_t &x) {
    return ASR::is_a<ASR::Real_t>(*type_get_past_pointer(&x));
}

static inline bool is_character(ASR::ttype_t &x) {
    return ASR::is_a<ASR::Character_t>(*type_get_past_pointer(&x));
}

static inline bool is_complex(ASR::ttype_t &x) {
    return ASR::is_a<ASR::Complex_t>(*type_get_past_pointer(&x));
}

static inline bool is_logical(ASR::ttype_t &x) {
    return ASR::is_a<ASR::Logical_t>(*type_get_past_pointer(&x));
}

inline bool is_array(ASR::ttype_t *x) {
    int n_dims = 0;
    switch (x->type) {
        case ASR::ttypeType::Integer: {
            n_dims = ASR::down_cast<ASR::Integer_t>(x)->n_dims;
            break;
        }
        case ASR::ttypeType::Real: {
            n_dims = ASR::down_cast<ASR::Real_t>(x)->n_dims;
            break;
        }
        case ASR::ttypeType::Complex: {
            n_dims = ASR::down_cast<ASR::Complex_t>(x)->n_dims;
            break;
        }
        case ASR::ttypeType::Character: {
            n_dims = ASR::down_cast<ASR::Character_t>(x)->n_dims;
            break;
        }
        case ASR::ttypeType::Logical: {
            n_dims = ASR::down_cast<ASR::Logical_t>(x)->n_dims;
            break;
        }
        case ASR::ttypeType::Derived: {
            n_dims = ASR::down_cast<ASR::Derived_t>(x)->n_dims;
            break;
        }
        case ASR::ttypeType::Class: {
            n_dims = ASR::down_cast<ASR::Class_t>(x)->n_dims;
            break;
        }
        case ASR::ttypeType::Pointer: {
            return is_array(ASR::down_cast<ASR::Pointer_t>(x)->m_type);
            break;
        }
        default:
            throw LFortranException("Not implemented.");
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
    bool res = source->type == ASR::down_cast<ASR::Pointer_t>(dest)->m_type->type;
    return res;
}

            inline int extract_kind_str(char* m_n, char *&kind_str) {
                char *p = m_n;
                while (*p != '\0') {
                    if (*p == '_') {
                        p++;
                        std::string kind = std::string(p);
                        int ikind = std::atoi(p);
                        if (ikind == 0) {
                            // Not an integer, return a string
                            kind_str = p;
                            return 0;
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

            template <typename SemanticError>
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

            template <typename SemanticError>
            inline int extract_len(ASR::expr_t* len_expr, const Location& loc) {
                int a_len = -10;
                switch( len_expr->type ) {
                    case ASR::exprType::ConstantInteger: {
                        a_len = ASR::down_cast<ASR::ConstantInteger_t>
                                (len_expr)->m_n;
                        break;
                    }
                    case ASR::exprType::Var: {
                        ASR::Var_t* len_var =
                            ASR::down_cast<ASR::Var_t>(len_expr);
                        ASR::Variable_t* len_variable =
                            ASR::down_cast<ASR::Variable_t>(
                                symbol_get_past_external(len_var->m_v));
                        if( len_variable->m_storage == ASR::storage_typeType::Parameter ) {
                            if( len_variable->m_type->type == ASR::ttypeType::Integer ) {
                                LFORTRAN_ASSERT( len_variable->m_value != nullptr );
                                a_len = ASR::down_cast<ASR::ConstantInteger_t>(len_variable->m_value)->m_n;
                            } else {
                                std::string msg = "Integer variable required. " + std::string(len_variable->m_name) +
                                                " is not an Integer variable.";
                                throw SemanticError(msg, loc);
                            }
                        } else {
                            // An expression is beind used for `len` that cannot be evaluated
                            a_len = -3;
                        }
                        break;
                    }
                    case ASR::exprType::FunctionCall: {
                        a_len = -3;
                        break;
                    }
                    case ASR::exprType::BinOp: {
                        a_len = -3;
                        break;
                    }
                    default: {
                        throw SemanticError("Only Integers or variables implemented so far for `len` expressions",
                                            loc);
                    }
                }
                LFORTRAN_ASSERT(a_len != -10)
                return a_len;
            }

            inline bool check_equal_type(ASR::ttype_t* x, ASR::ttype_t* y) {
                if( x->type == y->type ) {
                    return true;
                }

                return ASRUtils::is_same_type_pointer(x, y);
            }

int select_generic_procedure(const Vec<ASR::call_arg_t> &args,
        const ASR::GenericProcedure_t &p, Location loc,
        const std::function<void (const std::string &, const Location &)> err);

ASR::asr_t* symbol_resolve_external_generic_procedure_without_eval(
            const Location &loc,
            ASR::symbol_t *v, Vec<ASR::call_arg_t>& args,
            SymbolTable* current_scope, Allocator& al,
            const std::function<void (const std::string &, const Location &)> err);

ASR::asr_t* make_ImplicitCast_t_value(Allocator &al, const Location &a_loc, ASR::expr_t* a_arg, ASR::cast_kindType a_kind, ASR::ttype_t* a_type);

} // namespace ASRUtils

} // namespace LFortran

#endif // LFORTRAN_ASR_UTILS_H
