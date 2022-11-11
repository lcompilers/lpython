#ifndef LFORTRAN_ASR_UTILS_H
#define LFORTRAN_ASR_UTILS_H

#include <functional>
#include <map>
#include <set>
#include <limits>

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
    if (f && f->type == ASR::symbolType::ExternalSymbol) {
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

static inline ASR::ttype_t* expr_type(const ASR::expr_t *f)
{
    return ASR::expr_type0(f);
}

static inline ASR::ttype_t* symbol_type(const ASR::symbol_t *f)
{
    switch( f->type ) {
        case ASR::symbolType::Variable: {
            return ASR::down_cast<ASR::Variable_t>(f)->m_type;
        }
        case ASR::symbolType::EnumType: {
            return ASR::down_cast<ASR::EnumType_t>(f)->m_type;
        }
        default: {
            throw LCompilersException("Cannot return type of, " +
                                    std::to_string(f->type) + " symbol.");
        }
    }
    return nullptr;
}

static inline ASR::ttype_t* get_contained_type(ASR::ttype_t* asr_type) {
    switch( asr_type->type ) {
        case ASR::ttypeType::List: {
            return ASR::down_cast<ASR::List_t>(asr_type)->m_type;
        }
        case ASR::ttypeType::Set: {
            return ASR::down_cast<ASR::Set_t>(asr_type)->m_type;
        }
        case ASR::ttypeType::Enum: {
            ASR::Enum_t* enum_asr = ASR::down_cast<ASR::Enum_t>(asr_type);
            ASR::EnumType_t* enum_type = ASR::down_cast<ASR::EnumType_t>(enum_asr->m_enum_type);
            return enum_type->m_type;
        }
        case ASR::ttypeType::Pointer: {
            ASR::Pointer_t* pointer_asr = ASR::down_cast<ASR::Pointer_t>(asr_type);
            return pointer_asr->m_type;
        }
        case ASR::ttypeType::Const: {
            ASR::Const_t* const_asr = ASR::down_cast<ASR::Const_t>(asr_type);
            return const_asr->m_type;
        }
        default: {
            return asr_type;
        }
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
        case ASR::ttypeType::List: {
            return "list";
        }
        case ASR::ttypeType::Struct: {
            return "derived type";
        }
        case ASR::ttypeType::Union: {
            return "union";
        }
        case ASR::ttypeType::CPtr: {
            return "type(c_ptr)";
        }
        case ASR::ttypeType::Pointer: {
            return type_to_str(ASRUtils::type_get_past_pointer(
                        const_cast<ASR::ttype_t*>(t))) + " pointer";
        }
        case ASR::ttypeType::Const: {
            return type_to_str(ASRUtils::get_contained_type(
                        const_cast<ASR::ttype_t*>(t))) + " const";
        }
        case ASR::ttypeType::TypeParameter: {
            ASR::TypeParameter_t* tp = ASR::down_cast<ASR::TypeParameter_t>(t);
            return tp->m_param;
        }
        default : throw LCompilersException("Not implemented " + std::to_string(t->type) + ".");
    }
}

static inline std::string binop_to_str(const ASR::binopType t) {
    switch (t) {
        case (ASR::binopType::Add): { return " + "; }
        case (ASR::binopType::Sub): { return " - "; }
        case (ASR::binopType::Mul): { return "*"; }
        case (ASR::binopType::Div): { return "/"; }
        default : throw LCompilersException("Cannot represent the binary operator as a string");
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
        default : throw LCompilersException("Cannot represent the comparison as a string");
    }
}

static inline std::string logicalbinop_to_str_python(const ASR::logicalbinopType t) {
    switch (t) {
        case (ASR::logicalbinopType::And): { return " && "; }
        case (ASR::logicalbinopType::Or): { return " || "; }
        case (ASR::logicalbinopType::Eqv): { return " == "; }
        case (ASR::logicalbinopType::NEqv): { return " != "; }
        default : throw LCompilersException("Cannot represent the boolean operator as a string");
    }
}

static inline ASR::expr_t* expr_value(ASR::expr_t *f)
{
    return ASR::expr_value0(f);
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
        case ASR::symbolType::Function: {
            return ASR::down_cast<ASR::Function_t>(f)->m_name;
        }
        case ASR::symbolType::GenericProcedure: {
            return ASR::down_cast<ASR::GenericProcedure_t>(f)->m_name;
        }
        case ASR::symbolType::StructType: {
            return ASR::down_cast<ASR::StructType_t>(f)->m_name;
        }
        case ASR::symbolType::EnumType: {
            return ASR::down_cast<ASR::EnumType_t>(f)->m_name;
        }
        case ASR::symbolType::UnionType: {
            return ASR::down_cast<ASR::UnionType_t>(f)->m_name;
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
        case ASR::symbolType::AssociateBlock: {
            return ASR::down_cast<ASR::AssociateBlock_t>(f)->m_name;
        }
        case ASR::symbolType::Block: {
            return ASR::down_cast<ASR::Block_t>(f)->m_name;
        }
        default : throw LCompilersException("Not implemented");
    }
}

static inline std::pair<char**, size_t> symbol_dependencies(const ASR::symbol_t *f)
{
    switch (f->type) {
        case ASR::symbolType::Program: {
            ASR::Program_t* sym = ASR::down_cast<ASR::Program_t>(f);
            return std::make_pair(sym->m_dependencies, sym->n_dependencies);
        }
        case ASR::symbolType::Module: {
            ASR::Module_t* sym = ASR::down_cast<ASR::Module_t>(f);
            return std::make_pair(sym->m_dependencies, sym->n_dependencies);
        }
        case ASR::symbolType::Function: {
            ASR::Function_t* sym = ASR::down_cast<ASR::Function_t>(f);
            return std::make_pair(sym->m_dependencies, sym->n_dependencies);
        }
        case ASR::symbolType::StructType: {
            ASR::StructType_t* sym = ASR::down_cast<ASR::StructType_t>(f);
            return std::make_pair(sym->m_dependencies, sym->n_dependencies);
        }
        case ASR::symbolType::EnumType: {
            ASR::EnumType_t* sym = ASR::down_cast<ASR::EnumType_t>(f);
            return std::make_pair(sym->m_dependencies, sym->n_dependencies);
        }
        case ASR::symbolType::UnionType: {
            ASR::UnionType_t* sym = ASR::down_cast<ASR::UnionType_t>(f);
            return std::make_pair(sym->m_dependencies, sym->n_dependencies);
        }
        default : throw LCompilersException("Not implemented");
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
        case ASR::symbolType::Function: {
            return ASR::down_cast<ASR::Function_t>(f)->m_symtab->parent;
        }
        case ASR::symbolType::GenericProcedure: {
            return ASR::down_cast<ASR::GenericProcedure_t>(f)->m_parent_symtab;
        }
        case ASR::symbolType::StructType: {
            return ASR::down_cast<ASR::StructType_t>(f)->m_symtab->parent;
        }
        case ASR::symbolType::EnumType: {
            return ASR::down_cast<ASR::EnumType_t>(f)->m_symtab->parent;
        }
        case ASR::symbolType::UnionType: {
            return ASR::down_cast<ASR::UnionType_t>(f)->m_symtab->parent;
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
        case ASR::symbolType::AssociateBlock: {
            return ASR::down_cast<ASR::AssociateBlock_t>(f)->m_symtab->parent;
        }
        case ASR::symbolType::Block: {
            return ASR::down_cast<ASR::Block_t>(f)->m_symtab->parent;
        }
        default : throw LCompilersException("Not implemented");
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
        case ASR::symbolType::Function: {
            return ASR::down_cast<ASR::Function_t>(f)->m_symtab;
        }
        case ASR::symbolType::GenericProcedure: {
            return nullptr;
            //throw LCompilersException("GenericProcedure does not have a symtab");
        }
        case ASR::symbolType::StructType: {
            return ASR::down_cast<ASR::StructType_t>(f)->m_symtab;
        }
        case ASR::symbolType::EnumType: {
            return ASR::down_cast<ASR::EnumType_t>(f)->m_symtab;
        }
        case ASR::symbolType::UnionType: {
            return ASR::down_cast<ASR::UnionType_t>(f)->m_symtab;
        }
        case ASR::symbolType::Variable: {
            return nullptr;
            //throw LCompilersException("Variable does not have a symtab");
        }
        case ASR::symbolType::ExternalSymbol: {
            return nullptr;
            //throw LCompilersException("ExternalSymbol does not have a symtab");
        }
        case ASR::symbolType::ClassProcedure: {
            return nullptr;
            //throw LCompilersException("ClassProcedure does not have a symtab");
        }
        case ASR::symbolType::AssociateBlock: {
            return ASR::down_cast<ASR::AssociateBlock_t>(f)->m_symtab;
        }
        case ASR::symbolType::Block: {
            return ASR::down_cast<ASR::Block_t>(f)->m_symtab;
        }
        default : throw LCompilersException("Not implemented");
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
template <typename T>
static inline bool is_intrinsic_procedure(const T *fn) {
    ASR::symbol_t *sym = (ASR::symbol_t*)fn;
    ASR::Module_t *m = get_sym_module0(sym);
    if (m != nullptr) {
        if (startswith(m->m_name, "lfortran_intrinsic")) return true;
    }
    return false;
}

static inline bool is_intrinsic_symbol(const ASR::symbol_t *fn) {
    const ASR::symbol_t *sym = fn;
    ASR::Module_t *m = get_sym_module0(sym);
    if (m != nullptr) {
        if (m->m_intrinsic) {
            return true;
        }
        if (startswith(m->m_name, "lfortran_intrinsic")) return true;
    }
    return false;
}

// Returns true if the Function is intrinsic, otherwise false
// This version uses the `intrinsic` member of `Module`, so it
// should be used instead of is_intrinsic_procedure
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
    if (ASR::is_a<ASR::IntegerConstant_t>(*a_value)) {
        // OK
    } else if (ASR::is_a<ASR::RealConstant_t>(*a_value)) {
        // OK
    } else if (ASR::is_a<ASR::ComplexConstant_t>(*a_value)) {
        // OK
    } else if (ASR::is_a<ASR::LogicalConstant_t>(*a_value)) {
        // OK
    } else if (ASR::is_a<ASR::StringConstant_t>(*a_value)) {
        // OK
    } else {
        return false;
    }
    return true;
}

static inline bool is_value_constant(ASR::expr_t *a_value, int64_t& const_value) {
    if( a_value == nullptr ) {
        return false;
    }
    if (ASR::is_a<ASR::IntegerConstant_t>(*a_value)) {
        ASR::IntegerConstant_t* const_int = ASR::down_cast<ASR::IntegerConstant_t>(a_value);
        const_value = const_int->m_n;
    } else {
        return false;
    }
    return true;
}

static inline bool is_value_constant(ASR::expr_t *a_value, bool& const_value) {
    if( a_value == nullptr ) {
        return false;
    }
    if (ASR::is_a<ASR::LogicalConstant_t>(*a_value)) {
        ASR::LogicalConstant_t* const_logical = ASR::down_cast<ASR::LogicalConstant_t>(a_value);
        const_value = const_logical->m_value;
    } else {
        return false;
    }
    return true;
}

static inline bool is_value_constant(ASR::expr_t *a_value, double& const_value) {
    if( a_value == nullptr ) {
        return false;
    }
    if (ASR::is_a<ASR::IntegerConstant_t>(*a_value)) {
        ASR::IntegerConstant_t* const_int = ASR::down_cast<ASR::IntegerConstant_t>(a_value);
        const_value = const_int->m_n;
    } else if (ASR::is_a<ASR::RealConstant_t>(*a_value)) {
        ASR::RealConstant_t* const_real = ASR::down_cast<ASR::RealConstant_t>(a_value);
        const_value = const_real->m_r;
    } else {
        return false;
    }
    return true;
}

static inline bool is_value_constant(ASR::expr_t *a_value, std::string& const_value) {
    if( a_value == nullptr ) {
        return false;
    }
    if (ASR::is_a<ASR::StringConstant_t>(*a_value)) {
        ASR::StringConstant_t* const_string = ASR::down_cast<ASR::StringConstant_t>(a_value);
        const_value = std::string(const_string->m_s);
    } else {
        return false;
    }
    return true;
}

static inline bool is_value_equal(ASR::expr_t* test_expr, ASR::expr_t* desired_expr) {
    ASR::expr_t* test_value = expr_value(test_expr);
    ASR::expr_t* desired_value = expr_value(desired_expr);
    if( !is_value_constant(test_value) ||
        !is_value_constant(desired_value) ||
        test_value->type != desired_value->type ) {
        return false;
    }

    switch( desired_value->type ) {
        case ASR::exprType::IntegerConstant: {
            ASR::IntegerConstant_t* test_int = ASR::down_cast<ASR::IntegerConstant_t>(test_value);
            ASR::IntegerConstant_t* desired_int = ASR::down_cast<ASR::IntegerConstant_t>(desired_value);
            return test_int->m_n == desired_int->m_n;
        }
        case ASR::exprType::StringConstant: {
            ASR::StringConstant_t* test_str = ASR::down_cast<ASR::StringConstant_t>(test_value);
            ASR::StringConstant_t* desired_str = ASR::down_cast<ASR::StringConstant_t>(desired_value);
            return std::string(test_str->m_s) == std::string(desired_str->m_s);
        }
        default: {
            return false;
        }
    }
}

static inline bool is_value_in_range(ASR::expr_t* start, ASR::expr_t* end, ASR::expr_t* value) {
    ASR::expr_t *start_value = nullptr, *end_value = nullptr;
    if( start ) {
        start_value = expr_value(start);
    }
    if( end ) {
        end_value = expr_value(end);
    }
    ASR::expr_t* test_value = expr_value(value);


    double start_double = std::numeric_limits<double>::min();
    double end_double = std::numeric_limits<double>::max();
    double value_double;
    bool start_const = is_value_constant(start_value, start_double);
    bool end_const = is_value_constant(end_value, end_double);
    bool value_const = is_value_constant(test_value, value_double);
    if( !value_const || (!start_const && !end_const) ) {
        return false;
    }
    return value_double >= start_double && value_double <= end_double;
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
        case ASR::exprType::IntegerConstant: {
            ASR::IntegerConstant_t* const_int = ASR::down_cast<ASR::IntegerConstant_t>(value_expr);
            value = (T) const_int->m_n;
            break;
        }
        case ASR::exprType::LogicalConstant: {
            ASR::LogicalConstant_t* const_logical = ASR::down_cast<ASR::LogicalConstant_t>(value_expr);
            value = (T) const_logical->m_value;
            break;
        }
        case ASR::exprType::RealConstant: {
            ASR::RealConstant_t* const_real = ASR::down_cast<ASR::RealConstant_t>(value_expr);
            value = (T) const_real->m_r;
            break;
        }
        default:
            return false;
    }
    return true;
}

static inline std::string type_python_1dim_helper(const std::string & res,
                                                  const ASR::dimension_t* e )
{
    if( !e->m_length && !e->m_start ) {
        return res + "[:]";
    }

    if( ASRUtils::expr_value(e->m_length) ) {
        int64_t length_dim = -1;
        ASRUtils::extract_value(ASRUtils::expr_value(e->m_length), length_dim);
        return res + "[" + std::to_string(length_dim + 1) + "]";
    }

    return res;
}

static inline void encode_dimensions(size_t n_dims, std::string& res,
    bool use_underscore_sep=false) {
    if( n_dims == 0 ) {
        return ;
    }

    if( use_underscore_sep ) {
        res += "_";
    } else {
        res += "[";
    }

    for( size_t i = 0; i < n_dims; i++ ) {
        if( use_underscore_sep ) {
            res += "_";
        } else {
            res += ":";
        }
        if( i == n_dims - 1 ) {
            if( use_underscore_sep ) {
                res += "_";
            } else {
                res += "]";
            }
        } else {
            if( use_underscore_sep ) {
                res += "_";
            } else {
                res += ", ";
            }
        }
    }
}

static inline std::string get_type_code(const ASR::ttype_t *t, bool use_underscore_sep=false,
    bool encode_dimensions_=true, bool set_dimensional_hint=true)
{
    bool is_dimensional = false;
    std::string res = "";
    switch (t->type) {
        case ASR::ttypeType::Integer: {
            ASR::Integer_t *integer = ASR::down_cast<ASR::Integer_t>(t);
            res = "i" + std::to_string(integer->m_kind * 8);
            if( encode_dimensions_ ) {
                encode_dimensions(integer->n_dims, res, use_underscore_sep);
                return res;
            }
            is_dimensional = integer->n_dims > 0;
            break;
        }
        case ASR::ttypeType::Real: {
            ASR::Real_t *real = ASR::down_cast<ASR::Real_t>(t);
            res = "r" + std::to_string(real->m_kind * 8);
            if( encode_dimensions_ ) {
                encode_dimensions(real->n_dims, res, use_underscore_sep);
                return res;
            }
            is_dimensional = real->n_dims > 0;
            break;
        }
        case ASR::ttypeType::Complex: {
            ASR::Complex_t *complx = ASR::down_cast<ASR::Complex_t>(t);
            res = "c" + std::to_string(complx->m_kind * 8);
            if( encode_dimensions_ ) {
                encode_dimensions(complx->n_dims, res, use_underscore_sep);
                return res;
            }
            is_dimensional = complx->n_dims > 0;
            break;
        }
        case ASR::ttypeType::Logical: {
            ASR::Logical_t* bool_ = ASR::down_cast<ASR::Logical_t>(t);
            std::string res = "bool";
            if( encode_dimensions_ ) {
                encode_dimensions(bool_->n_dims, res, use_underscore_sep);
                return res;
            }
            is_dimensional = bool_->n_dims > 0;
            break;
        }
        case ASR::ttypeType::Character: {
            return "str";
        }
        case ASR::ttypeType::Tuple: {
            ASR::Tuple_t *tup = ASR::down_cast<ASR::Tuple_t>(t);
            std::string result = "tuple";
            if( use_underscore_sep ) {
                result += "_";
            } else {
                result += "[";
            }
            for (size_t i = 0; i < tup->n_type; i++) {
                result += get_type_code(tup->m_type[i], use_underscore_sep,
                                        encode_dimensions_, set_dimensional_hint);
                if (i + 1 != tup->n_type) {
                    if( use_underscore_sep ) {
                        result += "_";
                    } else {
                        result += ", ";
                    }
                }
            }
            if( use_underscore_sep ) {
                result += "_";
            } else {
                result += "]";
            }
            return result;
        }
        case ASR::ttypeType::Set: {
            ASR::Set_t *s = ASR::down_cast<ASR::Set_t>(t);
            if( use_underscore_sep ) {
                return "set_" + get_type_code(s->m_type, use_underscore_sep,
                                              encode_dimensions_, set_dimensional_hint) + "_";
            }
            return "set[" + get_type_code(s->m_type, use_underscore_sep,
                                          encode_dimensions_, set_dimensional_hint) + "]";
        }
        case ASR::ttypeType::Dict: {
            ASR::Dict_t *d = ASR::down_cast<ASR::Dict_t>(t);
            if( use_underscore_sep ) {
                return "dict_" + get_type_code(d->m_key_type, use_underscore_sep,
                                               encode_dimensions_, set_dimensional_hint) +
                    "_" + get_type_code(d->m_value_type, use_underscore_sep,
                                        encode_dimensions_, set_dimensional_hint) + "_";
            }
            return "dict[" + get_type_code(d->m_key_type, use_underscore_sep,
                                           encode_dimensions_, set_dimensional_hint) +
                    ", " + get_type_code(d->m_value_type, use_underscore_sep,
                                         encode_dimensions_, set_dimensional_hint) + "]";
        }
        case ASR::ttypeType::List: {
            ASR::List_t *l = ASR::down_cast<ASR::List_t>(t);
            if( use_underscore_sep ) {
                return "list_" + get_type_code(l->m_type, use_underscore_sep,
                                               encode_dimensions_, set_dimensional_hint) + "_";
            }
            return "list[" + get_type_code(l->m_type, use_underscore_sep,
                                           encode_dimensions_, set_dimensional_hint) + "]";
        }
        case ASR::ttypeType::CPtr: {
            return "CPtr";
        }
        case ASR::ttypeType::Struct: {
            ASR::Struct_t* d = ASR::down_cast<ASR::Struct_t>(t);
            res = symbol_name(d->m_derived_type);
            if( encode_dimensions_ ) {
                encode_dimensions(d->n_dims, res, use_underscore_sep);
                return res;
            }
            is_dimensional = d->n_dims > 0;
            break;
        }
        case ASR::ttypeType::Union: {
            ASR::Union_t* d = ASR::down_cast<ASR::Union_t>(t);
            res = symbol_name(d->m_union_type);
            if( encode_dimensions_ ) {
                encode_dimensions(d->n_dims, res, use_underscore_sep);
                return res;
            }
            is_dimensional = d->n_dims > 0;
            break;
        }
        case ASR::ttypeType::Pointer: {
            ASR::Pointer_t* p = ASR::down_cast<ASR::Pointer_t>(t);
            if( use_underscore_sep ) {
                return "Pointer_" + get_type_code(p->m_type, use_underscore_sep, encode_dimensions_, set_dimensional_hint) + "_";
            }
            return "Pointer[" + get_type_code(p->m_type, use_underscore_sep, encode_dimensions_, set_dimensional_hint) + "]";
        }
        case ASR::ttypeType::Const: {
            ASR::Const_t* p = ASR::down_cast<ASR::Const_t>(t);
            if( use_underscore_sep ) {
                return "Const_" + get_type_code(p->m_type, use_underscore_sep, encode_dimensions_, set_dimensional_hint) + "_";
            }
            return "Const[" + get_type_code(p->m_type, use_underscore_sep, encode_dimensions_, set_dimensional_hint) + "]";
        }
        default: {
            throw LCompilersException("Type encoding not implemented for "
                                      + std::to_string(t->type));
        }
    }
    if( is_dimensional && set_dimensional_hint ) {
        res += "dim";
    }
    return res;
}

static inline std::string get_type_code(ASR::ttype_t** types, size_t n_types,
    bool use_underscore_sep=false, bool encode_dimensions=true) {
    std::string code = "";
    for( size_t i = 0; i < n_types; i++ ) {
        code += get_type_code(types[i], use_underscore_sep, encode_dimensions) + "_";
    }
    return code;
}

static inline std::string type_to_str_python(const ASR::ttype_t *t,
                                             bool for_error_message=true)
{
    switch (t->type) {
        case ASR::ttypeType::Integer: {
            ASR::Integer_t *i = (ASR::Integer_t*)t;
            std::string res = "";
            switch (i->m_kind) {
                case 1: { res = "i8"; break; }
                case 2: { res = "i16"; break; }
                case 4: { res = "i32"; break; }
                case 8: { res = "i64"; break; }
                default: { throw LCompilersException("Integer kind not supported"); }
            }
            if (i->n_dims == 1 && for_error_message) {
                res = type_python_1dim_helper(res, i->m_dims);
            }
            return res;
        }
        case ASR::ttypeType::Real: {
            ASR::Real_t *r = (ASR::Real_t*)t;
            std::string res = "";
            switch (r->m_kind) {
                case 4: { res = "f32"; break; }
                case 8: { res = "f64"; break; }
                default: { throw LCompilersException("Float kind not supported"); }
            }
            if (r->n_dims == 1 && for_error_message) {
                res = type_python_1dim_helper(res, r->m_dims);
            }
            return res;
        }
        case ASR::ttypeType::Complex: {
            ASR::Complex_t *c = (ASR::Complex_t*)t;
            switch (c->m_kind) {
                case 4: { return "c32"; }
                case 8: { return "c64"; }
                default: { throw LCompilersException("Complex kind not supported"); }
            }
        }
        case ASR::ttypeType::Logical: {
            return "bool";
        }
        case ASR::ttypeType::Character: {
            return "str";
        }
        case ASR::ttypeType::Tuple: {
            ASR::Tuple_t *tup = ASR::down_cast<ASR::Tuple_t>(t);
            std::string result = "tuple[";
            for (size_t i=0; i<tup->n_type; i++) {
                result += type_to_str_python(tup->m_type[i]);
                if (i+1 != tup->n_type) {
                    result += ", ";
                }
            }
            result += "]";
            return result;
        }
        case ASR::ttypeType::Set: {
            ASR::Set_t *s = (ASR::Set_t *)t;
            return "set[" + type_to_str_python(s->m_type) + "]";
        }
        case ASR::ttypeType::Dict: {
            ASR::Dict_t *d = (ASR::Dict_t *)t;
            return "dict[" + type_to_str_python(d->m_key_type) + ", " + type_to_str_python(d->m_value_type) + "]";
        }
        case ASR::ttypeType::List: {
            ASR::List_t *l = (ASR::List_t *)t;
            return "list[" + type_to_str_python(l->m_type) + "]";
        }
        case ASR::ttypeType::CPtr: {
            return "CPtr";
        }
        case ASR::ttypeType::Struct: {
            ASR::Struct_t* d = ASR::down_cast<ASR::Struct_t>(t);
            return symbol_name(d->m_derived_type);
        }
        case ASR::ttypeType::Enum: {
            ASR::Enum_t* d = ASR::down_cast<ASR::Enum_t>(t);
            return symbol_name(d->m_enum_type);
        }
        case ASR::ttypeType::Union: {
            ASR::Union_t* d = ASR::down_cast<ASR::Union_t>(t);
            return symbol_name(d->m_union_type);
        }
        case ASR::ttypeType::Pointer: {
            ASR::Pointer_t* p = ASR::down_cast<ASR::Pointer_t>(t);
            return "Pointer[" + type_to_str_python(p->m_type) + "]";
        }
        case ASR::ttypeType::Const: {
            ASR::Const_t* p = ASR::down_cast<ASR::Const_t>(t);
            return "Const[" + type_to_str_python(p->m_type) + "]";
        }
        case ASR::ttypeType::TypeParameter: {
            ASR::TypeParameter_t *p = ASR::down_cast<ASR::TypeParameter_t>(t);
            return p->m_param;
        }
        default : throw LCompilersException("Not implemented " + std::to_string(t->type));
    }
}

static inline std::string binop_to_str_python(const ASR::binopType t) {
    switch (t) {
        case (ASR::binopType::Add): { return " + "; }
        case (ASR::binopType::Sub): { return " - "; }
        case (ASR::binopType::Mul): { return "*"; }
        case (ASR::binopType::Div): { return "/"; }
        case (ASR::binopType::BitAnd): { return "&"; }
        case (ASR::binopType::BitOr): { return "|"; }
        case (ASR::binopType::BitXor): { return "^"; }
        case (ASR::binopType::BitLShift): { return "<<"; }
        case (ASR::binopType::BitRShift): { return ">>"; }
        default : throw LCompilersException("Cannot represent the binary operator as a string");
    }
}

static inline bool is_immutable(const ASR::ttype_t *type) {
    return ((ASR::is_a<ASR::Character_t>(*type) || ASR::is_a<ASR::Tuple_t>(*type)
        || ASR::is_a<ASR::Complex_t>(*type)));
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

static inline ASR::expr_t* get_constant_expression_with_given_type(Allocator& al, ASR::ttype_t* asr_type) {
    switch (asr_type->type) {
        case ASR::ttypeType::Integer: {
            return ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, asr_type->base.loc, -1, asr_type));
        }
        case ASR::ttypeType::Real: {
            return ASRUtils::EXPR(ASR::make_RealConstant_t(al, asr_type->base.loc, 0.0, asr_type));
        }
        case ASR::ttypeType::Complex: {
            return ASRUtils::EXPR(ASR::make_ComplexConstant_t(al, asr_type->base.loc, 0.0, 0.0, asr_type));
        }
        case ASR::ttypeType::Logical: {
            return ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, asr_type->base.loc, false, asr_type));
        }
        default: {
            throw LCompilersException("Not implemented " + std::to_string(asr_type->type));
        }
    }
    return nullptr;
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
    for (auto &a : unit.m_global_scope->get_scope()) {
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

std::vector<std::string> determine_function_definition_order(
        SymbolTable* symtab);

void extract_module_python(const ASR::TranslationUnit_t &m,
        std::vector<std::pair<std::string, ASR::Module_t*>>& children_modules,
        std::string module_name);

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

ASR::asr_t* getStructInstanceMember_t(Allocator& al, const Location& loc,
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
                               Allocator &al, const Location& loc,
                               const std::function<void (const std::string &, const Location &)> err);

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
        case ASR::ttypeType::Const: {
             return extract_kind_from_ttype_t(ASR::down_cast<ASR::Const_t>(type)->m_type);
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

static inline bool is_generic(ASR::ttype_t &x) {
    switch (x.type) {
        case ASR::ttypeType::List: {
            ASR::List_t *list_type = ASR::down_cast<ASR::List_t>(type_get_past_pointer(&x));
            return is_generic(*list_type->m_type);
        }
        default : return ASR::is_a<ASR::TypeParameter_t>(*type_get_past_pointer(&x));
    }
}


static inline int get_body_size(ASR::symbol_t* s) {
    int n_body = 0;
    switch (s->type) {
        case ASR::symbolType::Function: {
            ASR::Function_t* f = ASR::down_cast<ASR::Function_t>(s);
            n_body = f->n_body;
            break;
        }
        case ASR::symbolType::Program: {
            ASR::Program_t* p = ASR::down_cast<ASR::Program_t>(s);
            n_body = p->n_body;
            break;
        }
        default: {
            n_body = -1;
        }
    }
    return n_body;
}

inline int extract_dimensions_from_ttype(ASR::ttype_t *x,
                                         ASR::dimension_t*& m_dims) {
    int n_dims = 0;
    switch (x->type) {
        case ASR::ttypeType::Integer: {
            ASR::Integer_t* Integer_type = ASR::down_cast<ASR::Integer_t>(x);
            n_dims = Integer_type->n_dims;
            m_dims = Integer_type->m_dims;
            break;
        }
        case ASR::ttypeType::Real: {
            ASR::Real_t* Real_type = ASR::down_cast<ASR::Real_t>(x);
            n_dims = Real_type->n_dims;
            m_dims = Real_type->m_dims;
            break;
        }
        case ASR::ttypeType::Complex: {
            ASR::Complex_t* Complex_type = ASR::down_cast<ASR::Complex_t>(x);
            n_dims = Complex_type->n_dims;
            m_dims = Complex_type->m_dims;
            break;
        }
        case ASR::ttypeType::Character: {
            ASR::Character_t* Character_type = ASR::down_cast<ASR::Character_t>(x);
            n_dims = Character_type->n_dims;
            m_dims = Character_type->m_dims;
            break;
        }
        case ASR::ttypeType::Logical: {
            ASR::Logical_t* Logical_type = ASR::down_cast<ASR::Logical_t>(x);
            n_dims = Logical_type->n_dims;
            m_dims = Logical_type->m_dims;
            break;
        }
        case ASR::ttypeType::Struct: {
            ASR::Struct_t* Struct_type = ASR::down_cast<ASR::Struct_t>(x);
            n_dims = Struct_type->n_dims;
            m_dims = Struct_type->m_dims;
            break;
        }
        case ASR::ttypeType::Enum: {
            ASR::Enum_t* Enum_type = ASR::down_cast<ASR::Enum_t>(x);
            n_dims = Enum_type->n_dims;
            m_dims = Enum_type->m_dims;
            break;
        }
        case ASR::ttypeType::Union: {
            ASR::Union_t* Union_type = ASR::down_cast<ASR::Union_t>(x);
            n_dims = Union_type->n_dims;
            m_dims = Union_type->m_dims;
            break;
        }
        case ASR::ttypeType::Class: {
            ASR::Class_t* Class_type = ASR::down_cast<ASR::Class_t>(x);
            n_dims = Class_type->n_dims;
            m_dims = Class_type->m_dims;
            break;
        }
        case ASR::ttypeType::Pointer: {
            n_dims = extract_dimensions_from_ttype(ASR::down_cast<ASR::Pointer_t>(x)->m_type, m_dims);
            break;
        }
        case ASR::ttypeType::Const: {
            n_dims = extract_dimensions_from_ttype(ASR::down_cast<ASR::Const_t>(x)->m_type, m_dims);
            break;
        }
        case ASR::ttypeType::List: {
            n_dims = 0;
            m_dims = nullptr;
            break;
        }
        case ASR::ttypeType::Tuple: {
            n_dims = 0;
            m_dims = nullptr;
            break;
        }
        case ASR::ttypeType::Dict: {
            n_dims = 0;
            m_dims = nullptr;
            break;
        }
        case ASR::ttypeType::CPtr: {
            n_dims = 0;
            m_dims = nullptr;
            break;
        }
        case ASR::ttypeType::TypeParameter: {
            ASR::TypeParameter_t* tp = ASR::down_cast<ASR::TypeParameter_t>(x);
            n_dims = tp->n_dims;
            m_dims = tp->m_dims;
            break;
        }
        default:
            throw LCompilersException("Not implemented.");
    }
    return n_dims;
}

// Sets the dimension member of `ttype_t`. Returns `true` if dimensions set.
// Returns `false` if the `ttype_t` does not have a dimension member.
inline bool ttype_set_dimensions(ASR::ttype_t *x,
            ASR::dimension_t *m_dims, int64_t n_dims) {
    switch (x->type) {
        case ASR::ttypeType::Integer: {
            ASR::Integer_t* Integer_type = ASR::down_cast<ASR::Integer_t>(x);
            Integer_type->n_dims = n_dims;
            Integer_type->m_dims = m_dims;
            return true;
        }
        case ASR::ttypeType::Real: {
            ASR::Real_t* Real_type = ASR::down_cast<ASR::Real_t>(x);
            Real_type->n_dims = n_dims;
            Real_type->m_dims = m_dims;
            return true;
        }
        case ASR::ttypeType::Complex: {
            ASR::Complex_t* Complex_type = ASR::down_cast<ASR::Complex_t>(x);
            Complex_type->n_dims = n_dims;
            Complex_type->m_dims = m_dims;
            return true;
        }
        case ASR::ttypeType::Character: {
            ASR::Character_t* Character_type = ASR::down_cast<ASR::Character_t>(x);
            Character_type->n_dims = n_dims;
            Character_type->m_dims = m_dims;
            return true;
        }
        case ASR::ttypeType::Logical: {
            ASR::Logical_t* Logical_type = ASR::down_cast<ASR::Logical_t>(x);
            n_dims = Logical_type->n_dims;
            m_dims = Logical_type->m_dims;
            return true;
        }
        case ASR::ttypeType::Struct: {
            ASR::Struct_t* Struct_type = ASR::down_cast<ASR::Struct_t>(x);
            n_dims = Struct_type->n_dims;
            m_dims = Struct_type->m_dims;
            return true;
        }
        case ASR::ttypeType::Class: {
            ASR::Class_t* Class_type = ASR::down_cast<ASR::Class_t>(x);
            Class_type->n_dims = n_dims;
            Class_type->m_dims = m_dims;
            return true;
        }
        case ASR::ttypeType::TypeParameter: {
            ASR::TypeParameter_t* tp = ASR::down_cast<ASR::TypeParameter_t>(x);
            n_dims = tp->n_dims;
            m_dims = tp->m_dims;
            return true;
        }
        case ASR::ttypeType::Pointer: {
            return ttype_set_dimensions(
                ASR::down_cast<ASR::Pointer_t>(x)->m_type, m_dims, n_dims);
        }
        default:
            return false;
    }
    return false;
}

inline bool is_array(ASR::ttype_t *x) {
    ASR::dimension_t* dims = nullptr;
    return extract_dimensions_from_ttype(x, dims) > 0;
}

static inline ASR::ttype_t* duplicate_type(Allocator& al, const ASR::ttype_t* t,
                                           Vec<ASR::dimension_t>* dims = nullptr) {
    switch (t->type) {
        case ASR::ttypeType::Integer: {
            ASR::Integer_t* tnew = ASR::down_cast<ASR::Integer_t>(t);
            ASR::dimension_t* dimsp = dims ? dims->p : tnew->m_dims;
            size_t dimsn = dims ? dims->n : tnew->n_dims;
            return ASRUtils::TYPE(ASR::make_Integer_t(al, t->base.loc,
                        tnew->m_kind, dimsp, dimsn));
        }
        case ASR::ttypeType::Real: {
            ASR::Real_t* tnew = ASR::down_cast<ASR::Real_t>(t);
            ASR::dimension_t* dimsp = dims ? dims->p : tnew->m_dims;
            size_t dimsn = dims ? dims->n : tnew->n_dims;
            return ASRUtils::TYPE(ASR::make_Real_t(al, t->base.loc,
                        tnew->m_kind, dimsp, dimsn));
        }
        case ASR::ttypeType::Complex: {
            ASR::Complex_t* tnew = ASR::down_cast<ASR::Complex_t>(t);
            ASR::dimension_t* dimsp = dims ? dims->p : tnew->m_dims;
            size_t dimsn = dims ? dims->n : tnew->n_dims;
            return ASRUtils::TYPE(ASR::make_Complex_t(al, t->base.loc,
                        tnew->m_kind, dimsp, dimsn));
        }
        case ASR::ttypeType::Logical: {
            ASR::Logical_t* tnew = ASR::down_cast<ASR::Logical_t>(t);
            ASR::dimension_t* dimsp = dims ? dims->p : tnew->m_dims;
            size_t dimsn = dims ? dims->n : tnew->n_dims;
            return ASRUtils::TYPE(ASR::make_Logical_t(al, t->base.loc,
                        tnew->m_kind, dimsp, dimsn));
        }
        case ASR::ttypeType::Character: {
            ASR::Character_t* tnew = ASR::down_cast<ASR::Character_t>(t);
            ASR::dimension_t* dimsp = dims ? dims->p : tnew->m_dims;
            size_t dimsn = dims ? dims->n : tnew->n_dims;
            return ASRUtils::TYPE(ASR::make_Character_t(al, t->base.loc,
                        tnew->m_kind, tnew->m_len, tnew->m_len_expr,
                        dimsp, dimsn));
        }
        case ASR::ttypeType::Struct: {
            ASR::Struct_t* tnew = ASR::down_cast<ASR::Struct_t>(t);
            ASR::dimension_t* dimsp = dims ? dims->p : tnew->m_dims;
            size_t dimsn = dims ? dims->n : tnew->n_dims;
            return ASRUtils::TYPE(ASR::make_Struct_t(al, t->base.loc,
                        tnew->m_derived_type, dimsp, dimsn));
        }
        case ASR::ttypeType::Pointer: {
            ASR::Pointer_t* ptr = ASR::down_cast<ASR::Pointer_t>(t);
            ASR::ttype_t* dup_type = duplicate_type(al, ptr->m_type, dims);
            return ASRUtils::TYPE(ASR::make_Pointer_t(al, ptr->base.base.loc,
                        dup_type));
        }
        case ASR::ttypeType::Const: {
            ASR::Const_t* c = ASR::down_cast<ASR::Const_t>(t);
            ASR::ttype_t* dup_type = duplicate_type(al, c->m_type, dims);
            return ASRUtils::TYPE(ASR::make_Const_t(al, c->base.base.loc,
                        dup_type));
        }
        case ASR::ttypeType::TypeParameter: {
            ASR::TypeParameter_t* tp = ASR::down_cast<ASR::TypeParameter_t>(t);
            ASR::dimension_t* dimsp = dims ? dims->p : tp->m_dims;
            size_t dimsn = dims ? dims->n : tp->n_dims;
            //return ASRUtils::TYPE(ASR::make_TypeParameter_t(al, t->base.loc,
            //            tp->m_param, dimsp, dimsn, tp->m_rt, tp->n_rt));
            return ASRUtils::TYPE(ASR::make_TypeParameter_t(al, t->base.loc,
                        tp->m_param, dimsp, dimsn));
        }
        default : throw LCompilersException("Not implemented " + std::to_string(t->type));
    }
}

static inline ASR::ttype_t* duplicate_type_without_dims(Allocator& al, const ASR::ttype_t* t, const Location& loc) {
    switch (t->type) {
        case ASR::ttypeType::Integer: {
            ASR::Integer_t* tnew = ASR::down_cast<ASR::Integer_t>(t);
            return ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
                        tnew->m_kind, nullptr, 0));
        }
        case ASR::ttypeType::Real: {
            ASR::Real_t* tnew = ASR::down_cast<ASR::Real_t>(t);
            return ASRUtils::TYPE(ASR::make_Real_t(al, loc,
                        tnew->m_kind, nullptr, 0));
        }
        case ASR::ttypeType::Complex: {
            ASR::Complex_t* tnew = ASR::down_cast<ASR::Complex_t>(t);
            return ASRUtils::TYPE(ASR::make_Complex_t(al, loc,
                        tnew->m_kind, nullptr, 0));
        }
        case ASR::ttypeType::Logical: {
            ASR::Logical_t* tnew = ASR::down_cast<ASR::Logical_t>(t);
            return ASRUtils::TYPE(ASR::make_Logical_t(al, loc,
                        tnew->m_kind, nullptr, 0));
        }
        case ASR::ttypeType::Character: {
            ASR::Character_t* tnew = ASR::down_cast<ASR::Character_t>(t);
            return ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                        tnew->m_kind, tnew->m_len, tnew->m_len_expr,
                        nullptr, 0));
        }
        case ASR::ttypeType::Struct: {
            ASR::Struct_t* tstruct = ASR::down_cast<ASR::Struct_t>(t);
            return ASRUtils::TYPE(ASR::make_Struct_t(al, loc,
                    tstruct->m_derived_type, nullptr, 0));
        }
        case ASR::ttypeType::TypeParameter: {
            ASR::TypeParameter_t* tp = ASR::down_cast<ASR::TypeParameter_t>(t);
            //return ASRUtils::TYPE(ASR::make_TypeParameter_t(al, t->base.loc,
            //            tp->m_param, nullptr, 0, tp->m_rt, tp->n_rt));
            return ASRUtils::TYPE(ASR::make_TypeParameter_t(al, loc,
                        tp->m_param, nullptr, 0));
        }
        default : throw LCompilersException("Not implemented " + std::to_string(t->type));
    }
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
        case ASR::exprType::IntegerConstant: {
            a_kind = ASR::down_cast<ASR::IntegerConstant_t>
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
                    a_kind = ASR::down_cast<ASR::IntegerConstant_t>(kind_variable->m_value)->m_n;
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
        case ASR::exprType::IntegerConstant: {
            a_len = ASR::down_cast<ASR::IntegerConstant_t>
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
                    a_len = ASR::down_cast<ASR::IntegerConstant_t>(len_variable->m_value)->m_n;
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
        case ASR::exprType::IntegerBinOp: {
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
    ASR::ttype_t *x_underlying, *y_underlying;
    x_underlying = nullptr;
    y_underlying = nullptr;
    if( ASR::is_a<ASR::Enum_t>(*x) ) {
        ASR::Enum_t *x_enum = ASR::down_cast<ASR::Enum_t>(x);
        ASR::EnumType_t *x_enum_type = ASR::down_cast<ASR::EnumType_t>(x_enum->m_enum_type);
        x_underlying = x_enum_type->m_type;
    }
    if( ASR::is_a<ASR::Enum_t>(*y) ) {
        ASR::Enum_t *y_enum = ASR::down_cast<ASR::Enum_t>(y);
        ASR::EnumType_t *y_enum_type = ASR::down_cast<ASR::EnumType_t>(y_enum->m_enum_type);
        y_underlying = y_enum_type->m_type;
    }
    if( x_underlying || y_underlying ) {
        if( x_underlying ) {
            x = x_underlying;
        }
        if( y_underlying ) {
            y = y_underlying;
        }
        return check_equal_type(x, y);
    }
    if( ASR::is_a<ASR::Pointer_t>(*x) ||
        ASR::is_a<ASR::Pointer_t>(*y) ) {
        x = ASRUtils::type_get_past_pointer(x);
        y = ASRUtils::type_get_past_pointer(y);
        return check_equal_type(x, y);
    } else if(ASR::is_a<ASR::Const_t>(*x) ||
              ASR::is_a<ASR::Const_t>(*y)) {
        x = ASRUtils::get_contained_type(x);
        y = ASRUtils::get_contained_type(y);
        return check_equal_type(x, y);
    } else if (ASR::is_a<ASR::List_t>(*x) && ASR::is_a<ASR::List_t>(*y)) {
        x = ASR::down_cast<ASR::List_t>(x)->m_type;
        y = ASR::down_cast<ASR::List_t>(y)->m_type;
        return check_equal_type(x, y);
    } else if (ASR::is_a<ASR::Set_t>(*x) && ASR::is_a<ASR::Set_t>(*y)) {
        x = ASR::down_cast<ASR::Set_t>(x)->m_type;
        y = ASR::down_cast<ASR::Set_t>(y)->m_type;
        return check_equal_type(x, y);
    } else if (ASR::is_a<ASR::Dict_t>(*x) && ASR::is_a<ASR::Dict_t>(*y)) {
        ASR::ttype_t *x_key_type = ASR::down_cast<ASR::Dict_t>(x)->m_key_type;
        ASR::ttype_t *y_key_type = ASR::down_cast<ASR::Dict_t>(y)->m_key_type;
        ASR::ttype_t *x_value_type = ASR::down_cast<ASR::Dict_t>(x)->m_value_type;
        ASR::ttype_t *y_value_type = ASR::down_cast<ASR::Dict_t>(y)->m_value_type;
        return (check_equal_type(x_key_type, y_key_type) &&
                check_equal_type(x_value_type, y_value_type));
    } else if (ASR::is_a<ASR::Tuple_t>(*x) && ASR::is_a<ASR::Tuple_t>(*y)) {
        ASR::Tuple_t *a = ASR::down_cast<ASR::Tuple_t>(x);
        ASR::Tuple_t *b = ASR::down_cast<ASR::Tuple_t>(y);
        if(a->n_type != b->n_type) {
            return false;
        }
        bool result = true;
        for (size_t i=0; i<a->n_type; i++) {
            result = result && check_equal_type(a->m_type[i], b->m_type[i]);
            if (!result) {
                return false;
            }
        }
        return result;
    } else if (ASR::is_a<ASR::TypeParameter_t>(*x) && ASR::is_a<ASR::TypeParameter_t>(*y)) {
        ASR::TypeParameter_t* left_tp = ASR::down_cast<ASR::TypeParameter_t>(x);
        ASR::TypeParameter_t* right_tp = ASR::down_cast<ASR::TypeParameter_t>(y);
        std::string left_param = left_tp->m_param;
        std::string right_param = right_tp->m_param;
        return left_param.compare(right_param) == 0;
    }

    int64_t x_kind = ASRUtils::extract_kind_from_ttype_t(x);
    int64_t y_kind = ASRUtils::extract_kind_from_ttype_t(y);

    if( x->type == y->type &&
        x_kind == y_kind ) {
        return true;
    }

    return false;
}

int select_generic_procedure(const Vec<ASR::call_arg_t> &args,
        const ASR::GenericProcedure_t &p, Location loc,
        const std::function<void (const std::string &, const Location &)> err,
        bool raise_error=true);

ASR::asr_t* symbol_resolve_external_generic_procedure_without_eval(
            const Location &loc,
            ASR::symbol_t *v, Vec<ASR::call_arg_t>& args,
            SymbolTable* current_scope, Allocator& al,
            const std::function<void (const std::string &, const Location &)> err);

static inline bool is_dimension_empty(ASR::dimension_t& dim) {
    return ((dim.m_length == nullptr) ||
            (dim.m_start == nullptr));
}

static inline bool is_dimension_empty(ASR::dimension_t* dims, size_t n) {
    for( size_t i = 0; i < n; i++ ) {
        if( is_dimension_empty(dims[i]) ) {
            return true;
        }
    }
    return false;
}

static inline ASR::ttype_t* get_type_parameter(ASR::ttype_t* t) {
    switch (t->type) {
        case ASR::ttypeType::TypeParameter: {
            return t;
        }
        case ASR::ttypeType::List: {
            ASR::List_t *tl = ASR::down_cast<ASR::List_t>(t);
            return get_type_parameter(tl->m_type);
        }
        default: throw LCompilersException("Cannot get type parameter from this type.");
    }
}

class ReplaceArgVisitor: public ASR::BaseExprReplacer<ReplaceArgVisitor> {

    private:

    Allocator& al;

    SymbolTable* current_scope;

    ASR::Function_t* orig_func;

    Vec<ASR::call_arg_t>& orig_args;

    public:

    ReplaceArgVisitor(Allocator& al_, SymbolTable* current_scope_,
                      ASR::Function_t* orig_func_, Vec<ASR::call_arg_t>& orig_args_) :
        al(al_), current_scope(current_scope_), orig_func(orig_func_),
        orig_args(orig_args_)
    {}

    void replace_FunctionCall(ASR::FunctionCall_t* x) {
        ASR::symbol_t *new_es = x->m_name;
        // Import a function as external only if necessary
        ASR::Function_t *f = nullptr;
        ASR::symbol_t* f_sym = nullptr;
        if (ASR::is_a<ASR::Function_t>(*x->m_name)) {
            f = ASR::down_cast<ASR::Function_t>(x->m_name);
        } else if( ASR::is_a<ASR::ExternalSymbol_t>(*x->m_name) ) {
            f_sym = ASRUtils::symbol_get_past_external(x->m_name);
            if( ASR::is_a<ASR::Function_t>(*f_sym) ) {
                f = ASR::down_cast<ASR::Function_t>(f_sym);
            }
        }
        ASR::Module_t *m = ASR::down_cast2<ASR::Module_t>(f->m_symtab->parent->asr_owner);
        char *modname = m->m_name;
        ASR::symbol_t *maybe_f = current_scope->resolve_symbol(std::string(f->m_name));
        ASR::symbol_t* maybe_f_actual = nullptr;
        std::string maybe_modname = "";
        if( maybe_f && ASR::is_a<ASR::ExternalSymbol_t>(*maybe_f) ) {
            maybe_modname = ASR::down_cast<ASR::ExternalSymbol_t>(maybe_f)->m_module_name;
            maybe_f_actual = ASRUtils::symbol_get_past_external(maybe_f);
        }
        // If the Function to be imported is already present
        // then do not import.
        if( maybe_modname == std::string(modname) &&
            f_sym == maybe_f_actual ) {
            new_es = maybe_f;
        } else {
            // Import while assigning a new name to avoid conflicts
            // For example, if someone is using `len` from a user
            // define module then `get_unique_name` will avoid conflict
            std::string unique_name = current_scope->get_unique_name(f->m_name);
            Str s; s.from_str_view(unique_name);
            char *unique_name_c = s.c_str(al);
            LFORTRAN_ASSERT(current_scope->get_symbol(unique_name) == nullptr);
            new_es = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
                al, f->base.base.loc,
                /* a_symtab */ current_scope,
                /* a_name */ unique_name_c,
                (ASR::symbol_t*)f,
                modname, nullptr, 0,
                f->m_name,
                ASR::accessType::Private
                ));
            current_scope->add_symbol(unique_name, new_es);
        }
        // The following substitutes args from the current scope
        for (size_t i = 0; i < x->n_args; i++) {
            current_expr_copy = current_expr;
            current_expr = &(x->m_args[i].m_value);
            replace_expr(x->m_args[i].m_value);
            current_expr = current_expr_copy;
        }
        x->m_name = new_es;
    }

    void replace_Var(ASR::Var_t* x) {
        size_t arg_idx = 0;
        bool idx_found = false;
        std::string arg_name = ASRUtils::symbol_name(x->m_v);
        // Finds the index of the argument to be used for substitution
        // Basically if we are calling maybe(string, ret_type=character(len=len(s)))
        // where string is a variable in current scope and s is one of the arguments
        // accepted by maybe i.e., maybe has a signature maybe(s). Then, we will
        // replace s with string. So, the call would become,
        // maybe(string, ret_type=character(len=len(string)))
        for( size_t j = 0; j < orig_func->n_args && !idx_found; j++ ) {
            if( ASR::is_a<ASR::Var_t>(*(orig_func->m_args[j])) ) {
                std::string arg_name_2 = std::string(ASRUtils::symbol_name(
                    ASR::down_cast<ASR::Var_t>(orig_func->m_args[j])->m_v));
                arg_idx = j;
                idx_found = arg_name_2 == arg_name;
            }
        }
        if( idx_found ) {
            LFORTRAN_ASSERT(current_expr);
            *current_expr = orig_args[arg_idx].m_value;
        }
    }

};

