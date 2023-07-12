#ifndef LFORTRAN_ASR_UTILS_H
#define LFORTRAN_ASR_UTILS_H

#include <functional>
#include <map>
#include <set>
#include <limits>

#include <libasr/assert.h>
#include <libasr/asr.h>
#include <libasr/string_utils.h>
#include <libasr/utils.h>

#include <complex>

namespace LCompilers  {

    namespace ASRUtils  {

ASR::symbol_t* import_class_procedure(Allocator &al, const Location& loc,
        ASR::symbol_t* original_sym, SymbolTable *current_scope);

static inline  double extract_real(const char *s) {
    // TODO: this is inefficient. We should
    // convert this in the tokenizer where we know most information
    std::string x = s;
    x = replace(x, "d", "e");
    x = replace(x, "D", "E");
    return std::atof(x.c_str());
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

static inline ASR::FunctionType_t* get_FunctionType(const ASR::Function_t* x) {
    return ASR::down_cast<ASR::FunctionType_t>(x->m_function_signature);
}

static inline ASR::FunctionType_t* get_FunctionType(const ASR::Function_t& x) {
    return ASR::down_cast<ASR::FunctionType_t>(x.m_function_signature);
}

static inline ASR::symbol_t *symbol_get_past_external(ASR::symbol_t *f)
{
    if (f && f->type == ASR::symbolType::ExternalSymbol) {
        ASR::ExternalSymbol_t *e = ASR::down_cast<ASR::ExternalSymbol_t>(f);
        if( e->m_external == nullptr ) {
            return nullptr;
        }
        LCOMPILERS_ASSERT(!ASR::is_a<ASR::ExternalSymbol_t>(*e->m_external));
        return e->m_external;
    } else {
        return f;
    }
}

static inline const ASR::symbol_t *symbol_get_past_external(const ASR::symbol_t *f)
{
    if (f->type == ASR::symbolType::ExternalSymbol) {
        ASR::ExternalSymbol_t *e = ASR::down_cast<ASR::ExternalSymbol_t>(f);
        LCOMPILERS_ASSERT(!ASR::is_a<ASR::ExternalSymbol_t>(*e->m_external));
        return e->m_external;
    } else {
        return f;
    }
}

static inline ASR::ttype_t *type_get_past_const(ASR::ttype_t *f)
{
    if (ASR::is_a<ASR::Const_t>(*f)) {
        ASR::Const_t *e = ASR::down_cast<ASR::Const_t>(f);
        LCOMPILERS_ASSERT(!ASR::is_a<ASR::Const_t>(*e->m_type));
        return e->m_type;
    } else {
        return f;
    }
}

static inline ASR::ttype_t *type_get_past_pointer(ASR::ttype_t *f)
{
    if (ASR::is_a<ASR::Pointer_t>(*f)) {
        ASR::Pointer_t *e = ASR::down_cast<ASR::Pointer_t>(f);
        LCOMPILERS_ASSERT(!ASR::is_a<ASR::Pointer_t>(*e->m_type));
        return e->m_type;
    } else {
        return f;
    }
}

static inline ASR::ttype_t *type_get_past_allocatable(ASR::ttype_t *f)
{
    if (ASR::is_a<ASR::Allocatable_t>(*f)) {
        ASR::Allocatable_t *e = ASR::down_cast<ASR::Allocatable_t>(f);
        LCOMPILERS_ASSERT(!ASR::is_a<ASR::Allocatable_t>(*e->m_type));
        return e->m_type;
    } else {
        return f;
    }
}

static inline ASR::ttype_t *type_get_past_array(ASR::ttype_t *f)
{
    if (ASR::is_a<ASR::Array_t>(*f)) {
        ASR::Array_t *e = ASR::down_cast<ASR::Array_t>(f);
        LCOMPILERS_ASSERT(!ASR::is_a<ASR::Array_t>(*e->m_type));
        return e->m_type;
    } else {
        return f;
    }
}

static inline int extract_kind_from_ttype_t(const ASR::ttype_t* type) {
    if (type == nullptr) {
        return -1;
    }
    switch (type->type) {
        case ASR::ttypeType::Array: {
            return extract_kind_from_ttype_t(ASR::down_cast<ASR::Array_t>(type)->m_type);
        }
        case ASR::ttypeType::Integer : {
            return ASR::down_cast<ASR::Integer_t>(type)->m_kind;
        }
        case ASR::ttypeType::UnsignedInteger : {
            return ASR::down_cast<ASR::UnsignedInteger_t>(type)->m_kind;
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
        case ASR::ttypeType::Allocatable: {
            return extract_kind_from_ttype_t(ASR::down_cast<ASR::Allocatable_t>(type)->m_type);
        }
        case ASR::ttypeType::Const: {
            return extract_kind_from_ttype_t(ASR::down_cast<ASR::Const_t>(type)->m_type);
        }
        default : {
            return -1;
        }
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

static inline ASR::ttype_t* subs_expr_type(std::map<std::string, ASR::ttype_t*> subs,
        const ASR::expr_t *expr) {
    ASR::ttype_t *ttype = ASRUtils::expr_type(expr);
    if (ASR::is_a<ASR::TypeParameter_t>(*ttype)) {
        ASR::TypeParameter_t *tparam = ASR::down_cast<ASR::TypeParameter_t>(ttype);
        ttype = subs[tparam->m_param];
    }
    return ttype;
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
        case ASR::symbolType::ExternalSymbol: {
            return symbol_type(ASRUtils::symbol_get_past_external(f));
        }
        case ASR::symbolType::Function: {
            return ASRUtils::expr_type(
                ASR::down_cast<ASR::Function_t>(f)->m_return_var);
        }
        default: {
            throw LCompilersException("Cannot return type of, " +
                                    std::to_string(f->type) + " symbol.");
        }
    }
    return nullptr;
}

static inline ASR::abiType symbol_abi(const ASR::symbol_t *f)
{
    switch( f->type ) {
        case ASR::symbolType::Variable: {
            return ASR::down_cast<ASR::Variable_t>(f)->m_abi;
        }
        case ASR::symbolType::EnumType: {
            return ASR::down_cast<ASR::EnumType_t>(f)->m_abi;
        }
        case ASR::symbolType::ExternalSymbol: {
            return symbol_abi(ASR::down_cast<ASR::ExternalSymbol_t>(f)->m_external);
        }
        default: {
            throw LCompilersException("Cannot return ABI of, " +
                                    std::to_string(f->type) + " symbol.");
        }
    }
    return ASR::abiType::Source;
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
        case ASR::ttypeType::Allocatable: {
            ASR::Allocatable_t* pointer_asr = ASR::down_cast<ASR::Allocatable_t>(asr_type);
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

static inline ASR::array_physical_typeType extract_physical_type(ASR::ttype_t* e) {
    switch( e->type ) {
        case ASR::ttypeType::Array: {
            return ASR::down_cast<ASR::Array_t>(e)->m_physical_type;
        }
        case ASR::ttypeType::Pointer: {
            return extract_physical_type(ASRUtils::type_get_past_pointer(e));
        }
        case ASR::ttypeType::Allocatable: {
            return extract_physical_type(ASRUtils::type_get_past_allocatable(e));
        }
        default:
            throw LCompilersException("Cannot extract the physical type of " +
                    std::to_string(e->type) + " type.");
    }
}

static inline ASR::abiType expr_abi(ASR::expr_t* e) {
    switch( e->type ) {
        case ASR::exprType::Var: {
            return ASRUtils::symbol_abi(ASR::down_cast<ASR::Var_t>(e)->m_v);
        }
        case ASR::exprType::StructInstanceMember: {
            return ASRUtils::symbol_abi(ASR::down_cast<ASR::StructInstanceMember_t>(e)->m_m);
        }
        case ASR::exprType::ArrayReshape: {
            return ASRUtils::expr_abi(ASR::down_cast<ASR::ArrayReshape_t>(e)->m_array);
        }
        case ASR::exprType::GetPointer: {
            return ASRUtils::expr_abi(ASR::down_cast<ASR::GetPointer_t>(e)->m_arg);
        }
        default:
            throw LCompilersException("Cannot extract the ABI of " +
                    std::to_string(e->type) + " expression.");
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
        case ASR::symbolType::Requirement: {
            return ASR::down_cast<ASR::Requirement_t>(f)->m_name;
        }
        case ASR::symbolType::Template: {
            return ASR::down_cast<ASR::Template_t>(f)->m_name;
        }
        default : throw LCompilersException("Not implemented");
    }
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
            return ASRUtils::symbol_name(ASR::down_cast<ASR::Struct_t>(t)->m_derived_type);
        }
        case ASR::ttypeType::Class: {
            return ASRUtils::symbol_name(ASR::down_cast<ASR::Class_t>(t)->m_class_type);
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
        case ASR::ttypeType::Allocatable: {
            return type_to_str(ASRUtils::type_get_past_allocatable(
                        const_cast<ASR::ttype_t*>(t))) + " allocatable";
        }
        case ASR::ttypeType::Array: {
            ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(t);
            std::string res = type_to_str(array_t->m_type);
            encode_dimensions(array_t->n_dims, res, false);
            return res;
        }
        case ASR::ttypeType::Const: {
            return type_to_str(ASRUtils::get_contained_type(
                        const_cast<ASR::ttype_t*>(t))) + " const";
        }
        case ASR::ttypeType::TypeParameter: {
            ASR::TypeParameter_t* tp = ASR::down_cast<ASR::TypeParameter_t>(t);
            return tp->m_param;
        }
        case ASR::ttypeType::SymbolicExpression: {
            return "symbolic expression";
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
        case ASR::symbolType::Requirement: {
            return ASR::down_cast<ASR::Requirement_t>(f)->m_symtab->parent;
        }
        case ASR::symbolType::Template: {
            return ASR::down_cast<ASR::Template_t>(f)->m_symtab->parent;
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
        case ASR::symbolType::Requirement: {
            return ASR::down_cast<ASR::Requirement_t>(f)->m_symtab;
        }
        case ASR::symbolType::Template: {
            return ASR::down_cast<ASR::Template_t>(f)->m_symtab;
        }
        default : throw LCompilersException("Not implemented");
    }
}

static inline ASR::symbol_t *get_asr_owner(const ASR::symbol_t *sym) {
    const SymbolTable *s = symbol_parent_symtab(sym);
    if( !ASR::is_a<ASR::symbol_t>(*s->asr_owner) ) {
        return nullptr;
    }
    return ASR::down_cast<ASR::symbol_t>(s->asr_owner);
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

static inline ASR::symbol_t *get_asr_owner(const ASR::expr_t *expr) {
    switch( expr->type ) {
        case ASR::exprType::Var: {
            return ASRUtils::get_asr_owner(ASR::down_cast<ASR::Var_t>(expr)->m_v);
        }
        case ASR::exprType::StructInstanceMember: {
            return ASRUtils::get_asr_owner(ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::StructInstanceMember_t>(expr)->m_m));
        }
        case ASR::exprType::GetPointer: {
            return ASRUtils::get_asr_owner(ASR::down_cast<ASR::GetPointer_t>(expr)->m_arg);
        }
        case ASR::exprType::FunctionCall: {
            return ASRUtils::get_asr_owner(ASR::down_cast<ASR::FunctionCall_t>(expr)->m_name);
        }
        default: {
            throw LCompilersException("Cannot find the ASR owner of underlying symbol of expression "
                                        + std::to_string(expr->type));
        }
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

static inline bool is_c_ptr(ASR::symbol_t* v, std::string v_name="") {
    if( v_name == "" ) {
        v_name = ASRUtils::symbol_name(v);
    }
    ASR::symbol_t* v_orig = ASRUtils::symbol_get_past_external(v);
    if( ASR::is_a<ASR::StructType_t>(*v_orig) ) {
        ASR::Module_t* der_type_module = ASRUtils::get_sym_module0(v_orig);
        return (der_type_module && std::string(der_type_module->m_name) ==
                "lfortran_intrinsic_iso_c_binding" &&
                der_type_module->m_intrinsic &&
                v_name == "c_ptr");
    }
    return false;
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
            ASRUtils::get_FunctionType(fn)->m_abi ==
            ASR::abiType::Intrinsic) {
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
    } else if (ASR::is_a<ASR::UnsignedIntegerConstant_t>(*a_value)) {
        // OK
    } else if (ASR::is_a<ASR::RealConstant_t>(*a_value)) {
        // OK
    } else if (ASR::is_a<ASR::ComplexConstant_t>(*a_value)) {
        // OK
    } else if (ASR::is_a<ASR::LogicalConstant_t>(*a_value)) {
        // OK
    } else if (ASR::is_a<ASR::StringConstant_t>(*a_value)) {
        // OK
    } else if(ASR::is_a<ASR::ArrayConstant_t>(*a_value)) {
        ASR::ArrayConstant_t* array_constant = ASR::down_cast<ASR::ArrayConstant_t>(a_value);
        for( size_t i = 0; i < array_constant->n_args; i++ ) {
            if( !ASRUtils::is_value_constant(array_constant->m_args[i]) &&
                !ASRUtils::is_value_constant(ASRUtils::expr_value(array_constant->m_args[i])) ) {
                return false;
            }
        }
        return true;
    } else if(ASR::is_a<ASR::FunctionCall_t>(*a_value)) {
        ASR::FunctionCall_t* func_call_t = ASR::down_cast<ASR::FunctionCall_t>(a_value);
        if( !ASRUtils::is_intrinsic_symbol(ASRUtils::symbol_get_past_external(func_call_t->m_name)) ) {
            return false;
        }
        for( size_t i = 0; i < func_call_t->n_args; i++ ) {
            if( !ASRUtils::is_value_constant(func_call_t->m_args[i].m_value) ) {
                return false;
            }
        }
        return true;
    } else if( ASR::is_a<ASR::StructInstanceMember_t>(*a_value) ) {
        ASR::StructInstanceMember_t* struct_member_t = ASR::down_cast<ASR::StructInstanceMember_t>(a_value);
        return is_value_constant(struct_member_t->m_v);
    } else if( ASR::is_a<ASR::Var_t>(*a_value) ) {
        ASR::Var_t* var_t = ASR::down_cast<ASR::Var_t>(a_value);
        LCOMPILERS_ASSERT(ASR::is_a<ASR::Variable_t>(*ASRUtils::symbol_get_past_external(var_t->m_v)));
        ASR::Variable_t* variable_t = ASR::down_cast<ASR::Variable_t>(
            ASRUtils::symbol_get_past_external(var_t->m_v));
        return variable_t->m_storage == ASR::storage_typeType::Parameter;

    } else if(ASR::is_a<ASR::ImpliedDoLoop_t>(*a_value)) {
        // OK
    } else if(ASR::is_a<ASR::Cast_t>(*a_value)) {
        ASR::Cast_t* cast_t = ASR::down_cast<ASR::Cast_t>(a_value);
        return is_value_constant(cast_t->m_arg);
    } else if(ASR::is_a<ASR::PointerNullConstant_t>(*a_value)) {
        // OK
    } else if(ASR::is_a<ASR::ArrayReshape_t>(*a_value)) {
        ASR::ArrayReshape_t* array_reshape = ASR::down_cast<ASR::ArrayReshape_t>(a_value);
        return is_value_constant(array_reshape->m_array) && is_value_constant(array_reshape->m_shape);
    } else if(ASR::is_a<ASR::ArrayPhysicalCast_t>(*a_value)) {
        ASR::ArrayPhysicalCast_t* array_physical_t = ASR::down_cast<ASR::ArrayPhysicalCast_t>(a_value);
        return is_value_constant(array_physical_t->m_arg);
    } else if( ASR::is_a<ASR::StructTypeConstructor_t>(*a_value) ) {
        ASR::StructTypeConstructor_t* struct_type_constructor =
            ASR::down_cast<ASR::StructTypeConstructor_t>(a_value);
        bool is_constant = true;
        for( size_t i = 0; i < struct_type_constructor->n_args; i++ ) {
            if( struct_type_constructor->m_args[i].m_value ) {
                is_constant = is_constant &&
                              (is_value_constant(
                                struct_type_constructor->m_args[i].m_value) ||
                              is_value_constant(
                                ASRUtils::expr_value(
                                    struct_type_constructor->m_args[i].m_value)));
            }
        }
        return is_constant;
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

static inline std::string get_mangled_name(ASR::Module_t* module, std::string symbol_name) {
    std::string module_name = module->m_name;
    if( module_name == symbol_name ) {
        return "__" + std::string(module->m_name) + "_" + symbol_name;
    } else {
        return symbol_name;
    }
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

static inline bool extract_value(ASR::expr_t* value_expr,
    std::complex<double>& value) {
    if( !ASR::is_a<ASR::ComplexConstant_t>(*value_expr) ) {
        return false;
    }

    ASR::ComplexConstant_t* value_const = ASR::down_cast<ASR::ComplexConstant_t>(value_expr);
    value = std::complex(value_const->m_re, value_const->m_im);
    return true;
}

template <typename T,
    typename = typename std::enable_if<
        std::is_same<T, std::complex<double>>::value == false &&
        std::is_same<T, std::complex<float>>::value == false>::type>
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
        case ASR::exprType::UnsignedIntegerConstant: {
            ASR::UnsignedIntegerConstant_t* const_int = ASR::down_cast<ASR::UnsignedIntegerConstant_t>(value_expr);
            value = (T) const_int->m_n;
            break;
        }
        case ASR::exprType::RealConstant: {
            ASR::RealConstant_t* const_real = ASR::down_cast<ASR::RealConstant_t>(value_expr);
            value = (T) const_real->m_r;
            break;
        }
        case ASR::exprType::LogicalConstant: {
            ASR::LogicalConstant_t* const_logical = ASR::down_cast<ASR::LogicalConstant_t>(value_expr);
            value = (T) const_logical->m_value;
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
        int64_t length_dim = 0;
        ASRUtils::extract_value(ASRUtils::expr_value(e->m_length), length_dim);
        return res + "[" + std::to_string(length_dim) + "]";
    }

    return res;
}

static inline std::string get_type_code(const ASR::ttype_t *t, bool use_underscore_sep=false,
    bool encode_dimensions_=true, bool set_dimensional_hint=true)
{
    bool is_dimensional = false;
    std::string res = "";
    switch (t->type) {
        case ASR::ttypeType::Array: {
            ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(t);
            res = get_type_code(array_t->m_type, use_underscore_sep, false, false);
            if( encode_dimensions_ ) {
                encode_dimensions(array_t->n_dims, res, use_underscore_sep);
                return res;
            }
            is_dimensional = true;
            break;
        }
        case ASR::ttypeType::Integer: {
            ASR::Integer_t *integer = ASR::down_cast<ASR::Integer_t>(t);
            res = "i" + std::to_string(integer->m_kind * 8);
            break;
        }
        case ASR::ttypeType::UnsignedInteger: {
            ASR::UnsignedInteger_t *integer = ASR::down_cast<ASR::UnsignedInteger_t>(t);
            res = "u" + std::to_string(integer->m_kind * 8);
            break;
        }
        case ASR::ttypeType::Real: {
            ASR::Real_t *real = ASR::down_cast<ASR::Real_t>(t);
            res = "r" + std::to_string(real->m_kind * 8);
            break;
        }
        case ASR::ttypeType::Complex: {
            ASR::Complex_t *complx = ASR::down_cast<ASR::Complex_t>(t);
            res = "c" + std::to_string(complx->m_kind * 8);
            break;
        }
        case ASR::ttypeType::Logical: {
            res = "i1";
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
            break;
        }
        case ASR::ttypeType::Class: {
            ASR::Class_t* d = ASR::down_cast<ASR::Class_t>(t);
            res = symbol_name(d->m_class_type);
            break;
        }
        case ASR::ttypeType::Union: {
            ASR::Union_t* d = ASR::down_cast<ASR::Union_t>(t);
            res = symbol_name(d->m_union_type);
            break;
        }
        case ASR::ttypeType::Pointer: {
            ASR::Pointer_t* p = ASR::down_cast<ASR::Pointer_t>(t);
            if( use_underscore_sep ) {
                return "Pointer_" + get_type_code(p->m_type, use_underscore_sep,
                                                  encode_dimensions_, set_dimensional_hint) + "_";
            }
            return "Pointer[" + get_type_code(p->m_type, use_underscore_sep,
                                              encode_dimensions_, set_dimensional_hint) + "]";
        }
        case ASR::ttypeType::Allocatable: {
            ASR::Allocatable_t* p = ASR::down_cast<ASR::Allocatable_t>(t);
            if( use_underscore_sep ) {
                return "Allocatable_" + get_type_code(p->m_type, use_underscore_sep,
                                                  encode_dimensions_, set_dimensional_hint) + "_";
            }
            return "Allocatable[" + get_type_code(p->m_type, use_underscore_sep,
                                              encode_dimensions_, set_dimensional_hint) + "]";
        }
        case ASR::ttypeType::Const: {
            ASR::Const_t* p = ASR::down_cast<ASR::Const_t>(t);
            if( use_underscore_sep ) {
                return "Const_" + get_type_code(p->m_type, use_underscore_sep,
                                                encode_dimensions_, set_dimensional_hint) + "_";
            }
            return "Const[" + get_type_code(p->m_type, use_underscore_sep,
                                            encode_dimensions_, set_dimensional_hint) + "]";
        }
        case ASR::ttypeType::SymbolicExpression: {
            return "S";
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
        case ASR::ttypeType::Array: {
            ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(t);
            std::string res = type_to_str_python(array_t->m_type, for_error_message);
            if (array_t->n_dims == 1 && for_error_message) {
                res = type_python_1dim_helper(res, array_t->m_dims);
            }
            return res;
        }
        case ASR::ttypeType::Integer: {
            ASR::Integer_t *i = ASR::down_cast<ASR::Integer_t>(t);
            std::string res = "";
            switch (i->m_kind) {
                case 1: { res = "i8"; break; }
                case 2: { res = "i16"; break; }
                case 4: { res = "i32"; break; }
                case 8: { res = "i64"; break; }
                default: { throw LCompilersException("Integer kind not supported"); }
            }
            return res;
        }
        case ASR::ttypeType::UnsignedInteger: {
            ASR::UnsignedInteger_t *i = ASR::down_cast<ASR::UnsignedInteger_t>(t);
            std::string res = "";
            switch (i->m_kind) {
                case 1: { res = "u8"; break; }
                case 2: { res = "u16"; break; }
                case 4: { res = "u32"; break; }
                case 8: { res = "u64"; break; }
                default: { throw LCompilersException("UnsignedInteger kind not supported"); }
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
            return "struct " + std::string(symbol_name(d->m_derived_type));
        }
        case ASR::ttypeType::Enum: {
            ASR::Enum_t* d = ASR::down_cast<ASR::Enum_t>(t);
            return "enum " + std::string(symbol_name(d->m_enum_type));
        }
        case ASR::ttypeType::Union: {
            ASR::Union_t* d = ASR::down_cast<ASR::Union_t>(t);
            return "union " + std::string(symbol_name(d->m_union_type));
        }
        case ASR::ttypeType::Pointer: {
            ASR::Pointer_t* p = ASR::down_cast<ASR::Pointer_t>(t);
            return "Pointer[" + type_to_str_python(p->m_type) + "]";
        }
        case ASR::ttypeType::Allocatable: {
            ASR::Allocatable_t* p = ASR::down_cast<ASR::Allocatable_t>(t);
            return "Allocatable[" + type_to_str_python(p->m_type) + "]";
        }
        case ASR::ttypeType::Const: {
            ASR::Const_t* p = ASR::down_cast<ASR::Const_t>(t);
            return "Const[" + type_to_str_python(p->m_type) + "]";
        }
        case ASR::ttypeType::TypeParameter: {
            ASR::TypeParameter_t *p = ASR::down_cast<ASR::TypeParameter_t>(t);
            return p->m_param;
        }
        case ASR::ttypeType::SymbolicExpression: {
            return "S";
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
    LCOMPILERS_ASSERT(ASR::is_a<ASR::unit_t>(*s->asr_owner))
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

static inline ASR::expr_t* get_constant_zero_with_given_type(Allocator& al, ASR::ttype_t* asr_type) {
    asr_type = ASRUtils::type_get_past_pointer(asr_type);
    asr_type = ASRUtils::type_get_past_array(asr_type);
    switch (asr_type->type) {
        case ASR::ttypeType::Integer: {
            return ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, asr_type->base.loc, 0, asr_type));
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
            throw LCompilersException("get_constant_zero_with_given_type: Not implemented " + std::to_string(asr_type->type));
        }
    }
    return nullptr;
}


static inline ASR::expr_t* get_constant_one_with_given_type(Allocator& al, ASR::ttype_t* asr_type) {
    asr_type = ASRUtils::type_get_past_array(asr_type);
    switch (asr_type->type) {
        case ASR::ttypeType::Integer: {
            return ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, asr_type->base.loc, 1, asr_type));
        }
        case ASR::ttypeType::Real: {
            return ASRUtils::EXPR(ASR::make_RealConstant_t(al, asr_type->base.loc, 1.0, asr_type));
        }
        case ASR::ttypeType::Complex: {
            return ASRUtils::EXPR(ASR::make_ComplexConstant_t(al, asr_type->base.loc, 1.0, 1.0, asr_type));
        }
        case ASR::ttypeType::Logical: {
            return ASRUtils::EXPR(ASR::make_LogicalConstant_t(al, asr_type->base.loc, true, asr_type));
        }
        default: {
            throw LCompilersException("get_constant_one_with_given_type: Not implemented " + std::to_string(asr_type->type));
        }
    }
    return nullptr;
}

static inline ASR::expr_t* get_minimum_value_with_given_type(Allocator& al, ASR::ttype_t* asr_type) {
    asr_type = ASRUtils::type_get_past_array(asr_type);
    int kind = ASRUtils::extract_kind_from_ttype_t(asr_type);
    switch (asr_type->type) {
        case ASR::ttypeType::Integer: {
            int64_t val;
            switch (kind) {
                case 1: val = std::numeric_limits<int8_t>::min(); break;
                case 2: val = std::numeric_limits<int16_t>::min(); break;
                case 4: val = std::numeric_limits<int32_t>::min(); break;
                case 8: val = std::numeric_limits<int64_t>::min(); break;
                default:
                    throw LCompilersException("get_minimum_value_with_given_type: Unsupported integer kind " + std::to_string(kind));
            }
            return ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, asr_type->base.loc, val, asr_type));
        }
        case ASR::ttypeType::Real: {
            double val;
            switch (kind) {
                case 4: val = std::numeric_limits<float>::lowest(); break;
                case 8: val = std::numeric_limits<double>::lowest(); break;
                default:
                    throw LCompilersException("get_minimum_value_with_given_type: Unsupported real kind " + std::to_string(kind));
            }
            return ASRUtils::EXPR(ASR::make_RealConstant_t(al, asr_type->base.loc, val, asr_type));
        }
        default: {
            throw LCompilersException("get_minimum_value_with_given_type: Not implemented " + std::to_string(asr_type->type));
        }
    }
    return nullptr;
}

static inline ASR::expr_t* get_maximum_value_with_given_type(Allocator& al, ASR::ttype_t* asr_type) {
    asr_type = ASRUtils::type_get_past_array(asr_type);
    int kind = ASRUtils::extract_kind_from_ttype_t(asr_type);
    switch (asr_type->type) {
        case ASR::ttypeType::Integer: {
            int64_t val;
            switch (kind) {
                case 1: val = std::numeric_limits<int8_t>::max(); break;
                case 2: val = std::numeric_limits<int16_t>::max(); break;
                case 4: val = std::numeric_limits<int32_t>::max(); break;
                case 8: val = std::numeric_limits<int64_t>::max(); break;
                default:
                    throw LCompilersException("get_maximum_value_with_given_type: Unsupported integer kind " + std::to_string(kind));
            }
            return ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, asr_type->base.loc, val, asr_type));
        }
        case ASR::ttypeType::Real: {
            double val;
            switch (kind) {
                case 4: val = std::numeric_limits<float>::max(); break;
                case 8: val = std::numeric_limits<double>::max(); break;
                default:
                    throw LCompilersException("get_maximum_value_with_given_type: Unsupported real kind " + std::to_string(kind));
            }
            return ASRUtils::EXPR(ASR::make_RealConstant_t(al, asr_type->base.loc, val, asr_type));
        }
        default: {
            throw LCompilersException("get_maximum_value_with_given_type: Not implemented " + std::to_string(asr_type->type));
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

std::vector<std::string> determine_variable_declaration_order(
         SymbolTable* symtab);

void extract_module_python(const ASR::TranslationUnit_t &m,
        std::vector<std::pair<std::string, ASR::Module_t*>>& children_modules,
        std::string module_name);

void update_call_args(Allocator &al, SymbolTable *current_scope, bool implicit_interface);

ASR::Module_t* extract_module(const ASR::TranslationUnit_t &m);

ASR::Module_t* load_module(Allocator &al, SymbolTable *symtab,
                            const std::string &module_name,
                            const Location &loc, bool intrinsic,
                            LCompilers::PassOptions& pass_options,
                            bool run_verify,
                            const std::function<void (const std::string &, const Location &)> err);

ASR::TranslationUnit_t* find_and_load_module(Allocator &al, const std::string &msym,
                                                SymbolTable &symtab, bool intrinsic,
                                                LCompilers::PassOptions& pass_options);

void set_intrinsic(ASR::TranslationUnit_t* trans_unit);

ASR::asr_t* getStructInstanceMember_t(Allocator& al, const Location& loc,
                            ASR::asr_t* v_var, ASR::symbol_t *v,
                            ASR::symbol_t* member, SymbolTable* current_scope);

bool use_overloaded(ASR::expr_t* left, ASR::expr_t* right,
                    ASR::binopType op, std::string& intrinsic_op_name,
                    SymbolTable* curr_scope, ASR::asr_t*& asr,
                    Allocator &al, const Location& loc,
                    SetChar& current_function_dependencies,
                    SetChar& current_module_dependencies,
                    const std::function<void (const std::string &, const Location &)> err);

bool use_overloaded_unary_minus(ASR::expr_t* operand,
    SymbolTable* curr_scope, ASR::asr_t*& asr, Allocator &al,
    const Location& loc, SetChar& current_function_dependencies,
    SetChar& current_module_dependencies,
    const std::function<void (const std::string &, const Location &)> err);

bool is_op_overloaded(ASR::binopType op, std::string& intrinsic_op_name,
                      SymbolTable* curr_scope, ASR::StructType_t* left_struct=nullptr);

bool use_overloaded(ASR::expr_t* left, ASR::expr_t* right,
                    ASR::cmpopType op, std::string& intrinsic_op_name,
                    SymbolTable* curr_scope, ASR::asr_t*& asr,
                    Allocator &al, const Location& loc,
                    SetChar& current_function_dependencies,
                    SetChar& current_module_dependencies,
                    const std::function<void (const std::string &, const Location &)> err);

bool is_op_overloaded(ASR::cmpopType op, std::string& intrinsic_op_name,
                      SymbolTable* curr_scope, ASR::StructType_t *left_struct);

bool use_overloaded_assignment(ASR::expr_t* target, ASR::expr_t* value,
                               SymbolTable* curr_scope, ASR::asr_t*& asr,
                               Allocator &al, const Location& loc,
                               SetChar& current_function_dependencies,
                               SetChar& /*current_module_dependencies*/,
                               const std::function<void (const std::string &, const Location &)> err);

void set_intrinsic(ASR::symbol_t* sym);

static inline bool is_pointer(ASR::ttype_t *x) {
    return ASR::is_a<ASR::Pointer_t>(*x);
}

static inline bool is_integer(ASR::ttype_t &x) {
    return ASR::is_a<ASR::Integer_t>(
        *type_get_past_array(
            type_get_past_allocatable(
                type_get_past_pointer(&x))));
}

static inline bool is_unsigned_integer(ASR::ttype_t &x) {
    return ASR::is_a<ASR::UnsignedInteger_t>(
        *type_get_past_array(
            type_get_past_allocatable(
                type_get_past_pointer(&x))));
}

static inline bool is_real(ASR::ttype_t &x) {
    return ASR::is_a<ASR::Real_t>(
        *type_get_past_array(
            type_get_past_allocatable(
                type_get_past_pointer(&x))));
}

static inline bool is_character(ASR::ttype_t &x) {
    return ASR::is_a<ASR::Character_t>(
        *type_get_past_array(
            type_get_past_allocatable(
                type_get_past_pointer(&x))));
}

static inline bool is_complex(ASR::ttype_t &x) {
    return ASR::is_a<ASR::Complex_t>(
        *type_get_past_array(
            type_get_past_allocatable(
                type_get_past_pointer(&x))));
}

static inline bool is_logical(ASR::ttype_t &x) {
    return ASR::is_a<ASR::Logical_t>(
        *type_get_past_array(
            type_get_past_allocatable(
                type_get_past_pointer(&x))));
}

static inline bool is_generic(ASR::ttype_t &x) {
    switch (x.type) {
        case ASR::ttypeType::List: {
            ASR::List_t *list_type = ASR::down_cast<ASR::List_t>(type_get_past_pointer(&x));
            return is_generic(*list_type->m_type);
        }
        case ASR::ttypeType::Array: {
            ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(type_get_past_pointer(&x));
            return is_generic(*array_t->m_type);
        }
        default : return ASR::is_a<ASR::TypeParameter_t>(*type_get_past_pointer(&x));
    }
}

static inline bool is_type_parameter(ASR::ttype_t &x) {
    switch (x.type) {
        case ASR::ttypeType::List: {
            ASR::List_t *list_type = ASR::down_cast<ASR::List_t>(type_get_past_pointer(&x));
            return is_type_parameter(*list_type->m_type);
        }
        default : return ASR::is_a<ASR::TypeParameter_t>(*type_get_past_pointer(&x));
    }
}

static inline bool is_template_function(ASR::symbol_t *x) {
    ASR::symbol_t* x2 = symbol_get_past_external(x);
    switch (x2->type) {
        case ASR::symbolType::Function: {
            const SymbolTable* parent_symtab = symbol_parent_symtab(x2);
            if (ASR::is_a<ASR::symbol_t>(*parent_symtab->asr_owner)) {
                ASR::symbol_t* parent_sym = ASR::down_cast<ASR::symbol_t>(parent_symtab->asr_owner);
                return ASR::is_a<ASR::Template_t>(*parent_sym);
            }
            return false;
        }
        default: return false;
    }
}

static inline bool is_requirement_function(ASR::symbol_t *x) {
    ASR::symbol_t* x2 = symbol_get_past_external(x);
    switch (x2->type) {
        case ASR::symbolType::Function: {
            ASR::Function_t *func_sym = ASR::down_cast<ASR::Function_t>(x2);
            return ASRUtils::get_FunctionType(func_sym)->m_is_restriction;
        }
        default: return false;
    }
}

static inline bool is_generic_function(ASR::symbol_t *x) {
    ASR::symbol_t* x2 = symbol_get_past_external(x);
    switch (x2->type) {
        case ASR::symbolType::Function: {
            ASR::Function_t *func_sym = ASR::down_cast<ASR::Function_t>(x2);
            return (ASRUtils::get_FunctionType(func_sym)->n_type_params > 0 &&
                   !ASRUtils::get_FunctionType(func_sym)->m_is_restriction);
        }
        default: return false;
    }
}

static inline bool is_restriction_function(ASR::symbol_t *x) {
    ASR::symbol_t* x2 = symbol_get_past_external(x);
    switch (x2->type) {
        case ASR::symbolType::Function: {
            ASR::Function_t *func_sym = ASR::down_cast<ASR::Function_t>(x2);
            return ASRUtils::get_FunctionType(func_sym)->m_is_restriction;
        }
        default: return false;
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
        case ASR::ttypeType::Array: {
            ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(x);
            n_dims = array_t->n_dims;
            m_dims = array_t->m_dims;
            break;
        }
        case ASR::ttypeType::Pointer: {
            n_dims = extract_dimensions_from_ttype(ASR::down_cast<ASR::Pointer_t>(x)->m_type, m_dims);
            break;
        }
        case ASR::ttypeType::Allocatable: {
            n_dims = extract_dimensions_from_ttype(ASR::down_cast<ASR::Allocatable_t>(x)->m_type, m_dims);
            break;
        }
        case ASR::ttypeType::Const: {
            n_dims = extract_dimensions_from_ttype(ASR::down_cast<ASR::Const_t>(x)->m_type, m_dims);
            break;
        }
        case ASR::ttypeType::SymbolicExpression:
        case ASR::ttypeType::Integer:
        case ASR::ttypeType::UnsignedInteger:
        case ASR::ttypeType::Real:
        case ASR::ttypeType::Complex:
        case ASR::ttypeType::Character:
        case ASR::ttypeType::Logical:
        case ASR::ttypeType::Struct:
        case ASR::ttypeType::Enum:
        case ASR::ttypeType::Union:
        case ASR::ttypeType::Class:
        case ASR::ttypeType::List:
        case ASR::ttypeType::Tuple:
        case ASR::ttypeType::Dict:
        case ASR::ttypeType::Set:
        case ASR::ttypeType::CPtr:
        case ASR::ttypeType::TypeParameter:
        case ASR::ttypeType::FunctionType: {
            n_dims = 0;
            m_dims = nullptr;
            break;
        }
        default:
            throw LCompilersException("Not implemented " + std::to_string(x->type) + ".");
    }
    return n_dims;
}

static inline bool is_fixed_size_array(ASR::dimension_t* m_dims, size_t n_dims) {
    if( n_dims == 0 ) {
        return false;
    }
    for( size_t i = 0; i < n_dims; i++ ) {
        int64_t dim_size = -1;
        if( m_dims[i].m_length == nullptr ) {
            return false;
        }
        if( !ASRUtils::extract_value(ASRUtils::expr_value(m_dims[i].m_length), dim_size) ) {
            return false;
        }
    }
    return true;
}

static inline bool is_fixed_size_array(ASR::ttype_t* type) {
    ASR::dimension_t* m_dims = nullptr;
    size_t n_dims = ASRUtils::extract_dimensions_from_ttype(type, m_dims);
    return ASRUtils::is_fixed_size_array(m_dims, n_dims);
}

static inline int64_t get_fixed_size_of_array(ASR::dimension_t* m_dims, size_t n_dims) {
    if( n_dims == 0 ) {
        return 0;
    }
    int64_t array_size = 1;
    for( size_t i = 0; i < n_dims; i++ ) {
        int64_t dim_size = -1;
        if( !ASRUtils::extract_value(ASRUtils::expr_value(m_dims[i].m_length), dim_size) ) {
            return -1;
        }
        array_size *= dim_size;
    }
    return array_size;
}

inline int extract_n_dims_from_ttype(ASR::ttype_t *x) {
    ASR::dimension_t* m_dims_temp = nullptr;
    return extract_dimensions_from_ttype(x, m_dims_temp);
}

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

inline ASR::ttype_t* make_Array_t_util(Allocator& al, const Location& loc,
    ASR::ttype_t* type, ASR::dimension_t* m_dims, size_t n_dims,
    ASR::abiType abi=ASR::abiType::Source, bool is_argument=false,
    ASR::array_physical_typeType physical_type=ASR::array_physical_typeType::DescriptorArray,
    bool override_physical_type=false) {
    if( n_dims == 0 ) {
        return type;
    }

    if( !override_physical_type ) {
        if( abi == ASR::abiType::BindC ) {
            physical_type = ASR::array_physical_typeType::PointerToDataArray;
        } else {
            if( ASRUtils::is_fixed_size_array(m_dims, n_dims) ) {
                if( is_argument ) {
                    physical_type = ASR::array_physical_typeType::PointerToDataArray;
                } else {
                    physical_type = ASR::array_physical_typeType::FixedSizeArray;
                }
            } else if( !ASRUtils::is_dimension_empty(m_dims, n_dims) ) {
                physical_type = ASR::array_physical_typeType::PointerToDataArray;
            }
        }
    }
    return ASRUtils::TYPE(ASR::make_Array_t(
        al, loc, type, m_dims, n_dims, physical_type));
}

// Sets the dimension member of `ttype_t`. Returns `true` if dimensions set.
// Returns `false` if the `ttype_t` does not have a dimension member.
inline bool ttype_set_dimensions(ASR::ttype_t** x,
            ASR::dimension_t *m_dims, int64_t n_dims,
            Allocator& al, ASR::abiType abi=ASR::abiType::Source,
            bool is_argument=false) {
    switch ((*x)->type) {
        case ASR::ttypeType::Array: {
            ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(*x);
            array_t->n_dims = n_dims;
            array_t->m_dims = m_dims;
            return true;
        }
        case ASR::ttypeType::Pointer: {
            return ttype_set_dimensions(
                &(ASR::down_cast<ASR::Pointer_t>(*x)->m_type), m_dims, n_dims, al);
        }
        case ASR::ttypeType::Allocatable: {
            return ttype_set_dimensions(
                &(ASR::down_cast<ASR::Allocatable_t>(*x)->m_type), m_dims, n_dims, al);
        }
        case ASR::ttypeType::Integer:
        case ASR::ttypeType::UnsignedInteger:
        case ASR::ttypeType::Real:
        case ASR::ttypeType::Complex:
        case ASR::ttypeType::Character:
        case ASR::ttypeType::Logical:
        case ASR::ttypeType::Struct:
        case ASR::ttypeType::Enum:
        case ASR::ttypeType::Union:
        case ASR::ttypeType::TypeParameter: {
            *x = ASRUtils::make_Array_t_util(al,
                (*x)->base.loc, *x, m_dims, n_dims, abi, is_argument);
            return true;
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

static inline bool is_aggregate_type(ASR::ttype_t* asr_type) {
    if( ASR::is_a<ASR::Const_t>(*asr_type) ) {
        asr_type = ASR::down_cast<ASR::Const_t>(asr_type)->m_type;
    }
    return ASRUtils::is_array(asr_type) ||
            !(ASR::is_a<ASR::Integer_t>(*asr_type) ||
              ASR::is_a<ASR::UnsignedInteger_t>(*asr_type) ||
              ASR::is_a<ASR::Real_t>(*asr_type) ||
              ASR::is_a<ASR::Complex_t>(*asr_type) ||
              ASR::is_a<ASR::Logical_t>(*asr_type));
}

static inline ASR::dimension_t* duplicate_dimensions(Allocator& al, ASR::dimension_t* m_dims, size_t n_dims);

static inline ASR::ttype_t* duplicate_type(Allocator& al, const ASR::ttype_t* t,
    Vec<ASR::dimension_t>* dims=nullptr,
    ASR::array_physical_typeType physical_type=ASR::array_physical_typeType::DescriptorArray,
    bool override_physical_type=false) {
    size_t dimsn = 0;
    ASR::dimension_t* dimsp = nullptr;
    if (dims != nullptr) {
        dimsp = dims->p;
        dimsn = dims->n;
    }
    ASR::ttype_t* t_ = nullptr;
    switch (t->type) {
        case ASR::ttypeType::Array: {
            ASR::Array_t* tnew = ASR::down_cast<ASR::Array_t>(t);
            ASR::ttype_t* duplicated_element_type = duplicate_type(al, tnew->m_type);
            if (dims == nullptr) {
                dimsp = duplicate_dimensions(al, tnew->m_dims, tnew->n_dims);
                dimsn = tnew->n_dims;
            }
            return ASRUtils::make_Array_t_util(al, tnew->base.base.loc,
                duplicated_element_type, dimsp, dimsn, ASR::abiType::Source,
                false, physical_type, override_physical_type);
        }
        case ASR::ttypeType::Integer: {
            ASR::Integer_t* tnew = ASR::down_cast<ASR::Integer_t>(t);
            t_ = ASRUtils::TYPE(ASR::make_Integer_t(al, t->base.loc, tnew->m_kind));
            break;
        }
        case ASR::ttypeType::UnsignedInteger: {
            ASR::UnsignedInteger_t* tnew = ASR::down_cast<ASR::UnsignedInteger_t>(t);
            t_ = ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al, t->base.loc, tnew->m_kind));
            break;
        }
        case ASR::ttypeType::Real: {
            ASR::Real_t* tnew = ASR::down_cast<ASR::Real_t>(t);
            t_ = ASRUtils::TYPE(ASR::make_Real_t(al, t->base.loc, tnew->m_kind));
            break;
        }
        case ASR::ttypeType::Complex: {
            ASR::Complex_t* tnew = ASR::down_cast<ASR::Complex_t>(t);
            t_ = ASRUtils::TYPE(ASR::make_Complex_t(al, t->base.loc, tnew->m_kind));
            break;
        }
        case ASR::ttypeType::Logical: {
            ASR::Logical_t* tnew = ASR::down_cast<ASR::Logical_t>(t);
            t_ = ASRUtils::TYPE(ASR::make_Logical_t(al, t->base.loc, tnew->m_kind));
            break;
        }
        case ASR::ttypeType::Character: {
            ASR::Character_t* tnew = ASR::down_cast<ASR::Character_t>(t);
            t_ = ASRUtils::TYPE(ASR::make_Character_t(al, t->base.loc,
                    tnew->m_kind, tnew->m_len, tnew->m_len_expr));
            break;
        }
        case ASR::ttypeType::Struct: {
            ASR::Struct_t* tnew = ASR::down_cast<ASR::Struct_t>(t);
            t_ = ASRUtils::TYPE(ASR::make_Struct_t(al, t->base.loc, tnew->m_derived_type));
            break;
        }
        case ASR::ttypeType::Class: {
            ASR::Class_t* tnew = ASR::down_cast<ASR::Class_t>(t);
            t_ = ASRUtils::TYPE(ASR::make_Class_t(al, t->base.loc, tnew->m_class_type));
            break;
        }
        case ASR::ttypeType::Pointer: {
            ASR::Pointer_t* ptr = ASR::down_cast<ASR::Pointer_t>(t);
            ASR::ttype_t* dup_type = duplicate_type(al, ptr->m_type, dims,
                physical_type, override_physical_type);
            if( override_physical_type &&
                physical_type == ASR::array_physical_typeType::FixedSizeArray ) {
                return dup_type;
            }
            return ASRUtils::TYPE(ASR::make_Pointer_t(al, ptr->base.base.loc,
                ASRUtils::type_get_past_allocatable(dup_type)));
        }
        case ASR::ttypeType::Allocatable: {
            ASR::Allocatable_t* alloc_ = ASR::down_cast<ASR::Allocatable_t>(t);
            ASR::ttype_t* dup_type = duplicate_type(al, alloc_->m_type, dims,
                physical_type, override_physical_type);
            if( override_physical_type &&
                physical_type == ASR::array_physical_typeType::FixedSizeArray ) {
                return dup_type;
            }
            return ASRUtils::TYPE(ASR::make_Allocatable_t(al, alloc_->base.base.loc,
                ASRUtils::type_get_past_allocatable(dup_type)));
        }
        case ASR::ttypeType::CPtr: {
            ASR::CPtr_t* ptr = ASR::down_cast<ASR::CPtr_t>(t);
            return ASRUtils::TYPE(ASR::make_CPtr_t(al, ptr->base.base.loc));
        }
        case ASR::ttypeType::Const: {
            ASR::Const_t* c = ASR::down_cast<ASR::Const_t>(t);
            ASR::ttype_t* dup_type = duplicate_type(al, c->m_type, dims);
            return ASRUtils::TYPE(ASR::make_Const_t(al, c->base.base.loc,
                        dup_type));
        }
        case ASR::ttypeType::List: {
            ASR::List_t* l = ASR::down_cast<ASR::List_t>(t);
            ASR::ttype_t* dup_type = duplicate_type(al, l->m_type);
            return ASRUtils::TYPE(ASR::make_List_t(al, l->base.base.loc,
                        dup_type));
        }
        case ASR::ttypeType::Dict: {
            ASR::Dict_t* d = ASR::down_cast<ASR::Dict_t>(t);
            ASR::ttype_t* dup_key_type = duplicate_type(al, d->m_key_type);
            ASR::ttype_t* dup_value_type = duplicate_type(al, d->m_value_type);
            return ASRUtils::TYPE(ASR::make_Dict_t(al, d->base.base.loc,
                        dup_key_type, dup_value_type));
        }
        case ASR::ttypeType::TypeParameter: {
            ASR::TypeParameter_t* tp = ASR::down_cast<ASR::TypeParameter_t>(t);
            //return ASRUtils::TYPE(ASR::make_TypeParameter_t(al, t->base.loc,
            //            tp->m_param, dimsp, dimsn, tp->m_rt, tp->n_rt));
            t_ = ASRUtils::TYPE(ASR::make_TypeParameter_t(al, t->base.loc, tp->m_param));
            break;
        }
        case ASR::ttypeType::FunctionType: {
            ASR::FunctionType_t* ft = ASR::down_cast<ASR::FunctionType_t>(t);
            //ASR::ttype_t* dup_type = duplicate_type(al, c->m_type, dims);
            Vec<ASR::ttype_t*> arg_types;
            arg_types.reserve(al, ft->n_arg_types);
            for( size_t i = 0; i < ft->n_arg_types; i++ ) {
                ASR::ttype_t *t = ASRUtils::duplicate_type(al, ft->m_arg_types[i],
                    nullptr, physical_type, override_physical_type);
                arg_types.push_back(al, t);
            }
            return ASRUtils::TYPE(ASR::make_FunctionType_t(al, ft->base.base.loc,
                arg_types.p, arg_types.size(), ft->m_return_var_type, ft->m_abi,
                ft->m_deftype, ft->m_bindc_name, ft->m_elemental, ft->m_pure, ft->m_module, ft->m_inline,
                ft->m_static, ft->m_type_params, ft->n_type_params, ft->m_restrictions, ft->n_restrictions,
                ft->m_is_restriction));
        }
        default : throw LCompilersException("Not implemented " + std::to_string(t->type));
    }
    LCOMPILERS_ASSERT(t_ != nullptr);
    return ASRUtils::make_Array_t_util(
        al, t_->base.loc, t_, dimsp, dimsn,
        ASR::abiType::Source, false, physical_type,
        override_physical_type);
}

static inline void set_absent_optional_arguments_to_null(
    Vec<ASR::call_arg_t>& args, ASR::Function_t* func, Allocator& al,
    ASR::expr_t* dt=nullptr) {
    int offset = (dt != nullptr);
    for( size_t i = args.size(); i < func->n_args - offset; i++ ) {
        if( ASR::is_a<ASR::Variable_t>(
                *ASR::down_cast<ASR::Var_t>(func->m_args[i + offset])->m_v) ) {
            LCOMPILERS_ASSERT(ASRUtils::EXPR2VAR(func->m_args[i + offset])->m_presence ==
                                ASR::presenceType::Optional);
            ASR::call_arg_t empty_arg;
            Location loc;
            loc.first = 1, loc.last = 1;
            empty_arg.loc = loc;
            empty_arg.m_value = nullptr;
            args.push_back(al, empty_arg);
        }
    }
    LCOMPILERS_ASSERT(args.size() == (func->n_args - offset));
}

static inline ASR::ttype_t* duplicate_type_with_empty_dims(Allocator& al, ASR::ttype_t* t,
    ASR::array_physical_typeType physical_type=ASR::array_physical_typeType::DescriptorArray,
    bool override_physical_type=false) {
    size_t n_dims = ASRUtils::extract_n_dims_from_ttype(t);
    Vec<ASR::dimension_t> empty_dims;
    empty_dims.reserve(al, n_dims);
    for( size_t i = 0; i < n_dims; i++ ) {
        ASR::dimension_t empty_dim;
        empty_dim.loc = t->base.loc;
        empty_dim.m_start = nullptr;
        empty_dim.m_length = nullptr;
        empty_dims.push_back(al, empty_dim);
    }
    return duplicate_type(al, t, &empty_dims, physical_type, override_physical_type);
}

static inline ASR::ttype_t* duplicate_type_without_dims(Allocator& al, const ASR::ttype_t* t, const Location& loc) {
    switch (t->type) {
        case ASR::ttypeType::Array: {
            return duplicate_type_without_dims(al, ASR::down_cast<ASR::Array_t>(t)->m_type, loc);
        }
        case ASR::ttypeType::Integer: {
            ASR::Integer_t* tnew = ASR::down_cast<ASR::Integer_t>(t);
            return ASRUtils::TYPE(ASR::make_Integer_t(al, loc, tnew->m_kind));
        }
        case ASR::ttypeType::UnsignedInteger: {
            ASR::UnsignedInteger_t* tnew = ASR::down_cast<ASR::UnsignedInteger_t>(t);
            return ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al, loc, tnew->m_kind));
        }
        case ASR::ttypeType::Real: {
            ASR::Real_t* tnew = ASR::down_cast<ASR::Real_t>(t);
            return ASRUtils::TYPE(ASR::make_Real_t(al, loc, tnew->m_kind));
        }
        case ASR::ttypeType::Complex: {
            ASR::Complex_t* tnew = ASR::down_cast<ASR::Complex_t>(t);
            return ASRUtils::TYPE(ASR::make_Complex_t(al, loc, tnew->m_kind));
        }
        case ASR::ttypeType::Logical: {
            ASR::Logical_t* tnew = ASR::down_cast<ASR::Logical_t>(t);
            return ASRUtils::TYPE(ASR::make_Logical_t(al, loc, tnew->m_kind));
        }
        case ASR::ttypeType::Character: {
            ASR::Character_t* tnew = ASR::down_cast<ASR::Character_t>(t);
            return ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                        tnew->m_kind, tnew->m_len, tnew->m_len_expr));
        }
        case ASR::ttypeType::Struct: {
            ASR::Struct_t* tstruct = ASR::down_cast<ASR::Struct_t>(t);
            return ASRUtils::TYPE(ASR::make_Struct_t(al, loc, tstruct->m_derived_type));
        }
        case ASR::ttypeType::Pointer: {
            ASR::Pointer_t* ptr = ASR::down_cast<ASR::Pointer_t>(t);
            ASR::ttype_t* dup_type = duplicate_type_without_dims(al, ptr->m_type, loc);
            return ASRUtils::TYPE(ASR::make_Pointer_t(al, ptr->base.base.loc,
                ASRUtils::type_get_past_allocatable(dup_type)));
        }
        case ASR::ttypeType::Allocatable: {
            ASR::Allocatable_t* alloc_ = ASR::down_cast<ASR::Allocatable_t>(t);
            ASR::ttype_t* dup_type = duplicate_type_without_dims(al, alloc_->m_type, loc);
            return ASRUtils::TYPE(ASR::make_Allocatable_t(al, alloc_->base.base.loc,
                ASRUtils::type_get_past_allocatable(dup_type)));
        }
        case ASR::ttypeType::TypeParameter: {
            ASR::TypeParameter_t* tp = ASR::down_cast<ASR::TypeParameter_t>(t);
            //return ASRUtils::TYPE(ASR::make_TypeParameter_t(al, t->base.loc,
            //            tp->m_param, nullptr, 0, tp->m_rt, tp->n_rt));
            return ASRUtils::TYPE(ASR::make_TypeParameter_t(al, loc, tp->m_param));
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
    dest = ASRUtils::type_get_past_array(ASR::down_cast<ASR::Pointer_t>(dest)->m_type);
    if( (ASR::is_a<ASR::Class_t>(*source) || ASR::is_a<ASR::Struct_t>(*source)) &&
        (ASR::is_a<ASR::Class_t>(*dest) || ASR::is_a<ASR::Struct_t>(*dest)) ) {
        return true;
    }
    bool res = source->type == dest->type;
    return res;
}

inline int extract_kind_str(char* m_n, char *&kind_str) {
    char *p = m_n;
    while (*p != '\0') {
        if (*p == '_') {
            p++;
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
            bool is_parent_enum = false;
            if (kind_variable->m_parent_symtab->asr_owner != nullptr) {
                ASR::symbol_t *s = ASR::down_cast<ASR::symbol_t>(
                    kind_variable->m_parent_symtab->asr_owner);
                is_parent_enum = ASR::is_a<ASR::EnumType_t>(*s);
            }
            if( is_parent_enum ) {
                a_kind = ASRUtils::extract_kind_from_ttype_t(kind_variable->m_type);
            } else if( kind_variable->m_storage == ASR::storage_typeType::Parameter ) {
                if( kind_variable->m_type->type == ASR::ttypeType::Integer ) {
                    LCOMPILERS_ASSERT( kind_variable->m_value != nullptr );
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
                    LCOMPILERS_ASSERT( len_variable->m_value != nullptr );
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
        case ASR::exprType::StringLen:
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
    LCOMPILERS_ASSERT(a_len != -10)
    return a_len;
}

inline bool is_parent(SymbolTable* a, SymbolTable* b) {
    SymbolTable* current_parent = b->parent;
    while( current_parent ) {
        if( current_parent == a ) {
            return true;
        }
        current_parent = current_parent->parent;
    }
    return false;
}

inline bool is_parent(ASR::StructType_t* a, ASR::StructType_t* b) {
    ASR::symbol_t* current_parent = b->m_parent;
    while( current_parent ) {
        current_parent = ASRUtils::symbol_get_past_external(current_parent);
        if( current_parent == (ASR::symbol_t*) a ) {
            return true;
        }
        LCOMPILERS_ASSERT(ASR::is_a<ASR::StructType_t>(*current_parent));
        current_parent = ASR::down_cast<ASR::StructType_t>(current_parent)->m_parent;
    }
    return false;
}

inline bool is_derived_type_similar(ASR::StructType_t* a, ASR::StructType_t* b) {
    return a == b || is_parent(a, b) || is_parent(b, a) ||
        (std::string(a->m_name) == "~abstract_type" &&
        std::string(b->m_name) == "~abstract_type");
}

// TODO: Scaled up implementation for all exprTypes
// One way is to do it in asdl_cpp.py
inline bool expr_equal(ASR::expr_t* x, ASR::expr_t* y) {
    if( x->type != y->type ) {
        return false;
    }

    switch( x->type ) {
        case ASR::exprType::IntegerBinOp: {
            ASR::IntegerBinOp_t* intbinop_x = ASR::down_cast<ASR::IntegerBinOp_t>(x);
            ASR::IntegerBinOp_t* intbinop_y = ASR::down_cast<ASR::IntegerBinOp_t>(y);
            if( intbinop_x->m_op != intbinop_y->m_op ) {
                return false;
            }
            bool left_left = expr_equal(intbinop_x->m_left, intbinop_y->m_left);
            bool left_right = expr_equal(intbinop_x->m_left, intbinop_y->m_right);
            bool right_left = expr_equal(intbinop_x->m_right, intbinop_y->m_left);
            bool right_right = expr_equal(intbinop_x->m_right, intbinop_y->m_right);
            switch( intbinop_x->m_op ) {
                case ASR::binopType::Add:
                case ASR::binopType::Mul:
                case ASR::binopType::BitAnd:
                case ASR::binopType::BitOr:
                case ASR::binopType::BitXor: {
                    return (left_left && right_right) || (left_right && right_left);
                }
                case ASR::binopType::Sub:
                case ASR::binopType::Div:
                case ASR::binopType::Pow:
                case ASR::binopType::BitLShift:
                case ASR::binopType::BitRShift: {
                    return (left_left && right_right);
                }
            }
            break;
        }
        case ASR::exprType::Var: {
            ASR::Var_t* var_x = ASR::down_cast<ASR::Var_t>(x);
            ASR::Var_t* var_y = ASR::down_cast<ASR::Var_t>(y);
            return var_x->m_v == var_y->m_v;
        }
        default: {
            // Let it pass for now.
            return true;
        }
    }

    // Let it pass for now.
    return true;
}

inline bool dimension_expr_equal(ASR::expr_t* dim_a, ASR::expr_t* dim_b) {
    if( !(dim_a && dim_b) ) {
        return true;
    }
    ASR::expr_t* dim_a_fallback = nullptr;
    ASR::expr_t* dim_b_fallback = nullptr;
    if( ASR::is_a<ASR::Var_t>(*dim_a) &&
        ASR::is_a<ASR::Variable_t>(
            *ASR::down_cast<ASR::Var_t>(dim_a)->m_v) ) {
        dim_a_fallback = ASRUtils::EXPR2VAR(dim_a)->m_symbolic_value;
    }
    if( ASR::is_a<ASR::Var_t>(*dim_b) &&
        ASR::is_a<ASR::Variable_t>(
            *ASR::down_cast<ASR::Var_t>(dim_b)->m_v) ) {
        dim_b_fallback = ASRUtils::EXPR2VAR(dim_b)->m_symbolic_value;
    }
    if( !ASRUtils::expr_equal(dim_a, dim_b) &&
        !(dim_a_fallback && ASRUtils::expr_equal(dim_a_fallback, dim_b)) &&
        !(dim_b_fallback && ASRUtils::expr_equal(dim_a, dim_b_fallback)) ) {
        return false;
    }
    return true;
}

inline bool dimensions_equal(ASR::dimension_t* dims_a, size_t n_dims_a,
    ASR::dimension_t* dims_b, size_t n_dims_b) {
    if( n_dims_a != n_dims_b ) {
        return false;
    }

    for( size_t i = 0; i < n_dims_a; i++ ) {
        ASR::dimension_t dim_a = dims_a[i];
        ASR::dimension_t dim_b = dims_b[i];
        if( !dimension_expr_equal(dim_a.m_length, dim_b.m_length) ||
            !dimension_expr_equal(dim_a.m_start, dim_b.m_start) ) {
            return false;
        }
    }
    return true;
}

inline bool types_equal(ASR::ttype_t *a, ASR::ttype_t *b,
    bool check_for_dimensions=false) {
    // TODO: If anyone of the input or argument is derived type then
    // add support for checking member wise types and do not compare
    // directly. From stdlib_string len(pattern) error
    a = ASRUtils::type_get_past_allocatable(ASRUtils::type_get_past_pointer(a));
    b = ASRUtils::type_get_past_allocatable(ASRUtils::type_get_past_pointer(b));
    if( !check_for_dimensions ) {
        a = ASRUtils::type_get_past_array(a);
        b = ASRUtils::type_get_past_array(b);
    }
    if (a->type == b->type) {
        // TODO: check dims
        // TODO: check all types
        switch (a->type) {
            case (ASR::ttypeType::Array): {
                ASR::Array_t* a2 = ASR::down_cast<ASR::Array_t>(a);
                ASR::Array_t* b2 = ASR::down_cast<ASR::Array_t>(b);
                if( !types_equal(a2->m_type, b2->m_type) ) {
                    return false;
                }

                return ASRUtils::dimensions_equal(
                            a2->m_dims, a2->n_dims,
                            b2->m_dims, b2->n_dims);
            }
            case (ASR::ttypeType::Integer) : {
                ASR::Integer_t *a2 = ASR::down_cast<ASR::Integer_t>(a);
                ASR::Integer_t *b2 = ASR::down_cast<ASR::Integer_t>(b);
                return (a2->m_kind == b2->m_kind);
            }
            case (ASR::ttypeType::UnsignedInteger) : {
                ASR::UnsignedInteger_t *a2 = ASR::down_cast<ASR::UnsignedInteger_t>(a);
                ASR::UnsignedInteger_t *b2 = ASR::down_cast<ASR::UnsignedInteger_t>(b);
                return (a2->m_kind == b2->m_kind);
            }
            case ASR::ttypeType::CPtr: {
                return true;
            }
            case ASR::ttypeType::SymbolicExpression: {
                return true;
            }
            case (ASR::ttypeType::Real) : {
                ASR::Real_t *a2 = ASR::down_cast<ASR::Real_t>(a);
                ASR::Real_t *b2 = ASR::down_cast<ASR::Real_t>(b);
                return (a2->m_kind == b2->m_kind);
            }
            case (ASR::ttypeType::Complex) : {
                ASR::Complex_t *a2 = ASR::down_cast<ASR::Complex_t>(a);
                ASR::Complex_t *b2 = ASR::down_cast<ASR::Complex_t>(b);
                return (a2->m_kind == b2->m_kind);
            }
            case (ASR::ttypeType::Logical) : {
                ASR::Logical_t *a2 = ASR::down_cast<ASR::Logical_t>(a);
                ASR::Logical_t *b2 = ASR::down_cast<ASR::Logical_t>(b);
                return (a2->m_kind == b2->m_kind);
            }
            case (ASR::ttypeType::Character) : {
                ASR::Character_t *a2 = ASR::down_cast<ASR::Character_t>(a);
                ASR::Character_t *b2 = ASR::down_cast<ASR::Character_t>(b);
                return (a2->m_kind == b2->m_kind);
            }
            case (ASR::ttypeType::List) : {
                ASR::List_t *a2 = ASR::down_cast<ASR::List_t>(a);
                ASR::List_t *b2 = ASR::down_cast<ASR::List_t>(b);
                return types_equal(a2->m_type, b2->m_type);
            }
            case (ASR::ttypeType::Struct) : {
                ASR::Struct_t *a2 = ASR::down_cast<ASR::Struct_t>(a);
                ASR::Struct_t *b2 = ASR::down_cast<ASR::Struct_t>(b);
                ASR::StructType_t *a2_type = ASR::down_cast<ASR::StructType_t>(
                                                ASRUtils::symbol_get_past_external(
                                                    a2->m_derived_type));
                ASR::StructType_t *b2_type = ASR::down_cast<ASR::StructType_t>(
                                                ASRUtils::symbol_get_past_external(
                                                    b2->m_derived_type));
                return a2_type == b2_type;
            }
            case (ASR::ttypeType::Class) : {
                ASR::Class_t *a2 = ASR::down_cast<ASR::Class_t>(a);
                ASR::Class_t *b2 = ASR::down_cast<ASR::Class_t>(b);
                ASR::symbol_t* a2_typesym = ASRUtils::symbol_get_past_external(a2->m_class_type);
                ASR::symbol_t* b2_typesym = ASRUtils::symbol_get_past_external(b2->m_class_type);
                if( a2_typesym->type != b2_typesym->type ) {
                    return false;
                }
                if( a2_typesym->type == ASR::symbolType::ClassType ) {
                    ASR::ClassType_t *a2_type = ASR::down_cast<ASR::ClassType_t>(a2_typesym);
                    ASR::ClassType_t *b2_type = ASR::down_cast<ASR::ClassType_t>(b2_typesym);
                    return a2_type == b2_type;
                } else if( a2_typesym->type == ASR::symbolType::StructType ) {
                    ASR::StructType_t *a2_type = ASR::down_cast<ASR::StructType_t>(a2_typesym);
                    ASR::StructType_t *b2_type = ASR::down_cast<ASR::StructType_t>(b2_typesym);
                    return is_derived_type_similar(a2_type, b2_type);
                }
                return false;
            }
            case (ASR::ttypeType::Union) : {
                ASR::Union_t *a2 = ASR::down_cast<ASR::Union_t>(a);
                ASR::Union_t *b2 = ASR::down_cast<ASR::Union_t>(b);
                ASR::UnionType_t *a2_type = ASR::down_cast<ASR::UnionType_t>(
                                                ASRUtils::symbol_get_past_external(
                                                    a2->m_union_type));
                ASR::UnionType_t *b2_type = ASR::down_cast<ASR::UnionType_t>(
                                                ASRUtils::symbol_get_past_external(
                                                    b2->m_union_type));
                return a2_type == b2_type;
            }
            default : return false;
        }
    } else if( a->type == ASR::ttypeType::Struct &&
               b->type == ASR::ttypeType::Class ) {
        ASR::Struct_t *a2 = ASR::down_cast<ASR::Struct_t>(a);
        ASR::Class_t *b2 = ASR::down_cast<ASR::Class_t>(b);
        ASR::symbol_t* a2_typesym = ASRUtils::symbol_get_past_external(a2->m_derived_type);
        ASR::symbol_t* b2_typesym = ASRUtils::symbol_get_past_external(b2->m_class_type);
        if( a2_typesym->type != b2_typesym->type ) {
            return false;
        }
        if( a2_typesym->type == ASR::symbolType::ClassType ) {
            ASR::ClassType_t *a2_type = ASR::down_cast<ASR::ClassType_t>(a2_typesym);
            ASR::ClassType_t *b2_type = ASR::down_cast<ASR::ClassType_t>(b2_typesym);
            return a2_type == b2_type;
        } else if( a2_typesym->type == ASR::symbolType::StructType ) {
            ASR::StructType_t *a2_type = ASR::down_cast<ASR::StructType_t>(a2_typesym);
            ASR::StructType_t *b2_type = ASR::down_cast<ASR::StructType_t>(b2_typesym);
            return is_derived_type_similar(a2_type, b2_type);
        }
    } else if( a->type == ASR::ttypeType::Class &&
               b->type == ASR::ttypeType::Struct ) {
        ASR::Class_t *a2 = ASR::down_cast<ASR::Class_t>(a);
        ASR::Struct_t *b2 = ASR::down_cast<ASR::Struct_t>(b);
        ASR::symbol_t* a2_typesym = ASRUtils::symbol_get_past_external(a2->m_class_type);
        ASR::symbol_t* b2_typesym = ASRUtils::symbol_get_past_external(b2->m_derived_type);
        if( a2_typesym->type != b2_typesym->type ) {
            return false;
        }
        if( a2_typesym->type == ASR::symbolType::ClassType ) {
            ASR::ClassType_t *a2_type = ASR::down_cast<ASR::ClassType_t>(a2_typesym);
            ASR::ClassType_t *b2_type = ASR::down_cast<ASR::ClassType_t>(b2_typesym);
            return a2_type == b2_type;
        } else if( a2_typesym->type == ASR::symbolType::StructType ) {
            ASR::StructType_t *a2_type = ASR::down_cast<ASR::StructType_t>(a2_typesym);
            ASR::StructType_t *b2_type = ASR::down_cast<ASR::StructType_t>(b2_typesym);
            return is_derived_type_similar(a2_type, b2_type);
        }
    }
    return false;
}

inline bool check_equal_type(ASR::ttype_t* x, ASR::ttype_t* y, bool check_for_dimensions=false) {
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
    } else if( ASR::is_a<ASR::Allocatable_t>(*x) ||
               ASR::is_a<ASR::Allocatable_t>(*y) ) {
        x = ASRUtils::type_get_past_allocatable(x);
        y = ASRUtils::type_get_past_allocatable(y);
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
    } else if (ASR::is_a<ASR::FunctionType_t>(*x) && ASR::is_a<ASR::FunctionType_t>(*y)) {
        ASR::FunctionType_t* left_ft = ASR::down_cast<ASR::FunctionType_t>(x);
        ASR::FunctionType_t* right_ft = ASR::down_cast<ASR::FunctionType_t>(y);
        if (left_ft->n_arg_types != right_ft->n_arg_types) {
            return false;
        }
        bool result;
        for (size_t i=0; i<left_ft->n_arg_types; i++) {
            result = check_equal_type(left_ft->m_arg_types[i],
                        right_ft->m_arg_types[i]);
            if (!result) return false;
        }
        if (left_ft->m_return_var_type == nullptr &&
                right_ft->m_return_var_type == nullptr) {
                return true;
        } else if (left_ft->m_return_var_type != nullptr &&
                right_ft->m_return_var_type != nullptr) {
                return check_equal_type(left_ft->m_return_var_type,
                        right_ft->m_return_var_type);
        }
        return false;
    }

    return types_equal(x, y, check_for_dimensions);
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

static inline ASR::intentType symbol_intent(const ASR::symbol_t *f)
{
    switch( f->type ) {
        case ASR::symbolType::Variable: {
            return ASR::down_cast<ASR::Variable_t>(f)->m_intent;
        }
        default: {
            throw LCompilersException("Cannot return intent of, " +
                                    std::to_string(f->type) + " symbol.");
        }
    }
    return ASR::intentType::Unspecified;
}

static inline ASR::intentType expr_intent(ASR::expr_t* expr) {
    switch( expr->type ) {
        case ASR::exprType::Var: {
            return ASRUtils::symbol_intent(ASR::down_cast<ASR::Var_t>(expr)->m_v);
        }
        default: {
            throw LCompilersException("Cannot extract intent of ASR::exprType::" +
                std::to_string(expr->type));
        }
    }
    return ASR::intentType::Unspecified;
}

static inline bool is_data_only_array(ASR::ttype_t* type, ASR::abiType abi) {
    ASR::dimension_t* m_dims = nullptr;
    size_t n_dims = ASRUtils::extract_dimensions_from_ttype(type, m_dims);
    if( n_dims == 0 ) {
        return false;
    }
    return (abi == ASR::abiType::BindC || !ASRUtils::is_dimension_empty(m_dims, n_dims));
}

static inline void insert_module_dependency(ASR::symbol_t* a,
    Allocator& al, SetChar& module_dependencies) {
    if( ASR::is_a<ASR::ExternalSymbol_t>(*a) ) {
        ASR::ExternalSymbol_t* a_ext = ASR::down_cast<ASR::ExternalSymbol_t>(a);
        ASR::symbol_t* a_sym_module = ASRUtils::get_asr_owner(a_ext->m_external);
        if( a_sym_module ) {
            while( a_sym_module && !ASR::is_a<ASR::Module_t>(*a_sym_module) ) {
                a_sym_module = ASRUtils::get_asr_owner(a_sym_module);
            }
            if( a_sym_module ) {
                module_dependencies.push_back(al, ASRUtils::symbol_name(a_sym_module));
            }
        }
    }
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
        case ASR::ttypeType::Array: {
            ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(t);
            return get_type_parameter(array_t->m_type);
        }
        default: throw LCompilersException("Cannot get type parameter from this type.");
    }
}

static inline ASR::symbol_t* import_struct_instance_member(Allocator& al, ASR::symbol_t* v,
    SymbolTable* scope, ASR::ttype_t*& mem_type) {
    ASR::ttype_t* mem_type_ = mem_type;
    v = ASRUtils::symbol_get_past_external(v);
    ASR::symbol_t* struct_t = ASRUtils::get_asr_owner(v);
    std::string v_name = ASRUtils::symbol_name(v);
    std::string struct_t_name = ASRUtils::symbol_name(struct_t);
    std::string struct_ext_name = struct_t_name;
    if( ASRUtils::symbol_get_past_external(
            scope->resolve_symbol(struct_t_name)) != struct_t ) {
        struct_ext_name = "1_" + struct_ext_name;
    }
    if( scope->resolve_symbol(struct_ext_name) == nullptr ) {
        ASR::symbol_t* struct_t_module = ASRUtils::get_asr_owner(
            ASRUtils::symbol_get_past_external(struct_t));
        LCOMPILERS_ASSERT(struct_t_module != nullptr);
        SymbolTable* import_struct_t_scope = scope;
        while( import_struct_t_scope->asr_owner == nullptr ||
               !ASR::is_a<ASR::Module_t>(*ASR::down_cast<ASR::symbol_t>(
                import_struct_t_scope->asr_owner)) ) {
            import_struct_t_scope = import_struct_t_scope->parent;
            if( import_struct_t_scope->asr_owner != nullptr &&
                !ASR::is_a<ASR::symbol_t>(*import_struct_t_scope->asr_owner) ) {
                break;
            }
        }
        LCOMPILERS_ASSERT(import_struct_t_scope != nullptr);
        ASR::symbol_t* struct_ext = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(al,
                                    v->base.loc, import_struct_t_scope, s2c(al, struct_ext_name), struct_t,
                                    ASRUtils::symbol_name(struct_t_module),
                                    nullptr, 0, s2c(al, struct_t_name), ASR::accessType::Public));
        import_struct_t_scope->add_symbol(struct_ext_name, struct_ext);
    }
    std::string v_ext_name = "1_" + struct_t_name + "_" + v_name;
    if( scope->get_symbol(v_ext_name) == nullptr ) {
        ASR::symbol_t* v_ext = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(al,
                                    v->base.loc, scope, s2c(al, v_ext_name), ASRUtils::symbol_get_past_external(v),
                                    s2c(al, struct_ext_name), nullptr, 0, s2c(al, v_name), ASR::accessType::Public));
        scope->add_symbol(v_ext_name, v_ext);
    }

    ASR::dimension_t* m_dims = nullptr;
    size_t n_dims = ASRUtils::extract_dimensions_from_ttype(mem_type, m_dims);
    mem_type = ASRUtils::type_get_past_array(
        ASRUtils::type_get_past_pointer(
            ASRUtils::type_get_past_allocatable(mem_type)));
    if( mem_type && ASR::is_a<ASR::Struct_t>(*mem_type) ) {
        ASR::Struct_t* struct_t = ASR::down_cast<ASR::Struct_t>(mem_type);
        std::string struct_type_name = ASRUtils::symbol_name(struct_t->m_derived_type);
        ASR::symbol_t* struct_t_m_derived_type = ASRUtils::symbol_get_past_external(struct_t->m_derived_type);
        if( scope->resolve_symbol(struct_type_name) == nullptr ) {
            std::string struct_type_name_ = "1_" + struct_type_name;
            if( scope->get_symbol(struct_type_name_) == nullptr ) {
                ASR::Module_t* struct_type_module = ASRUtils::get_sym_module(struct_t_m_derived_type);
                LCOMPILERS_ASSERT(struct_type_module != nullptr);
                ASR::symbol_t* imported_struct_type = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(al,
                    v->base.loc, scope, s2c(al, struct_type_name_), struct_t_m_derived_type, struct_type_module->m_name,
                    nullptr, 0, s2c(al, struct_type_name), ASR::accessType::Public));
                scope->add_symbol(struct_type_name_, imported_struct_type);
            }
            mem_type = ASRUtils::TYPE(ASR::make_Struct_t(al, mem_type->base.loc, scope->get_symbol(struct_type_name_)));
        } else {
            mem_type = ASRUtils::TYPE(ASR::make_Struct_t(al, mem_type->base.loc,
                scope->resolve_symbol(struct_type_name)));
        }
    }
    if( n_dims > 0 ) {
        mem_type = ASRUtils::make_Array_t_util(
            al, mem_type->base.loc, mem_type, m_dims, n_dims);
    }

    if( ASR::is_a<ASR::Allocatable_t>(*mem_type_) ) {
        mem_type = ASRUtils::TYPE(ASR::make_Allocatable_t(al,
        mem_type->base.loc, mem_type));
    } else if( ASR::is_a<ASR::Pointer_t>(*mem_type_) ) {
        mem_type = ASRUtils::TYPE(ASR::make_Pointer_t(al,
        mem_type->base.loc, mem_type));
    }

    return scope->get_symbol(v_ext_name);
}

static inline ASR::symbol_t* import_enum_member(Allocator& al, ASR::symbol_t* v,
    SymbolTable* scope) {
    v = ASRUtils::symbol_get_past_external(v);
    ASR::symbol_t* enum_t = ASRUtils::get_asr_owner(v);
    std::string v_name = ASRUtils::symbol_name(v);
    std::string enum_t_name = ASRUtils::symbol_name(enum_t);
    std::string enum_ext_name = enum_t_name;
    if( scope->resolve_symbol(enum_t_name) != enum_t ) {
        enum_ext_name = "1_" + enum_ext_name;
    }
    if( scope->resolve_symbol(enum_ext_name) == nullptr ) {
        ASR::symbol_t* enum_t_module = ASRUtils::get_asr_owner(
            ASRUtils::symbol_get_past_external(enum_t));
        LCOMPILERS_ASSERT(enum_t_module != nullptr);
        SymbolTable* import_enum_t_scope = scope;
        while( import_enum_t_scope->asr_owner == nullptr ||
               !ASR::is_a<ASR::Module_t>(*ASR::down_cast<ASR::symbol_t>(
                import_enum_t_scope->asr_owner)) ) {
            import_enum_t_scope = import_enum_t_scope->parent;
        }
        LCOMPILERS_ASSERT(import_enum_t_scope != nullptr);
        ASR::symbol_t* enum_ext = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(al,
                                    v->base.loc, import_enum_t_scope, s2c(al, enum_ext_name), enum_t,
                                    ASRUtils::symbol_name(enum_t_module),
                                    nullptr, 0, s2c(al, enum_t_name), ASR::accessType::Public));
        import_enum_t_scope->add_symbol(enum_ext_name, enum_ext);
    }
    std::string v_ext_name = "1_" + enum_t_name + "_" + v_name;
    if( scope->get_symbol(v_ext_name) == nullptr ) {
        ASR::symbol_t* v_ext = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(al,
                                    v->base.loc, scope, s2c(al, v_ext_name), ASRUtils::symbol_get_past_external(v),
                                    s2c(al, enum_ext_name), nullptr, 0, s2c(al, v_name), ASR::accessType::Public));
        scope->add_symbol(v_ext_name, v_ext);
    }

    return scope->get_symbol(v_ext_name);
}

class ReplaceArgVisitor: public ASR::BaseExprReplacer<ReplaceArgVisitor> {

    private:

    Allocator& al;

    SymbolTable* current_scope;

    ASR::Function_t* orig_func;

    Vec<ASR::call_arg_t>& orig_args;

    SetChar& current_function_dependencies;
    SetChar& current_module_dependencies;

    public:

    ReplaceArgVisitor(Allocator& al_, SymbolTable* current_scope_,
                      ASR::Function_t* orig_func_, Vec<ASR::call_arg_t>& orig_args_,
                      SetChar& current_function_dependencies_, SetChar& current_module_dependencies_) :
        al(al_), current_scope(current_scope_), orig_func(orig_func_),
        orig_args(orig_args_), current_function_dependencies(current_function_dependencies_),
        current_module_dependencies(current_module_dependencies_)
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
            std::string unique_name = current_scope->get_unique_name(f->m_name, false);
            Str s; s.from_str_view(unique_name);
            char *unique_name_c = s.c_str(al);
            LCOMPILERS_ASSERT(current_scope->get_symbol(unique_name) == nullptr);
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
            ASR::expr_t** current_expr_copy_ = current_expr;
            current_expr = &(x->m_args[i].m_value);
            replace_expr(x->m_args[i].m_value);
            current_expr = current_expr_copy_;
        }
        switch( x->m_type->type ) {
            case ASR::ttypeType::Character: {
                ASR::Character_t* char_type = ASR::down_cast<ASR::Character_t>(x->m_type);
                if( char_type->m_len_expr ) {
                    ASR::expr_t** current_expr_copy_ = current_expr;
                    current_expr = &(char_type->m_len_expr);
                    replace_expr(char_type->m_len_expr);
                    current_expr = current_expr_copy_;
                }
                break;
            }
            default:
                break;
        }
        current_function_dependencies.push_back(al, ASRUtils::symbol_name(new_es));
        ASRUtils::insert_module_dependency(new_es, al, current_module_dependencies);
        x->m_name = new_es;
    }

    void replace_Var(ASR::Var_t* x) {
        size_t arg_idx = 0;
        bool idx_found = false;
        // Finds the index of the argument to be used for substitution
        // Basically if we are calling maybe(string, ret_type=character(len=len(s)))
        // where string is a variable in current scope and s is one of the arguments
        // accepted by maybe i.e., maybe has a signature maybe(s). Then, we will
        // replace s with string. So, the call would become,
        // maybe(string, ret_type=character(len=len(string)))
        for( size_t j = 0; j < orig_func->n_args && !idx_found; j++ ) {
            if( ASR::is_a<ASR::Var_t>(*(orig_func->m_args[j])) ) {
                arg_idx = j;
                idx_found = ASR::down_cast<ASR::Var_t>(orig_func->m_args[j])->m_v == x->m_v;
            }
        }
        if( idx_found ) {
            LCOMPILERS_ASSERT(current_expr);
            *current_expr = orig_args[arg_idx].m_value;
        }
    }

};

// Finds the argument index that is equal to `v`, otherwise -1.
inline int64_t lookup_var_index(ASR::expr_t **args, size_t n_args, ASR::Var_t *v) {
    ASR::symbol_t *s = v->m_v;
    for (size_t i = 0; i < n_args; i++) {
        if (ASR::down_cast<ASR::Var_t>(args[i])->m_v == s) {
            return i;
        }
    }
    return -1;
}

class ExprStmtDuplicator: public ASR::BaseExprStmtDuplicator<ExprStmtDuplicator>
{
    public:

    ExprStmtDuplicator(Allocator &al): BaseExprStmtDuplicator(al) {}

};

class ReplaceWithFunctionParamVisitor: public ASR::BaseExprReplacer<ReplaceWithFunctionParamVisitor> {

    private:

    Allocator& al;

    ASR::expr_t** m_args;

    size_t n_args;

    public:

    ReplaceWithFunctionParamVisitor(Allocator& al_, ASR::expr_t** m_args_, size_t n_args_) :
        al(al_), m_args(m_args_), n_args(n_args_) {}

    void replace_Var(ASR::Var_t* x) {
        size_t arg_idx = 0;
        bool idx_found = false;
        std::string arg_name = ASRUtils::symbol_name(x->m_v);
        for( size_t j = 0; j < n_args && !idx_found; j++ ) {
            if( ASR::is_a<ASR::Var_t>(*(m_args[j])) ) {
                std::string arg_name_2 = std::string(ASRUtils::symbol_name(
                    ASR::down_cast<ASR::Var_t>(m_args[j])->m_v));
                arg_idx = j;
                idx_found = arg_name_2 == arg_name;
            }
        }

        if( idx_found ) {
            LCOMPILERS_ASSERT(current_expr);
            ASR::ttype_t* t_ = replace_args_with_FunctionParam(
                                ASRUtils::symbol_type(x->m_v));
            *current_expr = ASRUtils::EXPR(ASR::make_FunctionParam_t(
                                al, m_args[arg_idx]->base.loc, arg_idx,
                                t_, nullptr));
        }
    }

    ASR::ttype_t* replace_args_with_FunctionParam(ASR::ttype_t* t) {
        ASRUtils::ExprStmtDuplicator duplicator(al);
        duplicator.allow_procedure_calls = true;

        // We need to substitute all direct argument variable references with
        // FunctionParam.
        duplicator.success = true;
        t = duplicator.duplicate_ttype(t);
        LCOMPILERS_ASSERT(duplicator.success);
        replace_ttype(t);
        return t;
    }

};

inline ASR::asr_t* make_FunctionType_t_util(Allocator &al,
    const Location &a_loc, ASR::expr_t** a_args, size_t n_args,
    ASR::expr_t* a_return_var, ASR::abiType a_abi, ASR::deftypeType a_deftype,
    char* a_bindc_name, bool a_elemental, bool a_pure, bool a_module, bool a_inline,
    bool a_static, ASR::ttype_t** a_type_params, size_t n_type_params,
    ASR::symbol_t** a_restrictions, size_t n_restrictions, bool a_is_restriction) {
    Vec<ASR::ttype_t*> arg_types;
    arg_types.reserve(al, n_args);
    ReplaceWithFunctionParamVisitor replacer(al, a_args, n_args);
    for( size_t i = 0; i < n_args; i++ ) {
        // We need to substitute all direct argument variable references with
        // FunctionParam.
        ASR::ttype_t *t = replacer.replace_args_with_FunctionParam(
                            expr_type(a_args[i]));
        arg_types.push_back(al, t);
    }
    ASR::ttype_t* return_var_type = nullptr;
    if( a_return_var ) {
        return_var_type = replacer.replace_args_with_FunctionParam(
                            ASRUtils::expr_type(a_return_var));
    }

    LCOMPILERS_ASSERT(arg_types.size() == n_args);
    return ASR::make_FunctionType_t(
        al, a_loc, arg_types.p, arg_types.size(), return_var_type, a_abi, a_deftype,
        a_bindc_name, a_elemental, a_pure, a_module, a_inline,
        a_static, a_type_params, n_type_params, a_restrictions, n_restrictions,
        a_is_restriction);
}

inline ASR::asr_t* make_FunctionType_t_util(Allocator &al, const Location &a_loc,
    ASR::expr_t** a_args, size_t n_args, ASR::expr_t* a_return_var, ASR::FunctionType_t* ft) {
    return ASRUtils::make_FunctionType_t_util(al, a_loc, a_args, n_args, a_return_var,
        ft->m_abi, ft->m_deftype, ft->m_bindc_name, ft->m_elemental,
        ft->m_pure, ft->m_module, ft->m_inline, ft->m_static,
        ft->m_type_params, ft->n_type_params, ft->m_restrictions,
        ft->n_restrictions, ft->m_is_restriction);
}

inline ASR::asr_t* make_Function_t_util(Allocator& al, const Location& loc,
    SymbolTable* m_symtab, char* m_name, char** m_dependencies, size_t n_dependencies,
    ASR::expr_t** a_args, size_t n_args, ASR::stmt_t** m_body, size_t n_body,
    ASR::expr_t* m_return_var, ASR::abiType m_abi, ASR::accessType m_access,
    ASR::deftypeType m_deftype, char* m_bindc_name, bool m_elemental, bool m_pure,
    bool m_module, bool m_inline, bool m_static, ASR::ttype_t** m_type_params, size_t n_type_params,
    ASR::symbol_t** m_restrictions, size_t n_restrictions, bool m_is_restriction,
    bool m_deterministic, bool m_side_effect_free, char *m_c_header=nullptr) {
    ASR::ttype_t* func_type = ASRUtils::TYPE(ASRUtils::make_FunctionType_t_util(
        al, loc, a_args, n_args, m_return_var, m_abi, m_deftype, m_bindc_name,
        m_elemental, m_pure, m_module, m_inline, m_static, m_type_params, n_type_params,
        m_restrictions, n_restrictions, m_is_restriction));
    return ASR::make_Function_t(
        al, loc, m_symtab, m_name, func_type, m_dependencies, n_dependencies,
        a_args, n_args, m_body, n_body, m_return_var, m_access, m_deterministic,
        m_side_effect_free, m_c_header);
}

class SymbolDuplicator {

    private:

    Allocator& al;

    public:

    SymbolDuplicator(Allocator& al_):
    al(al_) {

    }

    void duplicate_SymbolTable(SymbolTable* symbol_table,
        SymbolTable* destination_symtab) {
        for( auto& item: symbol_table->get_scope() ) {
            duplicate_symbol(item.second, destination_symtab);
        }
    }

    void duplicate_symbol(ASR::symbol_t* symbol,
        SymbolTable* destination_symtab) {
        ASR::symbol_t* new_symbol = nullptr;
        std::string new_symbol_name = "";
        switch( symbol->type ) {
            case ASR::symbolType::Variable: {
                ASR::Variable_t* variable = ASR::down_cast<ASR::Variable_t>(symbol);
                new_symbol = duplicate_Variable(variable, destination_symtab);
                new_symbol_name = variable->m_name;
                break;
            }
            case ASR::symbolType::ExternalSymbol: {
                ASR::ExternalSymbol_t* external_symbol = ASR::down_cast<ASR::ExternalSymbol_t>(symbol);
                new_symbol = duplicate_ExternalSymbol(external_symbol, destination_symtab);
                new_symbol_name = external_symbol->m_name;
                break;
            }
            case ASR::symbolType::AssociateBlock: {
                ASR::AssociateBlock_t* associate_block = ASR::down_cast<ASR::AssociateBlock_t>(symbol);
                new_symbol = duplicate_AssociateBlock(associate_block, destination_symtab);
                new_symbol_name = associate_block->m_name;
                break;
            }
            case ASR::symbolType::Function: {
                ASR::Function_t* function = ASR::down_cast<ASR::Function_t>(symbol);
                new_symbol = duplicate_Function(function, destination_symtab);
                new_symbol_name = function->m_name;
                break;
            }
            case ASR::symbolType::Block: {
                ASR::Block_t* block = ASR::down_cast<ASR::Block_t>(symbol);
                new_symbol = duplicate_Block(block, destination_symtab);
                new_symbol_name = block->m_name;
                break;
            }
            default: {
                throw LCompilersException("Duplicating ASR::symbolType::" +
                        std::to_string(symbol->type) + " is not supported yet.");
            }
        }
        if( new_symbol ) {
            destination_symtab->add_symbol(new_symbol_name, new_symbol);
        }
    }

    ASR::symbol_t* duplicate_Variable(ASR::Variable_t* variable,
        SymbolTable* destination_symtab) {
        ExprStmtDuplicator node_duplicator(al);
        node_duplicator.success = true;
        ASR::expr_t* m_symbolic_value = node_duplicator.duplicate_expr(variable->m_symbolic_value);
        if( !node_duplicator.success ) {
            return nullptr;
        }
        node_duplicator.success = true;
        ASR::expr_t* m_value = node_duplicator.duplicate_expr(variable->m_value);
        if( !node_duplicator.success ) {
            return nullptr;
        }
        node_duplicator.success = true;
        ASR::ttype_t* m_type = node_duplicator.duplicate_ttype(variable->m_type);
        if( !node_duplicator.success ) {
            return nullptr;
        }
        return ASR::down_cast<ASR::symbol_t>(
            ASR::make_Variable_t(al, variable->base.base.loc, destination_symtab,
                variable->m_name, variable->m_dependencies, variable->n_dependencies,
                variable->m_intent, m_symbolic_value, m_value, variable->m_storage,
                m_type, variable->m_type_declaration, variable->m_abi, variable->m_access,
                variable->m_presence, variable->m_value_attr));
    }

    ASR::symbol_t* duplicate_ExternalSymbol(ASR::ExternalSymbol_t* external_symbol,
        SymbolTable* destination_symtab) {
            return ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
                al, external_symbol->base.base.loc, destination_symtab,
                external_symbol->m_name, external_symbol->m_external,
                external_symbol->m_module_name, external_symbol->m_scope_names,
                external_symbol->n_scope_names, external_symbol->m_original_name,
                external_symbol->m_access));
    }

    ASR::symbol_t* duplicate_AssociateBlock(ASR::AssociateBlock_t* associate_block,
        SymbolTable* destination_symtab) {
        SymbolTable* associate_block_symtab = al.make_new<SymbolTable>(destination_symtab);
        duplicate_SymbolTable(associate_block->m_symtab, associate_block_symtab);
        Vec<ASR::stmt_t*> new_body;
        new_body.reserve(al, associate_block->n_body);
        ASRUtils::ExprStmtDuplicator node_duplicator(al);
        node_duplicator.allow_procedure_calls = true;
        node_duplicator.allow_reshape = false;
        for( size_t i = 0; i < associate_block->n_body; i++ ) {
            node_duplicator.success = true;
            ASR::stmt_t* new_stmt = node_duplicator.duplicate_stmt(associate_block->m_body[i]);
            if( !node_duplicator.success ) {
                return nullptr;
            }
            new_body.push_back(al, new_stmt);
        }

        // node_duplicator_.allow_procedure_calls = true;

        return ASR::down_cast<ASR::symbol_t>(ASR::make_AssociateBlock_t(al,
                        associate_block->base.base.loc, associate_block_symtab,
                        associate_block->m_name, new_body.p, new_body.size()));
    }

    ASR::symbol_t* duplicate_Function(ASR::Function_t* function,
        SymbolTable* destination_symtab) {
        SymbolTable* function_symtab = al.make_new<SymbolTable>(destination_symtab);
        duplicate_SymbolTable(function->m_symtab, function_symtab);
        Vec<ASR::stmt_t*> new_body;
        new_body.reserve(al, function->n_body);
        ASRUtils::ExprStmtDuplicator node_duplicator(al);
        node_duplicator.allow_procedure_calls = true;
        node_duplicator.allow_reshape = false;
        for( size_t i = 0; i < function->n_body; i++ ) {
            node_duplicator.success = true;
            ASR::stmt_t* new_stmt = node_duplicator.duplicate_stmt(function->m_body[i]);
            if( !node_duplicator.success ) {
                return nullptr;
            }
            new_body.push_back(al, new_stmt);
        }

        Vec<ASR::expr_t*> new_args;
        new_args.reserve(al, function->n_args);
        for( size_t i = 0; i < function->n_args; i++ ) {
            node_duplicator.success = true;
            ASR::expr_t* new_arg = node_duplicator.duplicate_expr(function->m_args[i]);
            if (ASR::is_a<ASR::Var_t>(*new_arg)) {
                ASR::Var_t* var = ASR::down_cast<ASR::Var_t>(new_arg);
                if (ASR::is_a<ASR::Variable_t>(*(var->m_v))) {
                    ASR::Variable_t* variable = ASR::down_cast<ASR::Variable_t>(var->m_v);
                    ASR::symbol_t* arg_symbol = function_symtab->get_symbol(variable->m_name);
                    new_arg = ASRUtils::EXPR(make_Var_t(al, var->base.base.loc, arg_symbol));
                }
            }
            if( !node_duplicator.success ) {
                return nullptr;
            }
            new_args.push_back(al, new_arg);
        }

        ASR::expr_t* new_return_var = function->m_return_var;
        if( new_return_var ) {
            node_duplicator.success = true;
            new_return_var = node_duplicator.duplicate_expr(function->m_return_var);
            if( !node_duplicator.success ) {
                return nullptr;
            }
        }

        ASR::FunctionType_t* function_type = ASRUtils::get_FunctionType(function);

        return ASR::down_cast<ASR::symbol_t>(make_Function_t_util(al,
            function->base.base.loc, function_symtab, function->m_name,
            function->m_dependencies, function->n_dependencies, new_args.p,
            new_args.size(), new_body.p, new_body.size(), new_return_var,
            function_type->m_abi, function->m_access, function_type->m_deftype,
            function_type->m_bindc_name, function_type->m_elemental, function_type->m_pure,
            function_type->m_module, function_type->m_inline, function_type->m_static,
            function_type->m_type_params, function_type->n_type_params,
            function_type->m_restrictions, function_type->n_restrictions,
            function_type->m_is_restriction, function->m_deterministic,
            function->m_side_effect_free));
    }

    ASR::symbol_t* duplicate_Block(ASR::Block_t* block_t,
        SymbolTable* destination_symtab) {
        SymbolTable* block_symtab = al.make_new<SymbolTable>(destination_symtab);
        duplicate_SymbolTable(block_t->m_symtab, block_symtab);

        Vec<ASR::stmt_t*> new_body;
        new_body.reserve(al, block_t->n_body);
        ASRUtils::ExprStmtDuplicator node_duplicator(al);
        node_duplicator.allow_procedure_calls = true;
        node_duplicator.allow_reshape = false;
        for( size_t i = 0; i < block_t->n_body; i++ ) {
            node_duplicator.success = true;
            ASR::stmt_t* new_stmt = node_duplicator.duplicate_stmt(block_t->m_body[i]);
            if( !node_duplicator.success ) {
                return nullptr;
            }
            new_body.push_back(al, new_stmt);
        }

        return ASR::down_cast<ASR::symbol_t>(ASR::make_Block_t(al,
                block_t->base.base.loc, block_symtab, block_t->m_name,
                new_body.p, new_body.size()));
    }

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

static inline bool present(Vec<ASR::symbol_t*> &v, const ASR::symbol_t* name) {
    for (auto &a : v) {
        if (a == name) {
            return true;
        }
    }
    return false;
}

// Singleton LabelGenerator so that it generates
// unique labels for different statements, from
// wherever it is called (be it ASR passes, be it
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
            LCOMPILERS_ASSERT( node2label.find(node) == node2label.end() );
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
    // BindC interfaces already pass array by data pointer so we don't need to track
    // them and use extra variables for their dimensional information. Only those functions
    // need to be tracked which by default pass arrays by using descriptors.
    if ((ASRUtils::get_FunctionType(x)->m_abi == ASR::abiType::BindC
         || ASRUtils::get_FunctionType(x)->m_abi == ASR::abiType::BindPython)
        && ASRUtils::get_FunctionType(x)->m_deftype == ASR::deftypeType::Interface) {
        return false;
    }

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
        if( ASR::is_a<ASR::Class_t>(*typei) ||
            ASR::is_a<ASR::FunctionType_t>(*typei) ) {
            continue ;
        }
        int n_dims = ASRUtils::extract_dimensions_from_ttype(typei, dims);
        ASR::Variable_t* argi = ASRUtils::EXPR2VAR(x->m_args[i]);
        if( ASR::is_a<ASR::Pointer_t>(*argi->m_type) ) {
            return false;
        }

        // The following if check determines whether the i-th argument
        // can be called by just passing the data pointer and
        // dimensional information spearately via extra arguments.
        if( n_dims > 0 && ASRUtils::is_dimension_empty(dims, n_dims) &&
            (argi->m_intent == ASRUtils::intent_in ||
             argi->m_intent == ASRUtils::intent_out ||
             argi->m_intent == ASRUtils::intent_inout) &&
            !ASR::is_a<ASR::Allocatable_t>(*argi->m_type) &&
            !ASR::is_a<ASR::Struct_t>(*argi->m_type) &&
            !ASR::is_a<ASR::Character_t>(*argi->m_type)) {
            v.push_back(i);
        }
    }
    return v.size() > 0;
}

static inline ASR::expr_t* get_bound(ASR::expr_t* arr_expr, int dim,
                                     std::string bound, Allocator& al) {
    ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, arr_expr->base.loc, 4));
    ASR::expr_t* dim_expr = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arr_expr->base.loc,
                                                                       dim, int32_type));
    ASR::arrayboundType bound_type = ASR::arrayboundType::LBound;
    if( bound == "ubound" ) {
        bound_type = ASR::arrayboundType::UBound;
    }
    ASR::expr_t* bound_value = nullptr;
    ASR::dimension_t* arr_dims = nullptr;
    int arr_n_dims = ASRUtils::extract_dimensions_from_ttype(
        ASRUtils::expr_type(arr_expr), arr_dims);
    if( dim > arr_n_dims || dim < 1) {
        throw LCompilersException("Dimension " + std::to_string(dim) +
            " is invalid. Rank of the array, " + std::to_string(arr_n_dims));
    }
    dim = dim - 1;
    if( arr_dims[dim].m_start && arr_dims[dim].m_length ) {
        ASR::expr_t* arr_start = ASRUtils::expr_value(arr_dims[dim].m_start);
        ASR::expr_t* arr_length = ASRUtils::expr_value(arr_dims[dim].m_length);
        if( bound_type == ASR::arrayboundType::LBound &&
            ASRUtils::is_value_constant(arr_start) ) {
            int64_t const_lbound = -1;
            if( !ASRUtils::extract_value(arr_start, const_lbound) ) {
                LCOMPILERS_ASSERT(false);
            }
            bound_value = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                            al, arr_expr->base.loc, const_lbound, int32_type));
        } else if( bound_type == ASR::arrayboundType::UBound &&
            ASRUtils::is_value_constant(arr_start) &&
            ASRUtils::is_value_constant(arr_length) ) {
            int64_t const_lbound = -1;
            if( !ASRUtils::extract_value(arr_start, const_lbound) ) {
                LCOMPILERS_ASSERT(false);
            }
            int64_t const_length = -1;
            if( !ASRUtils::extract_value(arr_length, const_length) ) {
                LCOMPILERS_ASSERT(false);
            }
            bound_value = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                            al, arr_expr->base.loc,
                            const_lbound + const_length - 1, int32_type));
        }
    }
    return ASRUtils::EXPR(ASR::make_ArrayBound_t(al, arr_expr->base.loc, arr_expr, dim_expr,
                int32_type, bound_type, bound_value));
}

