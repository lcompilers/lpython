#ifndef LFORTRAN_ASR_UTILS_H
#define LFORTRAN_ASR_UTILS_H

#include <functional>
#include <map>
#include <limits>

#include <libasr/assert.h>
#include <libasr/asr.h>
#include <libasr/string_utils.h>
#include <libasr/utils.h>
#include <libasr/casting_utils.h>
#include <libasr/asr_expr_stmt_duplicator_visitor.h>
#include <libasr/asr_expr_base_replacer_visitor.h>
#include <libasr/asr_stmt_base_replacer_visitor.h>
#include <libasr/asr_expr_call_replacer_visitor.h>
#include <libasr/asr_expr_type_visitor.h>
#include <libasr/asr_expr_value_visitor.h>
#include <libasr/asr_walk_visitor.h>

#include <complex>

#define ADD_ASR_DEPENDENCIES(current_scope, final_sym, current_function_dependencies) ASR::symbol_t* asr_owner_sym = nullptr; \
    if(current_scope->asr_owner && ASR::is_a<ASR::symbol_t>(*current_scope->asr_owner) ) { \
        asr_owner_sym = ASR::down_cast<ASR::symbol_t>(current_scope->asr_owner); \
    } \
    SymbolTable* temp_scope = current_scope; \
    if (asr_owner_sym && temp_scope->get_counter() != ASRUtils::symbol_parent_symtab(final_sym)->get_counter() && \
            !ASR::is_a<ASR::ExternalSymbol_t>(*final_sym) && !ASR::is_a<ASR::Variable_t>(*final_sym)) { \
        if (ASR::is_a<ASR::AssociateBlock_t>(*asr_owner_sym) || ASR::is_a<ASR::Block_t>(*asr_owner_sym)) { \
            temp_scope = temp_scope->parent; \
            if (temp_scope->get_counter() != ASRUtils::symbol_parent_symtab(final_sym)->get_counter()) { \
                current_function_dependencies.push_back(al, ASRUtils::symbol_name(final_sym)); \
            } \
        } else { \
            current_function_dependencies.push_back(al, ASRUtils::symbol_name(final_sym)); \
        } \
    } \

#define ADD_ASR_DEPENDENCIES_WITH_NAME(current_scope, final_sym, current_function_dependencies, dep_name) ASR::symbol_t* asr_owner_sym = nullptr; \
    if(current_scope->asr_owner && ASR::is_a<ASR::symbol_t>(*current_scope->asr_owner) ) { \
        asr_owner_sym = ASR::down_cast<ASR::symbol_t>(current_scope->asr_owner); \
    } \
    SymbolTable* temp_scope = current_scope; \
    if (asr_owner_sym && temp_scope->get_counter() != ASRUtils::symbol_parent_symtab(final_sym)->get_counter() && \
            !ASR::is_a<ASR::ExternalSymbol_t>(*final_sym) && !ASR::is_a<ASR::Variable_t>(*final_sym)) { \
        if (ASR::is_a<ASR::AssociateBlock_t>(*asr_owner_sym) || ASR::is_a<ASR::Block_t>(*asr_owner_sym)) { \
            temp_scope = temp_scope->parent; \
            if (temp_scope->get_counter() != ASRUtils::symbol_parent_symtab(final_sym)->get_counter()) { \
                current_function_dependencies.push_back(al, dep_name); \
            } \
        } else { \
            current_function_dependencies.push_back(al, dep_name); \
        } \
    } \

namespace LCompilers  {