class ExprStmtDuplicator: public ASR::BaseExprStmtDuplicator<ExprStmtDuplicator>
{
    public:

    ExprStmtDuplicator(Allocator &al): BaseExprStmtDuplicator(al) {}

};

class ReplaceReturnWithGotoVisitor: public ASR::BaseStmtReplacer<ReplaceReturnWithGotoVisitor> {

    private:

    Allocator& al;

    uint64_t goto_label;

    public:

    ReplaceReturnWithGotoVisitor(Allocator& al_, uint64_t goto_label_) :
        al(al_), goto_label(goto_label_)
    {}

    void set_goto_label(uint64_t label) {
        goto_label = label;
    }

    void replace_Return(ASR::Return_t* x) {
        *current_stmt = ASRUtils::STMT(ASR::make_GoTo_t(al, x->base.base.loc, goto_label,
                            s2c(al, "__" + std::to_string(goto_label))));
        has_replacement_happened = true;
    }

};

// Singleton LabelGenerator so that it generates
// unique labels for different statements, from
// whereever it is called (be it ASR passes, be it
// AST to ASR transition, etc).
class LabelGenerator {
    private:

        static LabelGenerator *label_generator;
        uint64_t unique_label;
        std::map<ASR::asr_t*, uint64_t> node2label;