static inline ASR::asr_t* make_ArraySize_t_util(
    Allocator &al, const Location &a_loc, ASR::expr_t* a_v,
    ASR::expr_t* a_dim, ASR::ttype_t* a_type, ASR::expr_t* a_value) {
    if( ASR::is_a<ASR::ArrayPhysicalCast_t>(*a_v) ) {
        a_v = ASR::down_cast<ASR::ArrayPhysicalCast_t>(a_v)->m_arg;
    }

    return ASR::make_ArraySize_t(al, a_loc, a_v, a_dim, a_type, a_value);
}

static inline ASR::expr_t* get_size(ASR::expr_t* arr_expr, int dim,
                                    Allocator& al) {
    ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, arr_expr->base.loc, 4));
    ASR::expr_t* dim_expr = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arr_expr->base.loc, dim, int32_type));
    return ASRUtils::EXPR(ASRUtils::make_ArraySize_t_util(al, arr_expr->base.loc, arr_expr, dim_expr,
                                                int32_type, nullptr));
}

static inline ASR::expr_t* get_size(ASR::expr_t* arr_expr, Allocator& al) {
    ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, arr_expr->base.loc, 4));
    return ASRUtils::EXPR(ASRUtils::make_ArraySize_t_util(al, arr_expr->base.loc, arr_expr, nullptr, int32_type, nullptr));
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
    LCOMPILERS_ASSERT(ASR::is_a<ASR::EnumType_t>(*enum_type_cand));
    return ASR::down_cast<ASR::EnumType_t>(enum_type_cand);
}