    namespace ASRUtils  {

ASR::symbol_t* import_class_procedure(Allocator &al, const Location& loc,
        ASR::symbol_t* original_sym, SymbolTable *current_scope);

ASR::asr_t* make_Binop_util(Allocator &al, const Location& loc, ASR::binopType binop,
                        ASR::expr_t* lexpr, ASR::expr_t* rexpr, ASR::ttype_t* ttype);

ASR::asr_t* make_Cmpop_util(Allocator &al, const Location& loc, ASR::cmpopType cmpop,
                        ASR::expr_t* lexpr, ASR::expr_t* rexpr, ASR::ttype_t* ttype);

inline bool check_equal_type(ASR::ttype_t* x, ASR::ttype_t* y, bool check_for_dimensions=false);

static inline std::string type_to_str_python(const ASR::ttype_t *t, bool for_error_message=true);

static inline std::string extract_real(const char *s) {
    // TODO: this is inefficient. We should
    // convert this in the tokenizer where we know most information
    std::string x = s;
    x = replace(x, "d", "e");
    x = replace(x, "D", "E");
    return x;
}

static inline double extract_real_4(const char *s) {
    std::string r_str = ASRUtils::extract_real(s);
    float r = std::strtof(r_str.c_str(), nullptr);
    return r;
}

static inline double extract_real_8(const char *s) {
    std::string r_str = ASRUtils::extract_real(s);
    return std::strtod(r_str.c_str(), nullptr);
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

static inline ASR::symbol_t* symbol_get_past_ClassProcedure(ASR::symbol_t* f){
    LCOMPILERS_ASSERT(f != nullptr);
    if(ASR::is_a<ASR::ClassProcedure_t>(*f)){
        ASR::symbol_t* func = ASR::down_cast<ASR::ClassProcedure_t>(f)->m_proc;
        LCOMPILERS_ASSERT(func != nullptr);
        return func;
    }
    return f;
}

/*
Returns true, when the symbol 'f' is a procedure variable
*/
static inline bool is_symbol_procedure_variable(ASR::symbol_t* f) {
    if (ASR::is_a<ASR::Variable_t>(*f)) {
        ASR::Variable_t* v = ASR::down_cast<ASR::Variable_t>(f);
        return ASR::is_a<ASR::FunctionType_t>(*v->m_type);
    }
    return false;
}

template <typename T>
Location get_vec_loc(const Vec<T>& args) {
    LCOMPILERS_ASSERT(args.size() > 0);
    Location args_loc;
    args_loc.first = args[0].loc.first;
    args_loc.last = args[args.size() - 1].loc.last;
    return args_loc;
}

static inline ASR::FunctionType_t* get_FunctionType(ASR::symbol_t* x) {
    ASR::symbol_t* a_name_ = ASRUtils::symbol_get_past_external(x);
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
    return func_type;
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
        return type_get_past_allocatable(e->m_type);
    } else {
        return f;
    }
}

static inline ASR::expr_t* get_past_array_physical_cast(ASR::expr_t* x) {
    if( !ASR::is_a<ASR::ArrayPhysicalCast_t>(*x) ) {
        return x;
    }
    return ASR::down_cast<ASR::ArrayPhysicalCast_t>(x)->m_arg;
}

static inline ASR::expr_t* get_past_array_broadcast(ASR::expr_t* x) {
    if( !ASR::is_a<ASR::ArrayBroadcast_t>(*x) ) {
        return x;
    }
    return ASR::down_cast<ASR::ArrayBroadcast_t>(x)->m_array;
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

static inline ASR::ttype_t* extract_type(ASR::ttype_t *f) {
    return type_get_past_array(
            type_get_past_allocatable(
                type_get_past_pointer(f)));
}

static inline ASR::ttype_t* type_get_past_allocatable_pointer(ASR::ttype_t* f) {
    return type_get_past_allocatable(
        type_get_past_pointer(f)
    );
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
        case ASR::ttypeType::String: {
            return ASR::down_cast<ASR::String_t>(type)->m_kind;
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
        default : {
            return -1;
        }
    }
}

static inline void set_kind_to_ttype_t(ASR::ttype_t* type, int kind) {
    if (type == nullptr) {
        return;
    }
    switch (type->type) {
        case ASR::ttypeType::Array: {
            set_kind_to_ttype_t(ASR::down_cast<ASR::Array_t>(type)->m_type, kind);
            break;
        }
        case ASR::ttypeType::Integer : {
            ASR::down_cast<ASR::Integer_t>(type)->m_kind = kind;
            break;
        }
        case ASR::ttypeType::UnsignedInteger : {
            ASR::down_cast<ASR::UnsignedInteger_t>(type)->m_kind = kind;
            break;
        }
        case ASR::ttypeType::Real : {
            ASR::down_cast<ASR::Real_t>(type)->m_kind = kind;
            break;
        }
        case ASR::ttypeType::Complex: {
            ASR::down_cast<ASR::Complex_t>(type)->m_kind = kind;
            break;
        }
        case ASR::ttypeType::String: {
            ASR::down_cast<ASR::String_t>(type)->m_kind = kind;
            break;
        }
        case ASR::ttypeType::Logical: {
            ASR::down_cast<ASR::Logical_t>(type)->m_kind = kind;
            break;
        }
        case ASR::ttypeType::Pointer: {
            set_kind_to_ttype_t(ASR::down_cast<ASR::Pointer_t>(type)->m_type, kind);
            break;
        }
        case ASR::ttypeType::Allocatable: {
            set_kind_to_ttype_t(ASR::down_cast<ASR::Allocatable_t>(type)->m_type, kind);
            break;
        }
        default : {
            return;
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
        case ASR::symbolType::Enum: {
            return ASR::down_cast<ASR::Enum_t>(f)->m_type;
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

static inline std::string symbol_type_name(const ASR::symbol_t &s)
{
    switch( s.type ) {
        case ASR::symbolType::Program: return "Program";
        case ASR::symbolType::Module: return "Module";
        case ASR::symbolType::Function: return "Function";
        case ASR::symbolType::GenericProcedure: return "GenericProcedure";
        case ASR::symbolType::CustomOperator: return "CustomOperator";
        case ASR::symbolType::ExternalSymbol: return "ExternalSymbol";
        case ASR::symbolType::Struct: return "Struct";
        case ASR::symbolType::Enum: return "Enum";
        case ASR::symbolType::Union: return "Union";
        case ASR::symbolType::Variable: return "Variable";
        case ASR::symbolType::Class: return "Class";
        case ASR::symbolType::ClassProcedure: return "ClassProcedure";
        case ASR::symbolType::AssociateBlock: return "AssociateBlock";
        case ASR::symbolType::Block: return "Block";
        case ASR::symbolType::Requirement: return "Requirement";
        case ASR::symbolType::Template: return "Template";
        default: {
            LCOMPILERS_ASSERT(false);
        }
    }
    return "";
}

static inline ASR::abiType symbol_abi(const ASR::symbol_t *f)
{
    switch( f->type ) {
        case ASR::symbolType::Variable: {
            return ASR::down_cast<ASR::Variable_t>(f)->m_abi;
        }
        case ASR::symbolType::Enum: {
            return ASR::down_cast<ASR::Enum_t>(f)->m_abi;
        }
        case ASR::symbolType::ExternalSymbol: {
            return symbol_abi(ASR::down_cast<ASR::ExternalSymbol_t>(f)->m_external);
        }
        case ASR::symbolType::Function: {
            return ASRUtils::get_FunctionType(*ASR::down_cast<ASR::Function_t>(f))->m_abi;
        }
        default: {
            throw LCompilersException("Cannot return ABI of, " +
                                    std::to_string(f->type) + " symbol.");
        }
    }
    return ASR::abiType::Source;
}

static inline ASR::ttype_t* get_contained_type(ASR::ttype_t* asr_type, int overload=0) {
    switch( asr_type->type ) {
        case ASR::ttypeType::List: {
            return ASR::down_cast<ASR::List_t>(asr_type)->m_type;
        }
        case ASR::ttypeType::Set: {
            return ASR::down_cast<ASR::Set_t>(asr_type)->m_type;
        }
        case ASR::ttypeType::Dict: {
            switch( overload ) {
                case 0:
                    return ASR::down_cast<ASR::Dict_t>(asr_type)->m_key_type;
                case 1:
                    return ASR::down_cast<ASR::Dict_t>(asr_type)->m_value_type;
                default:
                    return asr_type;
            }
        }
        case ASR::ttypeType::EnumType: {
            ASR::EnumType_t* enum_asr = ASR::down_cast<ASR::EnumType_t>(asr_type);
            ASR::Enum_t* enum_type = ASR::down_cast<ASR::Enum_t>(enum_asr->m_enum_type);
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
                    ASRUtils::type_to_str_python(e) + " type.");
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
        case ASR::exprType::ComplexIm: {
            return ASRUtils::expr_abi(ASR::down_cast<ASR::ComplexIm_t>(e)->m_arg);
        }
        case ASR::exprType::ComplexRe: {
            return ASRUtils::expr_abi(ASR::down_cast<ASR::ComplexRe_t>(e)->m_arg);
        }
        case ASR::exprType::ArrayPhysicalCast: {
            return ASRUtils::expr_abi(ASR::down_cast<ASR::ArrayPhysicalCast_t>(e)->m_arg);
        }
        default:
            throw LCompilersException(std::string("Cannot extract the ABI of ") +
                "ASR::exprType::" + std::to_string(e->type) + " expression.");
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
        case ASR::symbolType::Struct: {
            return ASR::down_cast<ASR::Struct_t>(f)->m_name;
        }
        case ASR::symbolType::Enum: {
            return ASR::down_cast<ASR::Enum_t>(f)->m_name;
        }
        case ASR::symbolType::Union: {
            return ASR::down_cast<ASR::Union_t>(f)->m_name;
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

static inline bool get_class_proc_nopass_val(ASR::symbol_t* func_sym) {
    func_sym = ASRUtils::symbol_get_past_external(func_sym);
    bool nopass = false;
    if (ASR::is_a<ASR::ClassProcedure_t>(*func_sym)) {
        nopass = ASR::down_cast<ASR::ClassProcedure_t>(func_sym)->m_is_nopass;
    }
    return nopass;
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

static inline std::string type_to_str_fortran(const ASR::ttype_t *t)
{
    switch (t->type) {
        case ASR::ttypeType::Integer: {
            return "integer";
        }
        case ASR::ttypeType::UnsignedInteger: {
            return "unsigned integer";
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
        case ASR::ttypeType::String: {
            return "string";
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
        case ASR::ttypeType::StructType: {
            return ASRUtils::symbol_name(ASR::down_cast<ASR::StructType_t>(t)->m_derived_type);
        }
        case ASR::ttypeType::ClassType: {
            return ASRUtils::symbol_name(ASR::down_cast<ASR::ClassType_t>(t)->m_class_type);
        }
        case ASR::ttypeType::UnionType: {
            return "union";
        }
        case ASR::ttypeType::CPtr: {
            return "type(c_ptr)";
        }
        case ASR::ttypeType::Pointer: {
            return type_to_str_fortran(ASRUtils::type_get_past_pointer(
                        const_cast<ASR::ttype_t*>(t))) + " pointer";
        }
        case ASR::ttypeType::Allocatable: {
            return type_to_str_fortran(ASRUtils::type_get_past_allocatable(
                        const_cast<ASR::ttype_t*>(t))) + " allocatable";
        }
        case ASR::ttypeType::Array: {
            ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(t);
            std::string res = type_to_str_fortran(array_t->m_type);
            encode_dimensions(array_t->n_dims, res, false);
            return res;
        }
        case ASR::ttypeType::TypeParameter: {
            ASR::TypeParameter_t* tp = ASR::down_cast<ASR::TypeParameter_t>(t);
            return tp->m_param;
        }
        case ASR::ttypeType::SymbolicExpression: {
            return "symbolic expression";
        }
        case ASR::ttypeType::FunctionType: {
            ASR::FunctionType_t* ftp = ASR::down_cast<ASR::FunctionType_t>(t);
            std::string result = "(";
            for( size_t i = 0; i < ftp->n_arg_types; i++ ) {
                result += type_to_str_fortran(ftp->m_arg_types[i]) + ", ";
            }
            result += "return_type: ";
            if( ftp->m_return_var_type ) {
                result += type_to_str_fortran(ftp->m_return_var_type);
            } else {
                result += "void";
            }
            result += ")";
            return result;
        }
        default : throw LCompilersException("Not implemented " + ASRUtils::type_to_str_python(t) + ".");
    }
}

static inline std::string type_to_str_with_type(const ASR::ttype_t *t) {
    std::string type = type_to_str_fortran(t);
    std::string kind = std::to_string(extract_kind_from_ttype_t(t));
    return type + "(" + kind + ")";
}

static inline std::string type_to_str_with_substitution(const ASR::ttype_t *t,
    std::map<std::string, ASR::ttype_t*> subs)
{
    if (ASR::is_a<ASR::TypeParameter_t>(*t)) {
        ASR::TypeParameter_t* t_tp = ASR::down_cast<ASR::TypeParameter_t>(t);
        t = subs[t_tp->m_param];
    }
    switch (t->type) {
        case ASR::ttypeType::Pointer: {
            return type_to_str_with_substitution(ASRUtils::type_get_past_pointer(
                        const_cast<ASR::ttype_t*>(t)), subs) + " pointer";
        }
        case ASR::ttypeType::Allocatable: {
            return type_to_str_with_substitution(ASRUtils::type_get_past_allocatable(
                        const_cast<ASR::ttype_t*>(t)), subs) + " allocatable";
        }
        case ASR::ttypeType::Array: {
            ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(t);
            std::string res = type_to_str_with_substitution(array_t->m_type, subs);
            encode_dimensions(array_t->n_dims, res, false);
            return res;
        }
        case ASR::ttypeType::FunctionType: {
            ASR::FunctionType_t* ftp = ASR::down_cast<ASR::FunctionType_t>(t);
            std::string result = "(";
            for( size_t i = 0; i < ftp->n_arg_types; i++ ) {
                result += type_to_str_with_substitution(ftp->m_arg_types[i], subs) + ", ";
            }
            result += "return_type: ";
            if( ftp->m_return_var_type ) {
                result += type_to_str_with_substitution(ftp->m_return_var_type, subs);
            } else {
                result += "void";
            }
            result += ")";
            return result;
        }
        default : return type_to_str_fortran(t);
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
        case ASR::symbolType::Struct: {
            ASR::Struct_t* sym = ASR::down_cast<ASR::Struct_t>(f);
            return std::make_pair(sym->m_dependencies, sym->n_dependencies);
        }
        case ASR::symbolType::Enum: {
            ASR::Enum_t* sym = ASR::down_cast<ASR::Enum_t>(f);
            return std::make_pair(sym->m_dependencies, sym->n_dependencies);
        }
        case ASR::symbolType::Union: {
            ASR::Union_t* sym = ASR::down_cast<ASR::Union_t>(f);
            return std::make_pair(sym->m_dependencies, sym->n_dependencies);
        }
        default : throw LCompilersException("Not implemented");
    }
}

static inline bool is_present_in_current_scope(ASR::ExternalSymbol_t* external_symbol, SymbolTable* current_scope) {
        SymbolTable* scope = external_symbol->m_parent_symtab;
        while (scope != nullptr) {
            if (scope->get_counter() == current_scope->get_counter()) {
                return true;
            }
            scope = scope->parent;
        }
        return false;
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
        case ASR::symbolType::Struct: {
            return ASR::down_cast<ASR::Struct_t>(f)->m_symtab->parent;
        }
        case ASR::symbolType::Enum: {
            return ASR::down_cast<ASR::Enum_t>(f)->m_symtab->parent;
        }
        case ASR::symbolType::Union: {
            return ASR::down_cast<ASR::Union_t>(f)->m_symtab->parent;
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
        case ASR::symbolType::Struct: {
            return ASR::down_cast<ASR::Struct_t>(f)->m_symtab;
        }
        case ASR::symbolType::Enum: {
            return ASR::down_cast<ASR::Enum_t>(f)->m_symtab;
        }
        case ASR::symbolType::Union: {
            return ASR::down_cast<ASR::Union_t>(f)->m_symtab;
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
    if( s->asr_owner == nullptr ||
        !ASR::is_a<ASR::symbol_t>(*s->asr_owner) ) {
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
            return nullptr;
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
    if( ASR::is_a<ASR::Struct_t>(*v_orig) ) {
        ASR::Module_t* der_type_module = ASRUtils::get_sym_module0(v_orig);
        return (der_type_module && std::string(der_type_module->m_name) ==
                "lfortran_intrinsic_iso_c_binding" &&
                der_type_module->m_intrinsic &&
                v_name == "c_ptr");
    }
    return false;
}

static inline bool is_c_funptr(ASR::symbol_t* v, std::string v_name="") {
    if( v_name == "" ) {
        v_name = ASRUtils::symbol_name(v);
    }
    ASR::symbol_t* v_orig = ASRUtils::symbol_get_past_external(v);
    if( ASR::is_a<ASR::Struct_t>(*v_orig) ) {
        ASR::Module_t* der_type_module = ASRUtils::get_sym_module0(v_orig);
        return (der_type_module && std::string(der_type_module->m_name) ==
                "lfortran_intrinsic_iso_c_binding" &&
                der_type_module->m_intrinsic &&
                v_name == "c_funptr");
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

/*

This function determines if a given expression represents a variable according
to the rules of Fortran variable definitions. Here's an illustration of declaration's:

```fortran
    CHARACTER(len=5) x, y(3)
    INTEGER, PARAMETER :: i
    TYPE (EMPLOYEE) e
        INTEGER age
    END TYPE EMPLOYEE
```

then 'x', 'y', 'y(1)', 'y(1:2)', 'e % age' are all variables, while 'i' isn't

TODO: this definitely needs some extensions to include others as "variable"'s
*/
static inline bool is_variable(ASR::expr_t* a_value) {
    if (a_value == nullptr) {
        return false;
    }
    switch (a_value->type) {
        case ASR::exprType::ArrayItem: {
            ASR::ArrayItem_t* a_item = ASR::down_cast<ASR::ArrayItem_t>(a_value);
            ASR::expr_t* array_expr = a_item->m_v;
            ASR::Variable_t* array_var = ASRUtils::EXPR2VAR(array_expr);
            // a constant array's item isn't a "variable"
            return array_var->m_storage != ASR::storage_typeType::Parameter;
        }
        case ASR::exprType::Var: {
            ASR::Variable_t* variable_t = ASRUtils::EXPR2VAR(a_value);
            return variable_t->m_storage != ASR::storage_typeType::Parameter;
        }
        case ASR::exprType::StringItem:
        case ASR::exprType::StringSection:
        case ASR::exprType::ArraySection:
        case ASR::exprType::StructInstanceMember: {
            return true;
        }
        default: {
            return false;
        }
    }
}

static inline bool is_value_constant(ASR::expr_t *a_value) {
    if( a_value == nullptr ) {
        return false;
    }
    switch ( a_value->type ) {
        case ASR::exprType::IntegerConstant:
        case ASR::exprType::UnsignedIntegerConstant:
        case ASR::exprType::RealConstant:
        case ASR::exprType::ComplexConstant:
        case ASR::exprType::LogicalConstant:
        case ASR::exprType::ImpliedDoLoop:
        case ASR::exprType::PointerNullConstant:
        case ASR::exprType::ArrayConstant:
        case ASR::exprType::StringConstant:
        case ASR::exprType::StructConstant: {
            return true;
        }
        case ASR::exprType::RealBinOp:
        case ASR::exprType::IntegerUnaryMinus:
        case ASR::exprType::RealUnaryMinus:
        case ASR::exprType::IntegerBinOp:
        case ASR::exprType::ArrayConstructor:
        case ASR::exprType::StringLen: {
            return is_value_constant(expr_value(a_value));
        } case ASR::exprType::ListConstant: {
            ASR::ListConstant_t* list_constant = ASR::down_cast<ASR::ListConstant_t>(a_value);
            for( size_t i = 0; i < list_constant->n_args; i++ ) {
                if( !ASRUtils::is_value_constant(list_constant->m_args[i]) &&
                    !ASRUtils::is_value_constant(ASRUtils::expr_value(list_constant->m_args[i])) ) {
                    return false;
                }
            }
            return true;
        } case ASR::exprType::IntrinsicElementalFunction: {
            ASR::IntrinsicElementalFunction_t* intrinsic_elemental_function =
                ASR::down_cast<ASR::IntrinsicElementalFunction_t>(a_value);

            if (ASRUtils::is_value_constant(intrinsic_elemental_function->m_value)) {
                return true;
            }

            for( size_t i = 0; i < intrinsic_elemental_function->n_args; i++ ) {
                if( !ASRUtils::is_value_constant(intrinsic_elemental_function->m_args[i]) ) {
                    return false;
                }
            }
            return true;
        } case ASR::exprType::IntrinsicArrayFunction: {
            ASR::IntrinsicArrayFunction_t* intrinsic_array_function =
                ASR::down_cast<ASR::IntrinsicArrayFunction_t>(a_value);

            if (ASRUtils::is_value_constant(intrinsic_array_function->m_value)) {
                return true;
            }

            for( size_t i = 0; i < intrinsic_array_function->n_args; i++ ) {
                if( !ASRUtils::is_value_constant(intrinsic_array_function->m_args[i]) ) {
                    return false;
                }
            }
            return true;
        } case ASR::exprType::FunctionCall: {
            ASR::FunctionCall_t* func_call_t = ASR::down_cast<ASR::FunctionCall_t>(a_value);
            if( !ASRUtils::is_intrinsic_symbol(ASRUtils::symbol_get_past_external(func_call_t->m_name)) ) {
                return false;
            }

            ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(
                ASRUtils::symbol_get_past_external(func_call_t->m_name));
            for( size_t i = 0; i < func_call_t->n_args; i++ ) {
                if (func_call_t->m_args[i].m_value == nullptr &&
                    ASRUtils::EXPR2VAR(func->m_args[i])->m_presence == ASR::presenceType::Optional) {
                    continue;
                }
                if( !ASRUtils::is_value_constant(func_call_t->m_args[i].m_value) ) {
                    return false;
                }
            }
            return true;
        } case ASR::exprType::ArrayBroadcast: {
            ASR::ArrayBroadcast_t* array_broadcast = ASR::down_cast<ASR::ArrayBroadcast_t>(a_value);
            return is_value_constant(array_broadcast->m_value);
        } case ASR::exprType::StructInstanceMember: {
            ASR::StructInstanceMember_t*
                struct_member_t = ASR::down_cast<ASR::StructInstanceMember_t>(a_value);
            return is_value_constant(struct_member_t->m_v);
        } case ASR::exprType::Var: {
            ASR::Var_t* var_t = ASR::down_cast<ASR::Var_t>(a_value);
            if( ASR::is_a<ASR::Variable_t>(*ASRUtils::symbol_get_past_external(var_t->m_v)) ) {
                ASR::Variable_t* variable_t = ASR::down_cast<ASR::Variable_t>(
                    ASRUtils::symbol_get_past_external(var_t->m_v));
                return variable_t->m_storage == ASR::storage_typeType::Parameter;
            } else if(ASR::is_a<ASR::Function_t>(*ASRUtils::symbol_get_past_external(var_t->m_v))){
                return true;
            } else {
                return false;
            }
        } case ASR::exprType::Cast: {
            ASR::Cast_t* cast_t = ASR::down_cast<ASR::Cast_t>(a_value);
            return is_value_constant(cast_t->m_arg);
        } case ASR::exprType::ArrayReshape: {
            ASR::ArrayReshape_t*
                array_reshape = ASR::down_cast<ASR::ArrayReshape_t>(a_value);
            return is_value_constant(array_reshape->m_array) && is_value_constant(array_reshape->m_shape);
        } case ASR::exprType::ArrayIsContiguous: {
            ASR::ArrayIsContiguous_t*
                array_is_contiguous = ASR::down_cast<ASR::ArrayIsContiguous_t>(a_value);
            return is_value_constant(array_is_contiguous->m_array);
        } case ASR::exprType::ArrayPhysicalCast: {
            ASR::ArrayPhysicalCast_t*
                array_physical_t = ASR::down_cast<ASR::ArrayPhysicalCast_t>(a_value);
            return is_value_constant(array_physical_t->m_arg);
        } case ASR::exprType::StructConstructor: {
            ASR::StructConstructor_t* struct_type_constructor =
                ASR::down_cast<ASR::StructConstructor_t>(a_value);
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
        } default: {
            return false;
        }
    }
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
static inline bool all_args_evaluated(const Vec<ASR::expr_t*> &args, bool ignore_null=false) {
    for (auto &a : args) {
        if (ignore_null && !a) continue;
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
    if ( ASR::is_a<ASR::ComplexConstructor_t>(*value_expr) ) {
        value_expr = ASR::down_cast<ASR::ComplexConstructor_t>(value_expr)->m_value;
        if (!value_expr) {
            return false;
        }
    }
    if( !ASR::is_a<ASR::ComplexConstant_t>(*value_expr) ) {
        return false;
    }

    ASR::ComplexConstant_t* value_const = ASR::down_cast<ASR::ComplexConstant_t>(value_expr);
    value = std::complex(value_const->m_re, value_const->m_im);
    return true;
}

static inline bool extract_string_value(ASR::expr_t* value_expr,
    std::string& value) {
    if( !is_value_constant(value_expr) ) {
        return false;
    }
    switch (value_expr->type)
    {
        case ASR::exprType::StringConstant: {
            ASR::StringConstant_t* const_string = ASR::down_cast<ASR::StringConstant_t>(value_expr);
            value = std::string(const_string->m_s);
            break;
        }
        case ASR::exprType::Var: {
            ASR::Variable_t* var = EXPR2VAR(value_expr);
            if (var->m_storage == ASR::storage_typeType::Parameter
                    && !extract_string_value(var->m_value, value)) {
                return false;
            }
            break;
        }
        case ASR::exprType::FunctionCall: {
            ASR::FunctionCall_t* func_call = ASR::down_cast<ASR::FunctionCall_t>(value_expr);
            if (!extract_string_value(func_call->m_value, value)) {
                return false;
            }
            break;
        }
        default:
            return false;
    }
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
        case ASR::exprType::Var: {
            ASR::Variable_t* var = EXPR2VAR(value_expr);
            if (var->m_storage == ASR::storage_typeType::Parameter
                    && !extract_value(var->m_value, value)) {
                return false;
            }
            break;
        }
        case ASR::exprType::IntegerUnaryMinus:
        case ASR::exprType::RealUnaryMinus:
        case ASR::exprType::FunctionCall:
        case ASR::exprType::IntegerBinOp:
        case ASR::exprType::StringLen: {
            if (!extract_value(expr_value(value_expr), value)) {
                return false;
            }
            break;
        }
        default:
            return false;
    }
    return true;
}

static inline std::string extract_dim_value(ASR::expr_t* dim) {
    int64_t length_dim = 0;
    if( dim == nullptr ||
        !ASRUtils::extract_value(ASRUtils::expr_value(dim), length_dim)) {
        return ":";
    }

    return std::to_string(length_dim);
}

static inline std::int64_t extract_dim_value_int(ASR::expr_t* dim) {
    int64_t length_dim = 0;
    if( dim == nullptr ||
        !ASRUtils::extract_value(ASRUtils::expr_value(dim), length_dim)) {
        return -1;
    }

    return length_dim;
}

static inline std::string type_encode_dims(size_t n_dims, ASR::dimension_t* m_dims )
{
    std::string dims_str = "[";
    for( size_t i = 0; i < n_dims; i++ ) {
        ASR::dimension_t dim = m_dims[i];
        dims_str += extract_dim_value(dim.m_length);
        if (i + 1 < n_dims) {
            dims_str += ",";
        }
    }
    dims_str += "]";
    return dims_str;
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
        case ASR::ttypeType::String: {
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
        case ASR::ttypeType::StructType: {
            ASR::StructType_t* d = ASR::down_cast<ASR::StructType_t>(t);
            if( ASRUtils::symbol_get_past_external(d->m_derived_type) ) {
                res = symbol_name(ASRUtils::symbol_get_past_external(d->m_derived_type));
            } else {
                res = symbol_name(d->m_derived_type);
            }
            break;
        }
        case ASR::ttypeType::ClassType: {
            ASR::ClassType_t* d = ASR::down_cast<ASR::ClassType_t>(t);
            if( ASRUtils::symbol_get_past_external(d->m_class_type) ) {
                res = symbol_name(ASRUtils::symbol_get_past_external(d->m_class_type));
            } else {
                res = symbol_name(d->m_class_type);
            }
            break;
        }
        case ASR::ttypeType::UnionType: {
            ASR::UnionType_t* d = ASR::down_cast<ASR::UnionType_t>(t);
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
        case ASR::ttypeType::SymbolicExpression: {
            return "S";
        }
        case ASR::ttypeType::TypeParameter: {
            ASR::TypeParameter_t *tp = ASR::down_cast<ASR::TypeParameter_t>(t);
            return tp->m_param;
        }
        default: {
            throw LCompilersException("Type encoding not implemented for "
                                      + ASRUtils::type_to_str_python(t));
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

static inline std::string type_to_str_python(const ASR::ttype_t *t, bool for_error_message)
{
    switch (t->type) {
        case ASR::ttypeType::Array: {
            ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(t);
            std::string res = type_to_str_python(array_t->m_type, for_error_message);
            std::string dim_info = type_encode_dims(array_t->n_dims, array_t->m_dims);
            res += dim_info;
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
        case ASR::ttypeType::String: {
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
        case ASR::ttypeType::StructType: {
            ASR::StructType_t* d = ASR::down_cast<ASR::StructType_t>(t);
            return "struct " + std::string(symbol_name(d->m_derived_type));
        }
        case ASR::ttypeType::EnumType: {
            ASR::EnumType_t* d = ASR::down_cast<ASR::EnumType_t>(t);
            return "enum " + std::string(symbol_name(d->m_enum_type));
        }
        case ASR::ttypeType::UnionType: {
            ASR::UnionType_t* d = ASR::down_cast<ASR::UnionType_t>(t);
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
    return ((ASR::is_a<ASR::String_t>(*type) || ASR::is_a<ASR::Tuple_t>(*type)
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
            throw LCompilersException("get_constant_zero_with_given_type: Not implemented " + ASRUtils::type_to_str_python(asr_type));
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
            throw LCompilersException("get_constant_one_with_given_type: Not implemented " + ASRUtils::type_to_str_python(asr_type));
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
                case 1: val = std::numeric_limits<int8_t>::min()+1; break;
                case 2: val = std::numeric_limits<int16_t>::min()+1; break;
                case 4: val = std::numeric_limits<int32_t>::min()+1; break;
                case 8: val = std::numeric_limits<int64_t>::min()+1; break;
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
            throw LCompilersException("get_minimum_value_with_given_type: Not implemented " + ASRUtils::type_to_str_python(asr_type));
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
            throw LCompilersException("get_maximum_value_with_given_type: Not implemented " + ASRUtils::type_to_str_python(asr_type));
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
    for (auto &a : unit.m_symtab->get_scope()) {
        if (ASR::is_a<ASR::Program_t>(*a.second)) return true;
    }
    return false;
}

static inline bool global_function_present(const ASR::TranslationUnit_t &unit)
{
    bool contains_global_function = false;

    for (auto &a : unit.m_symtab->get_scope()) {
        if (ASR::is_a<ASR::Function_t>(*a.second)) {
            contains_global_function = true;
        } else if (ASR::is_a<ASR::Module_t>(*a.second)) {
            // If the ASR contains a module, then the global
            // function is not the only symbol present.
            return false;
        }
    }

    return contains_global_function;
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

static inline bool is_external_sym_changed(ASR::symbol_t* original_sym, ASR::symbol_t* external_sym) {
    if (!ASR::is_a<ASR::Function_t>(*original_sym) || !ASR::is_a<ASR::Function_t>(*external_sym)) {
        return false;
    }
    ASR::Function_t* original_func = ASR::down_cast<ASR::Function_t>(original_sym);
    ASR::Function_t* external_func = ASR::down_cast<ASR::Function_t>(external_sym);
    bool same_number_of_args = original_func->n_args == external_func->n_args;
    // TODO: Check if the arguments are the same
    return !(same_number_of_args);
}

void update_call_args(Allocator &al, SymbolTable *current_scope, bool implicit_interface,
        std::map<std::string, ASR::symbol_t*> changed_external_function_symbol);


ASR::Module_t* extract_module(const ASR::TranslationUnit_t &m);

ASR::Module_t* load_module(Allocator &al, SymbolTable *symtab,
                            const std::string &module_name,
                            const Location &loc, bool intrinsic,
                            LCompilers::PassOptions& pass_options,
                            bool run_verify,
                            const std::function<void (const std::string &, const Location &)> err,
                            LCompilers::LocationManager &lm);

ASR::TranslationUnit_t* find_and_load_module(Allocator &al, const std::string &msym,
                                                SymbolTable &symtab, bool intrinsic,
                                                LCompilers::PassOptions& pass_options,
                                                LCompilers::LocationManager &lm);

void set_intrinsic(ASR::TranslationUnit_t* trans_unit);

static inline bool is_const(ASR::expr_t *x) {
    if (ASR::is_a<ASR::Var_t>(*x)) {
        ASR::Var_t* v = ASR::down_cast<ASR::Var_t>(x);
        ASR::symbol_t* sym = ASRUtils::symbol_get_past_external(v->m_v);
        if (sym && ASR::is_a<ASR::Variable_t>(*sym)) {
            ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(sym);
            return var->m_storage == ASR::storage_typeType::Parameter;
        }
    }
    return false;
}

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
                      SymbolTable* curr_scope, ASR::Struct_t* left_struct=nullptr);

bool use_overloaded(ASR::expr_t* left, ASR::expr_t* right,
                    ASR::cmpopType op, std::string& intrinsic_op_name,
                    SymbolTable* curr_scope, ASR::asr_t*& asr,
                    Allocator &al, const Location& loc,
                    SetChar& current_function_dependencies,
                    SetChar& current_module_dependencies,
                    const std::function<void (const std::string &, const Location &)> err);

bool is_op_overloaded(ASR::cmpopType op, std::string& intrinsic_op_name,
                      SymbolTable* curr_scope, ASR::Struct_t *left_struct);

bool use_overloaded_assignment(ASR::expr_t* target, ASR::expr_t* value,
                               SymbolTable* curr_scope, ASR::asr_t*& asr,
                               Allocator &al, const Location& loc,
                               SetChar& current_function_dependencies,
                               SetChar& /*current_module_dependencies*/,
                               const std::function<void (const std::string &, const Location &)> err);

bool use_overloaded_file_read_write(std::string &read_write, Vec<ASR::expr_t*> args,
                               SymbolTable* curr_scope, ASR::asr_t*& asr,
                               Allocator &al, const Location& loc,
                               SetChar& current_function_dependencies,
                               SetChar& /*current_module_dependencies*/,
                               const std::function<void (const std::string &, const Location &)> err);

void set_intrinsic(ASR::symbol_t* sym);

void get_sliced_indices(ASR::ArraySection_t* arr_sec, std::vector<size_t> &sliced_indices);

static inline bool is_pointer(ASR::ttype_t *x) {
    return ASR::is_a<ASR::Pointer_t>(*x);
}

static inline bool is_integer(ASR::ttype_t &x) {
    // return ASR::is_a<ASR::Integer_t>(
    //     *type_get_past_const(
    //         type_get_past_array(
    //         type_get_past_allocatable(
    //             type_get_past_pointer(
    //                 type_get_past_const(&x))))));
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
    // return ASR::is_a<ASR::Real_t>(
    //     *type_get_past_const(
    //         type_get_past_array(
    //             type_get_past_allocatable(
    //                 type_get_past_pointer(&x)))));
    return ASR::is_a<ASR::Real_t>(
        *type_get_past_array(
            type_get_past_allocatable(
                type_get_past_pointer(&x))));
}

static inline bool is_character(ASR::ttype_t &x) {
    return ASR::is_a<ASR::String_t>(
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

template <int32_t kind>
static inline bool is_complex(ASR::ttype_t &x) {
    return is_complex(x) && ASRUtils::extract_kind_from_ttype_t(&x) == kind;
}


static inline bool is_logical(ASR::ttype_t &x) {
    return ASR::is_a<ASR::Logical_t>(
        *type_get_past_array(
            type_get_past_allocatable(
                type_get_past_pointer(&x))));
}

static inline bool is_struct(ASR::ttype_t& x) {
    return ASR::is_a<ASR::StructType_t>(
        *extract_type(&x));
}

// Checking if the ttype 't' is a type parameter
static inline bool is_type_parameter(ASR::ttype_t &x) {
    switch (x.type) {
        case ASR::ttypeType::List: {
            ASR::List_t *list_type = ASR::down_cast<ASR::List_t>(type_get_past_pointer(&x));
            return is_type_parameter(*list_type->m_type);
        }
        case ASR::ttypeType::Array: {
            ASR::Array_t *arr_type = ASR::down_cast<ASR::Array_t>(type_get_past_pointer(&x));
            return is_type_parameter(*arr_type->m_type);
        }
        default : return ASR::is_a<ASR::TypeParameter_t>(*type_get_past_pointer(&x));
    }
}

// Checking if the symbol 'x' is a virtual function defined inside a requirement
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

// Checking if the symbol 'x' is a generic function defined inside a template
static inline bool is_generic_function(ASR::symbol_t *x) {
    ASR::symbol_t* x2 = symbol_get_past_external(x);
    switch (x2->type) {
        case ASR::symbolType::Function: {
            if (is_requirement_function(x2)) {
                return false;
            }
            ASR::Function_t *func = ASR::down_cast<ASR::Function_t>(x2);
            ASR::FunctionType_t *func_type
                = ASR::down_cast<ASR::FunctionType_t>(func->m_function_signature);
            for (size_t i=0; i<func_type->n_arg_types; i++) {
                if (is_type_parameter(*func_type->m_arg_types[i])) {
                    return true;
                }
            }
            return func_type->m_return_var_type
                   && is_type_parameter(*func_type->m_return_var_type);
        }
        default: return false;
    }
}

// Checking if the string `arg_name` corresponds to one of the arguments of the template `x`
static inline bool is_template_arg(ASR::symbol_t *x, std::string arg_name) {
    switch (x->type) {
        case ASR::symbolType::Template: {
            ASR::Template_t *t = ASR::down_cast<ASR::Template_t>(x);
            for (size_t i=0; i < t->n_args; i++) {
                std::string arg = t->m_args[i];
                if (arg.compare(arg_name) == 0) {
                    return true;
                }
            }
            break;
        }
        default: {
            return false;
        }
    }
    return false;
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

inline size_t extract_dimensions_from_ttype(ASR::ttype_t *x,
                                         ASR::dimension_t*& m_dims) {
    size_t n_dims {};
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
        case ASR::ttypeType::SymbolicExpression:
        case ASR::ttypeType::Integer:
        case ASR::ttypeType::UnsignedInteger:
        case ASR::ttypeType::Real:
        case ASR::ttypeType::Complex:
        case ASR::ttypeType::String:
        case ASR::ttypeType::Logical:
        case ASR::ttypeType::StructType:
        case ASR::ttypeType::EnumType:
        case ASR::ttypeType::UnionType:
        case ASR::ttypeType::ClassType:
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
            throw LCompilersException("Not implemented " + ASRUtils::type_to_str_python(x) + ".");
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
        if( (m_dims[i].m_length == nullptr) ||
            !ASRUtils::extract_value(ASRUtils::expr_value(m_dims[i].m_length), dim_size) ) {
            return -1;
        }
        array_size *= dim_size;
    }
    return array_size;
}

static inline int64_t get_fixed_size_of_ArraySection(ASR::ArraySection_t* x) {
    if (x->n_args == 0) {
        return 0;
    }
    int64_t array_size = 1;
    for (size_t i = 0; i < x->n_args; i++) {
        if (x->m_args[i].m_left && x->m_args[i].m_right && ASRUtils::is_value_constant(x->m_args[i].m_right) &&
            ASRUtils::is_value_constant(x->m_args[i].m_left)) {
            ASR::IntegerConstant_t* start = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(x->m_args[i].m_left));
            ASR::IntegerConstant_t* end = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(x->m_args[i].m_right));
            array_size = array_size * (end->m_n - start->m_n + 1);
        } else {
            return -1;
        }
    }
    return array_size;
}

static inline int64_t get_fixed_size_of_array(ASR::ttype_t* type) {
    ASR::dimension_t* m_dims = nullptr;
    size_t n_dims = ASRUtils::extract_dimensions_from_ttype(type, m_dims);
    return ASRUtils::get_fixed_size_of_array(m_dims, n_dims);
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

static inline bool is_dimension_empty(ASR::ttype_t* type) {
    ASR::dimension_t* dims = nullptr;
    size_t n = ASRUtils::extract_dimensions_from_ttype(type, dims);
    return is_dimension_empty(dims, n);
}

static inline bool is_only_upper_bound_empty(ASR::dimension_t& dim) {
    return (dim.m_start != nullptr && dim.m_length == nullptr);
}

inline bool is_array(ASR::ttype_t *x) {
    ASR::dimension_t* dims = nullptr;
    return extract_dimensions_from_ttype(x, dims) > 0;
}

class ExprDependentOnlyOnArguments: public ASR::BaseWalkVisitor<ExprDependentOnlyOnArguments> {

    public:

        bool is_dependent_only_on_argument;

        ExprDependentOnlyOnArguments(): is_dependent_only_on_argument(false)
        {}

        void visit_Var(const ASR::Var_t& x) {
            if( ASR::is_a<ASR::Variable_t>(*x.m_v) ) {
                ASR::Variable_t* x_m_v = ASR::down_cast<ASR::Variable_t>(x.m_v);
                if ( ASRUtils::is_array(x_m_v->m_type) ) {
                    is_dependent_only_on_argument = is_dependent_only_on_argument && ASRUtils::is_arg_dummy(x_m_v->m_intent);
                } else {
                    is_dependent_only_on_argument = is_dependent_only_on_argument && (x_m_v->m_intent == ASR::intentType::In);
                }
            } else {
                is_dependent_only_on_argument = false;
            }
        }
};

static inline bool is_dimension_dependent_only_on_arguments(ASR::dimension_t* m_dims, size_t n_dims) {
    ExprDependentOnlyOnArguments visitor;
    for( size_t i = 0; i < n_dims; i++ ) {
        visitor.is_dependent_only_on_argument = true;
        if( m_dims[i].m_length == nullptr ) {
            return false;
        }
        visitor.visit_expr(*m_dims[i].m_length);
        if( !visitor.is_dependent_only_on_argument ) {
            return false;
        }
    }
    return true;
}

static inline bool is_dimension_dependent_only_on_arguments(ASR::ttype_t* type) {
    ASR::dimension_t* m_dims;
    size_t n_dims = ASRUtils::extract_dimensions_from_ttype(type, m_dims);
    return is_dimension_dependent_only_on_arguments(m_dims, n_dims);
}

static inline bool is_binop_expr(ASR::expr_t* x) {
    switch( x->type ) {
        case ASR::exprType::IntegerBinOp:
        case ASR::exprType::RealBinOp:
        case ASR::exprType::ComplexBinOp:
        case ASR::exprType::LogicalBinOp:
        case ASR::exprType::UnsignedIntegerBinOp:
        case ASR::exprType::IntegerCompare:
        case ASR::exprType::RealCompare:
        case ASR::exprType::ComplexCompare:
        case ASR::exprType::LogicalCompare:
        case ASR::exprType::UnsignedIntegerCompare:
        case ASR::exprType::StringCompare: {
            return true;
        }
        default: {
            return false;
        }
    }
}

static inline bool is_unaryop_expr(ASR::expr_t* x) {
    switch( x->type ) {
        case ASR::exprType::IntegerUnaryMinus:
        case ASR::exprType::RealUnaryMinus:
        case ASR::exprType::ComplexUnaryMinus:
        case ASR::exprType::LogicalNot: {
            return true;
        }
        default: {
            return false;
        }
    }
}

static inline ASR::expr_t* extract_member_from_unaryop(ASR::expr_t* x) {
    #define UNARYOP_MEMBER_CASE(X, X_t) \
    case (ASR::exprType::X) : { \
        return ASR::down_cast<ASR::X_t>(x)->m_arg; \
    }

    switch (x->type) {
        UNARYOP_MEMBER_CASE(IntegerUnaryMinus, IntegerUnaryMinus_t)
        UNARYOP_MEMBER_CASE(RealUnaryMinus, RealUnaryMinus_t)
        UNARYOP_MEMBER_CASE(ComplexUnaryMinus, ComplexUnaryMinus_t)
        UNARYOP_MEMBER_CASE(LogicalNot, LogicalNot_t)
        default: {
            LCOMPILERS_ASSERT(false)
        }
    }

    return nullptr;
}

static inline ASR::expr_t* extract_member_from_binop(ASR::expr_t* x, int8_t member) {
    #define BINOP_MEMBER_CASE(X, X_t) \
    case (ASR::exprType::X) : { \
        if( member == 0 ) { \
            return ASR::down_cast<ASR::X_t>(x)->m_left; \
        } else { \
            return ASR::down_cast<ASR::X_t>(x)->m_right; \
        } \
    }

    switch (x->type) {
        BINOP_MEMBER_CASE(IntegerBinOp, IntegerBinOp_t)
        BINOP_MEMBER_CASE(RealBinOp, RealBinOp_t)
        BINOP_MEMBER_CASE(ComplexBinOp, ComplexBinOp_t)
        BINOP_MEMBER_CASE(LogicalBinOp, LogicalBinOp_t)
        BINOP_MEMBER_CASE(UnsignedIntegerBinOp, UnsignedIntegerBinOp_t)
        BINOP_MEMBER_CASE(IntegerCompare, IntegerCompare_t)
        BINOP_MEMBER_CASE(RealCompare, RealCompare_t)
        BINOP_MEMBER_CASE(ComplexCompare, ComplexCompare_t)
        BINOP_MEMBER_CASE(LogicalCompare, LogicalCompare_t)
        BINOP_MEMBER_CASE(UnsignedIntegerCompare, UnsignedIntegerCompare_t)
        BINOP_MEMBER_CASE(StringCompare, StringCompare_t)
        default: {
            LCOMPILERS_ASSERT(false)
        }
    }

    return nullptr;
}

size_t get_constant_ArrayConstant_size(ASR::ArrayConstant_t* x);

ASR::expr_t* get_compile_time_array_size(Allocator& al, ASR::ttype_t* array_type);
ASR::expr_t* get_ArrayConstant_size(Allocator& al, ASR::ArrayConstant_t* x);

ASR::expr_t* get_ImpliedDoLoop_size(Allocator& al, ASR::ImpliedDoLoop_t* implied_doloop);

ASR::expr_t* get_ArrayConstructor_size(Allocator& al, ASR::ArrayConstructor_t* x);

ASR::asr_t* make_ArraySize_t_util(
    Allocator &al, const Location &a_loc, ASR::expr_t* a_v,
    ASR::expr_t* a_dim, ASR::ttype_t* a_type, ASR::expr_t* a_value,
    bool for_type=true);

inline ASR::asr_t* make_Variable_t_util(Allocator &al, const Location &a_loc,
    SymbolTable* a_parent_symtab, char* a_name, char** a_dependencies, size_t n_dependencies,
    ASR::intentType a_intent, ASR::expr_t* a_symbolic_value, ASR::expr_t* a_value, ASR::storage_typeType a_storage,
    ASR::ttype_t* a_type, ASR::symbol_t* a_type_declaration, ASR::abiType a_abi, ASR::accessType a_access, ASR::presenceType a_presence,
    bool a_value_attr, bool a_target_attr = false, bool contiguous_attr = false) {
    return ASR::make_Variable_t(al, a_loc, a_parent_symtab, a_name, a_dependencies, n_dependencies, a_intent,
    a_symbolic_value,  a_value,  a_storage,  a_type,  a_type_declaration,  a_abi, a_access, a_presence, a_value_attr, a_target_attr, contiguous_attr);
}

inline ASR::ttype_t* make_Array_t_util(Allocator& al, const Location& loc,
    ASR::ttype_t* type, ASR::dimension_t* m_dims, size_t n_dims,
    ASR::abiType abi=ASR::abiType::Source, bool is_argument=false,
    ASR::array_physical_typeType physical_type=ASR::array_physical_typeType::DescriptorArray,
    bool override_physical_type=false, bool is_dimension_star=false, bool for_type=true) {
    if( n_dims == 0 ) {
        return type;
    }

    for( size_t i = 0; i < n_dims; i++ ) {
        if( m_dims[i].m_length && ASR::is_a<ASR::ArraySize_t>(*m_dims[i].m_length) ) {
            ASR::ArraySize_t* as = ASR::down_cast<ASR::ArraySize_t>(m_dims[i].m_length);
            m_dims[i].m_length = ASRUtils::EXPR(ASRUtils::make_ArraySize_t_util(
                al, as->base.base.loc, as->m_v, as->m_dim, as->m_type, nullptr, for_type));
        }
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
            } else if ( is_dimension_star && ASRUtils::is_only_upper_bound_empty(m_dims[n_dims-1]) ) {
                physical_type = ASR::array_physical_typeType::UnboundedPointerToDataArray;
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
            bool is_argument=false, bool is_dimension_star=false) {
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
        case ASR::ttypeType::String:
        case ASR::ttypeType::Logical:
        case ASR::ttypeType::StructType:
        case ASR::ttypeType::EnumType:
        case ASR::ttypeType::UnionType:
        case ASR::ttypeType::TypeParameter: {
            *x = ASRUtils::make_Array_t_util(al,
                (*x)->base.loc, *x, m_dims, n_dims, abi, is_argument, ASR::array_physical_typeType::DescriptorArray, false, is_dimension_star);
            return true;
        }
        default:
            return false;
    }
    return false;
}

static inline bool is_aggregate_type(ASR::ttype_t* asr_type) {
    return ASRUtils::is_array(asr_type) ||
            !(ASR::is_a<ASR::Integer_t>(*asr_type) ||
              ASR::is_a<ASR::UnsignedInteger_t>(*asr_type) ||
              ASR::is_a<ASR::Real_t>(*asr_type) ||
              ASR::is_a<ASR::Complex_t>(*asr_type) ||
              ASR::is_a<ASR::Logical_t>(*asr_type) ||
              ASR::is_a<ASR::String_t>(
                *ASRUtils::type_get_past_pointer(
                    ASRUtils::type_get_past_allocatable(asr_type))) ||
              ASR::is_a<ASR::TypeParameter_t>(*asr_type));
}

static inline ASR::dimension_t* duplicate_dimensions(Allocator& al, ASR::dimension_t* m_dims, size_t n_dims);

static inline ASR::asr_t* make_StructType_t_util(Allocator& al, Location loc, ASR::symbol_t* der){
    ASR::Struct_t* st = ASR::down_cast<ASR::Struct_t>(ASRUtils::symbol_get_past_external(der));
    Vec<ASR::ttype_t*> members;
    members.reserve(al, st->n_members);
    SymbolTable* current_scope = st->m_symtab;
    for(size_t i = 0; i < st->n_members; i++){
        ASR::symbol_t* temp = current_scope->get_symbol(st->m_members[i]);
        if(ASR::is_a<ASR::Variable_t>(*temp)){
            ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(
                                                ASRUtils::symbol_get_past_external(temp));
            members.push_back(al,var->m_type);
        }
    }
    return ASR::make_StructType_t(al,
                                loc,
                                nullptr, // TODO: FIXME: Use the member function types computed above
                                0,       // TODO: FIXME: Use the length of member function types computed above
                                nullptr, //Correct this when mem fn added to Struct_t
                                0,       //Correct this when mem fn added to Struct_t
                                true,    //Correct this when mem fn added to Struct_t
                                der);

}

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
        case ASR::ttypeType::String: {
            ASR::String_t* tnew = ASR::down_cast<ASR::String_t>(t);
            t_ = ASRUtils::TYPE(ASR::make_String_t(al, t->base.loc,
                    tnew->m_kind, tnew->m_len, tnew->m_len_expr, tnew->m_physical_type));
            break;
        }
        case ASR::ttypeType::StructType: {
            ASR::StructType_t* tnew = ASR::down_cast<ASR::StructType_t>(t);
            t_ = ASRUtils::TYPE(ASR::make_StructType_t(al, t->base.loc,
                tnew->m_data_member_types,
                tnew->n_data_member_types,
                tnew->m_member_function_types,
                tnew->n_member_function_types,
                tnew->m_is_cstruct,
                tnew->m_derived_type));
            break;
        }
        case ASR::ttypeType::ClassType: {
            ASR::ClassType_t* tnew = ASR::down_cast<ASR::ClassType_t>(t);
            t_ = ASRUtils::TYPE(ASR::make_ClassType_t(al, t->base.loc, tnew->m_class_type));
            break;
        }
        case ASR::ttypeType::Pointer: {
            ASR::Pointer_t* ptr = ASR::down_cast<ASR::Pointer_t>(t);
            ASR::ttype_t* dup_type = duplicate_type(al, ptr->m_type, dims,
                physical_type, override_physical_type);
            if( override_physical_type &&
                (physical_type == ASR::array_physical_typeType::FixedSizeArray ||
                (physical_type == ASR::array_physical_typeType::StringArraySinglePointer &&
                dims != nullptr) ) ) {
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
                ft->m_static, ft->m_restrictions, ft->n_restrictions,
                ft->m_is_restriction));
        }
        case ASR::ttypeType::SymbolicExpression: {
            return ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, t->base.loc));
        }
        case ASR::ttypeType::Tuple: {
            ASR::Tuple_t* tup = ASR::down_cast<ASR::Tuple_t>(t);
            Vec<ASR::ttype_t*> types;
            types.reserve(al, tup->n_type);
            for( size_t i = 0; i < tup->n_type; i++ ) {
                ASR::ttype_t *t = ASRUtils::duplicate_type(al, tup->m_type[i],
                    nullptr, physical_type, override_physical_type);
                types.push_back(al, t);
            }
            return ASRUtils::TYPE(ASR::make_Tuple_t(al, tup->base.base.loc,
                types.p, types.size()));
        }
        default : throw LCompilersException("Not implemented " + ASRUtils::type_to_str_python(t));
    }
    LCOMPILERS_ASSERT(t_ != nullptr);
    return ASRUtils::make_Array_t_util(
        al, t_->base.loc, t_, dimsp, dimsn,
        ASR::abiType::Source, false, physical_type,
        override_physical_type);
}

static inline void set_absent_optional_arguments_to_null(
    Vec<ASR::call_arg_t>& args, ASR::Function_t* func, Allocator& al,
    ASR::expr_t* dt=nullptr, bool nopass = false) {
    int offset = (dt != nullptr) && (!nopass);
    for( size_t i = args.size(); i + offset < func->n_args; i++ ) {
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
    LCOMPILERS_ASSERT(args.size() + offset == (func->n_args));
}

// Check if the passed ttype node is character type node of
// physical type `DescriptorString`.
static inline bool is_descriptorString(ASR::ttype_t* t){
    return is_character(*t) &&
        ASR::down_cast<ASR::String_t>(
        extract_type(t))->m_physical_type == ASR::string_physical_typeType::DescriptorString;
}

// Create `StringPhysicalCast` node from  `PointerString` --> `DescriptorString`.
static inline ASR::expr_t* cast_string_pointer_to_descriptor(Allocator& al, ASR::expr_t* string){
    LCOMPILERS_ASSERT(is_character(*ASRUtils::expr_type(string)) &&
        !is_descriptorString(expr_type(string)));
    ASR::ttype_t* string_type = ASRUtils::expr_type(string);
    ASR::ttype_t* stringDescriptor_type = ASRUtils::duplicate_type(al,
        ASRUtils::extract_type(string_type));
    ASR::down_cast<ASR::String_t>(stringDescriptor_type)->m_physical_type = ASR::string_physical_typeType::DescriptorString;
    ASR::ttype_t* alloctable_stringDescriptor_type = ASRUtils::TYPE(
        ASR::make_Allocatable_t(al, string->base.loc, stringDescriptor_type));
    // Create pointerString to descriptorString cast node
    ASR::expr_t* ptr_to_desc_string_cast = ASRUtils::EXPR(
        ASR::make_StringPhysicalCast_t(al, string->base.loc , string,
        ASR::string_physical_typeType::PointerString, ASR::string_physical_typeType::DescriptorString,
        alloctable_stringDescriptor_type, nullptr));
    return ptr_to_desc_string_cast;
}

// Create `StringPhysicalCast` node from `DescriptorString` --> `PointerString`.
static inline ASR::expr_t* cast_string_descriptor_to_pointer(Allocator& al, ASR::expr_t* string){
    LCOMPILERS_ASSERT(is_character(*ASRUtils::expr_type(string)) &&
    is_descriptorString(expr_type(string)));
    // Create string node with `PointerString` physical type
    ASR::ttype_t* stringPointer_type = ASRUtils::duplicate_type(al, ASRUtils::expr_type(string));
    ASR::down_cast<ASR::String_t>(ASRUtils::type_get_past_allocatable(stringPointer_type))->m_physical_type = ASR::string_physical_typeType::PointerString;
    // Create descriptorString to pointerString cast node
    ASR::expr_t* des_to_ptr_string_cast = ASRUtils::EXPR(
        ASR::make_StringPhysicalCast_t(al, string->base.loc , string,
        ASR::string_physical_typeType::DescriptorString, ASR::string_physical_typeType::PointerString,
        stringPointer_type, nullptr));
    return des_to_ptr_string_cast;
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
        case ASR::ttypeType::String: {
            ASR::String_t* tnew = ASR::down_cast<ASR::String_t>(t);
            return ASRUtils::TYPE(ASR::make_String_t(al, loc,
                        tnew->m_kind, tnew->m_len, tnew->m_len_expr, ASR::string_physical_typeType::PointerString));
        }
        case ASR::ttypeType::StructType: {
            ASR::StructType_t* tstruct = ASR::down_cast<ASR::StructType_t>(t);
            return ASRUtils::TYPE(ASR::make_StructType_t(al, t->base.loc,
                tstruct->m_data_member_types,
                tstruct->n_data_member_types,
                tstruct->m_member_function_types,
                tstruct->n_member_function_types,
                tstruct->m_is_cstruct,
                tstruct->m_derived_type));
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
            return ASRUtils::TYPE(ASR::make_TypeParameter_t(al, loc, tp->m_param));
        }
        default : throw LCompilersException("Not implemented " + ASRUtils::type_to_str_python(t));
    }
}

static inline ASR::asr_t* make_Allocatable_t_util(Allocator& al, const Location& loc, ASR::ttype_t* type) {
    return ASR::make_Allocatable_t(
        al, loc, duplicate_type_with_empty_dims(al, type));
}

inline std::string remove_trailing_white_spaces(std::string str) {
    int end = str.size() - 1;
    while (end >= 0 && str[end] == ' ') {
        end--;
    }
    return str.substr(0, end + 1);
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
    if( (ASR::is_a<ASR::ClassType_t>(*source) || ASR::is_a<ASR::StructType_t>(*source)) &&
        (ASR::is_a<ASR::ClassType_t>(*dest) || ASR::is_a<ASR::StructType_t>(*dest)) ) {
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

// this function only extract's the 'kind' and raises an error when it's of
// inappropriate type (e.g. float), but doesn't ensure 'kind' is appropriate
// for whose kind it is
template <typename SemanticAbort>
inline int extract_kind(ASR::expr_t* kind_expr, const Location& loc, diag::Diagnostics &diag) {
    switch( kind_expr->type ) {
        case ASR::exprType::Var: {
            ASR::Var_t* kind_var = ASR::down_cast<ASR::Var_t>(kind_expr);
            ASR::Variable_t* kind_variable = ASR::down_cast<ASR::Variable_t>(
                    symbol_get_past_external(kind_var->m_v));
            bool is_parent_enum = false;
            if (kind_variable->m_parent_symtab->asr_owner != nullptr) {
                ASR::symbol_t *s = ASR::down_cast<ASR::symbol_t>(
                    kind_variable->m_parent_symtab->asr_owner);
                is_parent_enum = ASR::is_a<ASR::Enum_t>(*s);
            }
            if( is_parent_enum ) {
                return ASRUtils::extract_kind_from_ttype_t(kind_variable->m_type);
            } else if( kind_variable->m_storage == ASR::storage_typeType::Parameter ) {
                if( kind_variable->m_type->type == ASR::ttypeType::Integer ) {
                    LCOMPILERS_ASSERT( kind_variable->m_value != nullptr );
                    return ASR::down_cast<ASR::IntegerConstant_t>(kind_variable->m_value)->m_n;
                } else {
                    diag.add(diag::Diagnostic(
                        "Integer variable required. " + std::string(kind_variable->m_name) +
                                    " is not an Integer variable.",
                        diag::Level::Error, diag::Stage::Semantic, {
                            diag::Label("", {loc})}));
                    throw SemanticAbort();
                }
            } else {
                diag.add(diag::Diagnostic(
                    "Parameter '" + std::string(kind_variable->m_name) +
                    "' is a variable, which does not reduce to a constant expression",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("", {loc})}));
                throw SemanticAbort();
            }
        }
        case ASR::exprType::IntrinsicElementalFunction: {
            ASR::IntrinsicElementalFunction_t* kind_isf =
                ASR::down_cast<ASR::IntrinsicElementalFunction_t>(kind_expr);
            if ( kind_isf->m_value &&
                    ASR::is_a<ASR::IntegerConstant_t>(*kind_isf->m_value) ) {
                return ASR::down_cast<ASR::IntegerConstant_t>(kind_isf->m_value)->m_n;
            } else {
                diag.add(diag::Diagnostic(
                    "Only Integer literals or expressions which "
                    "reduce to constant Integer are accepted as kind parameters",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("", {loc})}));
                throw SemanticAbort();
            }
            break;
        }
        case ASR::exprType::TypeInquiry: {
            ASR::TypeInquiry_t* kind_ti =
                ASR::down_cast<ASR::TypeInquiry_t>(kind_expr);
            if (kind_ti->m_value) {
                return ASR::down_cast<ASR::IntegerConstant_t>(kind_ti->m_value)->m_n;
            } else {
                diag.add(diag::Diagnostic(
                    "Only Integer literals or expressions which "
                    "reduce to constant Integer are accepted as kind parameters",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("", {loc})}));
                throw SemanticAbort();
            }
            break;
        }
        case ASR::exprType::BitCast: {
            ASR::BitCast_t* kind_bc = ASR::down_cast<ASR::BitCast_t>(kind_expr);
            if (kind_bc->m_value) {
                return ASR::down_cast<ASR::IntegerConstant_t>(kind_bc->m_value)->m_n;
            } else {
                diag.add(diag::Diagnostic(
                    "Only Integer literals or expressions which "
                    "reduce to constant Integer are accepted as kind parameters",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("", {loc})}));
                throw SemanticAbort();
            }
            break;
        }
        // allow integer binary operator kinds (e.g. '1 + 7')
        case ASR::exprType::IntegerBinOp:
        // allow integer kinds (e.g. 4, 8 etc.)
        case ASR::exprType::IntegerConstant: {
            int a_kind = -1;
            if (!ASRUtils::extract_value(kind_expr, a_kind)) {
                // we still need to ensure that values are constant
                // e.g. "integer :: a = 4; real(1*a) :: x" is an invalid kind,
                // as 'a' isn't a constant.
                // ToDo: we should raise a better error, by "locating" just
                // 'a' as well, instead of the whole '1*a'
                diag.add(diag::Diagnostic(
                    "Only Integer literals or expressions which "
                    "reduce to constant Integer are accepted as kind parameters",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("", {loc})}));
                throw SemanticAbort();
            }
            return a_kind;
        }
        // make sure not to allow kind having "RealConstant" (e.g. 4.0),
        // and everything else
        default: {
            diag.add(diag::Diagnostic(
                "Only Integer literals or expressions which "
                "reduce to constant Integer are accepted as kind parameters",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("", {loc})}));
            throw SemanticAbort();
        }
    }
}

template <typename SemanticAbort>
inline int extract_len(ASR::expr_t* len_expr, const Location& loc, diag::Diagnostics &diag) {
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
                    diag.add(diag::Diagnostic(
                        "Integer variable required. " + std::string(len_variable->m_name) +
                        " is not an Integer variable",
                        diag::Level::Error, diag::Stage::Semantic, {
                            diag::Label("", {loc})}));
                    throw SemanticAbort();
                }
            } else {
                // An expression is being used for `len` that cannot be evaluated
                a_len = -3;
            }
            break;
        }
        case ASR::exprType::StringLen:
        case ASR::exprType::FunctionCall: {
            a_len = -3;
            break;
        }
        case ASR::exprType::ArraySize:
        case ASR::exprType::IntegerBinOp: {
            a_len = -3;
            break;
        }
        case ASR::exprType::IntrinsicElementalFunction: {
            a_len = -3;
            break;
        }
        default: {
            diag.add(diag::Diagnostic(
                "Only Integers or variables implemented so far for `len` expressions, found: " + ASRUtils::type_to_str_python(ASRUtils::expr_type(len_expr)),
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("", {loc})}));
            throw SemanticAbort();
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

inline bool is_parent(ASR::Struct_t* a, ASR::Struct_t* b) {
    ASR::symbol_t* current_parent = b->m_parent;
    while( current_parent ) {
        current_parent = ASRUtils::symbol_get_past_external(current_parent);
        if( current_parent == (ASR::symbol_t*) a ) {
            return true;
        }
        LCOMPILERS_ASSERT(ASR::is_a<ASR::Struct_t>(*current_parent));
        current_parent = ASR::down_cast<ASR::Struct_t>(current_parent)->m_parent;
    }
    return false;
}

inline bool is_derived_type_similar(ASR::Struct_t* a, ASR::Struct_t* b) {
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
            return check_equal_type(expr_type(&var_x->base), expr_type(&var_y->base), true);
        }
        case ASR::exprType::IntegerConstant: {
            ASR::IntegerConstant_t* intconst_x = ASR::down_cast<ASR::IntegerConstant_t>(x);
            ASR::IntegerConstant_t* intconst_y = ASR::down_cast<ASR::IntegerConstant_t>(y);
            return intconst_x->m_n == intconst_y->m_n;
        }
        case ASR::exprType::RealConstant: {
            ASR::RealConstant_t* realconst_x = ASR::down_cast<ASR::RealConstant_t>(x);
            ASR::RealConstant_t* realconst_y = ASR::down_cast<ASR::RealConstant_t>(y);
            return realconst_x->m_r == realconst_y->m_r;
        }
        default: {
            // Let it pass for now.
            return true;
        }
    }

    // Let it pass for now.
    return true;
}

// Compares two dimension expressions for equality.
// Optionally allows skipping determination in certain cases.
inline bool dimension_expr_equal(
    ASR::expr_t* dim_a,
    ASR::expr_t* dim_b
) {
    // If either dimension is null, consider them equal by default.
    if (!(dim_a && dim_b)) {
        return true;
    }
    int dim_a_int {-1};
    int dim_b_int {-1};

    if (ASRUtils::extract_value(ASRUtils::expr_value(dim_a), dim_a_int) &&
        ASRUtils::extract_value(ASRUtils::expr_value(dim_b), dim_b_int)) {
        return dim_a_int == dim_b_int;
    }

    if (!ASRUtils::expr_equal(dim_a, dim_b)) {
        return false;
    }

    return true;
}

inline bool dimensions_compatible(ASR::dimension_t* dims_a, size_t n_dims_a,
    ASR::dimension_t* dims_b, size_t n_dims_b,
    bool check_n_dims= true){

    if (check_n_dims && (n_dims_a != n_dims_b)) {
        return false;
    }
    int total_a = get_fixed_size_of_array(dims_a,n_dims_a);
    int total_b = get_fixed_size_of_array(dims_b,n_dims_b);
    // -1 means found dimension with no value at compile time, then return true anyway.
    return (total_a == -1) || (total_b == -1) || (total_a >= total_b);
}

inline bool types_equal(ASR::ttype_t *a, ASR::ttype_t *b,
    bool check_for_dimensions=false) {
    // TODO: If anyone of the input or argument is derived type then
    // add support for checking member wise types and do not compare
    // directly. From stdlib_string len(pattern) error
    if( a == nullptr && b == nullptr ) {
        return true;
    }
    // a = ASRUtils::type_get_past_const(
    //         ASRUtils::type_get_past_allocatable(
    //             ASRUtils::type_get_past_pointer(a)));
    // b = ASRUtils::type_get_past_const(
    //         ASRUtils::type_get_past_allocatable(
    //             ASRUtils::type_get_past_pointer(b)));
    a = ASRUtils::type_get_past_allocatable(ASRUtils::type_get_past_pointer(a));
    b = ASRUtils::type_get_past_allocatable(ASRUtils::type_get_past_pointer(b));
    if( !check_for_dimensions ) {
        a = ASRUtils::type_get_past_array(a);
        b = ASRUtils::type_get_past_array(b);
    }
    // If either argument is a polymorphic type, return true.
    if (ASR::is_a<ASR::ClassType_t>(*a)) {
        if (ASRUtils::symbol_name(
            ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::ClassType_t>(a)->m_class_type)) == std::string("~abstract_type")) {
            return true;
        }
    } else if (ASR::is_a<ASR::ClassType_t>(*b)) {
        if (ASRUtils::symbol_name(
            ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::ClassType_t>(b)->m_class_type)) == std::string("~abstract_type")) {
            return true;
        }
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

                return ASRUtils::dimensions_compatible(
                            a2->m_dims, a2->n_dims,
                            b2->m_dims, b2->n_dims);
            }
            case (ASR::ttypeType::TypeParameter) : {
                ASR::TypeParameter_t* left_tp = ASR::down_cast<ASR::TypeParameter_t>(a);
                ASR::TypeParameter_t* right_tp = ASR::down_cast<ASR::TypeParameter_t>(b);
                std::string left_param = left_tp->m_param;
                std::string right_param = right_tp->m_param;
                return left_param == right_param;
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
            case (ASR::ttypeType::String) : {
                ASR::String_t *a2 = ASR::down_cast<ASR::String_t>(a);
                ASR::String_t *b2 = ASR::down_cast<ASR::String_t>(b);
                return (a2->m_kind == b2->m_kind);
            }
            case (ASR::ttypeType::List) : {
                ASR::List_t *a2 = ASR::down_cast<ASR::List_t>(a);
                ASR::List_t *b2 = ASR::down_cast<ASR::List_t>(b);
                return types_equal(a2->m_type, b2->m_type);
            }
            case (ASR::ttypeType::StructType) : {
                ASR::StructType_t *a2 = ASR::down_cast<ASR::StructType_t>(a);
                ASR::StructType_t *b2 = ASR::down_cast<ASR::StructType_t>(b);
                ASR::Struct_t *a2_type = ASR::down_cast<ASR::Struct_t>(
                                                ASRUtils::symbol_get_past_external(
                                                    a2->m_derived_type));
                ASR::Struct_t *b2_type = ASR::down_cast<ASR::Struct_t>(
                                                ASRUtils::symbol_get_past_external(
                                                    b2->m_derived_type));
                return a2_type == b2_type;
            }
            case (ASR::ttypeType::ClassType) : {
                ASR::ClassType_t *a2 = ASR::down_cast<ASR::ClassType_t>(a);
                ASR::ClassType_t *b2 = ASR::down_cast<ASR::ClassType_t>(b);
                ASR::symbol_t* a2_typesym = ASRUtils::symbol_get_past_external(a2->m_class_type);
                ASR::symbol_t* b2_typesym = ASRUtils::symbol_get_past_external(b2->m_class_type);
                if( a2_typesym->type != b2_typesym->type ) {
                    return false;
                }
                if( a2_typesym->type == ASR::symbolType::Class ) {
                    ASR::Class_t *a2_type = ASR::down_cast<ASR::Class_t>(a2_typesym);
                    ASR::Class_t *b2_type = ASR::down_cast<ASR::Class_t>(b2_typesym);
                    return a2_type == b2_type;
                } else if( a2_typesym->type == ASR::symbolType::Struct ) {
                    ASR::Struct_t *a2_type = ASR::down_cast<ASR::Struct_t>(a2_typesym);
                    ASR::Struct_t *b2_type = ASR::down_cast<ASR::Struct_t>(b2_typesym);
                    return is_derived_type_similar(a2_type, b2_type);
                }
                return false;
            }
            case (ASR::ttypeType::UnionType) : {
                ASR::UnionType_t *a2 = ASR::down_cast<ASR::UnionType_t>(a);
                ASR::UnionType_t *b2 = ASR::down_cast<ASR::UnionType_t>(b);
                ASR::Union_t *a2_type = ASR::down_cast<ASR::Union_t>(
                                                ASRUtils::symbol_get_past_external(
                                                    a2->m_union_type));
                ASR::Union_t *b2_type = ASR::down_cast<ASR::Union_t>(
                                                ASRUtils::symbol_get_past_external(
                                                    b2->m_union_type));
                return a2_type == b2_type;
            }
            case ASR::ttypeType::FunctionType: {
                ASR::FunctionType_t* a2 = ASR::down_cast<ASR::FunctionType_t>(a);
                ASR::FunctionType_t* b2 = ASR::down_cast<ASR::FunctionType_t>(b);
                if( a2->n_arg_types != b2->n_arg_types ||
                    (a2->m_return_var_type != nullptr && b2->m_return_var_type == nullptr) ||
                    (a2->m_return_var_type == nullptr && b2->m_return_var_type != nullptr) ) {
                    return false;
                }
                for( size_t i = 0; i < a2->n_arg_types; i++ ) {
                    if( !types_equal(a2->m_arg_types[i], b2->m_arg_types[i], true) ) {
                        return false;
                    }
                }
                if( !types_equal(a2->m_return_var_type, b2->m_return_var_type, true) ) {
                    return false;
                }
                return true;
            }
            default : return false;
        }
    } else if (a->type == ASR::ttypeType::StructType && b->type == ASR::ttypeType::ClassType) {
        ASR::StructType_t *a2 = ASR::down_cast<ASR::StructType_t>(a);
        ASR::ClassType_t *b2 = ASR::down_cast<ASR::ClassType_t>(b);
        ASR::symbol_t* a2_typesym = ASRUtils::symbol_get_past_external(a2->m_derived_type);
        ASR::symbol_t* b2_typesym = ASRUtils::symbol_get_past_external(b2->m_class_type);
        if( a2_typesym->type != b2_typesym->type ) {
            return false;
        }
        if( a2_typesym->type == ASR::symbolType::Class ) {
            ASR::Class_t *a2_type = ASR::down_cast<ASR::Class_t>(a2_typesym);
            ASR::Class_t *b2_type = ASR::down_cast<ASR::Class_t>(b2_typesym);
            return a2_type == b2_type;
        } else if( a2_typesym->type == ASR::symbolType::Struct ) {
            ASR::Struct_t *a2_type = ASR::down_cast<ASR::Struct_t>(a2_typesym);
            ASR::Struct_t *b2_type = ASR::down_cast<ASR::Struct_t>(b2_typesym);
            return is_derived_type_similar(a2_type, b2_type);
        }
    } else if (a->type == ASR::ttypeType::ClassType && b->type == ASR::ttypeType::StructType) {
        ASR::ClassType_t *a2 = ASR::down_cast<ASR::ClassType_t>(a);
        ASR::StructType_t *b2 = ASR::down_cast<ASR::StructType_t>(b);
        ASR::symbol_t* a2_typesym = ASRUtils::symbol_get_past_external(a2->m_class_type);
        ASR::symbol_t* b2_typesym = ASRUtils::symbol_get_past_external(b2->m_derived_type);
        if( a2_typesym->type != b2_typesym->type ) {
            return false;
        }
        if( a2_typesym->type == ASR::symbolType::Class ) {
            ASR::Class_t *a2_type = ASR::down_cast<ASR::Class_t>(a2_typesym);
            ASR::Class_t *b2_type = ASR::down_cast<ASR::Class_t>(b2_typesym);
            return a2_type == b2_type;
        } else if( a2_typesym->type == ASR::symbolType::Struct ) {
            ASR::Struct_t *a2_type = ASR::down_cast<ASR::Struct_t>(a2_typesym);
            ASR::Struct_t *b2_type = ASR::down_cast<ASR::Struct_t>(b2_typesym);
            return is_derived_type_similar(a2_type, b2_type);
        }
    }
    return false;
}

inline bool types_equal_with_substitution(ASR::ttype_t *a, ASR::ttype_t *b,
    std::map<std::string, ASR::ttype_t*> subs,
    bool check_for_dimensions=false) {
    // TODO: If anyone of the input or argument is derived type then
    // add support for checking member wise types and do not compare
    // directly. From stdlib_string len(pattern) error
    if( a == nullptr && b == nullptr ) {
        return true;
    }
    a = ASRUtils::type_get_past_allocatable(ASRUtils::type_get_past_pointer(a));
    b = ASRUtils::type_get_past_allocatable(ASRUtils::type_get_past_pointer(b));
    if( !check_for_dimensions ) {
        a = ASRUtils::type_get_past_array(a);
        b = ASRUtils::type_get_past_array(b);
    }
    if (ASR::is_a<ASR::TypeParameter_t>(*a)) {
        ASR::TypeParameter_t* a_tp = ASR::down_cast<ASR::TypeParameter_t>(a);
        a = subs[a_tp->m_param];
    }
    if (a->type == b->type) {
        // TODO: check dims
        // TODO: check all types
        switch (a->type) {
            case (ASR::ttypeType::Array): {
                ASR::Array_t* a2 = ASR::down_cast<ASR::Array_t>(a);
                ASR::Array_t* b2 = ASR::down_cast<ASR::Array_t>(b);
                if( !types_equal_with_substitution(a2->m_type, b2->m_type, subs) ) {
                    return false;
                }

                return ASRUtils::dimensions_compatible(
                            a2->m_dims, a2->n_dims,
                            b2->m_dims, b2->n_dims);
            }
            case (ASR::ttypeType::TypeParameter) : {
                ASR::TypeParameter_t* left_tp = ASR::down_cast<ASR::TypeParameter_t>(a);
                ASR::TypeParameter_t* right_tp = ASR::down_cast<ASR::TypeParameter_t>(b);
                std::string left_param = left_tp->m_param;
                std::string right_param = right_tp->m_param;
                return left_param == right_param;
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
            case (ASR::ttypeType::String) : {
                ASR::String_t *a2 = ASR::down_cast<ASR::String_t>(a);
                ASR::String_t *b2 = ASR::down_cast<ASR::String_t>(b);
                return (a2->m_kind == b2->m_kind);
            }
            case (ASR::ttypeType::List) : {
                ASR::List_t *a2 = ASR::down_cast<ASR::List_t>(a);
                ASR::List_t *b2 = ASR::down_cast<ASR::List_t>(b);
                return types_equal_with_substitution(a2->m_type, b2->m_type, subs);
            }
            case (ASR::ttypeType::StructType) : {
                ASR::StructType_t *a2 = ASR::down_cast<ASR::StructType_t>(a);
                ASR::StructType_t *b2 = ASR::down_cast<ASR::StructType_t>(b);
                ASR::Struct_t *a2_type = ASR::down_cast<ASR::Struct_t>(
                                                ASRUtils::symbol_get_past_external(
                                                    a2->m_derived_type));
                ASR::Struct_t *b2_type = ASR::down_cast<ASR::Struct_t>(
                                                ASRUtils::symbol_get_past_external(
                                                    b2->m_derived_type));
                return a2_type == b2_type;
            }
            case (ASR::ttypeType::ClassType) : {
                ASR::ClassType_t *a2 = ASR::down_cast<ASR::ClassType_t>(a);
                ASR::ClassType_t *b2 = ASR::down_cast<ASR::ClassType_t>(b);
                ASR::symbol_t* a2_typesym = ASRUtils::symbol_get_past_external(a2->m_class_type);
                ASR::symbol_t* b2_typesym = ASRUtils::symbol_get_past_external(b2->m_class_type);
                if( a2_typesym->type != b2_typesym->type ) {
                    return false;
                }
                if( a2_typesym->type == ASR::symbolType::Class ) {
                    ASR::Class_t *a2_type = ASR::down_cast<ASR::Class_t>(a2_typesym);
                    ASR::Class_t *b2_type = ASR::down_cast<ASR::Class_t>(b2_typesym);
                    return a2_type == b2_type;
                } else if( a2_typesym->type == ASR::symbolType::Struct ) {
                    ASR::Struct_t *a2_type = ASR::down_cast<ASR::Struct_t>(a2_typesym);
                    ASR::Struct_t *b2_type = ASR::down_cast<ASR::Struct_t>(b2_typesym);
                    return is_derived_type_similar(a2_type, b2_type);
                }
                return false;
            }
            case (ASR::ttypeType::UnionType) : {
                ASR::UnionType_t *a2 = ASR::down_cast<ASR::UnionType_t>(a);
                ASR::UnionType_t *b2 = ASR::down_cast<ASR::UnionType_t>(b);
                ASR::Union_t *a2_type = ASR::down_cast<ASR::Union_t>(
                                                ASRUtils::symbol_get_past_external(
                                                    a2->m_union_type));
                ASR::Union_t *b2_type = ASR::down_cast<ASR::Union_t>(
                                                ASRUtils::symbol_get_past_external(
                                                    b2->m_union_type));
                return a2_type == b2_type;
            }
            case ASR::ttypeType::FunctionType: {
                ASR::FunctionType_t* a2 = ASR::down_cast<ASR::FunctionType_t>(a);
                ASR::FunctionType_t* b2 = ASR::down_cast<ASR::FunctionType_t>(b);
                if( a2->n_arg_types != b2->n_arg_types ||
                    (a2->m_return_var_type != nullptr && b2->m_return_var_type == nullptr) ||
                    (a2->m_return_var_type == nullptr && b2->m_return_var_type != nullptr) ) {
                    return false;
                }
                for( size_t i = 0; i < a2->n_arg_types; i++ ) {
                    if( !types_equal_with_substitution(a2->m_arg_types[i], b2->m_arg_types[i], subs, true) ) {
                        return false;
                    }
                }
                if( !types_equal_with_substitution(a2->m_return_var_type, b2->m_return_var_type, subs, true) ) {
                    return false;
                }
                return true;
            }
            default : return false;
        }
    } else if( a->type == ASR::ttypeType::StructType &&
               b->type == ASR::ttypeType::ClassType ) {
        ASR::StructType_t *a2 = ASR::down_cast<ASR::StructType_t>(a);
        ASR::ClassType_t *b2 = ASR::down_cast<ASR::ClassType_t>(b);
        ASR::symbol_t* a2_typesym = ASRUtils::symbol_get_past_external(a2->m_derived_type);
        ASR::symbol_t* b2_typesym = ASRUtils::symbol_get_past_external(b2->m_class_type);
        if( a2_typesym->type != b2_typesym->type ) {
            return false;
        }
        if( a2_typesym->type == ASR::symbolType::Class ) {
            ASR::Class_t *a2_type = ASR::down_cast<ASR::Class_t>(a2_typesym);
            ASR::Class_t *b2_type = ASR::down_cast<ASR::Class_t>(b2_typesym);
            return a2_type == b2_type;
        } else if( a2_typesym->type == ASR::symbolType::Struct ) {
            ASR::Struct_t *a2_type = ASR::down_cast<ASR::Struct_t>(a2_typesym);
            ASR::Struct_t *b2_type = ASR::down_cast<ASR::Struct_t>(b2_typesym);
            return is_derived_type_similar(a2_type, b2_type);
        }
    } else if( a->type == ASR::ttypeType::ClassType &&
               b->type == ASR::ttypeType::StructType ) {
        ASR::ClassType_t *a2 = ASR::down_cast<ASR::ClassType_t>(a);
        ASR::StructType_t *b2 = ASR::down_cast<ASR::StructType_t>(b);
        ASR::symbol_t* a2_typesym = ASRUtils::symbol_get_past_external(a2->m_class_type);
        ASR::symbol_t* b2_typesym = ASRUtils::symbol_get_past_external(b2->m_derived_type);
        if( a2_typesym->type != b2_typesym->type ) {
            return false;
        }
        if( a2_typesym->type == ASR::symbolType::Class ) {
            ASR::Class_t *a2_type = ASR::down_cast<ASR::Class_t>(a2_typesym);
            ASR::Class_t *b2_type = ASR::down_cast<ASR::Class_t>(b2_typesym);
            return a2_type == b2_type;
        } else if( a2_typesym->type == ASR::symbolType::Struct ) {
            ASR::Struct_t *a2_type = ASR::down_cast<ASR::Struct_t>(a2_typesym);
            ASR::Struct_t *b2_type = ASR::down_cast<ASR::Struct_t>(b2_typesym);
            return is_derived_type_similar(a2_type, b2_type);
        }
    }
    return false;
}

inline bool check_equal_type(ASR::ttype_t* x, ASR::ttype_t* y, bool check_for_dimensions) {
    ASR::ttype_t *x_underlying, *y_underlying;
    x_underlying = nullptr;
    y_underlying = nullptr;
    if( ASR::is_a<ASR::EnumType_t>(*x) ) {
        ASR::EnumType_t *x_enum = ASR::down_cast<ASR::EnumType_t>(x);
        ASR::Enum_t *x_enum_type = ASR::down_cast<ASR::Enum_t>(x_enum->m_enum_type);
        x_underlying = x_enum_type->m_type;
    }
    if( ASR::is_a<ASR::EnumType_t>(*y) ) {
        ASR::EnumType_t *y_enum = ASR::down_cast<ASR::EnumType_t>(y);
        ASR::Enum_t *y_enum_type = ASR::down_cast<ASR::Enum_t>(y_enum->m_enum_type);
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

bool select_func_subrout(const ASR::symbol_t* proc, const Vec<ASR::call_arg_t>& args,
    Location& loc, const std::function<void (const std::string &, const Location &)> err);

template <typename T>
int select_generic_procedure(const Vec<ASR::call_arg_t> &args,
    const T &p, Location loc,
    const std::function<void (const std::string &, const Location &)> err,
    bool raise_error=true) {
    for (size_t i=0; i < p.n_procs; i++) {
        if( ASR::is_a<ASR::ClassProcedure_t>(*p.m_procs[i]) ) {
            ASR::ClassProcedure_t *clss_fn
                = ASR::down_cast<ASR::ClassProcedure_t>(p.m_procs[i]);
            const ASR::symbol_t *proc = ASRUtils::symbol_get_past_external(clss_fn->m_proc);
            if( select_func_subrout(proc, args, loc, err) ) {
                return i;
            }
        } else {
            if( select_func_subrout(p.m_procs[i], args, loc, err) ) {
                return i;
            }
        }
    }
    if( raise_error ) {
        err("Arguments do not match for any generic procedure, " + std::string(p.m_name), loc);
    }
    return -1;
}

ASR::asr_t* symbol_resolve_external_generic_procedure_without_eval(
            const Location &loc,
            ASR::symbol_t *v, Vec<ASR::call_arg_t>& args,
            SymbolTable* current_scope, Allocator& al,
            const std::function<void (const std::string &, const Location &)> err);

static inline ASR::storage_typeType symbol_StorageType(const ASR::symbol_t* s){
    switch( s->type ) {
        case ASR::symbolType::Variable: {
            return ASR::down_cast<ASR::Variable_t>(s)->m_storage;
        }
        default: {
            throw LCompilersException("Cannot return storage type of, " +
                                    std::to_string(s->type) + " symbol.");
        }
    }
}

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
                ASRUtils::type_to_str_python(ASRUtils::expr_type(expr)));
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
    if( mem_type && ASR::is_a<ASR::StructType_t>(*mem_type) ) {
        ASR::StructType_t* struct_t = ASR::down_cast<ASR::StructType_t>(mem_type);
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
            mem_type = ASRUtils::TYPE(ASRUtils::make_StructType_t_util(al, mem_type->base.loc, scope->get_symbol(struct_type_name_)));
        } else {
            mem_type = ASRUtils::TYPE(ASRUtils::make_StructType_t_util(al, mem_type->base.loc,
                scope->resolve_symbol(struct_type_name)));
        }
    }
    if( n_dims > 0 ) {
        mem_type = ASRUtils::make_Array_t_util(
            al, mem_type->base.loc, mem_type, m_dims, n_dims);
    }

    if( ASR::is_a<ASR::Allocatable_t>(*mem_type_) ) {
        mem_type = ASRUtils::TYPE(ASRUtils::make_Allocatable_t_util(al,
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
        ASR::Module_t *m = nullptr;
        if (ASR::is_a<ASR::symbol_t>(*f->m_symtab->parent->asr_owner)) {
            ASR::symbol_t* sym = ASR::down_cast<ASR::symbol_t>(f->m_symtab->parent->asr_owner);
            if (ASR::is_a<ASR::Module_t>(*sym)) {
                m = ASR::down_cast<ASR::Module_t>(sym);
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
                replace_ttype(x->m_type);
                if (ASRUtils::symbol_parent_symtab(new_es)->get_counter() != current_scope->get_counter()) {
                    ADD_ASR_DEPENDENCIES(current_scope, new_es, current_function_dependencies);
                }
                ASRUtils::insert_module_dependency(new_es, al, current_module_dependencies);
                x->m_name = new_es;
                if( x->m_original_name ) {
                    ASR::symbol_t* x_original_name = current_scope->resolve_symbol(ASRUtils::symbol_name(x->m_original_name));
                    if( x_original_name ) {
                        x->m_original_name = x_original_name;
                    }
                }
                return;
            } else {
                return;
            }
        }
        // iterate over the arguments and replace them
        for (size_t i = 0; i < x->n_args; i++) {
            ASR::expr_t** current_expr_copy_ = current_expr;
            current_expr = &(x->m_args[i].m_value);
            if (x->m_args[i].m_value) {
                replace_expr(x->m_args[i].m_value);
            }
            current_expr = current_expr_copy_;
        }
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

    void replace_ArraySize(ASR::ArraySize_t* x) {
        ASR::BaseExprReplacer<ReplaceArgVisitor>::replace_ArraySize(x);
        if( ASR::is_a<ASR::ArraySection_t>(*x->m_v) ) {
            *current_expr = ASRUtils::EXPR(ASRUtils::make_ArraySize_t_util(
                al, x->base.base.loc, x->m_v, x->m_dim, x->m_type, x->m_value, true));
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

class ExprStmtWithScopeDuplicator: public ASR::BaseExprStmtDuplicator<ExprStmtWithScopeDuplicator>
{
    public:
    SymbolTable* current_scope;
    ExprStmtWithScopeDuplicator(Allocator &al, SymbolTable* current_scope): BaseExprStmtDuplicator(al), current_scope(current_scope) {}

    ASR::asr_t* duplicate_Var(ASR::Var_t* x) {
        ASR::symbol_t* m_v = current_scope->get_symbol(ASRUtils::symbol_name(x->m_v));
        if (m_v == nullptr) {
            // we are dealing with an external/statement function, duplicate node with same symbol
            return ASR::make_Var_t(al, x->base.base.loc, x->m_v);
        }
        return ASR::make_Var_t(al, x->base.base.loc, m_v);
    }

};

class FixScopedTypeVisitor: public ASR::BaseExprReplacer<FixScopedTypeVisitor> {

    private:

    Allocator& al;
    SymbolTable* current_scope;

    public:

    FixScopedTypeVisitor(Allocator& al_, SymbolTable* current_scope_) :
        al(al_), current_scope(current_scope_) {}

    void replace_StructType(ASR::StructType_t* x) {
        ASR::symbol_t* m_derived_type = current_scope->resolve_symbol(
            ASRUtils::symbol_name(x->m_derived_type));
        if (m_derived_type == nullptr) {
            std::string imported_name = current_scope->get_unique_name(
                ASRUtils::symbol_name(x->m_derived_type));
            m_derived_type = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
                al, x->base.base.loc, current_scope, s2c(al, imported_name),
                x->m_derived_type, ASRUtils::get_sym_module(
                    ASRUtils::symbol_get_past_external(x->m_derived_type))->m_name,
                    nullptr, 0, ASRUtils::symbol_name(
                        ASRUtils::symbol_get_past_external(x->m_derived_type)),
                ASR::accessType::Public));
            current_scope->add_symbol(imported_name, m_derived_type);
        }
        x->m_derived_type = m_derived_type;
    }

};

static inline ASR::ttype_t* fix_scoped_type(Allocator& al,
    ASR::ttype_t* type, SymbolTable* scope) {
    ASRUtils::ExprStmtDuplicator expr_duplicator(al);
    expr_duplicator.allow_procedure_calls = true;
    ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(type);
    ASRUtils::FixScopedTypeVisitor fixer(al, scope);
    fixer.replace_ttype(type_);
    return type_;

}

class ReplaceWithFunctionParamVisitor: public ASR::BaseExprReplacer<ReplaceWithFunctionParamVisitor> {

    private:

    Allocator& al;

    ASR::expr_t** m_args;

    size_t n_args;

    SymbolTable* current_scope;

    public:

    ReplaceWithFunctionParamVisitor(Allocator& al_, ASR::expr_t** m_args_, size_t n_args_) :
        al(al_), m_args(m_args_), n_args(n_args_), current_scope(nullptr) {}

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
                                ASRUtils::symbol_type(x->m_v), current_scope);
            *current_expr = ASRUtils::EXPR(ASR::make_FunctionParam_t(
                                al, m_args[arg_idx]->base.loc, arg_idx,
                                t_, nullptr));
        }
    }

    void replace_StructType(ASR::StructType_t *x) {
        std::string derived_type_name = ASRUtils::symbol_name(x->m_derived_type);
        ASR::symbol_t* derived_type_sym = current_scope->resolve_symbol(derived_type_name);
        LCOMPILERS_ASSERT_MSG( derived_type_sym != nullptr,
                    "derived_type_sym cannot be nullptr");
        if (derived_type_sym != x->m_derived_type) {
            x->m_derived_type = derived_type_sym;
        }
    }

    ASR::ttype_t* replace_args_with_FunctionParam(ASR::ttype_t* t, SymbolTable* current_scope) {
        this->current_scope = current_scope;

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

class ReplaceFunctionParamVisitor: public ASR::BaseExprReplacer<ReplaceFunctionParamVisitor> {

    private:

    ASR::call_arg_t* m_args;

    public:

    ReplaceFunctionParamVisitor(ASR::call_arg_t* m_args_) :
        m_args(m_args_) {}

    void replace_FunctionParam(ASR::FunctionParam_t* x) {
        *current_expr = m_args[x->m_param_number].m_value;
    }

};

inline ASR::asr_t* make_FunctionType_t_util(Allocator &al,
    const Location &a_loc, ASR::expr_t** a_args, size_t n_args,
    ASR::expr_t* a_return_var, ASR::abiType a_abi, ASR::deftypeType a_deftype,
    char* a_bindc_name, bool a_elemental, bool a_pure, bool a_module, bool a_inline,
    bool a_static,
    ASR::symbol_t** a_restrictions, size_t n_restrictions, bool a_is_restriction, SymbolTable* current_scope) {
    Vec<ASR::ttype_t*> arg_types;
    arg_types.reserve(al, n_args);
    ReplaceWithFunctionParamVisitor replacer(al, a_args, n_args);
    for( size_t i = 0; i < n_args; i++ ) {
        // We need to substitute all direct argument variable references with
        // FunctionParam.
        ASR::ttype_t *t = replacer.replace_args_with_FunctionParam(
                            expr_type(a_args[i]), current_scope);
        arg_types.push_back(al, t);
    }
    ASR::ttype_t* return_var_type = nullptr;
    if( a_return_var ) {
        return_var_type = replacer.replace_args_with_FunctionParam(
                            ASRUtils::expr_type(a_return_var), current_scope);
    }

    LCOMPILERS_ASSERT(arg_types.size() == n_args);
    return ASR::make_FunctionType_t(
        al, a_loc, arg_types.p, arg_types.size(), return_var_type, a_abi, a_deftype,
        a_bindc_name, a_elemental, a_pure, a_module, a_inline,
        a_static, a_restrictions, n_restrictions,
        a_is_restriction);
}

inline ASR::asr_t* make_FunctionType_t_util(Allocator &al, const Location &a_loc,
    ASR::expr_t** a_args, size_t n_args, ASR::expr_t* a_return_var, ASR::FunctionType_t* ft, SymbolTable* current_scope) {
    return ASRUtils::make_FunctionType_t_util(al, a_loc, a_args, n_args, a_return_var,
        ft->m_abi, ft->m_deftype, ft->m_bindc_name, ft->m_elemental,
        ft->m_pure, ft->m_module, ft->m_inline, ft->m_static,
        ft->m_restrictions,
        ft->n_restrictions, ft->m_is_restriction, current_scope);
}

inline ASR::asr_t* make_Function_t_util(Allocator& al, const Location& loc,
    SymbolTable* m_symtab, char* m_name, char** m_dependencies, size_t n_dependencies,
    ASR::expr_t** a_args, size_t n_args, ASR::stmt_t** m_body, size_t n_body,
    ASR::expr_t* m_return_var, ASR::abiType m_abi, ASR::accessType m_access,
    ASR::deftypeType m_deftype, char* m_bindc_name, bool m_elemental, bool m_pure,
    bool m_module, bool m_inline, bool m_static,
    ASR::symbol_t** m_restrictions, size_t n_restrictions, bool m_is_restriction,
    bool m_deterministic, bool m_side_effect_free, char *m_c_header=nullptr, Location* m_start_name = nullptr,
    Location* m_end_name = nullptr) {
    ASR::ttype_t* func_type = ASRUtils::TYPE(ASRUtils::make_FunctionType_t_util(
        al, loc, a_args, n_args, m_return_var, m_abi, m_deftype, m_bindc_name,
        m_elemental, m_pure, m_module, m_inline, m_static,
        m_restrictions, n_restrictions, m_is_restriction, m_symtab));
    return ASR::make_Function_t(
        al, loc, m_symtab, m_name, func_type, m_dependencies, n_dependencies,
        a_args, n_args, m_body, n_body, m_return_var, m_access, m_deterministic,
        m_side_effect_free, m_c_header, m_start_name, m_end_name);
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
            case ASR::symbolType::Struct: {
                ASR::Struct_t* struct_type = ASR::down_cast<ASR::Struct_t>(symbol);
                new_symbol = duplicate_Struct(struct_type, destination_symtab);
                new_symbol_name = struct_type->m_name;
                break;
            }
            case ASR::symbolType::GenericProcedure: {
                ASR::GenericProcedure_t* generic_procedure = ASR::down_cast<ASR::GenericProcedure_t>(symbol);
                new_symbol = duplicate_GenericProcedure(generic_procedure, destination_symtab);
                new_symbol_name = generic_procedure->m_name;
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
        if (ASR::is_a<ASR::StructType_t>(*m_type)) {
            ASR::StructType_t* st = ASR::down_cast<ASR::StructType_t>(m_type);
            std::string derived_type_name = ASRUtils::symbol_name(st->m_derived_type);
            ASR::symbol_t* derived_type_sym = destination_symtab->resolve_symbol(derived_type_name);
            LCOMPILERS_ASSERT_MSG( derived_type_sym != nullptr, "derived_type_sym cannot be nullptr");
            if (derived_type_sym != st->m_derived_type) {
                st->m_derived_type = derived_type_sym;
            }
        }
        return ASR::down_cast<ASR::symbol_t>(
            ASRUtils::make_Variable_t_util(al, variable->base.base.loc, destination_symtab,
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

        ASRUtils::ExprStmtWithScopeDuplicator scoped_node_duplicator(al, function_symtab);
        scoped_node_duplicator.allow_procedure_calls = true;
        scoped_node_duplicator.allow_reshape = false;

        for( size_t i = 0; i < function->n_body; i++ ) {
            scoped_node_duplicator.success = true;
            ASR::stmt_t* new_stmt = scoped_node_duplicator.duplicate_stmt(function->m_body[i]);
            if (!scoped_node_duplicator.success) {
                return nullptr;
            }
            new_body.push_back(al, new_stmt);
        }

        Vec<ASR::expr_t*> new_args;
        new_args.reserve(al, function->n_args);
        for( size_t i = 0; i < function->n_args; i++ ) {
            scoped_node_duplicator.success = true;
            ASR::expr_t* new_arg = scoped_node_duplicator.duplicate_expr(function->m_args[i]);
            if (!scoped_node_duplicator.success) {
                return nullptr;
            }
            new_args.push_back(al, new_arg);
        }

        ASR::expr_t* new_return_var = function->m_return_var;
        if( new_return_var ) {
            scoped_node_duplicator.success = true;
            new_return_var = scoped_node_duplicator.duplicate_expr(function->m_return_var);
            if (!scoped_node_duplicator.success) {
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

    ASR::symbol_t* duplicate_Struct(ASR::Struct_t* struct_type_t,
        SymbolTable* destination_symtab) {
        SymbolTable* struct_type_symtab = al.make_new<SymbolTable>(destination_symtab);
        duplicate_SymbolTable(struct_type_t->m_symtab, struct_type_symtab);
        return ASR::down_cast<ASR::symbol_t>(ASR::make_Struct_t(
            al, struct_type_t->base.base.loc, struct_type_symtab,
            struct_type_t->m_name, struct_type_t->m_dependencies, struct_type_t->n_dependencies,
            struct_type_t->m_members, struct_type_t->n_members,
            struct_type_t->m_member_functions, struct_type_t->n_member_functions, struct_type_t->m_abi,
            struct_type_t->m_access, struct_type_t->m_is_packed, struct_type_t->m_is_abstract,
            struct_type_t->m_initializers, struct_type_t->n_initializers, struct_type_t->m_alignment,
            struct_type_t->m_parent));
    }
    ASR::symbol_t* duplicate_GenericProcedure(ASR::GenericProcedure_t* genericProcedure, SymbolTable* destination_symtab){
        return ASR::down_cast<ASR::symbol_t>(ASR::make_GenericProcedure_t(
            al, genericProcedure->base.base.loc, destination_symtab,
            genericProcedure->m_name, genericProcedure->m_procs,
            genericProcedure->n_procs, genericProcedure->m_access));
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
    ASR::expr_t* start_value = nullptr;
    ASR::expr_t* end_value = nullptr;
    if (start != nullptr) {
        start_value = ASRUtils::expr_value(start);
    }
    if (end != nullptr) {
        end_value = ASRUtils::expr_value(end);
    }

    // If both start and end have compile time values
    // then length can be computed easily by extracting
    // compile time values of end and start.
    if( start_value && end_value ) {
        int64_t start_int = -1, end_int = -1;
        ASRUtils::extract_value(start_value, start_int);
        ASRUtils::extract_value(end_value, end_int);
        end_int = end_int < 0 ? 0 : end_int;
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
        if( ASR::is_a<ASR::ClassType_t>(*typei) ||
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
            !ASR::is_a<ASR::StructType_t>(*argi->m_type) &&
            !ASR::is_a<ASR::String_t>(*argi->m_type) &&
            argi->m_presence != ASR::presenceType::Optional) {
            v.push_back(i);
        }
    }
    return v.size() > 0;
}

template <typename SemanticAbort>
static inline ASR::expr_t* get_bound(ASR::expr_t* arr_expr, int dim,
                                     std::string bound, Allocator& al, diag::Diagnostics &diag) {
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
        if ( ASR::is_a<ASR::Var_t>(*arr_expr )) {
            ASR::Var_t* non_array_var = ASR::down_cast<ASR::Var_t>(arr_expr);
            ASR::Variable_t* non_array_variable = ASR::down_cast<ASR::Variable_t>(
                symbol_get_past_external(non_array_var->m_v));
            std::string msg;
            if (arr_n_dims == 0) {
                msg = "Variable " + std::string(non_array_variable->m_name) +
                            " is not an array so it cannot be indexed.";
            } else {
                msg = "Variable " + std::string(non_array_variable->m_name) +
                            " does not have enough dimensions.";
            }
            diag.add(diag::Diagnostic(
                msg, diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("", {arr_expr->base.loc})}));
            throw SemanticAbort();
        } else if ( ASR::is_a<ASR::StructInstanceMember_t>(*arr_expr )) {
            ASR::StructInstanceMember_t* non_array_struct_inst_mem = ASR::down_cast<ASR::StructInstanceMember_t>(arr_expr);
            ASR::Variable_t* non_array_variable = ASR::down_cast<ASR::Variable_t>(
                symbol_get_past_external(non_array_struct_inst_mem->m_m));
            std::string msg;
            if (arr_n_dims == 0) {
                msg = "Type member " + std::string(non_array_variable->m_name) +
                            " is not an array so it cannot be indexed.";
            } else {
                msg = "Type member " + std::string(non_array_variable->m_name) +
                            " does not have enough dimensions.";
            }
            diag.add(diag::Diagnostic(
                msg, diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("", {arr_expr->base.loc})}));
            throw SemanticAbort();
        } else {
            diag.add(diag::Diagnostic(
                "Expression cannot be indexed",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("", {arr_expr->base.loc})}));
            throw SemanticAbort();
        }
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

static inline ASR::expr_t* get_size(ASR::expr_t* arr_expr, int dim,
                                    Allocator& al) {
    ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, arr_expr->base.loc, 4));
    ASR::expr_t* dim_expr = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arr_expr->base.loc, dim, int32_type));
    return ASRUtils::EXPR(ASRUtils::make_ArraySize_t_util(al, arr_expr->base.loc, arr_expr, dim_expr,
                                                int32_type, nullptr));
}

static inline ASR::expr_t* get_size(ASR::expr_t* arr_expr, Allocator& al, bool for_type=true) {
    ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, arr_expr->base.loc, 4));
    return ASRUtils::EXPR(ASRUtils::make_ArraySize_t_util(al, arr_expr->base.loc, arr_expr, nullptr, int32_type, nullptr, for_type));
}

static inline ASR::Enum_t* get_Enum_from_symbol(ASR::symbol_t* s) {
    ASR::Variable_t* s_var = ASR::down_cast<ASR::Variable_t>(s);
    if( ASR::is_a<ASR::EnumType_t>(*s_var->m_type) ) {
        ASR::EnumType_t* enum_ = ASR::down_cast<ASR::EnumType_t>(s_var->m_type);
        return ASR::down_cast<ASR::Enum_t>(enum_->m_enum_type);
    }
    ASR::symbol_t* enum_type_cand = ASR::down_cast<ASR::symbol_t>(s_var->m_parent_symtab->asr_owner);
    LCOMPILERS_ASSERT(ASR::is_a<ASR::Enum_t>(*enum_type_cand));
    return ASR::down_cast<ASR::Enum_t>(enum_type_cand);
}

static inline bool is_abstract_class_type(ASR::ttype_t* type) {
    type = ASRUtils::type_get_past_array(type);
    if( !ASR::is_a<ASR::ClassType_t>(*type) ) {
        return false;
    }
    ASR::ClassType_t* class_t = ASR::down_cast<ASR::ClassType_t>(type);
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
        std::string current_name;

    public:

        CollectIdentifiersFromASRExpression(Allocator& al_, SetChar& identifiers_, std::string current_name_ = "") :
        al(al_), identifiers(identifiers_), current_name(current_name_)
        {}

        void visit_Var(const ASR::Var_t& x) {
            if (ASRUtils::symbol_name(x.m_v) != this->current_name) {
                identifiers.push_back(al, ASRUtils::symbol_name(x.m_v));
            }
        }
};

static inline void collect_variable_dependencies(Allocator& al, SetChar& deps_vec,
    ASR::ttype_t* type=nullptr, ASR::expr_t* init_expr=nullptr,
    ASR::expr_t* value=nullptr, std::string current_name="") {
    ASRUtils::CollectIdentifiersFromASRExpression collector(al, deps_vec, current_name);
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

static inline int KMP_string_match_count(std::string &s_var, std::string &sub) {
    int str_len = s_var.size();
    int sub_len = sub.size();
    int count = 0;
    std::vector<int> lps(sub_len, 0);
    if (sub_len == 0) {
        count = str_len + 1;
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
        for (int i = 0, j = 0; (str_len - i) >= (sub_len - j);) {
            if (sub[j] == s_var[i]) {
                j++, i++;
            }
            if (j == sub_len) {
                count++;
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
    return count;
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
        diagnostics.message_label(error_msg,
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

static inline bool is_allocatable(ASR::ttype_t* type) {
    return ASR::is_a<ASR::Allocatable_t>(*type);
}

static inline void import_struct_t(Allocator& al,
    const Location& loc, ASR::ttype_t*& var_type,
    ASR::intentType intent, SymbolTable* current_scope) {
    bool is_pointer = ASRUtils::is_pointer(var_type);
    bool is_allocatable = ASRUtils::is_allocatable(var_type);
    bool is_array = ASRUtils::is_array(var_type);
    ASR::dimension_t* m_dims = nullptr;
    size_t n_dims = ASRUtils::extract_dimensions_from_ttype(var_type, m_dims);
    ASR::array_physical_typeType ptype = ASR::array_physical_typeType::DescriptorArray;
    if( is_array ) {
        ptype = ASRUtils::extract_physical_type(var_type);
    }
    ASR::ttype_t* var_type_unwrapped = ASRUtils::type_get_past_allocatable(
        ASRUtils::type_get_past_pointer(ASRUtils::type_get_past_array(var_type)));
    if( ASR::is_a<ASR::StructType_t>(*var_type_unwrapped) ) {
        ASR::symbol_t* der_sym = ASR::down_cast<ASR::StructType_t>(var_type_unwrapped)->m_derived_type;
        if( (ASR::asr_t*) ASRUtils::get_asr_owner(der_sym) != current_scope->asr_owner ) {
            std::string sym_name = ASRUtils::symbol_name(ASRUtils::symbol_get_past_external(der_sym));
            if( current_scope->resolve_symbol(sym_name) == nullptr ) {
                std::string unique_name = current_scope->get_unique_name(sym_name);
                der_sym = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
                    al, loc, current_scope, s2c(al, unique_name), ASRUtils::symbol_get_past_external(der_sym),
                    ASRUtils::symbol_name(ASRUtils::get_asr_owner(ASRUtils::symbol_get_past_external(der_sym))), nullptr, 0,
                    ASRUtils::symbol_name(ASRUtils::symbol_get_past_external(der_sym)), ASR::accessType::Public));
                current_scope->add_symbol(unique_name, der_sym);
            } else {
                der_sym = current_scope->resolve_symbol(sym_name);
            }
            var_type = ASRUtils::TYPE(ASRUtils::make_StructType_t_util(al, loc, der_sym));
            if( is_array ) {
                var_type = ASRUtils::make_Array_t_util(al, loc, var_type, m_dims, n_dims,
                    ASR::abiType::Source, false, ptype, true);
            }
            if( is_pointer ) {
                var_type = ASRUtils::TYPE(ASR::make_Pointer_t(al, loc, var_type));
            } else if( is_allocatable ) {
                var_type = ASRUtils::TYPE(ASRUtils::make_Allocatable_t_util(al, loc, var_type));
            }
        }
    } else if( ASR::is_a<ASR::String_t>(*var_type_unwrapped) ) {
        ASR::String_t* char_t = ASR::down_cast<ASR::String_t>(var_type_unwrapped);
        if( char_t->m_len == -1 && intent == ASR::intentType::Local ) {
            var_type = ASRUtils::TYPE(ASR::make_String_t(al, loc, char_t->m_kind, 1, nullptr, ASR::string_physical_typeType::PointerString));
            if( is_array ) {
                var_type = ASRUtils::make_Array_t_util(al, loc, var_type, m_dims, n_dims,
                    ASR::abiType::Source, false, ptype, true);
            }
            if( is_pointer ) {
                var_type = ASRUtils::TYPE(ASR::make_Pointer_t(al, loc, var_type));
            } else if( is_allocatable ) {
                var_type = ASRUtils::TYPE(ASRUtils::make_Allocatable_t_util(al, loc, var_type));
            }
        }
    }
}

static inline ASR::asr_t* make_ArrayPhysicalCast_t_util(Allocator &al, const Location &a_loc,
    ASR::expr_t* a_arg, ASR::array_physical_typeType a_old, ASR::array_physical_typeType a_new,
    ASR::ttype_t* a_type, ASR::expr_t* a_value, SymbolTable* current_scope=nullptr) {
    if( ASR::is_a<ASR::ArrayPhysicalCast_t>(*a_arg) ) {
        ASR::ArrayPhysicalCast_t* a_arg_ = ASR::down_cast<ASR::ArrayPhysicalCast_t>(a_arg);
        a_arg = a_arg_->m_arg;
        a_old = ASRUtils::extract_physical_type(ASRUtils::expr_type(a_arg_->m_arg));
    }

    LCOMPILERS_ASSERT(ASRUtils::extract_physical_type(ASRUtils::expr_type(a_arg)) == a_old);
    // TODO: Allow for DescriptorArray to DescriptorArray physical cast for allocatables
    // later on
    if( (a_old == a_new && a_old != ASR::array_physical_typeType::DescriptorArray) ||
        (a_old == a_new && a_old == ASR::array_physical_typeType::DescriptorArray &&
         (ASR::is_a<ASR::Allocatable_t>(*ASRUtils::expr_type(a_arg)) ||
         ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(a_arg)))) ) {
            return (ASR::asr_t*) a_arg;
    }

    if( current_scope ) {
        import_struct_t(al, a_loc, a_type,
            ASR::intentType::Unspecified, current_scope);
    }
    return ASR::make_ArrayPhysicalCast_t(al, a_loc, a_arg, a_old, a_new, a_type, a_value);
}

// inline void flatten_ArrayConstant(Allocator& al, void* data, size_t n_args, Vec<ASR::expr_t*> &new_args) {
//     for (size_t i = 0; i < n_args; i++) {
//         if (ASR::is_a<ASR::ArrayConstant_t>(*a_args[i])) {
//             ASR::ArrayConstant_t* a_arg = ASR::down_cast<ASR::ArrayConstant_t>(a_args[i]);
//             flatten_ArrayConstant(al, a_arg->m_args, a_arg->n_args, new_args);
//         } else if (ASR::is_a<ASR::ArrayConstant_t>(*ASRUtils::expr_value(a_args[i]))) {
//             ASR::ArrayConstant_t* a_arg = ASR::down_cast<ASR::ArrayConstant_t>(ASRUtils::expr_value(a_args[i]));
//             flatten_ArrayConstant(al, a_arg->m_args, a_arg->n_args, new_args);
//         } else {
//             new_args.push_back(al, ASRUtils::expr_value(a_args[i]));
//         }
//     }
// }

// Assigns "x->m_data[i] = value", casting the internal data pointer correctly using the type of "value"
inline void set_ArrayConstant_value(ASR::ArrayConstant_t* x, ASR::expr_t* value, int i) {
    value = ASRUtils::expr_value(value);

    ASR::ttype_t* type = ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(ASRUtils::expr_type(value)));
    int kind = ASRUtils::extract_kind_from_ttype_t(type);

    switch (type->type) {
        case ASR::ttypeType::Integer: {
            ASR::IntegerConstant_t* value_int = ASR::down_cast<ASR::IntegerConstant_t>(value);
            switch (kind) {
                case 1: ((int8_t*)x->m_data)[i] = value_int->m_n; break;
                case 2: ((int16_t*)x->m_data)[i] = value_int->m_n; break;
                case 4: ((int32_t*)x->m_data)[i] = value_int->m_n; break;
                case 8: ((int64_t*)x->m_data)[i] = value_int->m_n; break;
                default:
                    throw LCompilersException("Unsupported kind for integer array constant.");
            }
        }
        case ASR::ttypeType::Real: {
            ASR::RealConstant_t* value_real = ASR::down_cast<ASR::RealConstant_t>(value);
            switch (kind) {
                case 4: ((float*)x->m_data)[i] = value_real->m_r; break;
                case 8: ((double*)x->m_data)[i] = value_real->m_r; break;
                default:
                    throw LCompilersException("Unsupported kind for real array constant.");
            }
        }
        case ASR::ttypeType::UnsignedInteger: {
            ASR::IntegerConstant_t* value_int = ASR::down_cast<ASR::IntegerConstant_t>(value);
            switch (kind) {
                case 1: ((uint8_t*)x->m_data)[i] = value_int->m_n; break;
                case 2: ((uint16_t*)x->m_data)[i] = value_int->m_n; break;
                case 4: ((uint32_t*)x->m_data)[i] = value_int->m_n; break;
                case 8: ((uint64_t*)x->m_data)[i] = value_int->m_n; break;
                default:
                    throw LCompilersException("Unsupported kind for unsigned integer array constant.");
            }
        }
        case ASR::ttypeType::Complex: {
            ASR::ComplexConstant_t* value_complex = ASR::down_cast<ASR::ComplexConstant_t>(value);
            switch (kind) {
                case 4:
                    ((float*)x->m_data)[i] = value_complex->m_re;
                    ((float*)x->m_data)[i+1] = value_complex->m_im; break;
                case 8:
                    ((double*)x->m_data)[i] = value_complex->m_re;
                    ((double*)x->m_data)[i+1] = value_complex->m_im; break;
                default:
                    throw LCompilersException("Unsupported kind for complex array constant.");
            }
        }
        case ASR::ttypeType::Logical: {
            ASR::LogicalConstant_t* value_logical = ASR::down_cast<ASR::LogicalConstant_t>(value);
            ((bool*)x->m_data)[i] = value_logical->m_value;
            break;
        }
        case ASR::ttypeType::String: {
            ASR::String_t* char_type = ASR::down_cast<ASR::String_t>(type);
            int len = char_type->m_len;
            ASR::StringConstant_t* value_str = ASR::down_cast<ASR::StringConstant_t>(value);
            char* data = value_str->m_s;
            for (int j = 0; j < len; j++) {
                *(((char*)x->m_data) + i*len + j) = data[j];
            }
            break;
        }
        default:
            throw LCompilersException("Unsupported type for array constant.");
    }
}

template <typename T>
inline std::string to_string_with_precision(const T a_value, const int n) {
    std::ostringstream out;
    out.precision(n);
    out << std::scientific << a_value;
    return std::move(out).str();
}

inline std::string fetch_ArrayConstant_value(void *data, ASR::ttype_t* type, int i) {
   int kind = ASRUtils::extract_kind_from_ttype_t(type);

    switch (type->type) {
        case ASR::ttypeType::Integer: {
            switch (kind) {
                case 1: return std::to_string(((int8_t*)data)[i]);
                case 2: return std::to_string(((int16_t*)data)[i]);
                case 4: return std::to_string(((int32_t*)data)[i]);
                case 8: return std::to_string(((int64_t*)data)[i]);
                default:
                    throw LCompilersException("Unsupported kind for integer array constant.");
            }
        }
        case ASR::ttypeType::Real: {
            switch (kind) {
                case 4: return to_string_with_precision(((float*)data)[i], 8);
                case 8: return to_string_with_precision(((double*)data)[i], 16);
                default:
                    throw LCompilersException("Unsupported kind for real array constant.");
            }
        }
        case ASR::ttypeType::UnsignedInteger: {
            switch (kind) {
                case 1: return std::to_string(((uint8_t*)data)[i]);
                case 2: return std::to_string(((uint16_t*)data)[i]);
                case 4: return std::to_string(((uint32_t*)data)[i]);
                case 8: return std::to_string(((uint64_t*)data)[i]);
                default:
                    throw LCompilersException("Unsupported kind for unsigned integer array constant.");
            }
        }
        case ASR::ttypeType::Complex: {
            switch (kind) {
                case 4: return "("+(to_string_with_precision(*(((float*)data) + 2*i), 8))+", "+ (to_string_with_precision(*(((float*)data) + 2*i + 1), 8)) + ")";
                case 8: return "("+(to_string_with_precision(*(((double*)data) + 2*i), 16))+", "+ (to_string_with_precision(*(((double*)data) + 2*i + 1), 16)) + ")";
                default:
                    throw LCompilersException("Unsupported kind for complex array constant.");
            }
        }
        case ASR::ttypeType::Logical: {
            if (((bool*)data)[i] == 1) return ".true.";
            return ".false.";
        }
        case ASR::ttypeType::String: {
            ASR::String_t* char_type = ASR::down_cast<ASR::String_t>(type);
            int len = char_type->m_len;
            char* data_char = (char*)data + i*len;
            // take first len characters
            char* new_char = new char[len + 1];
            for (int j = 0; j < len; j++) {
                new_char[j] = data_char[j];
            }
            new_char[len] = '\0';
            return '\"' + std::string(new_char) + '\"';
        }
        default:
            throw LCompilersException("Unsupported type for array constant.");
    }
}

inline std::string fetch_ArrayConstant_value(ASR::ArrayConstant_t* x, int i) {
    ASR::ttype_t* type = ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(x->m_type));
    return fetch_ArrayConstant_value(x->m_data, type, i);
}

inline std::string fetch_ArrayConstant_value(ASR::ArrayConstant_t &x, int i) {
    ASR::ttype_t* type = ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(x.m_type));
    return fetch_ArrayConstant_value(x.m_data, type, i);
}

inline std::string fetch_ArrayConstant_value(const ASR::ArrayConstant_t &x, int i) {
    ASR::ttype_t* type = ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(x.m_type));
    return fetch_ArrayConstant_value(x.m_data, type, i);
}

inline ASR::expr_t* fetch_ArrayConstant_value_helper(Allocator &al, const Location& loc, void *data, ASR::ttype_t* type, int i) {
    int kind = ASRUtils::extract_kind_from_ttype_t(type);
    ASR::expr_t* value = nullptr;
    switch (type->type) {
        case ASR::ttypeType::Integer : {
            switch (kind) {
                case 1: value = EXPR(ASR::make_IntegerConstant_t(al, loc,
                                    ((int8_t*)data)[i], type)); break;
                case 2: value = EXPR(ASR::make_IntegerConstant_t(al, loc,
                                    ((int16_t*)data)[i], type)); break;
                case 4: value = EXPR(ASR::make_IntegerConstant_t(al, loc,
                                    ((int32_t*)data)[i], type)); break;
                case 8: value = EXPR(ASR::make_IntegerConstant_t(al, loc,
                                    ((int64_t*)data)[i], type)); break;
                default:
                    throw LCompilersException("Unsupported kind for integer array constant.");
            }
            return value;
        }
        case ASR::ttypeType::Real: {
            switch (kind) {
                case 4: value = EXPR(ASR::make_RealConstant_t(al, loc,
                                    ((float*)data)[i], type)); break;
                case 8: value = EXPR(ASR::make_RealConstant_t(al, loc,
                                    ((double*)data)[i], type)); break;
                default:
                    throw LCompilersException("Unsupported kind for real array constant.");
            }
            return value;
        }
        case ASR::ttypeType::UnsignedInteger: {
            switch (kind) {
                case 1: value = EXPR(ASR::make_IntegerConstant_t(al, loc,
                                    ((uint8_t*)data)[i], type)); break;
                case 2: value = EXPR(ASR::make_IntegerConstant_t(al, loc,
                                    ((uint16_t*)data)[i], type)); break;
                case 4: value = EXPR(ASR::make_IntegerConstant_t(al, loc,
                                    ((uint32_t*)data)[i], type)); break;
                case 8: value = EXPR(ASR::make_IntegerConstant_t(al, loc,
                                    ((uint64_t*)data)[i], type)); break;
                default:
                    throw LCompilersException("Unsupported kind for unsigned integer array constant.");
            }
            return value;
        }
        case ASR::ttypeType::Complex: {
            switch (kind) {
                case 4: value = EXPR(ASR::make_ComplexConstant_t(al, loc,
                                    *(((float*)data) + 2*i), *(((float*)data) + 2*i + 1), type)); break;
                case 8: value = EXPR(ASR::make_ComplexConstant_t(al, loc,
                                    *(((double*)data) + 2*i), *(((double*)data) + 2*i + 1), type)); break;
                default:
                    throw LCompilersException("Unsupported kind for complex array constant.");
            }
            return value;
        }
        case ASR::ttypeType::Logical: {
            value = EXPR(ASR::make_LogicalConstant_t(al, loc,
                                ((bool*)data)[i], type));
            return value;
        }
        case ASR::ttypeType::String: {
            ASR::String_t* char_type = ASR::down_cast<ASR::String_t>(type);
            int len = char_type->m_len;
            char* data_char = (char*)data;
            std::string str = std::string(data_char + i*len, len);
            value = EXPR(ASR::make_StringConstant_t(al, loc,
                                s2c(al, str), type));
            return value;
        }
        default:
            throw LCompilersException("Unsupported type for array constant.");
    }
}

inline ASR::expr_t* fetch_ArrayConstant_value(Allocator &al, ASR::ArrayConstant_t* x, int i) {
    ASR::ttype_t* type = ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(x->m_type));
    return fetch_ArrayConstant_value_helper(al, x->base.base.loc, x->m_data, type, i);
}

inline ASR::expr_t* fetch_ArrayConstant_value(Allocator &al, ASR::ArrayConstant_t &x, int i) {
    ASR::ttype_t* type = ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(x.m_type));
    return fetch_ArrayConstant_value_helper(al, x.base.base.loc, x.m_data, type, i);
}

inline ASR::expr_t* fetch_ArrayConstant_value(Allocator &al, const ASR::ArrayConstant_t &x, int i) {
    ASR::ttype_t* type = ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(x.m_type));
    return fetch_ArrayConstant_value_helper(al, x.base.base.loc, x.m_data, type, i);
}

template<typename T>
T* set_data_int(T* data, ASR::expr_t** a_args, size_t n_args) {
    for (size_t i = 0; i < n_args; i++) {
        data[i] = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(a_args[i]))->m_n;
    }
    return data;
}

template<typename T>
T* set_data_real(T* data, ASR::expr_t** a_args, size_t n_args) {
    for (size_t i = 0; i < n_args; i++) {
        data[i] = ASR::down_cast<ASR::RealConstant_t>(ASRUtils::expr_value(a_args[i]))->m_r;
    }
    return data;
}

template<typename T>
T* set_data_complex(T* data, ASR::expr_t** a_args, size_t n_args) {
    for (size_t i = 0; i < n_args; i++) {
        data[2*i] = ASR::down_cast<ASR::ComplexConstant_t>(ASRUtils::expr_value(a_args[i]))->m_re;
        data[2*i + 1] = ASR::down_cast<ASR::ComplexConstant_t>(ASRUtils::expr_value(a_args[i]))->m_im;
    }
    return data;
}

inline void* set_ArrayConstant_data(ASR::expr_t** a_args, size_t n_args, ASR::ttype_t* a_type) {
    int kind = ASRUtils::extract_kind_from_ttype_t(a_type);
    switch (a_type->type) {
        case ASR::ttypeType::Integer: {
            switch (kind) {
                case 1: return set_data_int(new int8_t[n_args], a_args, n_args);
                case 2: return set_data_int(new int16_t[n_args], a_args, n_args);
                case 4: return set_data_int(new int32_t[n_args], a_args, n_args);
                case 8: return set_data_int(new int64_t[n_args], a_args, n_args);
                default:
                    throw LCompilersException("Unsupported kind for integer array constant.");
            }
        }
        case ASR::ttypeType::Real: {
            switch (kind) {
                case 4: return set_data_real(new float[n_args], a_args, n_args);
                case 8: return set_data_real(new double[n_args], a_args, n_args);
                default:
                    throw LCompilersException("Unsupported kind for real array constant.");
            }
        }
        case ASR::ttypeType::UnsignedInteger: {
            switch (kind) {
                case 1: return set_data_int(new uint8_t[n_args], a_args, n_args);
                case 2: return set_data_int(new uint16_t[n_args], a_args, n_args);
                case 4: return set_data_int(new uint32_t[n_args], a_args, n_args);
                case 8: return set_data_int(new uint64_t[n_args], a_args, n_args);
                default:
                    throw LCompilersException("Unsupported kind for unsigned integer array constant.");
            }
        }
        case ASR::ttypeType::Complex: {
            switch (kind) {
                case 4: return set_data_complex(new float[2*n_args], a_args, n_args);
                case 8: return set_data_complex(new double[2*n_args], a_args, n_args);
                default:
                    throw LCompilersException("Unsupported kind for complex array constant.");
            }
        }
        case ASR::ttypeType::Logical: {
            bool* data = new bool[n_args];
            for (size_t i = 0; i < n_args; i++) {
                data[i] = ASR::down_cast<ASR::LogicalConstant_t>(ASRUtils::expr_value(a_args[i]))->m_value;
            }
            return (void*) data;
        }
        case ASR::ttypeType::String: {
            int len = ASR::down_cast<ASR::String_t>(a_type)->m_len;
            char* data = new char[len*n_args + 1];
            for (size_t i = 0; i < n_args; i++) {
                char* value = ASR::down_cast<ASR::StringConstant_t>(ASRUtils::expr_value(a_args[i]))->m_s;
                for (int j = 0; j < len; j++) {
                    data[i*len + j] = value[j];
                }
            }
            data[len*n_args] = '\0';
            return (void*) data;
        }
        default:
            throw LCompilersException("Unsupported type for array constant.");
    }
}

inline void flatten_ArrayConstant_data(Allocator &al, Vec<ASR::expr_t*> &data, ASR::expr_t** a_args, size_t n_args, ASR::ttype_t* a_type, int &curr_idx, ASR::ArrayConstant_t* x = nullptr) {
    if (x != nullptr) {
        // this is array constant, we have it's data available
        void* x_data = x->m_data;
        for (size_t i = 0; i < (size_t) ASRUtils::get_fixed_size_of_array(x->m_type); i++) {
            ASR::expr_t* value = fetch_ArrayConstant_value_helper(al, x->base.base.loc, x_data, a_type, i);
            if (ASR::is_a<ASR::ArrayConstant_t>(*value)) {
                ASR::ArrayConstant_t* value_ = ASR::down_cast<ASR::ArrayConstant_t>(value);
                flatten_ArrayConstant_data(al, data, a_args, n_args, a_type, curr_idx, value_);
            } else {
                data.push_back(al, value);
                curr_idx++;
            }
        }
        return;
    }
    for (size_t i = 0; i < n_args; i++) {
        ASR::expr_t* a_value = ASRUtils::expr_value(a_args[i]);
        if (ASR::is_a<ASR::ArrayConstant_t>(*a_value)) {
            ASR::ArrayConstant_t* a_value_ = ASR::down_cast<ASR::ArrayConstant_t>(a_value);
            flatten_ArrayConstant_data(al, data, a_args, n_args, a_type, curr_idx, a_value_);
        } else {
            data.push_back(al, a_value);
            curr_idx++;
        }
    }
}

inline ASR::asr_t* make_ArrayConstructor_t_util(Allocator &al, const Location &a_loc,
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
    bool all_expr_evaluated = n_args > 0;
    bool is_array_item_constant = n_args > 0 && (ASR::is_a<ASR::IntegerConstant_t>(*a_args[0]) ||
                                ASR::is_a<ASR::RealConstant_t>(*a_args[0]) ||
                                ASR::is_a<ASR::ComplexConstant_t>(*a_args[0]) ||
                                ASR::is_a<ASR::LogicalConstant_t>(*a_args[0]) ||
                                ASR::is_a<ASR::StringConstant_t>(*a_args[0]) ||
                                ASR::is_a<ASR::IntegerUnaryMinus_t>(*a_args[0]) ||
                                ASR::is_a<ASR::RealUnaryMinus_t>(*a_args[0]));
    if( n_args > 0 ) {
        is_array_item_constant = is_array_item_constant || ASR::is_a<ASR::ArrayConstant_t>(*a_args[0]);
    }
    ASR::expr_t* value = nullptr;
    for (size_t i = 0; i < n_args; i++) {
        ASR::expr_t* a_value = ASRUtils::expr_value(a_args[i]);
        if (!is_value_constant(a_value)) {
            all_expr_evaluated = false;
        }
    }
    if (all_expr_evaluated) {
        Vec<ASR::expr_t*> a_args_values; a_args_values.reserve(al, n_args);
        int curr_idx = 0;
        a_type = ASRUtils::type_get_past_pointer(a_type);
        ASR::Array_t* a_type_ = ASR::down_cast<ASR::Array_t>(a_type);
        flatten_ArrayConstant_data(al, a_args_values, a_args, n_args, a_type_->m_type, curr_idx, nullptr);
        Vec<ASR::dimension_t> dims; dims.reserve(al, 1);
        ASR::dimension_t dim; dim.loc = a_type_->m_dims[0].loc; dim.m_start = a_type_->m_dims[0].m_start;
        dim.m_length = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, a_type_->m_dims[0].loc,
                        curr_idx,
                        ASRUtils::TYPE(ASR::make_Integer_t(al, a_loc, 4))));
        dims.push_back(al, dim);
        ASR::ttype_t* new_type = ASRUtils::TYPE(ASR::make_Array_t(al, a_type->base.loc, a_type_->m_type,
            dims.p, dims.n, a_type_->m_physical_type));
        void *data = set_ArrayConstant_data(a_args_values.p, curr_idx, a_type_->m_type);
        // data is always allocated to n_data bytes
        int64_t n_data = curr_idx * extract_kind_from_ttype_t(a_type_->m_type);
        if (is_character(*a_type_->m_type)) {
            n_data = curr_idx * ASR::down_cast<ASR::String_t>(a_type_->m_type)->m_len;
        }
        value = ASRUtils::EXPR(ASR::make_ArrayConstant_t(al, a_loc, n_data, data, new_type, a_storage_format));
    }

    return is_array_item_constant && all_expr_evaluated ? (ASR::asr_t*) value :
            ASR::make_ArrayConstructor_t(al, a_loc, a_args, n_args, a_type,
            value, a_storage_format);
}

void make_ArrayBroadcast_t_util(Allocator& al, const Location& loc,
    ASR::expr_t*& expr1, ASR::expr_t*& expr2);

// Wraps argument in stringformat if it's not a single argument of type Character.
static inline ASR::asr_t* make_print_t_util(Allocator& al, const Location& loc,
    ASR::expr_t** a_args, size_t n_args){
    LCOMPILERS_ASSERT(n_args > 0);
    if(n_args == 1 && ASR::is_a<ASR::String_t>(*ASRUtils::expr_type(a_args[0]))){
        return ASR::make_Print_t(al, loc, a_args[0]);
    } else {
        ASR::ttype_t *char_type = ASRUtils::TYPE(ASR::make_String_t(
            al, loc, -1, 0, nullptr, ASR::string_physical_typeType::PointerString));
        return ASR::make_Print_t(al, loc,
            ASRUtils::EXPR(ASR::make_StringFormat_t(al, loc, nullptr, a_args,n_args,
            ASR::string_format_kindType::FormatFortran, char_type, nullptr)));
    }
}


static inline void Call_t_body(Allocator& al, ASR::symbol_t* a_name,
    ASR::call_arg_t* a_args, size_t n_args, ASR::expr_t* a_dt, ASR::stmt_t** cast_stmt,
    bool implicit_argument_casting, bool nopass) {
    bool is_method = (a_dt != nullptr) && (!nopass);
    ASR::symbol_t* a_name_ = ASRUtils::symbol_get_past_external(a_name);
    if( ASR::is_a<ASR::Variable_t>(*a_name_) ) {
        is_method = false;
    }
    ASR::FunctionType_t* func_type = get_FunctionType(a_name);

    for( size_t i = 0; i < n_args; i++ ) {
        if( a_args[i].m_value == nullptr ) {
            continue;
        }
        ASR::expr_t* arg = a_args[i].m_value;
        ASR::ttype_t* arg_type = ASRUtils::type_get_past_allocatable(
            ASRUtils::type_get_past_pointer(ASRUtils::expr_type(arg)));
        ASR::ttype_t* orig_arg_type = ASRUtils::type_get_past_allocatable(
            ASRUtils::type_get_past_pointer(func_type->m_arg_types[i + is_method]));
        // cast string source based on the dest
        if( ASRUtils::is_character(*orig_arg_type) &&
            !ASRUtils::is_descriptorString(orig_arg_type) &&
            ASRUtils::is_descriptorString(ASRUtils::expr_type(a_args[i].m_value))){
            a_args[i].m_value = ASRUtils::cast_string_descriptor_to_pointer(al, a_args[i].m_value);
        }
        if( !ASRUtils::is_intrinsic_symbol(a_name_) &&
            !(ASR::is_a<ASR::ClassType_t>(*ASRUtils::type_get_past_array(arg_type)) ||
              ASR::is_a<ASR::ClassType_t>(*ASRUtils::type_get_past_array(orig_arg_type))) &&
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
                        ASR::asr_t* cast_ = ASRUtils::make_Variable_t_util(al, arg->base.loc,
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
                        ASR::asr_t* array_constant = ASRUtils::make_ArrayConstructor_t_util(al, arg->base.loc, args_.p, args_.size(), array_type, ASR::arraystorageType::ColMajor);

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
            ASR::Array_t* arg_array_t = ASR::down_cast<ASR::Array_t>(
                ASRUtils::type_get_past_pointer(arg_type));
            ASR::Array_t* orig_arg_array_t = ASR::down_cast<ASR::Array_t>(
                ASRUtils::type_get_past_pointer(orig_arg_type));
            if( (arg_array_t->m_physical_type != orig_arg_array_t->m_physical_type) ||
                (arg_array_t->m_physical_type == ASR::array_physical_typeType::DescriptorArray &&
                 arg_array_t->m_physical_type == orig_arg_array_t->m_physical_type &&
                 !ASRUtils::is_intrinsic_symbol(a_name_)) ) {
                ASR::call_arg_t physical_cast_arg;
                physical_cast_arg.loc = arg->base.loc;
                Vec<ASR::dimension_t>* dimensions = nullptr;
                Vec<ASR::dimension_t> dimension_;
                if( ASRUtils::is_fixed_size_array(orig_arg_array_t->m_dims, orig_arg_array_t->n_dims) ) {
                    dimension_.reserve(al, orig_arg_array_t->n_dims);
                    dimension_.from_pointer_n_copy(al, orig_arg_array_t->m_dims, orig_arg_array_t->n_dims);
                    dimensions = &dimension_;
                }
                //TO DO : Add appropriate errors in 'asr_uttils.h'.
                LCOMPILERS_ASSERT_MSG(dimensions_compatible(arg_array_t->m_dims, arg_array_t->n_dims,
                    orig_arg_array_t->m_dims, orig_arg_array_t->n_dims, false),
                    "Incompatible dimensions passed to " + (std::string)(ASR::down_cast<ASR::Function_t>(a_name_)->m_name)
                    + "(" + std::to_string(get_fixed_size_of_array(arg_array_t->m_dims,arg_array_t->n_dims)) + "/" + std::to_string(get_fixed_size_of_array(orig_arg_array_t->m_dims,orig_arg_array_t->n_dims))+")");

                ASR::ttype_t* physical_cast_type = ASRUtils::type_get_past_allocatable(
                                                        ASRUtils::expr_type(arg));
                physical_cast_arg.m_value = ASRUtils::EXPR(
                                                ASRUtils::make_ArrayPhysicalCast_t_util(
                                                    al,
                                                    arg->base.loc,
                                                    arg,
                                                    arg_array_t->m_physical_type,
                                                    orig_arg_array_t->m_physical_type,
                                                    ASRUtils::duplicate_type(al,
                                                                            physical_cast_type,
                                                                            dimensions,
                                                                            orig_arg_array_t->m_physical_type,
                                                                            true),
                                                    nullptr));
                a_args[i] = physical_cast_arg;
            }
        }
    }
}

static inline bool is_elemental(ASR::symbol_t* x) {
    x = ASRUtils::symbol_get_past_external(x);
    if( !ASR::is_a<ASR::Function_t>(*x) ) {
        return false;
    }
    return ASRUtils::get_FunctionType(
        ASR::down_cast<ASR::Function_t>(x))->m_elemental;
}

static inline ASR::asr_t* make_FunctionCall_t_util(
    Allocator &al, const Location &a_loc, ASR::symbol_t* a_name,
    ASR::symbol_t* a_original_name, ASR::call_arg_t* a_args, size_t n_args,
    ASR::ttype_t* a_type, ASR::expr_t* a_value, ASR::expr_t* a_dt, bool nopass=false) {

    Call_t_body(al, a_name, a_args, n_args, a_dt, nullptr, false, nopass);

    if( ASRUtils::is_array(a_type) && ASRUtils::is_elemental(a_name) &&
        !ASRUtils::is_fixed_size_array(a_type) &&
        !ASRUtils::is_dimension_dependent_only_on_arguments(a_type) ) {
        ASR::ttype_t* type_ = ASRUtils::extract_type(a_type);
        #define i32j(j) ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, a_loc, j, \
            ASRUtils::TYPE(ASR::make_Integer_t(al, a_loc, 4))))
        ASR::expr_t* i32one = i32j(1);
        for( size_t i = 0; i < n_args; i++ ) {
            ASR::ttype_t* type = ASRUtils::expr_type(a_args[i].m_value);
            if (ASRUtils::is_array(type)) {
                ASR::dimension_t* m_dims = nullptr;
                size_t n_dims = ASRUtils::extract_dimensions_from_ttype(type, m_dims);
                if( ASRUtils::is_dimension_empty(m_dims, n_dims) ) {
                    bool is_arr_sec = ASR::is_a<ASR::ArraySection_t>(*a_args[i].m_value);
                    std::vector<size_t> sliced_indices;
                    if (is_arr_sec) {
                        get_sliced_indices(ASR::down_cast<ASR::ArraySection_t>(a_args[i].m_value), sliced_indices);
                    }
                    Vec<ASR::dimension_t> m_dims_vec; m_dims_vec.reserve(al, n_dims);
                    for( size_t j = 0; j < n_dims; j++ ) {
                        ASR::dimension_t m_dim_vec;
                        m_dim_vec.loc = m_dims[j].loc;
                        m_dim_vec.m_start = i32one;
                        size_t dim = is_arr_sec ? sliced_indices[j] : (j + 1);
                        m_dim_vec.m_length = ASRUtils::EXPR(ASRUtils::make_ArraySize_t_util(al, m_dims[j].loc,
                            a_args[i].m_value, i32j(dim), ASRUtils::expr_type(i32one), nullptr));
                        m_dims_vec.push_back(al, m_dim_vec);
                    }
                    m_dims = m_dims_vec.p;
                    n_dims = m_dims_vec.size();
                }
                a_type = ASRUtils::make_Array_t_util(al, type->base.loc, type_, m_dims, n_dims,
                    ASR::abiType::Source, false, ASR::array_physical_typeType::DescriptorArray, true);
                break;
            }
        }
    }

    return ASR::make_FunctionCall_t(al, a_loc, a_name, a_original_name,
            a_args, n_args, a_type, a_value, a_dt);
}

static inline ASR::asr_t* make_SubroutineCall_t_util(
    Allocator &al, const Location &a_loc, ASR::symbol_t* a_name,
    ASR::symbol_t* a_original_name, ASR::call_arg_t* a_args, size_t n_args,
    ASR::expr_t* a_dt, ASR::stmt_t** cast_stmt, bool implicit_argument_casting, bool nopass) {

    Call_t_body(al, a_name, a_args, n_args, a_dt, cast_stmt, implicit_argument_casting, nopass);

    if( a_dt && ASR::is_a<ASR::Variable_t>(
        *ASRUtils::symbol_get_past_external(a_name)) &&
        ASR::is_a<ASR::FunctionType_t>(*ASRUtils::symbol_type(a_name)) ) {
        a_dt = ASRUtils::EXPR(ASR::make_StructInstanceMember_t(al, a_loc,
            a_dt, a_name, ASRUtils::duplicate_type(al, ASRUtils::symbol_type(a_name)), nullptr));
    }

    return ASR::make_SubroutineCall_t(al, a_loc, a_name, a_original_name, a_args, n_args, a_dt);
}

/*
    Checks if the function `f` is elemental and any of argument in
    `args` is an array type, if yes, it returns the first array
    argument, otherwise returns nullptr
*/
static inline ASR::expr_t* find_first_array_arg_if_elemental(
    const ASR::Function_t* f,
    const Vec<ASR::call_arg_t>& args
) {
    ASR::expr_t* first_array_argument = nullptr;
    bool is_elemental = ASRUtils::get_FunctionType(f)->m_elemental;
    if (!is_elemental || f->n_args == 0) {
        return first_array_argument;
    }
    for (size_t i=0; i < args.size(); i++) {
        if (args[i].m_value && is_array(ASRUtils::expr_type(args[i].m_value))) {
            // return the very first *array* argument
            first_array_argument = args[i].m_value;
            break;
        }
    }
    return first_array_argument;
}

static inline void promote_ints_to_kind_8(ASR::expr_t** m_args, size_t n_args,
    Allocator& al, const Location& loc) {
    for (size_t i = 0; i < n_args; i++) {
        if (ASRUtils::is_integer(*ASRUtils::expr_type(m_args[i]))) {
            ASR::ttype_t* arg_type = ASRUtils::expr_type(m_args[i]);
            ASR::ttype_t* dest_type = ASRUtils::duplicate_type(al, arg_type);
            ASRUtils::set_kind_to_ttype_t(dest_type, 8);
            m_args[i] = CastingUtil::perform_casting(m_args[i], dest_type, al, loc);
        }
    }
}

static inline ASR::asr_t* make_StringFormat_t_util(Allocator &al, const Location &a_loc,
        ASR::expr_t* a_fmt, ASR::expr_t** a_args, size_t n_args, ASR::string_format_kindType a_kind,
        ASR::ttype_t* a_type, ASR::expr_t* a_value) {
    if (a_fmt && ASR::is_a<ASR::Var_t>(*a_fmt)) {
        ASR::Variable_t* fmt_str = ASR::down_cast<ASR::Variable_t>(ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::Var_t>(a_fmt)->m_v));
        if (ASR::is_a<ASR::String_t>(
                *ASRUtils::extract_type(fmt_str->m_type))) {
            ASR::String_t* str_type = ASR::down_cast<ASR::String_t>(
                ASRUtils::extract_type(fmt_str->m_type));
            if (str_type->m_physical_type != ASR::string_physical_typeType::PointerString) {
                a_fmt = ASRUtils::EXPR(ASR::make_StringPhysicalCast_t(
                    al,
                    a_fmt->base.loc,
                    a_fmt,
                    str_type->m_physical_type,
                    ASR::string_physical_typeType::PointerString,
                    a_type,
                    nullptr));
            }
        }
    }
    return ASR::make_StringFormat_t(al, a_loc, a_fmt, a_args, n_args, a_kind, a_type, a_value);
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

static inline ASR::asr_t* make_IntrinsicElementalFunction_t_util(
    Allocator &al, const Location &a_loc, int64_t a_intrinsic_id,
    ASR::expr_t** a_args, size_t n_args, int64_t a_overload_id,
    ASR::ttype_t* a_type, ASR::expr_t* a_value) {

    for( size_t i = 0; i < n_args; i++ ) {
        if( a_args[i] == nullptr ) {
            continue;
        }
        ASR::expr_t* arg = a_args[i];
        ASR::ttype_t* arg_type = ASRUtils::type_get_past_allocatable(
            ASRUtils::type_get_past_pointer(ASRUtils::expr_type(arg)));

        if( ASRUtils::is_array(arg_type) ) {
            a_args[i] = cast_to_descriptor(al, arg);
            if( !ASRUtils::is_array(a_type) ) {
                ASR::ttype_t* underlying_type = ASRUtils::extract_type(arg_type);
                ASR::Array_t* e = ASR::down_cast<ASR::Array_t>(
                    ASRUtils::type_get_past_allocatable(
                        ASRUtils::type_get_past_pointer(arg_type)));
                a_type = TYPE(ASR::make_Array_t(al, a_type->base.loc, underlying_type,
                                    e->m_dims, e->n_dims, e->m_physical_type));
            }
        }
    }

    return ASR::make_IntrinsicElementalFunction_t(al, a_loc, a_intrinsic_id,
        a_args, n_args, a_overload_id, a_type, a_value);
}

static inline ASR::asr_t* make_IntrinsicArrayFunction_t_util(
    Allocator &al, const Location &a_loc, int64_t arr_intrinsic_id,
    ASR::expr_t** a_args, size_t n_args, int64_t a_overload_id,
    ASR::ttype_t* a_type, ASR::expr_t* a_value) {

    for( size_t i = 0; i < n_args; i++ ) {
        if( a_args[i] == nullptr ) {
            continue;
        }
        ASR::expr_t* arg = a_args[i];
        ASR::ttype_t* arg_type = ASRUtils::type_get_past_allocatable(
            ASRUtils::type_get_past_pointer(ASRUtils::expr_type(arg)));

        if( ASRUtils::is_array(arg_type) ) {
            a_args[i] = cast_to_descriptor(al, arg);
        }
    }

    return ASR::make_IntrinsicArrayFunction_t(al, a_loc, arr_intrinsic_id,
        a_args, n_args, a_overload_id, a_type, a_value);
}

static inline ASR::asr_t* make_Associate_t_util(
    Allocator &al, const Location &a_loc,
    ASR::expr_t* a_target, ASR::expr_t* a_value,
    SymbolTable* current_scope=nullptr) {
    ASR::ttype_t* target_type = ASRUtils::expr_type(a_target);
    ASR::ttype_t* value_type = ASRUtils::expr_type(a_value);
    if( ASRUtils::is_array(target_type) && ASRUtils::is_array(value_type) ) {
        ASR::array_physical_typeType target_ptype = ASRUtils::extract_physical_type(target_type);
        ASR::array_physical_typeType value_ptype = ASRUtils::extract_physical_type(value_type);
        if( target_ptype != value_ptype ) {
            ASR::dimension_t *target_m_dims = nullptr, *value_m_dims = nullptr;
            size_t target_n_dims = ASRUtils::extract_dimensions_from_ttype(target_type, target_m_dims);
            size_t value_n_dims = ASRUtils::extract_dimensions_from_ttype(value_type, value_m_dims);
            Vec<ASR::dimension_t> dim_vec;
            Vec<ASR::dimension_t>* dim_vec_ptr = nullptr;
            if( (!ASRUtils::is_dimension_empty(target_m_dims, target_n_dims) ||
                !ASRUtils::is_dimension_empty(value_m_dims, value_n_dims)) &&
                target_ptype == ASR::array_physical_typeType::FixedSizeArray ) {
                if( !ASRUtils::is_dimension_empty(target_m_dims, target_n_dims) ) {
                    dim_vec.from_pointer_n(target_m_dims, target_n_dims);
                } else {
                    dim_vec.from_pointer_n(value_m_dims, value_n_dims);
                }
                dim_vec_ptr = &dim_vec;
            }
            a_value = ASRUtils::EXPR(ASRUtils::make_ArrayPhysicalCast_t_util(al, a_loc, a_value,
                        value_ptype, target_ptype, ASRUtils::duplicate_type(al,
                        value_type, dim_vec_ptr, target_ptype, true), nullptr, current_scope));
        }
    }
    return ASR::make_Associate_t(al, a_loc, a_target, a_value);
}

static inline ASR::expr_t* extract_array_variable(ASR::expr_t* x) {
    LCOMPILERS_ASSERT(ASRUtils::is_array(ASRUtils::expr_type(x)));
    if( x->type == ASR::exprType::ArrayItem ) {
        return ASR::down_cast<ASR::ArrayItem_t>(x)->m_v;
    } else if( x->type == ASR::exprType::ArraySection ) {
        return ASR::down_cast<ASR::ArraySection_t>(x)->m_v;
    }

    return x;
}

static inline void extract_array_indices(ASR::expr_t* x, Allocator &al,
    Vec<ASR::array_index_t>& m_args, int& n_args) {
    if( x->type == ASR::exprType::ArrayItem ) {
        ASR::ArrayItem_t* arr = ASR::down_cast<ASR::ArrayItem_t>(x);
        for(size_t i = 0; i < arr->n_args; i++){
            if((arr->m_args[i].m_left && arr->m_args[i].m_right && arr->m_args[i].m_step) ||
                  (arr->m_args[i].m_right && ASRUtils::is_array(ASRUtils::expr_type(arr->m_args[i].m_right)))){
                m_args.push_back(al, arr->m_args[i]);
                n_args++;
            }
        }
        return ;
    } else if( x->type == ASR::exprType::ArraySection ) {
        ASR::ArraySection_t* arr = ASR::down_cast<ASR::ArraySection_t>(x);
        for(size_t i = 0; i < arr->n_args; i++){
            if((arr->m_args[i].m_left && arr->m_args[i].m_right && arr->m_args[i].m_step) ||
                  (arr->m_args[i].m_right && ASRUtils::is_array(ASRUtils::expr_type(arr->m_args[i].m_right)))){
                m_args.push_back(al, arr->m_args[i]);
                n_args++;
            }
        }
    }
}

static inline void extract_indices(ASR::expr_t* x,
    ASR::array_index_t*& m_args, size_t& n_args) {
    if( x->type == ASR::exprType::ArrayItem ) {
        m_args = ASR::down_cast<ASR::ArrayItem_t>(x)->m_args;
        n_args = ASR::down_cast<ASR::ArrayItem_t>(x)->n_args;
        return ;
    } else if( x->type == ASR::exprType::ArraySection ) {
        m_args = ASR::down_cast<ASR::ArraySection_t>(x)->m_args;
        n_args = ASR::down_cast<ASR::ArraySection_t>(x)->n_args;
        return ;
    }

    m_args = nullptr;
    n_args = 0;
}

static inline bool is_array_indexed_with_array_indices(ASR::array_index_t* m_args, size_t n_args) {
    for( size_t i = 0; i < n_args; i++ ) {
        if( m_args[i].m_left == nullptr &&
            m_args[i].m_right != nullptr &&
            m_args[i].m_step == nullptr &&
            ASRUtils::is_array(
                ASRUtils::expr_type(m_args[i].m_right)) ) {
            return true;
        }
    }

    return false;
}

template <typename T>
static inline bool is_array_indexed_with_array_indices(T* x) {
    return is_array_indexed_with_array_indices(x->m_args, x->n_args);
}

static inline ASR::ttype_t* create_array_type_with_empty_dims(Allocator& al,
    size_t value_n_dims, ASR::ttype_t* value_type) {
    Vec<ASR::dimension_t> empty_dims; empty_dims.reserve(al, value_n_dims);
    for( size_t i = 0; i < value_n_dims; i++ ) {
        ASR::dimension_t empty_dim;
        Location loc; loc.first = 1, loc.last = 1;
        empty_dim.loc = loc;
        empty_dim.m_length = nullptr;
        empty_dim.m_start = nullptr;
        empty_dims.push_back(al, empty_dim);
    }
    return ASRUtils::make_Array_t_util(al, value_type->base.loc,
        ASRUtils::extract_type(value_type), empty_dims.p, empty_dims.size());
}


static inline ASR::asr_t* make_ArrayItem_t_util(Allocator &al, const Location &a_loc,
    ASR::expr_t* a_v, ASR::array_index_t* a_args, size_t n_args, ASR::ttype_t* a_type,
    ASR::arraystorageType a_storage_format, ASR::expr_t* a_value) {
    if( ASR::is_a<ASR::ArrayPhysicalCast_t>(*a_v) ) {
        a_v = ASR::down_cast<ASR::ArrayPhysicalCast_t>(a_v)->m_arg;
    }

    if( !ASRUtils::is_array_indexed_with_array_indices(a_args, n_args) ) {
        a_type = ASRUtils::extract_type(a_type);
    }

    if( ASRUtils::is_allocatable(a_type) ) {
        ASR::dimension_t* m_dims = nullptr;
        size_t n_dims = ASRUtils::extract_dimensions_from_ttype(a_type, m_dims);
        if( !ASRUtils::is_dimension_empty(m_dims, n_dims) ) {
            a_type = ASRUtils::create_array_type_with_empty_dims(
                al, n_dims, ASRUtils::extract_type(a_type));
            a_type = ASRUtils::TYPE(ASR::make_Allocatable_t(al, a_loc, a_type));
        }
    }

    ASRUtils::ExprStmtDuplicator type_duplicator(al);
    a_type = type_duplicator.duplicate_ttype(a_type);
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

int64_t compute_trailing_zeros(int64_t number, int64_t kind);
int64_t compute_leading_zeros(int64_t number, int64_t kind);
void append_error(diag::Diagnostics& diag, const std::string& msg,
                const Location& loc);

static inline bool is_simd_array(ASR::ttype_t* t) {
    return (ASR::is_a<ASR::Array_t>(*t) &&
        ASR::down_cast<ASR::Array_t>(t)->m_physical_type
            == ASR::array_physical_typeType::SIMDArray);
}

static inline bool is_simd_array(ASR::expr_t *v) {
    return is_simd_array(expr_type(v));
}

static inline bool is_argument_of_type_CPtr(ASR::expr_t *var) {
    bool is_argument = false;
    if (ASR::is_a<ASR::CPtr_t>(*expr_type(var))) {
        if (ASR::is_a<ASR::Var_t>(*var)) {
            ASR::symbol_t *var_sym = ASR::down_cast<ASR::Var_t>(var)->m_v;
            if (ASR::is_a<ASR::Variable_t>(*var_sym)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(var_sym);
                if (v->m_intent == intent_local ||
                    v->m_intent == intent_return_var ||
                    !v->m_intent) {
                    is_argument = false;
                } else {
                    is_argument = true;
                }
            }
        }
    }
    return is_argument;
}

static inline void promote_arguments_kinds(Allocator &al, const Location &loc,
        Vec<ASR::expr_t*> &args, diag::Diagnostics &diag) {
    int target_kind = -1;
    for (size_t i = 0; i < args.size(); i++) {
        ASR::ttype_t *arg_type = ASRUtils::expr_type(args[i]);
        int kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
        if (is_array(arg_type)){
            target_kind = kind;
            break;
        }
        if (kind > target_kind) {
            target_kind = kind;
        }
    }

    for (size_t i = 0; i < args.size(); i++) {
        ASR::ttype_t *arg_type = ASRUtils::expr_type(args[i]);
        int kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
        if (kind==target_kind) {
            continue;
        }
        if (ASR::is_a<ASR::Real_t>(*arg_type)) {
            if (ASR::is_a<ASR::RealConstant_t>(*args[i])) {
                args.p[i] = EXPR(ASR::make_RealConstant_t(
                    al, loc, ASR::down_cast<ASR::RealConstant_t>(args[i])->m_r,
                    ASRUtils::TYPE(ASR::make_Real_t(al, loc, target_kind))));
            } else {
                args.p[i] = EXPR(ASR::make_Cast_t(
                    al, loc, args.p[i], ASR::cast_kindType::RealToReal,
                    ASRUtils::TYPE(ASR::make_Real_t(al, loc, target_kind)), nullptr));
            }
        } else if (ASR::is_a<ASR::Integer_t>(*arg_type)) {
            if (ASR::is_a<ASR::IntegerConstant_t>(*args[i])) {
                args.p[i] = EXPR(ASR::make_IntegerConstant_t(
                    al, loc, ASR::down_cast<ASR::IntegerConstant_t>(args[i])->m_n,
                    ASRUtils::TYPE(ASR::make_Integer_t(al, loc, target_kind))));
            } else {
                args.p[i] = EXPR(ASR::make_Cast_t(
                    al, loc, args[i], ASR::cast_kindType::IntegerToInteger,
                    ASRUtils::TYPE(ASR::make_Integer_t(al, loc, target_kind)), nullptr));
            }
        } else {
            diag.semantic_error_label("Unsupported argument type for kind adjustment", {loc},
                "help: ensure all arguments are of a convertible type");
        }
    }
}

class RemoveArrayProcessingNodeReplacer: public ASR::BaseExprReplacer<RemoveArrayProcessingNodeReplacer> {

    public:

    Allocator& al;

    RemoveArrayProcessingNodeReplacer(Allocator& al_): al(al_) {
    }

    void replace_ArrayBroadcast(ASR::ArrayBroadcast_t* x) {
        *current_expr = x->m_array;
    }

    void replace_ArrayPhysicalCast(ASR::ArrayPhysicalCast_t* x) {
        if( x->m_new == ASR::array_physical_typeType::SIMDArray ) {
            return ;
        }
        ASR::BaseExprReplacer<RemoveArrayProcessingNodeReplacer>::replace_ArrayPhysicalCast(x);
        if( !ASRUtils::is_array(ASRUtils::expr_type(x->m_arg)) ) {
            *current_expr = x->m_arg;
        }
    }

};

class RemoveArrayProcessingNodeVisitor: public ASR::CallReplacerOnExpressionsVisitor<RemoveArrayProcessingNodeVisitor> {

    private:

    RemoveArrayProcessingNodeReplacer replacer;

    public:

    void call_replacer() {
        replacer.current_expr = current_expr;
        replacer.replace_expr(*current_expr);
    }

    RemoveArrayProcessingNodeVisitor(Allocator& al_): replacer(al_) {}

    void visit_ArrayPhysicalCast(const ASR::ArrayPhysicalCast_t& x) {
        if( x.m_new == ASR::array_physical_typeType::SIMDArray ) {
            return ;
        }

        ASR::CallReplacerOnExpressionsVisitor<RemoveArrayProcessingNodeVisitor>::visit_ArrayPhysicalCast(x);
    }

};

} // namespace ASRUtils

} // namespace LCompilers

#endif // LFORTRAN_ASR_UTILS_H