        // Private constructor so that more than
        // one object cannot be created by calling the
        // constructor.
        LabelGenerator() {
            unique_label = 0;
        }

    public:

        static LabelGenerator *get_instance() {
            if (!label_generator) {
                label_generator = new LabelGenerator;
            }
            return label_generator;
        }

        int get_unique_label() {
            unique_label += 1;
            return unique_label;
        }

        void add_node_with_unique_label(ASR::asr_t* node, uint64_t label) {
            LFORTRAN_ASSERT( node2label.find(node) == node2label.end() );
            node2label[node] = label;
        }

        bool verify(ASR::asr_t* node) {
            return node2label.find(node) != node2label.end();
        }
};

ASR::asr_t* make_Cast_t_value(Allocator &al, const Location &a_loc,
        ASR::expr_t* a_arg, ASR::cast_kindType a_kind, ASR::ttype_t* a_type);

static inline ASR::expr_t* compute_length_from_start_end(Allocator& al, ASR::expr_t* start, ASR::expr_t* end) {
    ASR::expr_t* start_value = ASRUtils::expr_value(start);
    ASR::expr_t* end_value = ASRUtils::expr_value(end);

    // If both start and end have compile time values
    // then length can be computed easily by extracting
    // compile time values of end and start.
    if( start_value && end_value ) {
        int64_t start_int = -1, end_int = -1;
        ASRUtils::extract_value(start_value, start_int);
        ASRUtils::extract_value(end_value, end_int);
        return ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, start->base.loc,
                              end_int - start_int + 1,
                              ASRUtils::expr_type(start)));
    }

    // If start has a compile time value and
    // end is a variable then length can be
    // simplified by computing 1 - start as a constant
    // and then analysing the end expression.
    if( start_value && !end_value ) {
        int64_t start_int = -1;
        ASRUtils::extract_value(start_value, start_int);
        int64_t remaining_portion = 1 - start_int;

        // If 1 - start is 0 then length is clearly the
        // end expression.
        if( remaining_portion == 0 ) {
            return end;
        }

        // If end is a binary expression of Add, Sub
        // type.
        if( ASR::is_a<ASR::IntegerBinOp_t>(*end) ) {
            ASR::IntegerBinOp_t* end_binop = ASR::down_cast<ASR::IntegerBinOp_t>(end);
            if( end_binop->m_op == ASR::binopType::Add ||
                end_binop->m_op == ASR::binopType::Sub) {
                ASR::expr_t* end_left = end_binop->m_left;
                ASR::expr_t* end_right = end_binop->m_right;
                ASR::expr_t* end_leftv = ASRUtils::expr_value(end_left);
                ASR::expr_t* end_rightv = ASRUtils::expr_value(end_right);
                if( end_leftv ) {
                    // If left part of end is a compile time constant
                    // then it can be merged with 1 - start.
                    int64_t el_int = -1;
                    ASRUtils::extract_value(end_leftv, el_int);
                    remaining_portion += el_int;

                    // If 1 - start + end_left is 0
                    // and end is an addition operation
                    // then clearly end_right is the length.
                    if( remaining_portion == 0 &&
                        end_binop->m_op == ASR::binopType::Add ) {
                        return end_right;
                    }

                    // In all other cases the length would be (1 - start + end_left) endop end_right
                    // endop is the operation of end expression and 1 - start + end_left is a constant.
                    ASR::expr_t* remaining_expr = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al,
                                                    end->base.loc, remaining_portion,
                                                    ASRUtils::expr_type(end)));
                    return ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, end->base.loc, remaining_expr,
                                end_binop->m_op, end_right, end_binop->m_type, end_binop->m_value));
                } else if( end_rightv ) {
                    // If right part of end is a compile time constant
                    // then it can be merged with 1 - start. The sign
                    // of end_right depends on the operation in
                    // end expression.
                    int64_t er_int = -1;
                    ASRUtils::extract_value(end_rightv, er_int);
                    if( end_binop->m_op == ASR::binopType::Sub ) {
                        er_int = -er_int;
                    }
                    remaining_portion += er_int;

                    // If (1 - start endop end_right) is 0
                    // then clearly end_left is the length expression.
                    if( remaining_portion == 0 ) {
                        return end_left;
                    }

                    // Otherwise, length is end_left Add (1 - start endop end_right)
                    // where endop is the operation in end expression and
                    // (1 - start endop end_right) is a compile time constant.
                    ASR::expr_t* remaining_expr = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al,
                                                    end->base.loc, remaining_portion,
                                                    ASRUtils::expr_type(end)));
                    return ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, end->base.loc, end_left,
                                ASR::binopType::Add, remaining_expr, end_binop->m_type, end_binop->m_value));
                }
            }
        }

        // If start is a variable and end is a compile time constant
        // then compute (end + 1) as a constant and then return
        // (end + 1) - start as the length expression.
        if( !start_value && end_value ) {
            int64_t end_int = -1;
            ASRUtils::extract_value(end_value, end_int);
            int64_t remaining_portion = end_int + 1;
            ASR::expr_t* remaining_expr = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al,
                                                end->base.loc, remaining_portion,
                                                ASRUtils::expr_type(end)));
            return ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, end->base.loc, remaining_expr,
                        ASR::binopType::Sub, start, ASRUtils::expr_type(end), nullptr));
        }

        // For all the other cases
        ASR::expr_t* remaining_expr = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al,
                                                end->base.loc, remaining_portion,
                                                ASRUtils::expr_type(end)));
        return ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, end->base.loc, end,
                    ASR::binopType::Add, remaining_expr, ASRUtils::expr_type(end),
                    nullptr));
    }

    ASR::expr_t* diff = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, end->base.loc, end,
                                       ASR::binopType::Sub, start, ASRUtils::expr_type(end),
                                       nullptr));
    ASR::expr_t *constant_one = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                                            al, diff->base.loc, 1, ASRUtils::expr_type(diff)));
    return ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, end->base.loc, diff,
                          ASR::binopType::Add, constant_one, ASRUtils::expr_type(end),
                          nullptr));
}