static inline bool is_abstract_class_type(ASR::ttype_t* type) {
    type = ASRUtils::type_get_past_array(type);
    if( !ASR::is_a<ASR::Class_t>(*type) ) {
        return false;
    }
    ASR::Class_t* class_t = ASR::down_cast<ASR::Class_t>(type);
    return std::string( ASRUtils::symbol_name(
                ASRUtils::symbol_get_past_external(class_t->m_class_type))
                ) == "~abstract_type";
}

static inline void set_enum_value_type(ASR::enumtypeType &enum_value_type,
        SymbolTable *scope) {
    int8_t IntegerConsecutiveFromZero = 1;
    int8_t IntegerNotUnique = 0;
    int8_t IntegerUnique = 1;
    std::map<int64_t, int64_t> value2count;
    for( auto sym: scope->get_scope() ) {
        ASR::Variable_t* member_var = ASR::down_cast<ASR::Variable_t>(sym.second);
        ASR::expr_t* value = ASRUtils::expr_value(member_var->m_symbolic_value);
        int64_t value_int64 = -1;
        ASRUtils::extract_value(value, value_int64);
        if( value2count.find(value_int64) == value2count.end() ) {
            value2count[value_int64] = 0;
        }
        value2count[value_int64] += 1;
    }
    int64_t prev = -1;
    for( auto itr: value2count ) {
        if( itr.second > 1 ) {
            IntegerNotUnique = 1;
            IntegerUnique = 0;
            IntegerConsecutiveFromZero = 0;
            break ;
        }
        if( itr.first - prev != 1 ) {
            IntegerConsecutiveFromZero = 0;
        }
        prev = itr.first;
    }
    if( IntegerConsecutiveFromZero ) {
        if( value2count.find(0) == value2count.end() ) {
            IntegerConsecutiveFromZero = 0;
            IntegerUnique = 1;
        } else {
            IntegerUnique = 0;
        }
    }
    LCOMPILERS_ASSERT(IntegerConsecutiveFromZero + IntegerNotUnique + IntegerUnique == 1);
    if( IntegerConsecutiveFromZero ) {
        enum_value_type = ASR::enumtypeType::IntegerConsecutiveFromZero;
    } else if( IntegerNotUnique ) {
        enum_value_type = ASR::enumtypeType::IntegerNotUnique;
    } else if( IntegerUnique ) {
        enum_value_type = ASR::enumtypeType::IntegerUnique;
    }
}

