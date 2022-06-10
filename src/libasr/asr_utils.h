#ifndef LFORTRAN_ASR_UTILS_H
#define LFORTRAN_ASR_UTILS_H

#include <functional>
#include <map>
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
    return ASR::expr_type0(f);
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
        case ASR::ttypeType::Derived: {
            return "derived type";
        }
        case ASR::ttypeType::CPtr: {
            return "type(c_ptr)";
        }
        case ASR::ttypeType::Pointer: {
            return type_to_str(ASRUtils::type_get_past_pointer(
                        const_cast<ASR::ttype_t*>(t))) + " pointer";
        }
        default : throw LFortranException("Not implemented " + std::to_string(t->type) + ".");
    }
}

static inline std::string type_to_str_python(const ASR::ttype_t *t)
{
    switch (t->type) {
        case ASR::ttypeType::Integer: {
            ASR::Integer_t *i = (ASR::Integer_t*)t;
            switch (i->m_kind) {
                case 1: { return "i8"; }
                case 2: { return "i16"; }
                case 4: { return "i32"; }
                case 8: { return "i64"; }
                default: { throw LFortranException("Integer kind not supported"); }
            }
        }
        case ASR::ttypeType::Real: {
            ASR::Real_t *r = (ASR::Real_t*)t;
            switch (r->m_kind) {
                case 4: { return "f32"; }
                case 8: { return "f64"; }
                default: { throw LFortranException("Float kind not supported"); }
            }
        }
        case ASR::ttypeType::Complex: {
            ASR::Complex_t *c = (ASR::Complex_t*)t;
            switch (c->m_kind) {
                case 4: { return "c32"; }
                case 8: { return "c64"; }
                default: { throw LFortranException("Complex kind not supported"); }
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
        case ASR::symbolType::AssociateBlock: {
            return ASR::down_cast<ASR::AssociateBlock_t>(f)->m_name;
        }
        case ASR::symbolType::Block: {
            return ASR::down_cast<ASR::Block_t>(f)->m_name;
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
        case ASR::symbolType::AssociateBlock: {
            return ASR::down_cast<ASR::AssociateBlock_t>(f)->m_symtab->parent;
        }
        case ASR::symbolType::Block: {
            return ASR::down_cast<ASR::Block_t>(f)->m_symtab->parent;
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
        case ASR::symbolType::AssociateBlock: {
            return ASR::down_cast<ASR::AssociateBlock_t>(f)->m_symtab;
        }
        case ASR::symbolType::Block: {
            return ASR::down_cast<ASR::Block_t>(f)->m_symtab;
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
        case ASR::ttypeType::Derived: {
            ASR::Derived_t* Derived_type = ASR::down_cast<ASR::Derived_t>(x);
            n_dims = Derived_type->n_dims;
            m_dims = Derived_type->m_dims;
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
        case ASR::ttypeType::List: {
            n_dims = 0;
            m_dims = nullptr;
            break;
        }
        case ASR::ttypeType::CPtr: {
            n_dims = 0;
            m_dims = nullptr;
            break;
        }
        default:
            throw LFortranException("Not implemented.");
    }
    return n_dims;
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
        default : throw LFortranException("Not implemented");
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
                if (ASR::is_a<ASR::List_t>(*x) && ASR::is_a<ASR::List_t>(*y)) {
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
                }
                if( x->type == y->type ) {
                    return true;
                }

                return ASRUtils::is_same_type_pointer(x, y);
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

ASR::asr_t* make_Cast_t_value(Allocator &al, const Location &a_loc,
        ASR::expr_t* a_arg, ASR::cast_kindType a_kind, ASR::ttype_t* a_type);


} // namespace ASRUtils

} // namespace LFortran

#endif // LFORTRAN_ASR_UTILS_H
