#ifndef LIBASR_BUILDER_H
#define LIBASR_BUILDER_H

#include <libasr/asr.h>
#include <libasr/asr_utils.h>
#include <libasr/containers.h>
#include <libasr/pass/pass_utils.h>

namespace LCompilers::ASRUtils {

class ASRBuilder {
    private:

    Allocator& al;
    // TODO: use the location to point C++ code in `intrinsic_function_registry`
    const Location &loc;

    public:

    ASRBuilder(Allocator& al_, const Location& loc_): al(al_), loc(loc_) {}

    #define make_ConstantWithKind(Constructor, TypeConstructor, value, kind, loc) ASRUtils::EXPR( \
        ASR::Constructor( al, loc, value, \
            ASRUtils::TYPE(ASR::TypeConstructor(al, loc, kind)))) \

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
            ASRUtils::make_Variable_t_util(al, loc, symtab, s2c(al, var_name), nullptr, 0,
            intent, nullptr, nullptr, ASR::storage_typeType::Default, type, nullptr, abi,
            ASR::Public, ASR::presenceType::Required, a_value_attr));
        symtab->add_symbol(s2c(al, var_name), sym);
        return ASRUtils::EXPR(ASR::make_Var_t(al, loc, sym));
    }

    void VariableDeclaration(SymbolTable *symtab, std::string var_name,
            ASR::ttype_t *type, ASR::intentType intent,
            ASR::abiType abi=ASR::abiType::Source, bool a_value_attr=false) {
        ASR::symbol_t* sym = ASR::down_cast<ASR::symbol_t>(
            ASRUtils::make_Variable_t_util(al, loc, symtab, s2c(al, var_name), nullptr, 0,
            intent, nullptr, nullptr, ASR::storage_typeType::Default, type, nullptr, abi,
            ASR::Public, ASR::presenceType::Required, a_value_attr));
        symtab->add_symbol(s2c(al, var_name), sym);
        return;
    }

    ASR::expr_t *VariableOverwrite(SymbolTable *symtab, std::string var_name,
            ASR::ttype_t *type, ASR::intentType intent,
            ASR::abiType abi=ASR::abiType::Source, bool a_value_attr=false) {
        ASR::symbol_t* sym = ASR::down_cast<ASR::symbol_t>(
            ASRUtils::make_Variable_t_util(al, loc, symtab, s2c(al, var_name), nullptr, 0,
            intent, nullptr, nullptr, ASR::storage_typeType::Default, type, nullptr, abi,
            ASR::Public, ASR::presenceType::Required, a_value_attr));
        symtab->add_or_overwrite_symbol(s2c(al, var_name), sym);
        return ASRUtils::EXPR(ASR::make_Var_t(al, loc, sym));
    }

    #define declare(var_name, type, intent)                                     \
        b.Variable(fn_symtab, var_name, type, ASR::intentType::intent)

    #define fill_func_arg(arg_name, type) {                                     \
        auto arg = declare(arg_name, type, In);                                 \
        args.push_back(al, arg); }

    #define fill_func_arg_sub(arg_name, type, intent) {                                     \
        auto arg = declare(arg_name, type, intent);                                 \
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
    #define int8         ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 1))
    #define int16        ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 2))
    #define int32        ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4))
    #define int64        ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 8))
    #define real32       ASRUtils::TYPE(ASR::make_Real_t(al, loc, 4))
    #define real64       ASRUtils::TYPE(ASR::make_Real_t(al, loc, 8))
    #define complex32    ASRUtils::TYPE(ASR::make_Complex_t(al, loc, 4))
    #define complex64    ASRUtils::TYPE(ASR::make_Complex_t(al, loc, 8))
    #define logical      ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4))
    #define character(x) ASRUtils::TYPE(ASR::make_String_t(al, loc, 1, x, nullptr, ASR::string_physical_typeType::PointerString))
    #define List(x)      ASRUtils::TYPE(ASR::make_List_t(al, loc, x))

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

    ASR::ttype_t* CPtr() {
        return TYPE(ASR::make_CPtr_t(al, loc));
    }

    // Expressions -------------------------------------------------------------
    ASR::expr_t* Var(ASR::symbol_t* sym) {
        return ASRUtils::EXPR(ASR::make_Var_t(al, loc, sym));
    }

    ASR::expr_t* ArrayUBound(ASR::expr_t* x, int64_t dim) {
        ASR::expr_t* value = nullptr;
        ASR::ttype_t* type = ASRUtils::expr_type(x);
        if ( ASRUtils::is_array(type) ) {
            ASR::Array_t* array_type = ASR::down_cast<ASR::Array_t>(ASRUtils::type_get_past_pointer(ASRUtils::type_get_past_allocatable(type)));
            ASR::dimension_t* dims = array_type->m_dims;
            ASRUtils::extract_dimensions_from_ttype(type, dims);
            int new_dim = dim - 1;
            if( dims[new_dim].m_start && dims[new_dim].m_length ) {
                ASR::expr_t* start = ASRUtils::expr_value(dims[new_dim].m_start);
                ASR::expr_t* length = ASRUtils::expr_value(dims[new_dim].m_length);
                if( ASRUtils::is_value_constant(start) &&
                ASRUtils::is_value_constant(length) ) {
                    int64_t const_lbound = -1;
                    if( !ASRUtils::extract_value(start, const_lbound) ) {
                        LCOMPILERS_ASSERT(false);
                    }
                    int64_t const_length = -1;
                    if( !ASRUtils::extract_value(length, const_length) ) {
                        LCOMPILERS_ASSERT(false);
                    }
                    value = i32(const_lbound + const_length - 1);
                }
            }
        }
        return ASRUtils::EXPR(ASR::make_ArrayBound_t(al, loc, x, i32(dim), int32, ASR::arrayboundType::UBound, value));
    }

    ASR::expr_t* ArrayLBound(ASR::expr_t* x, int64_t dim) {
        ASR::expr_t* value = nullptr;
        ASR::ttype_t* type = ASRUtils::expr_type(x);
        if ( ASRUtils::is_array(type) ) {
            ASR::Array_t* array_type = ASR::down_cast<ASR::Array_t>(ASRUtils::type_get_past_pointer(ASRUtils::type_get_past_allocatable(type)));
            ASR::dimension_t* dims = array_type->m_dims;
            ASRUtils::extract_dimensions_from_ttype(type, dims);
            int new_dim = dim - 1;
            if( dims[new_dim].m_start ) {
                ASR::expr_t* start = ASRUtils::expr_value(dims[new_dim].m_start);
                if( ASRUtils::is_value_constant(start) ) {
                    int64_t const_lbound = -1;
                    if( !ASRUtils::extract_value(start, const_lbound) ) {
                        LCOMPILERS_ASSERT(false);
                    }
                    value = i32(const_lbound);
                }
            }
        }
        return ASRUtils::EXPR(ASR::make_ArrayBound_t(al, loc, x, i32(dim), int32, ASR::arrayboundType::LBound, value));
    }

    inline ASR::expr_t* i_t(int64_t x, ASR::ttype_t* t) {
        return EXPR(ASR::make_IntegerConstant_t(al, loc, x, t));
    }

    inline ASR::expr_t* logical_true() {
        return EXPR(ASR::make_LogicalConstant_t(al, loc, true, logical));
    }

    inline ASR::expr_t* logical_false() {
        return EXPR(ASR::make_LogicalConstant_t(al, loc, false, logical));
    }

    inline ASR::expr_t* i32(int64_t x) {
        return EXPR(ASR::make_IntegerConstant_t(al, loc, x, int32));
    }

    inline ASR::expr_t* i64(int64_t x) {
        return EXPR(ASR::make_IntegerConstant_t(al, loc, x, int64));
    }

    inline ASR::expr_t* i_neg(ASR::expr_t* x, ASR::ttype_t* t) {
        return EXPR(ASR::make_IntegerUnaryMinus_t(al, loc, x, t, nullptr));
    }

    inline ASR::expr_t* f_t(double x, ASR::ttype_t* t) {
        return EXPR(ASR::make_RealConstant_t(al, loc, x, t));
    }

    inline ASR::expr_t* f32(double x) {
        return EXPR(ASR::make_RealConstant_t(al, loc, x, real32));
    }

    inline ASR::expr_t* f64(double x) {
        return EXPR(ASR::make_RealConstant_t(al, loc, x, real64));
    }

    inline ASR::expr_t* f_neg(ASR::expr_t* x, ASR::ttype_t* t) {
        return EXPR(ASR::make_RealUnaryMinus_t(al, loc, x, t, nullptr));
    }

    inline ASR::expr_t* bool_t(bool x, ASR::ttype_t* t) {
        return EXPR(ASR::make_LogicalConstant_t(al, loc, x, t));
    }

    inline ASR::expr_t* complex_t(double x, double y, ASR::ttype_t* t) {
        return EXPR(ASR::make_ComplexConstant_t(al, loc, x, y, t));
    }

    inline ASR::expr_t* c32(double x, double y) {
        return EXPR(ASR::make_ComplexConstant_t(al, loc, x, y, complex32));
    }

    inline ASR::expr_t* c64(double x, double y) {
        return EXPR(ASR::make_ComplexConstant_t(al, loc, x, y, complex64));
    }

    inline ASR::expr_t* constant_t(double x, ASR::ttype_t* t, double y = 0.0) {
        if (ASRUtils::is_integer(*t)) {
            return i_t(x, t);
        } else if (ASRUtils::is_real(*t)) {
            return f_t(x, t);
        } else if (ASRUtils::is_complex(*t)) {
            return complex_t(x, y, t);
        } else if (ASRUtils::is_logical(*t)) {
            if (x == 0.0) {
                return bool_t(false, t);
            } else {
                return bool_t(true, t);
            }
        } else {
            throw LCompilersException("Type not supported");
        }
    }

    inline ASR::expr_t* ListItem(ASR::expr_t* x, ASR::expr_t* pos, ASR::ttype_t* type) {
        return EXPR(ASR::make_ListItem_t(al, loc, x, pos, type, nullptr));
    }

    inline ASR::stmt_t* ListAppend(ASR::expr_t* x, ASR::expr_t* val) {
        return STMT(ASR::make_ListAppend_t(al, loc, x, val));
    }

    inline ASR::expr_t* StringSection(ASR::expr_t* s, ASR::expr_t* start, ASR::expr_t* end) {
        return EXPR(ASR::make_StringSection_t(al, loc, s, start, end, i32(1), character(-2), nullptr));
    }

    inline ASR::expr_t* StringItem(ASR::expr_t* x, ASR::expr_t* idx) {
        return EXPR(ASR::make_StringItem_t(al, loc, x, idx, character(-2), nullptr));
    }

    inline ASR::expr_t* StringConstant(std::string s, ASR::ttype_t* type) {
        return EXPR(ASR::make_StringConstant_t(al, loc, s2c(al, s), type));
    }

    inline ASR::expr_t* StringLen(ASR::expr_t* s) {
        return EXPR(ASR::make_StringLen_t(al, loc, s, int32, nullptr));
    }

    inline ASR::expr_t* StringConcat(ASR::expr_t* s1, ASR::expr_t* s2, ASR::ttype_t* type) {
        return EXPR(ASR::make_StringConcat_t(al, loc, s1, s2, type, nullptr));
    }

    inline ASR::expr_t* ArraySize(ASR::expr_t* x, ASR::expr_t* dim, ASR::ttype_t* t) {
        return EXPR(make_ArraySize_t_util(al, loc, x, dim, t, nullptr));
    }

    inline ASR::expr_t* Ichar(std::string s, ASR::ttype_t* type, ASR::ttype_t* t) {
        return EXPR(ASR::make_Ichar_t(al, loc,
            EXPR(ASR::make_StringConstant_t(al, loc, s2c(al, s), type)), t, nullptr));
    }

    inline ASR::expr_t* PointerToCPtr(ASR::expr_t* x, ASR::ttype_t* t) {
        return EXPR(ASR::make_PointerToCPtr_t(al, loc, x, t, nullptr));
    }

    // Cast --------------------------------------------------------------------

    #define avoid_cast(x, t) if( ASRUtils::extract_kind_from_ttype_t(t) <= \
        ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(x)) ) { \
        return x; \
    } \

    inline ASR::expr_t* r2i_t(ASR::expr_t* x, ASR::ttype_t* t) {
        ASR::expr_t* value = ASRUtils::expr_value(x);
        if ( value != nullptr ) {
            double val = ASR::down_cast<ASR::RealConstant_t>(value)->m_r;
            value = i_t(val, t);
        }
        return EXPR(ASR::make_Cast_t(al, loc, x, ASR::cast_kindType::RealToInteger, t, value));
    }

    inline ASR::expr_t* c2i_t(ASR::expr_t* x, ASR::ttype_t* t) {
        // TODO: handle value
        return EXPR(ASR::make_Cast_t(al, loc, x, ASR::cast_kindType::ComplexToInteger, t, nullptr));
    }

    inline ASR::expr_t* i2r_t(ASR::expr_t* x, ASR::ttype_t* t) {
        ASR::expr_t* value = ASRUtils::expr_value(x);
        if ( value != nullptr ) {
            int64_t val = ASR::down_cast<ASR::IntegerConstant_t>(value)->m_n;
            value = f_t(val, t);
        }
        return EXPR(ASR::make_Cast_t(al, loc, x, ASR::cast_kindType::IntegerToReal, t, value));
    }

    inline ASR::expr_t* i2i_t(ASR::expr_t* x, ASR::ttype_t* t) {
        // avoid_cast(x, t); // TODO: adding this makes intrinsics_61 fail, that shall not happen, add a flag for force casting
        ASR::expr_t* value = ASRUtils::expr_value(x);
        if ( value != nullptr ) {
            int64_t val = ASR::down_cast<ASR::IntegerConstant_t>(value)->m_n;
            value = i_t(val, t);
        }
        return EXPR(ASR::make_Cast_t(al, loc, x, ASR::cast_kindType::IntegerToInteger, t, value));
    }

    inline ASR::expr_t* r2r_t(ASR::expr_t* x, ASR::ttype_t* t) {
        int kind_x = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(x));
        int kind_t = ASRUtils::extract_kind_from_ttype_t(t);
        if (kind_x == kind_t) {
            return x;
        }
        ASR::expr_t* value = ASRUtils::expr_value(x);
        if ( value != nullptr ) {
            double val = ASR::down_cast<ASR::RealConstant_t>(value)->m_r;
            value = f_t(val, t);
        }
        return EXPR(ASR::make_Cast_t(al, loc, x, ASR::cast_kindType::RealToReal, t, value));
    }

    inline ASR::expr_t* c2r_t(ASR::expr_t* x, ASR::ttype_t* t) {
        // TODO: handle value
        return EXPR(ASR::make_Cast_t(al, loc, x, ASR::cast_kindType::ComplexToReal, t, nullptr));
    }

    inline ASR::expr_t* t2t(ASR::expr_t* x, ASR::ttype_t* t1, ASR::ttype_t* t2) {
        // TODO: improve this function to handle all types
        if (ASRUtils::is_real(*t1)) {
            if (ASRUtils::is_real(*t2)) {
                return r2r_t(x, t2);
            } else if (ASRUtils::is_integer(*t2)) {
                return r2i_t(x, t2);
            } else {
                throw LCompilersException("Type not supported");
            }
        } else if (ASRUtils::is_integer(*t1)) {
            if (ASRUtils::is_real(*t2)) {
                return i2r_t(x, t2);
            } else if (ASRUtils::is_integer(*t2)) {
                return i2i_t(x, t2);
            } else {
                throw LCompilersException("Type not supported");
            }
        } else if (ASRUtils::is_complex(*t1)) {
            if (ASRUtils::is_real(*t2)) {
                return c2r_t(x, t2);
            } else if (ASRUtils::is_integer(*t2)) {
                return c2i_t(x, t2);
            } else {
                throw LCompilersException("Type not supported");
            }
        } else {
            throw LCompilersException("Type not supported");
        }
    }

    // Binop -------------------------------------------------------------------

    inline ASR::expr_t* BitRshift(ASR::expr_t* n, ASR::expr_t* bits, ASR::ttype_t* t) {
        return EXPR(ASR::make_IntegerBinOp_t(al, loc, n, ASR::binopType::BitRShift, bits, t, nullptr));
    }

    inline ASR::expr_t* BitLshift(ASR::expr_t* n, ASR::expr_t* bits, ASR::ttype_t* t) {
        return EXPR(ASR::make_IntegerBinOp_t(al, loc, n, ASR::binopType::BitLShift, bits, t, nullptr));
    }

    ASR::expr_t *And(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        ASRUtils::make_ArrayBroadcast_t_util(al, loc, left, right);
        switch (type->type) {
            case ASR::ttypeType::Integer: {
                return EXPR(ASR::make_IntegerBinOp_t(al, loc, left, ASR::binopType::BitAnd, right, type, nullptr));
            }
            case ASR::ttypeType::Logical: {
                return EXPR(ASR::make_LogicalBinOp_t(al, loc, left, ASR::logicalbinopType::And, right, logical, nullptr));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(left)) + " not yet supported");
                return nullptr;
            }
        }
    }

    ASR::expr_t *Or(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        ASRUtils::make_ArrayBroadcast_t_util(al, loc, left, right);
        switch (type->type) {
            case ASR::ttypeType::Integer: {
                return EXPR(ASR::make_IntegerBinOp_t(al, loc, left, ASR::binopType::BitOr, right, type, nullptr));
            }
            case ASR::ttypeType::Logical: {
                return EXPR(ASR::make_LogicalBinOp_t(al, loc, left, ASR::logicalbinopType::Or, right, logical, nullptr));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(left)) + " not yet supported");
                return nullptr;
            }
        }
    }

    ASR::expr_t *Xor(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        ASRUtils::make_ArrayBroadcast_t_util(al, loc, left, right);
        switch (type->type) {
            case ASR::ttypeType::Integer: {
                return EXPR(ASR::make_IntegerBinOp_t(al, loc, left, ASR::binopType::BitXor, right, type, nullptr));
            }
            case ASR::ttypeType::Logical: {
                return EXPR(ASR::make_LogicalBinOp_t(al, loc, left, ASR::logicalbinopType::Xor, right, logical, nullptr));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(left)) + " not yet supported");
                return nullptr;
            }
        }
    }

    ASR::expr_t *Not(ASR::expr_t *x) {
        ASR::ttype_t *type = expr_type(x);
        switch (type->type) {
            case ASR::ttypeType::Integer: {
                return EXPR(ASR::make_IntegerBitNot_t(al, loc, x, type, nullptr));
            }
            case ASR::ttypeType::Logical: {
                return EXPR(ASR::make_LogicalNot_t(al, loc, x, logical, nullptr));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(x)) + " not yet supported");
                return nullptr;
            }
        }
    }

    ASR::expr_t *Add(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        ASRUtils::make_ArrayBroadcast_t_util(al, loc, left, right);
        switch (type->type) {
            case ASR::ttypeType::Integer : {
                return EXPR(ASR::make_IntegerBinOp_t(al, loc, left,
                    ASR::binopType::Add, right, type, nullptr));
            }
            case ASR::ttypeType::Real : {
                return EXPR(ASR::make_RealBinOp_t(al, loc, left,
                    ASR::binopType::Add, right, type, nullptr));
            }
            case ASR::ttypeType::String : {
                return EXPR(ASR::make_StringConcat_t(al, loc, left,
                    right, type, nullptr));
            }
            case ASR::ttypeType::Complex : {
                return EXPR(ASR::make_ComplexBinOp_t(al, loc, left,
                    ASR::binopType::Add, right, type, nullptr));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(left)) + " not yet supported");
                return nullptr;
            }
        }
    }

    ASR::expr_t *Sub(ASR::expr_t *left, ASR::expr_t *right, ASR::expr_t* value = nullptr) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        ASRUtils::make_ArrayBroadcast_t_util(al, loc, left, right);
        switch (type->type) {
            case ASR::ttypeType::Integer: {
                return EXPR(ASR::make_IntegerBinOp_t(al, loc, left,
                    ASR::binopType::Sub, right, type, value));
            }
            case ASR::ttypeType::Real: {
                return EXPR(ASR::make_RealBinOp_t(al, loc, left,
                    ASR::binopType::Sub, right, type, value));
            }
            case ASR::ttypeType::Complex: {
                return EXPR(ASR::make_ComplexBinOp_t(al, loc, left,
                    ASR::binopType::Sub, right, type, value));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(left)) + " not yet supported");
                return nullptr;
            }
        }
    }

    ASR::expr_t *Mul(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        ASRUtils::make_ArrayBroadcast_t_util(al, loc, left, right);
        switch (type->type) {
            case ASR::ttypeType::Integer: {
                int64_t left_value = 0, right_value = 0;
                ASR::expr_t* value = nullptr;
                if( ASRUtils::extract_value(left, left_value) &&
                    ASRUtils::extract_value(right, right_value) ) {
                    int64_t mul_value = left_value * right_value;
                    value = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, mul_value, type));
                }
                return EXPR(ASR::make_IntegerBinOp_t(al, loc, left,
                    ASR::binopType::Mul, right, type, value));
            }
            case ASR::ttypeType::Real: {
                double left_value = 0, right_value = 0;
                ASR::expr_t* value = nullptr;
                if( ASRUtils::extract_value(left, left_value) &&
                    ASRUtils::extract_value(right, right_value) ) {
                    double mul_value = left_value * right_value;
                    value = ASRUtils::EXPR(ASR::make_RealConstant_t(al, loc, mul_value, type));
                }
                return EXPR(ASR::make_RealBinOp_t(al, loc, left,
                    ASR::binopType::Mul, right, type, value));
            }
            case ASR::ttypeType::Complex: {
                return EXPR(ASR::make_ComplexBinOp_t(al, loc, left,
                    ASR::binopType::Mul, right, type, nullptr));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(left)) + " not yet supported");
                return nullptr;
            }
        }
    }

    ASR::expr_t *Div(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        ASRUtils::make_ArrayBroadcast_t_util(al, loc, left, right);
        switch (type->type) {
            case ASR::ttypeType::Integer: {
                return EXPR(ASR::make_IntegerBinOp_t(al, loc, left,
                    ASR::binopType::Div, right, type, nullptr));
            }
            case ASR::ttypeType::Real: {
                return EXPR(ASR::make_RealBinOp_t(al, loc, left,
                    ASR::binopType::Div, right, type, nullptr));
            }
            case ASR::ttypeType::Complex: {
                return EXPR(ASR::make_ComplexBinOp_t(al, loc, left,
                    ASR::binopType::Div, right, type, nullptr));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(left)) + " not yet supported");
                return nullptr;
            }
        }
    }

    ASR::expr_t *Pow(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        ASRUtils::make_ArrayBroadcast_t_util(al, loc, left, right);
        switch (type->type) {
            case ASR::ttypeType::Integer: {
                return EXPR(ASR::make_IntegerBinOp_t(al, loc, left,
                    ASR::binopType::Pow, right, type, nullptr));
            }
            case ASR::ttypeType::Real: {
                return EXPR(ASR::make_RealBinOp_t(al, loc, left,
                    ASR::binopType::Pow, right, type, nullptr));
            }
            case ASR::ttypeType::Complex: {
                return EXPR(ASR::make_ComplexBinOp_t(al, loc, left,
                    ASR::binopType::Pow, right, type, nullptr));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(left)) + " not yet supported");
                return nullptr;
            }
        }
    }

    ASR::expr_t* Max(ASR::expr_t* left, ASR::expr_t* right) {
        return ASRUtils::EXPR(ASR::make_IfExp_t(al, loc, Gt(left, right), left, right, ASRUtils::expr_type(left), nullptr));
    }

    ASR::expr_t* Min(ASR::expr_t* left, ASR::expr_t* right) {
        return ASRUtils::EXPR(ASR::make_IfExp_t(al, loc, Lt(left, right), left, right, ASRUtils::expr_type(left), nullptr));
    }

    ASR::stmt_t* CallIntrinsicSubroutine(SymbolTable* scope, std::vector<ASR::ttype_t*> types,
                                std::vector<ASR::expr_t*> args, int64_t overload_id,
                                ASR::stmt_t* (*intrinsic_subroutine)(Allocator &, const Location &, SymbolTable *,
                                Vec<ASR::ttype_t*>&, Vec<ASR::call_arg_t>&, int64_t)) {
        Vec<ASR::ttype_t*> arg_types; arg_types.reserve(al, types.size());
        for (auto &x: types) arg_types.push_back(al, x);

        Vec<ASR::call_arg_t> new_args; new_args.reserve(al, args.size());
        for (auto &x: args) {
            ASR::call_arg_t call_arg; call_arg.loc = loc; call_arg.m_value = x;
            new_args.push_back(al, call_arg);
        }

        return intrinsic_subroutine(al, loc, scope, arg_types, new_args, overload_id);
    }

    ASR::expr_t* CallIntrinsic(SymbolTable* scope, std::vector<ASR::ttype_t*> types,
                                std::vector<ASR::expr_t*> args,  ASR::ttype_t* return_type, int64_t overload_id,
                                ASR::expr_t* (*intrinsic_func)(Allocator &, const Location &, SymbolTable *,
                                Vec<ASR::ttype_t*>&, ASR::ttype_t *, Vec<ASR::call_arg_t>&, int64_t)) {
        Vec<ASR::ttype_t*> arg_types; arg_types.reserve(al, types.size());
        for (auto &x: types) arg_types.push_back(al, x);

        Vec<ASR::call_arg_t> new_args; new_args.reserve(al, args.size());
        for (auto &x: args) {
            ASR::call_arg_t call_arg; call_arg.loc = loc; call_arg.m_value = x;
            new_args.push_back(al, call_arg);
        }

        return intrinsic_func(al, loc, scope, arg_types, return_type, new_args, overload_id);
    }

    // Compare -----------------------------------------------------------------
    ASR::expr_t *Gt(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        switch(type->type){
            case ASR::ttypeType::Integer: {
                return EXPR(ASR::make_IntegerCompare_t(al, loc, left, ASR::cmpopType::Gt, right, logical, nullptr));
            }
            case ASR::ttypeType::Real: {
                return EXPR(ASR::make_RealCompare_t(al, loc, left, ASR::cmpopType::Gt, right, logical, nullptr));
            }
            case ASR::ttypeType::String: {
                return EXPR(ASR::make_StringCompare_t(al, loc, left, ASR::cmpopType::Gt, right, logical, nullptr));
            }
            case ASR::ttypeType::Logical: {
                return EXPR(ASR::make_LogicalCompare_t(al, loc, left, ASR::cmpopType::Gt, right, logical, nullptr));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(left)) + " not yet supported");
                return nullptr;
            }
        }
    }

    ASR::expr_t *Lt(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        switch(type->type){
            case ASR::ttypeType::Integer: {
                return EXPR(ASR::make_IntegerCompare_t(al, loc, left, ASR::cmpopType::Lt, right, logical, nullptr));
            }
            case ASR::ttypeType::Real: {
                return EXPR(ASR::make_RealCompare_t(al, loc, left, ASR::cmpopType::Lt, right, logical, nullptr));
            }
            case ASR::ttypeType::String: {
                return EXPR(ASR::make_StringCompare_t(al, loc, left, ASR::cmpopType::Lt, right, logical, nullptr));
            }
            case ASR::ttypeType::Logical: {
                return EXPR(ASR::make_LogicalCompare_t(al, loc, left, ASR::cmpopType::Lt, right, logical, nullptr));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(left)) + " not yet supported");
                return nullptr;
            }
        }
    }

    ASR::expr_t *GtE(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        switch(type->type){
            case ASR::ttypeType::Integer: {
                return EXPR(ASR::make_IntegerCompare_t(al, loc, left, ASR::cmpopType::GtE, right, logical, nullptr));
            }
            case ASR::ttypeType::Real: {
                return EXPR(ASR::make_RealCompare_t(al, loc, left, ASR::cmpopType::GtE, right, logical, nullptr));
            }
            case ASR::ttypeType::String: {
                return EXPR(ASR::make_StringCompare_t(al, loc, left, ASR::cmpopType::GtE, right, logical, nullptr));
            }
            case ASR::ttypeType::Logical: {
                return EXPR(ASR::make_LogicalCompare_t(al, loc, left, ASR::cmpopType::GtE, right, logical, nullptr));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(left)) + " not yet supported");
                return nullptr;
            }
        }
    }

    ASR::expr_t *LtE(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        switch(type->type){
            case ASR::ttypeType::Integer: {
                return EXPR(ASR::make_IntegerCompare_t(al, loc, left, ASR::cmpopType::LtE, right, logical, nullptr));
            }
            case ASR::ttypeType::Real: {
                return EXPR(ASR::make_RealCompare_t(al, loc, left, ASR::cmpopType::LtE, right, logical, nullptr));
            }
            case ASR::ttypeType::String: {
                return EXPR(ASR::make_StringCompare_t(al, loc, left, ASR::cmpopType::LtE, right, logical, nullptr));
            }
            case ASR::ttypeType::Logical: {
                return EXPR(ASR::make_LogicalCompare_t(al, loc, left, ASR::cmpopType::LtE, right, logical, nullptr));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(left)) + " not yet supported");
                return nullptr;
            }
        }
    }

    ASR::expr_t *Eq(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        switch(type->type){
            case ASR::ttypeType::Integer: {
                return EXPR(ASR::make_IntegerCompare_t(al, loc, left, ASR::cmpopType::Eq, right, logical, nullptr));
            }
            case ASR::ttypeType::Real: {
                return EXPR(ASR::make_RealCompare_t(al, loc, left, ASR::cmpopType::Eq, right, logical, nullptr));
            }
            case ASR::ttypeType::String: {
                return EXPR(ASR::make_StringCompare_t(al, loc, left, ASR::cmpopType::Eq, right, logical, nullptr));
            }
            case ASR::ttypeType::Logical: {
                return EXPR(ASR::make_LogicalCompare_t(al, loc, left, ASR::cmpopType::Eq, right, logical, nullptr));
            }
            case ASR::ttypeType::Complex: {
                return EXPR(ASR::make_ComplexCompare_t(al, loc, left, ASR::cmpopType::Eq, right, logical, nullptr));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(left)) + " not yet supported");
                return nullptr;
            }
        }
    }

    ASR::expr_t *NotEq(ASR::expr_t *left, ASR::expr_t *right) {
        LCOMPILERS_ASSERT(check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = expr_type(left);
        switch(type->type){
            case ASR::ttypeType::Integer: {
                return EXPR(ASR::make_IntegerCompare_t(al, loc, left, ASR::cmpopType::NotEq, right, logical, nullptr));
            }
            case ASR::ttypeType::Real: {
                return EXPR(ASR::make_RealCompare_t(al, loc, left, ASR::cmpopType::NotEq, right, logical, nullptr));
            }
            case ASR::ttypeType::String: {
                return EXPR(ASR::make_StringCompare_t(al, loc, left, ASR::cmpopType::NotEq, right, logical, nullptr));
            }
            case ASR::ttypeType::Logical: {
                return EXPR(ASR::make_LogicalCompare_t(al, loc, left, ASR::cmpopType::NotEq, right, logical, nullptr));
            }
            default: {
                throw LCompilersException("Expression type, " +
                    ASRUtils::type_to_str_python(expr_type(left)) + " not yet supported");
                return nullptr;
            }
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
            m_body.p, m_body.n, nullptr, 0));
    }

    ASR::expr_t *TupleConstant(std::vector<ASR::expr_t*> ele, ASR::ttype_t *type) {
        Vec<ASR::expr_t*> m_ele; m_ele.reserve(al, 3);
        for (auto &x: ele) m_ele.push_back(al, x);
        return EXPR(ASR::make_TupleConstant_t(al, loc, m_ele.p, m_ele.n, type));
    }

    ASR::expr_t* Call(ASR::symbol_t* s, Vec<ASR::call_arg_t>& args,
                      ASR::ttype_t* return_type) {
        return ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, loc,
                s, s, args.p, args.size(), return_type, nullptr, nullptr,
                false));
    }

    ASR::expr_t* Call(ASR::symbol_t* s, Vec<ASR::expr_t *>& args,
                      ASR::ttype_t* return_type) {
        Vec<ASR::call_arg_t> args_; args_.reserve(al, 2);
        visit_expr_list(al, args, args_);
        return ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, loc,
                s, s, args_.p, args_.size(), return_type, nullptr, nullptr,
                false));
    }

    ASR::expr_t* Call(ASR::symbol_t* s, Vec<ASR::call_arg_t>& args,
                      ASR::ttype_t* return_type, ASR::expr_t* value) {
        return ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al, loc,
                s, s, args.p, args.size(), return_type, value, nullptr,
                false));
    }

    ASR::stmt_t* SubroutineCall(ASR::symbol_t* s, Vec<ASR::call_arg_t>& args) {
        return ASRUtils::STMT(ASRUtils::make_SubroutineCall_t_util(al, loc,
                s, s, args.p, args.size(), nullptr, nullptr, false, false));
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
        ASR::expr_t *arr_constant = EXPR(ASRUtils::make_ArrayConstructor_t_util(al, loc,
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
    ASR::stmt_t *Exit() {
        return STMT(ASR::make_Exit_t(al, loc, nullptr));
    }

    ASR::stmt_t *Return() {
        return STMT(ASR::make_Return_t(al, loc));
    }

    ASR::stmt_t *Assignment(ASR::expr_t *lhs, ASR::expr_t *rhs) {
        LCOMPILERS_ASSERT_MSG(check_equal_type(expr_type(lhs), expr_type(rhs)),
            type_to_str_python(expr_type(lhs)) + ", " + type_to_str_python(expr_type(rhs)));
        return STMT(ASR::make_Assignment_t(al, loc, lhs, rhs, nullptr));
    }

    ASR::stmt_t* CPtrToPointer(ASR::expr_t* cptr, ASR::expr_t* ptr, ASR::expr_t* shape = nullptr, ASR::expr_t* lower_bounds = nullptr) {
        return STMT(ASR::make_CPtrToPointer_t(al, loc, cptr, ptr, shape, lower_bounds));
    }

    template <typename T>
    ASR::stmt_t *Assign_Constant(ASR::expr_t *lhs, T init_value) {
        ASR::ttype_t *type = expr_type(lhs);
        switch(type->type) {
            case ASR::ttypeType::Integer : {
                return Assignment(lhs, i_t(init_value, type));
            }
            case ASR::ttypeType::Real : {
                return Assignment(lhs, f_t(init_value, type));
            }
            case ASR::ttypeType::Complex : {
                return Assignment(lhs, complex_t(init_value, init_value, type));
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

    ASR::stmt_t *Allocate(ASR::expr_t *m_a, ASR::dimension_t* m_dims, size_t n_dims) {
        Vec<ASR::alloc_arg_t> alloc_args; alloc_args.reserve(al, 1);
        ASR::alloc_arg_t alloc_arg;
        alloc_arg.loc = loc;
        alloc_arg.m_a = m_a;
        alloc_arg.m_dims = m_dims;
        alloc_arg.n_dims = n_dims;
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
        return STMT(ASR::make_DoLoop_t(al, loc, nullptr, head, body.p, body.n, nullptr, 0));
    }

    /*
    if loop_body contains A(i, j, k) then set idx_vars=(k, j, i)
    in order to iterate on the array left to right, the inner-most
    loop on the fastest index on the left, as is common in Fortran
    ```
        idx_vars=(k, j, i)
        body A(i, j, k)

        produces

        do k = 1, n
            do j = 1, n
                do i = 1, n
                    A(i, j, k)
                end do
            end do
        end do
    ```
    */
    template <typename LOOP_BODY>
    ASR::stmt_t* create_do_loop(
        const Location& loc, int rank, ASR::expr_t* array,
        SymbolTable* scope, Vec<ASR::expr_t*>& idx_vars,
        Vec<ASR::stmt_t*>& doloop_body, LOOP_BODY loop_body) {
        PassUtils::create_idx_vars(idx_vars, rank, loc, al, scope, "_i");

        ASR::stmt_t* doloop = nullptr;
        for ( int i = 0; i < (int) idx_vars.size(); i++ ) {
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
                        head, doloop_body.p, doloop_body.size(), nullptr, 0));
        }
        return doloop;
    }

    template <typename LOOP_BODY>
    ASR::stmt_t* create_do_loop(
        const Location& loc, ASR::expr_t* array,
        Vec<ASR::expr_t*>& loop_vars, std::vector<int>& loop_dims,
        Vec<ASR::stmt_t*>& doloop_body, LOOP_BODY loop_body) {

        ASR::stmt_t* doloop = nullptr;
        for ( int i = 0; i < (int) loop_vars.size(); i++ ) {
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
                        head, doloop_body.p, doloop_body.size(), nullptr, 0));
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
            ASR::expr_t* test_expr = Eq(dim, current_dim);
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
        return STMT(ASRUtils::make_print_t_util(al, loc, x_exprs.p, x_exprs.n));
    }

    ASR::symbol_t* create_c_func(std::string c_func_name, SymbolTable* fn_symtab, ASR::ttype_t* return_type, int n_args, Vec<ASR::ttype_t*>& arg_types) {
        SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);
        Vec<ASR::expr_t*> args_1; args_1.reserve(al, n_args);
        for (int i = 0; i < n_args; i++) {
            args_1.push_back(al, this->Variable(fn_symtab_1, "x_"+std::to_string(i), arg_types[i],
                ASR::intentType::In, ASR::abiType::BindC, true));
        }
        ASR::expr_t *return_var_1 = this->Variable(fn_symtab_1, c_func_name,
            return_type, ASRUtils::intent_return_var, ASR::abiType::BindC, false);

        SetChar dep_1; dep_1.reserve(al, 1);
        Vec<ASR::stmt_t*> body_1; body_1.reserve(al, 1);
        ASR::symbol_t *s = make_ASR_Function_t(c_func_name, fn_symtab_1, dep_1, args_1,
            body_1, return_var_1, ASR::abiType::BindC, ASR::deftypeType::Interface, s2c(al, c_func_name));
        return s;
    }

    ASR::symbol_t* create_c_func_subroutines(std::string c_func_name, SymbolTable* fn_symtab, int n_args, ASR::ttype_t* arg_types) {
        SymbolTable *fn_symtab_1 = al.make_new<SymbolTable>(fn_symtab);
        Vec<ASR::expr_t*> args_1; args_1.reserve(al, 0);
        for (int i = 0; i < n_args; i++) {
            args_1.push_back(al, this->Variable(fn_symtab_1, "x_"+std::to_string(i), arg_types,
                ASR::intentType::InOut, ASR::abiType::BindC, true));
        }
        ASR::expr_t *return_var_1 = this->Variable(fn_symtab_1, c_func_name,
           ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(arg_types)),
           ASRUtils::intent_return_var, ASR::abiType::BindC, false);
        SetChar dep_1; dep_1.reserve(al, 1);
        Vec<ASR::stmt_t*> body_1; body_1.reserve(al, 1);
        ASR::symbol_t *s = make_ASR_Function_t(c_func_name, fn_symtab_1, dep_1, args_1,
            body_1, return_var_1, ASR::abiType::BindC, ASR::deftypeType::Interface, s2c(al, c_func_name));
        return s;
    }

};

} // namespace LCompilers::ASRUtils

#endif // LIBASR_BUILDER_H