class CollectIdentifiersFromASRExpression: public ASR::BaseWalkVisitor<CollectIdentifiersFromASRExpression> {
    private:

        Allocator& al;
        SetChar& identifiers;

    public:

        CollectIdentifiersFromASRExpression(Allocator& al_, SetChar& identifiers_) :
        al(al_), identifiers(identifiers_)
        {}

        void visit_Var(const ASR::Var_t& x) {
            identifiers.push_back(al, ASRUtils::symbol_name(x.m_v));
        }
};

static inline void collect_variable_dependencies(Allocator& al, SetChar& deps_vec,
    ASR::ttype_t* type=nullptr, ASR::expr_t* init_expr=nullptr,
    ASR::expr_t* value=nullptr) {
    ASRUtils::CollectIdentifiersFromASRExpression collector(al, deps_vec);
    if( init_expr ) {
        collector.visit_expr(*init_expr);
    }
    if( value ) {
        collector.visit_expr(*value);
    }
    if( type ) {
        collector.visit_ttype(*type);
    }
}

static inline int KMP_string_match(std::string &s_var, std::string &sub) {
    int str_len = s_var.size();
    int sub_len = sub.size();
    bool flag = 0;
    int res = -1;
    std::vector<int> lps(sub_len, 0);
    if (str_len == 0 || sub_len == 0) {
        res = (!sub_len || (sub_len == str_len))? 0: -1;
    } else {
        for(int i = 1, len = 0; i < sub_len;) {
            if (sub[i] == sub[len]) {
                lps[i++] = ++len;
            } else {
                if (len != 0) {
                    len = lps[len - 1];
                } else {
                    lps[i++] = 0;
                }
            }
        }
        for (int i = 0, j = 0; (str_len - i) >= (sub_len - j) && !flag;) {
            if (sub[j] == s_var[i]) {
                j++, i++;
            }
            if (j == sub_len) {
                res = i - j;
                flag = 1;
                j = lps[j - 1];
            } else if (i < str_len && sub[j] != s_var[i]) {
                if (j != 0) {
                    j = lps[j - 1];
                } else {
                    i = i + 1;
                }
            }
        }
    }
    return res;
}