static inline bool is_pass_array_by_data_possible(ASR::Function_t* x, std::vector<size_t>& v) {
    ASR::ttype_t* typei = nullptr;
    ASR::dimension_t* dims = nullptr;
    for( size_t i = 0; i < x->n_args; i++ ) {
        if( !ASR::is_a<ASR::Var_t>(*x->m_args[i]) ) {
            continue;
        }
        ASR::Var_t* arg_Var = ASR::down_cast<ASR::Var_t>(x->m_args[i]);
        if( !ASR::is_a<ASR::Variable_t>(*arg_Var->m_v) ) {
            continue;
        }
        typei = ASRUtils::expr_type(x->m_args[i]);
        int n_dims = ASRUtils::extract_dimensions_from_ttype(typei, dims);
        ASR::Variable_t* argi = ASRUtils::EXPR2VAR(x->m_args[i]);
        if( ASRUtils::is_dimension_empty(dims, n_dims) &&
            (argi->m_intent == ASRUtils::intent_in ||
             argi->m_intent == ASRUtils::intent_out) &&
            argi->m_storage != ASR::storage_typeType::Allocatable) {
            v.push_back(i);
        }
    }
    return v.size() > 0;
}

static inline ASR::expr_t* get_bound(ASR::expr_t* arr_expr, int dim,
                                     std::string bound, Allocator& al) {
    ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, arr_expr->base.loc,
                                                                  4, nullptr, 0));
    ASR::expr_t* dim_expr = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arr_expr->base.loc,
                                                                       dim, int32_type));
    ASR::arrayboundType bound_type = ASR::arrayboundType::LBound;
    if( bound == "ubound" ) {
        bound_type = ASR::arrayboundType::UBound;
    }
    return ASRUtils::EXPR(ASR::make_ArrayBound_t(al, arr_expr->base.loc, arr_expr, dim_expr,
                int32_type, bound_type, nullptr));
}