static inline void visit_expr_list(Allocator &al, Vec<ASR::call_arg_t>& exprs,
        Vec<ASR::expr_t*>& exprs_vec) {
    LCOMPILERS_ASSERT(exprs_vec.reserve_called);
    for( size_t i = 0; i < exprs.n; i++ ) {
        exprs_vec.push_back(al, exprs[i].m_value);
    }
}

static inline void visit_expr_list(Allocator &al, Vec<ASR::expr_t *> exprs,
        Vec<ASR::call_arg_t>& exprs_vec) {
    LCOMPILERS_ASSERT(exprs_vec.reserve_called);
    for( size_t i = 0; i < exprs.n; i++ ) {
        ASR::call_arg_t arg;
        arg.loc = exprs[i]->base.loc;
        arg.m_value = exprs[i];
        exprs_vec.push_back(al, arg);
    }
}

class VerifyAbort {};

static inline void require_impl(bool cond, const std::string &error_msg,
    const Location &loc, diag::Diagnostics &diagnostics) {
    if (!cond) {
        diagnostics.message_label("ASR verify: " + error_msg,
            {loc}, "failed here",
            diag::Level::Error, diag::Stage::ASRVerify);
        throw VerifyAbort();
    }
}

static inline ASR::dimension_t* duplicate_dimensions(Allocator& al, ASR::dimension_t* m_dims, size_t n_dims) {
    Vec<ASR::dimension_t> dims;
    dims.reserve(al, n_dims);
    ASRUtils::ExprStmtDuplicator expr_duplicator(al);
    for (size_t i = 0; i < n_dims; i++) {
        ASR::expr_t* start = m_dims[i].m_start;
        if( start != nullptr ) {
            start = expr_duplicator.duplicate_expr(start);
        }
        ASR::expr_t* length = m_dims[i].m_length;
        if( length != nullptr ) {
            length = expr_duplicator.duplicate_expr(length);
        }
        ASR::dimension_t t;
        t.loc = m_dims[i].loc;
        t.m_start = start;
        t.m_length = length;
        dims.push_back(al, t);
    }
    return dims.p;
}

static inline bool is_allocatable(ASR::expr_t* expr) {
    return ASR::is_a<ASR::Allocatable_t>(*ASRUtils::expr_type(expr));
}

static inline ASR::asr_t* make_ArrayPhysicalCast_t_util(Allocator &al, const Location &a_loc,
    ASR::expr_t* a_arg, ASR::array_physical_typeType a_old, ASR::array_physical_typeType a_new,
    ASR::ttype_t* a_type, ASR::expr_t* a_value) {
    if( ASR::is_a<ASR::ArrayPhysicalCast_t>(*a_arg) ) {
        ASR::ArrayPhysicalCast_t* a_arg_ = ASR::down_cast<ASR::ArrayPhysicalCast_t>(a_arg);
        a_arg = a_arg_->m_arg;
        a_old = ASRUtils::extract_physical_type(ASRUtils::expr_type(a_arg_->m_arg));
    }

    LCOMPILERS_ASSERT(ASRUtils::extract_physical_type(ASRUtils::expr_type(a_arg)) == a_old);
    if( a_old == a_new ) {
        return (ASR::asr_t*) a_arg;
    }

    return ASR::make_ArrayPhysicalCast_t(al, a_loc, a_arg, a_old, a_new, a_type, a_value);
}

inline ASR::asr_t* make_ArrayConstant_t_util(Allocator &al, const Location &a_loc,
    ASR::expr_t** a_args, size_t n_args, ASR::ttype_t* a_type, ASR::arraystorageType a_storage_format) {
    if( !ASRUtils::is_array(a_type) ) {
        Vec<ASR::dimension_t> dims;
        dims.reserve(al, 1);
        ASR::dimension_t dim;
        dim.loc = a_loc;
        dim.m_length = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
            al, a_loc, n_args, ASRUtils::TYPE(ASR::make_Integer_t(al, a_loc, 4))));
        dim.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
            al, a_loc, 0, ASRUtils::TYPE(ASR::make_Integer_t(al, a_loc, 4))));
        dims.push_back(al, dim);
        a_type = ASRUtils::make_Array_t_util(al, dim.loc,
            a_type, dims.p, dims.size(), ASR::abiType::Source,
            false, ASR::array_physical_typeType::PointerToDataArray, true);
    } else if( ASR::is_a<ASR::Allocatable_t>(*a_type) ) {
        ASR::dimension_t* m_dims = nullptr;
        int n_dims = ASRUtils::extract_dimensions_from_ttype(a_type, m_dims);
        if( !ASRUtils::is_dimension_empty(m_dims, n_dims) ) {
            a_type = ASRUtils::duplicate_type_with_empty_dims(al, a_type);
        }
    }

    LCOMPILERS_ASSERT(ASRUtils::is_array(a_type));
    return ASR::make_ArrayConstant_t(al, a_loc, a_args, n_args, a_type, a_storage_format);
}