static inline ASR::expr_t* get_size(ASR::expr_t* arr_expr, int dim,
                                    Allocator& al) {
    ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, arr_expr->base.loc, 4, nullptr, 0));
    ASR::expr_t* dim_expr = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arr_expr->base.loc, dim, int32_type));
    return ASRUtils::EXPR(ASR::make_ArraySize_t(al, arr_expr->base.loc, arr_expr, dim_expr,
                                                int32_type, nullptr));
}

static inline void get_dimensions(ASR::expr_t* array, Vec<ASR::expr_t*>& dims,
                                  Allocator& al) {
    ASR::ttype_t* array_type = ASRUtils::expr_type(array);
    ASR::dimension_t* compile_time_dims = nullptr;
    int n_dims = extract_dimensions_from_ttype(array_type, compile_time_dims);
    for( int i = 0; i < n_dims; i++ ) {
        ASR::expr_t* start = compile_time_dims[i].m_start;
        if( start == nullptr ) {
            start = get_bound(array, i + 1, "lbound", al);
        }
        ASR::expr_t* length = compile_time_dims[i].m_length;
        if( length == nullptr ) {
            length = get_size(array, i + 1, al);
        }
        dims.push_back(al, start);
        dims.push_back(al, length);
    }
}

static inline ASR::EnumType_t* get_EnumType_from_symbol(ASR::symbol_t* s) {
    ASR::Variable_t* s_var = ASR::down_cast<ASR::Variable_t>(s);
    if( ASR::is_a<ASR::Enum_t>(*s_var->m_type) ) {
        ASR::Enum_t* enum_ = ASR::down_cast<ASR::Enum_t>(s_var->m_type);
        return ASR::down_cast<ASR::EnumType_t>(enum_->m_enum_type);
    }
    ASR::symbol_t* enum_type_cand = ASR::down_cast<ASR::symbol_t>(s_var->m_parent_symtab->asr_owner);
    LFORTRAN_ASSERT(ASR::is_a<ASR::EnumType_t>(*enum_type_cand));
    return ASR::down_cast<ASR::EnumType_t>(enum_type_cand);
}

} // namespace ASRUtils

} // namespace LFortran

#endif // LFORTRAN_ASR_UTILS_H