static inline void Call_t_body(Allocator& al, ASR::symbol_t* a_name,
    ASR::call_arg_t* a_args, size_t n_args, ASR::expr_t* a_dt, ASR::stmt_t** cast_stmt, bool implicit_argument_casting) {
    bool is_method = a_dt != nullptr;
    ASR::symbol_t* a_name_ = ASRUtils::symbol_get_past_external(a_name);
    ASR::FunctionType_t* func_type = nullptr;
    if( ASR::is_a<ASR::Function_t>(*a_name_) ) {
        func_type = ASR::down_cast<ASR::FunctionType_t>(
            ASR::down_cast<ASR::Function_t>(a_name_)->m_function_signature);
    } else if( ASR::is_a<ASR::Variable_t>(*a_name_) ) {
        func_type = ASR::down_cast<ASR::FunctionType_t>(
            ASR::down_cast<ASR::Variable_t>(a_name_)->m_type);
    } else if( ASR::is_a<ASR::ClassProcedure_t>(*a_name_) ) {
        ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(
            ASRUtils::symbol_get_past_external(
            ASR::down_cast<ASR::ClassProcedure_t>(a_name_)->m_proc));
        func_type = ASR::down_cast<ASR::FunctionType_t>(func->m_function_signature);
    } else {
        LCOMPILERS_ASSERT(false);
    }

    for( size_t i = 0; i < n_args; i++ ) {
        if( a_args[i].m_value == nullptr ||
            ASR::is_a<ASR::IntegerBOZ_t>(*a_args[i].m_value) ) {
            continue;
        }
        ASR::expr_t* arg = a_args[i].m_value;
        ASR::ttype_t* arg_type = ASRUtils::type_get_past_allocatable(
            ASRUtils::type_get_past_pointer(ASRUtils::expr_type(arg)));
        ASR::ttype_t* orig_arg_type = ASRUtils::type_get_past_allocatable(
            ASRUtils::type_get_past_pointer(func_type->m_arg_types[i + is_method]));
        if( !ASRUtils::is_intrinsic_symbol(a_name_) &&
            !(ASR::is_a<ASR::Class_t>(*ASRUtils::type_get_past_array(arg_type)) ||
              ASR::is_a<ASR::Class_t>(*ASRUtils::type_get_past_array(orig_arg_type))) &&
            !(ASR::is_a<ASR::FunctionType_t>(*ASRUtils::type_get_past_array(arg_type)) ||
              ASR::is_a<ASR::FunctionType_t>(*ASRUtils::type_get_past_array(orig_arg_type))) &&
            a_dt == nullptr ) {
            if (implicit_argument_casting && !ASRUtils::check_equal_type(arg_type, orig_arg_type)) {
                if (ASR::is_a<ASR::Function_t>(*a_name_)) {
                    // get current_scope
                    SymbolTable* current_scope = nullptr;
                    std::string sym_name = "";
                    if (ASR::is_a<ASR::Var_t>(*arg)) {
                        ASR::Var_t* arg_var = ASR::down_cast<ASR::Var_t>(arg);
                        if (ASR::is_a<ASR::Variable_t>(*(arg_var->m_v))) {
                            ASR::Variable_t* arg_var_ = ASR::down_cast<ASR::Variable_t>(arg_var->m_v);
                            current_scope = arg_var_->m_parent_symtab;
                            sym_name = arg_var_->m_name;
                        }
                    } else if (ASR::is_a<ASR::ArrayItem_t>(*arg)) {
                        ASR::expr_t* arg_expr = ASR::down_cast<ASR::ArrayItem_t>(arg)->m_v;
                        ASR::Variable_t* arg_var = ASRUtils::EXPR2VAR(arg_expr);
                        current_scope = arg_var->m_parent_symtab;
                        sym_name = arg_var->m_name;
                    }
                    if (current_scope) {
                        ASR::Array_t* orig_arg_array_t = nullptr;
                        ASR::Array_t* arg_array_t = nullptr;
                        if (orig_arg_type->type == ASR::ttypeType::Array) {
                            orig_arg_array_t = ASR::down_cast<ASR::Array_t>(orig_arg_type);
                            Vec<ASR::dimension_t> dim;
                            dim.reserve(al, 1);
                            ASR::dimension_t dim_;
                            dim_.m_start = nullptr;
                            dim_.m_length = nullptr;
                            dim_.loc = arg->base.loc;
                            dim.push_back(al, dim_);
                            arg_array_t = (ASR::Array_t*) ASR::make_Array_t(al, arg->base.loc, orig_arg_array_t->m_type,
                            dim.p, dim.size(), ASR::array_physical_typeType::DescriptorArray);
                        }
                        ASR::ttype_t* arg_array_type = (ASR::ttype_t*) arg_array_t;
                        ASR::ttype_t* pointer_type = ASRUtils::TYPE(ASR::make_Pointer_t(al, orig_arg_type->base.loc, arg_array_type));

                        std::string cast_sym_name = current_scope->get_unique_name(sym_name + "_cast", false);
                        ASR::asr_t* cast_ = ASR::make_Variable_t(al, arg->base.loc,
                                        current_scope, s2c(al, cast_sym_name), nullptr,
                                        0, ASR::intentType::Local, nullptr, nullptr,
                                        ASR::storage_typeType::Default, pointer_type, nullptr,
                                        ASR::abiType::Source, ASR::accessType::Public,
                                        ASR::presenceType::Required, false);

                        ASR::symbol_t* cast_sym = ASR::down_cast<ASR::symbol_t>(cast_);
                        current_scope->add_symbol(cast_sym_name, cast_sym);

                        ASR::expr_t* cast_expr = ASRUtils::EXPR(ASR::make_Var_t(al,arg->base.loc, cast_sym));

                        ASR::ttype_t* pointer_type_ = ASRUtils::TYPE(ASR::make_Pointer_t(al, arg->base.loc, ASRUtils::type_get_past_array(arg_type)));

                        ASR::asr_t* get_pointer = ASR::make_GetPointer_t(al, arg->base.loc, arg, pointer_type_, nullptr);

                        ASR::ttype_t* cptr = ASRUtils::TYPE(ASR::make_CPtr_t(al, arg->base.loc));

                        ASR::asr_t* pointer_to_cptr = ASR::make_PointerToCPtr_t(al, arg->base.loc, ASRUtils::EXPR(get_pointer), cptr, nullptr);

                        Vec<ASR::expr_t*> args_;
                        args_.reserve(al, 1);

                        ASR::ttype_t *int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, arg->base.loc, 4));
                        ASR::expr_t* thousand = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arg->base.loc, 1000, int32_type));
                        ASR::expr_t* one = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arg->base.loc, 1, int32_type));

                        args_.push_back(al, thousand);

                        Vec<ASR::dimension_t> dim;
                        dim.reserve(al, 1);
                        ASR::dimension_t dim_;
                        dim_.m_start = one;
                        dim_.m_length = one;
                        dim_.loc = arg->base.loc;
                        dim.push_back(al, dim_);

                        ASR::ttype_t* array_type = ASRUtils::TYPE(ASR::make_Array_t(al, arg->base.loc, int32_type, dim.p, dim.size(), ASR::array_physical_typeType::FixedSizeArray));
                        ASR::asr_t* array_constant = ASRUtils::make_ArrayConstant_t_util(al, arg->base.loc, args_.p, args_.size(), array_type, ASR::arraystorageType::ColMajor);

                        ASR::asr_t* cptr_to_pointer = ASR::make_CPtrToPointer_t(al, arg->base.loc, ASRUtils::EXPR(pointer_to_cptr), cast_expr, ASRUtils::EXPR(array_constant), nullptr);
                        *cast_stmt = ASRUtils::STMT(cptr_to_pointer);

                        ASR::asr_t* array_t = nullptr;

                        Vec<ASR::dimension_t> dims;
                        dims.reserve(al, 1);
                        ASR::dimension_t dims_;
                        dims_.m_start = nullptr;
                        dims_.m_length = nullptr;
                        dim_.loc = arg->base.loc;
                        dims.push_back(al, dims_);

                        array_t = ASR::make_Array_t(al, arg->base.loc, orig_arg_array_t->m_type,
                        dims.p, dims.size(), ASR::array_physical_typeType::PointerToDataArray);
                        ASR::ttype_t* pointer_array_t = ASRUtils::TYPE(ASR::make_Pointer_t(al, arg->base.loc, ASRUtils::TYPE(array_t)));
                        ASR::asr_t* array_physical_cast = ASR::make_ArrayPhysicalCast_t(al, arg->base.loc, cast_expr, ASR::array_physical_typeType::DescriptorArray,
                                                        ASR::array_physical_typeType::PointerToDataArray, pointer_array_t, nullptr);

                        a_args[i].m_value = ASRUtils::EXPR(array_physical_cast);
                    }
                }
            } else {
                // TODO: Make this a regular error. The current asr_utils.h is
                // not setup to return errors, so we need to refactor things.
                // For now we just do an assert.
                /*TODO: Remove this if check once intrinsic procedures are implemented correctly*/
                LCOMPILERS_ASSERT_MSG( ASRUtils::check_equal_type(arg_type, orig_arg_type),
                    "ASRUtils::check_equal_type(" + ASRUtils::get_type_code(arg_type) + ", " +
                        ASRUtils::get_type_code(orig_arg_type) + ")");
            }
        }
        if( ASRUtils::is_array(arg_type) && ASRUtils::is_array(orig_arg_type) ) {
            ASR::Array_t* arg_array_t = ASR::down_cast<ASR::Array_t>(ASRUtils::type_get_past_const(arg_type));
            ASR::Array_t* orig_arg_array_t = ASR::down_cast<ASR::Array_t>(ASRUtils::type_get_past_const(orig_arg_type));
            if( arg_array_t->m_physical_type != orig_arg_array_t->m_physical_type ) {
                ASR::call_arg_t physical_cast_arg;
                physical_cast_arg.loc = arg->base.loc;
                Vec<ASR::dimension_t>* dimensions = nullptr;
                Vec<ASR::dimension_t> dimension_;
                if( ASRUtils::is_fixed_size_array(orig_arg_array_t->m_dims, orig_arg_array_t->n_dims) ) {
                    dimension_.reserve(al, orig_arg_array_t->n_dims);
                    dimension_.from_pointer_n_copy(al, orig_arg_array_t->m_dims, orig_arg_array_t->n_dims);
                    dimensions = &dimension_;
                }

                physical_cast_arg.m_value = ASRUtils::EXPR(ASRUtils::make_ArrayPhysicalCast_t_util(
                    al, arg->base.loc, arg, arg_array_t->m_physical_type, orig_arg_array_t->m_physical_type,
                    ASRUtils::duplicate_type(al, ASRUtils::expr_type(arg), dimensions, orig_arg_array_t->m_physical_type, true),
                    nullptr));
                a_args[i] = physical_cast_arg;
            }
        }
    }
}

static inline ASR::asr_t* make_FunctionCall_t_util(
    Allocator &al, const Location &a_loc, ASR::symbol_t* a_name,
    ASR::symbol_t* a_original_name, ASR::call_arg_t* a_args, size_t n_args,
    ASR::ttype_t* a_type, ASR::expr_t* a_value, ASR::expr_t* a_dt) {

    Call_t_body(al, a_name, a_args, n_args, a_dt, nullptr, false);

    return ASR::make_FunctionCall_t(al, a_loc, a_name, a_original_name,
            a_args, n_args, a_type, a_value, a_dt);
}

static inline ASR::asr_t* make_SubroutineCall_t_util(
    Allocator &al, const Location &a_loc, ASR::symbol_t* a_name,
    ASR::symbol_t* a_original_name, ASR::call_arg_t* a_args, size_t n_args,
    ASR::expr_t* a_dt, ASR::stmt_t** cast_stmt, bool implicit_argument_casting) {

    Call_t_body(al, a_name, a_args, n_args, a_dt, cast_stmt, implicit_argument_casting);

    return ASR::make_SubroutineCall_t(al, a_loc, a_name, a_original_name,
            a_args, n_args, a_dt);
}

static inline ASR::expr_t* cast_to_descriptor(Allocator& al, ASR::expr_t* arg) {
    ASR::ttype_t* arg_type = ASRUtils::expr_type(arg);
    ASR::Array_t* arg_array_t = ASR::down_cast<ASR::Array_t>(
        ASRUtils::type_get_past_pointer(
            ASRUtils::type_get_past_allocatable(arg_type)));
    if( arg_array_t->m_physical_type != ASR::array_physical_typeType::DescriptorArray ) {
        return ASRUtils::EXPR(ASRUtils::make_ArrayPhysicalCast_t_util(
            al, arg->base.loc, arg, arg_array_t->m_physical_type,
            ASR::array_physical_typeType::DescriptorArray,
            ASRUtils::duplicate_type(al, ASRUtils::expr_type(arg),
                nullptr, ASR::array_physical_typeType::DescriptorArray, true),
            nullptr));
    }
    return arg;
}

static inline ASR::asr_t* make_IntrinsicFunction_t_util(
    Allocator &al, const Location &a_loc, int64_t a_intrinsic_id,
    ASR::expr_t** a_args, size_t n_args, int64_t a_overload_id,
    ASR::ttype_t* a_type, ASR::expr_t* a_value) {

    for( size_t i = 0; i < n_args; i++ ) {
        if( a_args[i] == nullptr ||
            ASR::is_a<ASR::IntegerBOZ_t>(*a_args[i]) ) {
            continue;
        }
        ASR::expr_t* arg = a_args[i];
        ASR::ttype_t* arg_type = ASRUtils::type_get_past_allocatable(
            ASRUtils::type_get_past_pointer(ASRUtils::expr_type(arg)));

        if( ASRUtils::is_array(arg_type) ) {
            a_args[i] = cast_to_descriptor(al, arg);
        }
    }

    return ASR::make_IntrinsicFunction_t(al, a_loc, a_intrinsic_id,
        a_args, n_args, a_overload_id, a_type, a_value);
}

static inline ASR::asr_t* make_Associate_t_util(
    Allocator &al, const Location &a_loc,
    ASR::expr_t* a_target, ASR::expr_t* a_value) {
    ASR::ttype_t* target_type = ASRUtils::expr_type(a_target);
    ASR::ttype_t* value_type = ASRUtils::expr_type(a_value);
    if( ASRUtils::is_array(target_type) && ASRUtils::is_array(value_type) ) {
        ASR::array_physical_typeType target_ptype = ASRUtils::extract_physical_type(target_type);
        ASR::array_physical_typeType value_ptype = ASRUtils::extract_physical_type(value_type);
        if( target_ptype != value_ptype ) {
            a_value = ASRUtils::EXPR(ASRUtils::make_ArrayPhysicalCast_t_util(al, a_loc, a_value,
                        value_ptype, target_ptype, ASRUtils::duplicate_type(al,
                        value_type, nullptr, target_ptype, true), nullptr));
        }
    }
    return ASR::make_Associate_t(al, a_loc, a_target, a_value);
}

static inline ASR::asr_t* make_ArrayItem_t_util(Allocator &al, const Location &a_loc,
    ASR::expr_t* a_v, ASR::array_index_t* a_args, size_t n_args, ASR::ttype_t* a_type,
    ASR::arraystorageType a_storage_format, ASR::expr_t* a_value) {
    if( ASR::is_a<ASR::ArrayPhysicalCast_t>(*a_v) ) {
        a_v = ASR::down_cast<ASR::ArrayPhysicalCast_t>(a_v)->m_arg;
    }

    return ASR::make_ArrayItem_t(al, a_loc, a_v, a_args,
        n_args, a_type, a_storage_format, a_value);
}

inline ASR::ttype_t* make_Pointer_t_util(Allocator& al, const Location& loc, ASR::ttype_t* type) {
    if( ASRUtils::is_array(type) ) {
        ASR::dimension_t* m_dims = nullptr;
        int n_dims = ASRUtils::extract_dimensions_from_ttype(type, m_dims);
        if( !ASRUtils::is_dimension_empty(m_dims, n_dims) ) {
            type = ASRUtils::duplicate_type_with_empty_dims(al, type);
        }
    }

    return ASRUtils::TYPE(ASR::make_Pointer_t(al, loc, type));
}

} // namespace ASRUtils

} // namespace LCompilers

#endif // LFORTRAN_ASR_UTILS_H
