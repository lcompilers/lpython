#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pickle.h>
#include <lfortran/modfile.h>
#include <lfortran/serialization.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/parser/parser_stype.h>
#include <lfortran/string_utils.h>
#include <lfortran/utils.h>

#define num_types 12

namespace LFortran {

    namespace HelperMethods {


            inline bool is_pointer(ASR::ttype_t* x) {
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
                bool is_under_score = false;
                char kind_str[2] = {'0', '0'};
                int i = 1, j = 0;
                for( ; m_n[i] != '\0'; i++ ) {
                    is_under_score = m_n[i-1] == '_' && !is_under_score ? true : is_under_score;
                    if( is_under_score ) {
                        kind_str[j] = m_n[i];
                        j++;
                    }
                }
                if( kind_str[0] != '0' && kind_str[1] == '0'  ) {
                    return kind_str[0] - '0';
                } else if( kind_str[0] != '0' && kind_str[0] != '0' ) {
                    return (kind_str[0] - '0')*10 + (kind_str[1] - '0');
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
                                if (ASR::is_a<ASR::ConstantInteger_t>(
                                        *(kind_variable->m_value))) {
                                    a_kind = ASR::down_cast
                                        <ASR::ConstantInteger_t>
                                        (kind_variable->m_value)->m_n;
                                } else if (ASR::is_a<ASR::FunctionCall_t>(
                                        *(kind_variable->m_value))) {
                                    ASR::FunctionCall_t *fc =
                                        ASR::down_cast<ASR::FunctionCall_t>(
                                        kind_variable->m_value);
                                    ASR::Function_t *fn =
                                        ASR::down_cast<ASR::Function_t>(
                                        symbol_get_past_external(fc->m_name));
                                    if (std::string(fn->m_name)=="kind") {
                                        if (fc->n_args == 1 &&
                                            ASR::is_a<ASR::ConstantLogical_t>(
                                                    *fc->m_args[0])) {
                                            ASR::ConstantLogical_t *l = ASR::down_cast<
                                                ASR::ConstantLogical_t>(
                                                fc->m_args[0]);
                                            ASR::Logical_t *lt = ASR::down_cast<
                                                ASR::Logical_t>(l->m_type);
                                            a_kind = lt->m_kind;
                                        } else {
                                            throw SemanticError("kind",
                                                loc);
                                        }
                                    } else if (std::string(fn->m_name)
                                            == "selected_int_kind") {
                                        if (fc->n_args == 1 &&
                                            ASR::is_a<ASR::ConstantInteger_t>(
                                                    *fc->m_args[0])) {
                                            ASR::ConstantInteger_t *i = ASR::down_cast<
                                                ASR::ConstantInteger_t>(
                                                fc->m_args[0]);
                                            int R = i->m_n;
                                            if (R < 10) {
                                                a_kind = 4;
                                            } else {
                                                a_kind = 8;
                                            }
                                        } else {
                                            throw SemanticError("selected_int_kind",
                                                loc);
                                        }
                                    } else if (std::string(fn->m_name)
                                            == "selected_real_kind") {
                                        if (fc->n_args == 1 &&
                                            ASR::is_a<ASR::ConstantInteger_t>(
                                                    *fc->m_args[0])) {
                                            ASR::ConstantInteger_t *i = ASR::down_cast<
                                                ASR::ConstantInteger_t>(
                                                fc->m_args[0]);
                                            int R = i->m_n;
                                            if (R < 7) {
                                                a_kind = 4;
                                            } else {
                                                a_kind = 8;
                                            }
                                        } else {
                                            throw SemanticError("selected_real_kind",
                                                loc);
                                        }
                                    } else {
                                        throw SemanticError("FunctionCall to '"
                                            + std::string(fn->m_name)
                                            + "' unsupported",
                                        loc);
                                    }
                                } else {
                                    throw SemanticError("So far only ConstantInteger or FunctionCall supported as kind variable value",
                                        loc);
                                }
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

                return HelperMethods::is_same_type_pointer(x, y);
            }

            inline int extract_kind_from_ttype_t(const ASR::ttype_t* curr_type) {
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
    }

    class ImplicitCastRules {

        private:

            //! Default case when no conversion is needed.
            static const int default_case = -1;
            //! Error case when conversion is not possible or is illegal.
            static const int error_case = -2;
            static const int integer_to_real = ASR::cast_kindType::IntegerToReal;
            static const int integer_to_integer = ASR::cast_kindType::IntegerToInteger;
            static const int real_to_integer = ASR::cast_kindType::RealToInteger;
            static const int real_to_complex = ASR::cast_kindType::RealToComplex;
            static const int integer_to_complex = ASR::cast_kindType::IntegerToComplex;
            static const int integer_to_logical = ASR::cast_kindType::IntegerToLogical;
            static const int complex_to_complex = ASR::cast_kindType::ComplexToComplex;
            static const int real_to_real = ASR::cast_kindType::RealToReal;

            //! Stores the variable part of error messages to be passed to SemanticError.
            static constexpr const char* type_names[num_types][2] = {
                {"Integer", "Integer Pointer"}, 
                {"Real", "Integer or Real or Real Pointer"}, 
                {"Complex", "Integer, Real or Complex or Complex Pointer"},
                {"Character", "Character Pointer"},
                {"Logical", "Integer or Logical Pointer"},
                {"Derived", "Derived Pointer"},
                {"Integer Pointer", "Integer"},
                {"Real Pointer", "Integer"},
                {"Complex Pointer", "Integer"},
                {"Character Pointer", "Integer"},
                {"Logical Pointer", "Integer"},
                {"Derived Pointer", "Integer"}
            }; 

            /*
            * Rule map for performing implicit cast represented by a 2D integer array.
            * 
            * Key is the pair of indices with row index denoting the source type
            * and column index denoting the destination type.
            */
            static constexpr const int rule_map[num_types/2][num_types] = {

                {integer_to_integer, integer_to_real, integer_to_complex, error_case, integer_to_logical, error_case,
                 integer_to_integer, integer_to_real, integer_to_complex, error_case, integer_to_logical, error_case},
                {real_to_integer, real_to_real, real_to_complex, default_case, default_case, default_case,
                 real_to_integer, real_to_real, real_to_complex, default_case, default_case, default_case},
                {default_case, default_case, complex_to_complex, default_case, default_case, default_case,
                 default_case, default_case, complex_to_complex, default_case, default_case, default_case},
                {default_case, default_case, default_case, default_case, default_case, default_case,
                 default_case, default_case, default_case, default_case, default_case, default_case},
                {default_case, default_case, default_case, default_case, default_case, default_case,
                 default_case, default_case, default_case, default_case, default_case, default_case},
                {default_case, default_case, default_case, default_case, default_case, default_case,
                 default_case, default_case, default_case, default_case, default_case, default_case}

            };

            /*
            * Priority of different types to be used in conversion
            * when source and destination are directly not deducible.
            */
            static constexpr const int type_priority[num_types/2] = {
                4, // Integer or IntegerPointer
                5, // Real or RealPointer
                6, // Complex or ComplexPointer
                -1, // Character or CharacterPointer
                -1, // Logical or LogicalPointer
                -1 // Derived or DerivedPointer
            };
    
        public:

            /*
            * Adds ImplicitCast node if necessary.
            * 
            * @param al Allocator&
            * @param a_loc Location&
            * @param convert_can ASR::expr_t** Address of the pointer to 
            *                                 conversion candidate.
            * @param source_type ASR::ttype_t* Source type.
            * @param dest_type AST::ttype_t* Destination type.
            */
            static void set_converted_value
            (Allocator &al, const Location &a_loc, 
             ASR::expr_t** convert_can, ASR::ttype_t* source_type, ASR::ttype_t* dest_type) {
                if( source_type->type == dest_type->type || HelperMethods::is_same_type_pointer(source_type, dest_type) ) {
                    bool is_source_pointer = HelperMethods::is_pointer(source_type);
                    bool is_dest_pointer = HelperMethods::is_pointer(dest_type);
                    if( is_source_pointer && !is_dest_pointer ) {
                        ASR::ttype_t* temp = source_type;
                        source_type = dest_type;
                        dest_type = temp;
                    }
                    int source_kind = 0, dest_kind = 1;
                    source_kind = HelperMethods::extract_kind_from_ttype_t(source_type);
                    dest_kind = HelperMethods::extract_kind_from_ttype_t(dest_type);
                    if( source_kind == dest_kind ) {
                        return ;
                    }
                }
                int cast_kind = rule_map[source_type->type%(num_types/2)][dest_type->type];
                if( cast_kind == error_case )
                {
                    std::string allowed_types_str = type_names[dest_type->type][1];
                    std::string dest_type_str = type_names[dest_type->type][0];
                    std::string error_msg = "Only " + allowed_types_str + 
                                            " can be assigned to " + dest_type_str;
                    throw SemanticError(error_msg, a_loc);
                }
                else if( cast_kind != default_case )
                {
                    *convert_can = (ASR::expr_t*) ASR::make_ImplicitCast_t(
                        al, a_loc, *convert_can, (ASR::cast_kindType) cast_kind, 
                        dest_type
                    );
                }
            }

            /*
            * Deduces the candidate which is to be casted
            * based on the priority of types.
            * 
            * @param left ASR::expr_t** Address of the pointer to left 
            *                           element in the operation.
            * @param right ASR::expr_t** Address of the pointer to right
            *                            element in the operation.
            * @param left_type ASR::ttype_t* Pointer to the type of left element.
            * @param right_type ASR::ttype_t* Pointer to the type of right element.
            * @param conversion_cand ASR::expr_t**& Reference to the address of
            *                                      the pointer of conversion
            *                                      candidate.
            * @param source_type ASR::ttype_t** For storing the address of pointer
            *                                  to source type.
            * @param dest_type ASR::ttype_t** For stroing the address of pointer to
            *                                destination type.
            * 
            * Note
            * ====
            * 
            * Address of pointer have been used so that the contents 
            * of the pointer variables are modified which are then 
            * used in making the node of different operations. If directly
            * the pointer values are used, then no effect on left or right
            * is observed and ASR construction fails.
            */
            static void find_conversion_candidate
            (ASR::expr_t** left, ASR::expr_t** right,
             ASR::ttype_t* left_type, ASR::ttype_t* right_type,
             ASR::expr_t** &conversion_cand, 
             ASR::ttype_t** source_type, ASR::ttype_t** dest_type) {

                int left_type_p = type_priority[left_type->type%(num_types/2)];
                int right_type_p = type_priority[right_type->type%(num_types/2)];
                if( left_type_p >= right_type_p ) {
                    conversion_cand = right;
                    *source_type = right_type;
                    *dest_type = left_type;
                }
                else {
                    conversion_cand = left;
                    *source_type = left_type;
                    *dest_type = right_type;
                }
            }

    };

class CommonVisitorMethods {

    public:

    inline static void visit_BinOp(Allocator& al, const AST::BinOp_t &x, 
                            ASR::expr_t*& left, ASR::expr_t*& right, ASR::asr_t*& asr) {
        ASR::binopType op;
        switch (x.m_op) {
            case (AST::Add) :
                op = ASR::Add;
                break;
            case (AST::Sub) :
                op = ASR::Sub;
                break;
            case (AST::Mul) :
                op = ASR::Mul;
                break;
            case (AST::Div) :
                op = ASR::Div;
                break;
            case (AST::Pow) :
                op = ASR::Pow;
                break;
            // Fix compiler warning:
            default : { LFORTRAN_ASSERT(false); op = ASR::binopType::Pow; }
        }

        // Cast LHS or RHS if necessary
        ASR::ttype_t *left_type = expr_type(left);
        ASR::ttype_t *right_type = expr_type(right);
        ASR::expr_t **conversion_cand = &left;
        ASR::ttype_t *source_type = left_type;
        ASR::ttype_t *dest_type = right_type;

        ImplicitCastRules::find_conversion_candidate(
            &left, &right, left_type, right_type, 
            conversion_cand, &source_type, &dest_type);
        ImplicitCastRules::set_converted_value(
            al, x.base.base.loc, conversion_cand,
            source_type, dest_type);

        LFORTRAN_ASSERT(HelperMethods::check_equal_type(expr_type(left), expr_type(right)));
        asr = ASR::make_BinOp_t(al, x.base.base.loc, left, op, right, dest_type);
    }

    inline static void visit_Compare(Allocator& al, const AST::Compare_t &x, ASR::expr_t*& left, 
                       ASR::expr_t*& right, ASR::asr_t*& asr) {
        // Cast LHS or RHS if necessary
        ASR::ttype_t *left_type = expr_type(left);
        ASR::ttype_t *right_type = expr_type(right);
        if( (left_type->type != ASR::ttypeType::Real && 
            left_type->type != ASR::ttypeType::Integer) &&
            (right_type->type != ASR::ttypeType::Real &&
             right_type->type != ASR::ttypeType::Integer) ) {
            throw SemanticError(
                "Compare: only Integer or Real can be on the LHS and RHS", 
            x.base.base.loc);
        }
        else
        {
            ASR::expr_t **conversion_cand = &left;
            ASR::ttype_t *dest_type = right_type;
            ASR::ttype_t *source_type = left_type;
            ImplicitCastRules::find_conversion_candidate
            (&left, &right, left_type, right_type, 
             conversion_cand, &source_type, &dest_type);

            ImplicitCastRules::set_converted_value
            (al, x.base.base.loc, conversion_cand, 
             source_type, dest_type);
        }

        LFORTRAN_ASSERT(HelperMethods::check_equal_type(expr_type(left), expr_type(right)));
        ASR::ttype_t *type = TYPE(ASR::make_Logical_t(al, x.base.base.loc,
                4, nullptr, 0));
        ASR::cmpopType asr_op;
        switch (x.m_op) {
            case (AST::cmpopType::Eq) : { asr_op = ASR::cmpopType::Eq; break;}
            case (AST::cmpopType::Gt) : { asr_op = ASR::cmpopType::Gt; break;}
            case (AST::cmpopType::GtE) : { asr_op = ASR::cmpopType::GtE; break;}
            case (AST::cmpopType::Lt) : { asr_op = ASR::cmpopType::Lt; break;}
            case (AST::cmpopType::LtE) : { asr_op = ASR::cmpopType::LtE; break;}
            case (AST::cmpopType::NotEq) : { asr_op = ASR::cmpopType::NotEq; break;}
            default : {
                throw SemanticError("Comparison operator not implemented",
                        x.base.base.loc);
            }
        }
        asr = ASR::make_Compare_t(al, x.base.base.loc,
            left, asr_op, right, type);
    }

    inline static void visit_BoolOp(Allocator& al, const AST::BoolOp_t &x, ASR::expr_t*& left, 
                                    ASR::expr_t*& right, ASR::asr_t*& asr) {
        ASR::boolopType op;
        switch (x.m_op) {
            case (AST::And):
                op = ASR::And;
                break;
            case (AST::Or):
                op = ASR::Or;
                break;
            case (AST::NEqv):
                op = ASR::NEqv;
                break;
            case (AST::Eqv):
                op = ASR::Eqv;
                break;
            default:
                throw SemanticError(R"""(Only .and., .or., .neqv., .eqv. 
                                    implemented for logical type operands.)""", 
                                    x.base.base.loc);
        }

        // Cast LHS or RHS if necessary
        ASR::ttype_t *left_type = expr_type(left);
        ASR::ttype_t *right_type = expr_type(right);
        ASR::expr_t **conversion_cand = &left;
        ASR::ttype_t *source_type = left_type;
        ASR::ttype_t *dest_type = right_type;

        ImplicitCastRules::find_conversion_candidate(
            &left, &right, left_type, right_type, 
            conversion_cand, &source_type, &dest_type);
        ImplicitCastRules::set_converted_value(
            al, x.base.base.loc, conversion_cand,
            source_type, dest_type);

        LFORTRAN_ASSERT(HelperMethods::check_equal_type(expr_type(left), expr_type(right)));
        asr = ASR::make_BoolOp_t(al, x.base.base.loc,
                left, op, right, dest_type);
    }

    inline static void visit_UnaryOp(Allocator& al, const AST::UnaryOp_t &x, 
                                     ASR::expr_t*& operand, ASR::asr_t*& asr) {
        ASR::unaryopType op;
        switch (x.m_op) {
            case (AST::unaryopType::Invert) :
                op = ASR::unaryopType::Invert;
                break;
            case (AST::unaryopType::Not) :
                op = ASR::unaryopType::Not;
                break;
            case (AST::unaryopType::UAdd) :
                op = ASR::unaryopType::UAdd;
                break;
            case (AST::unaryopType::USub) :
                op = ASR::unaryopType::USub;
                break;
            // Fix compiler warning:
            default : { LFORTRAN_ASSERT(false); op = ASR::unaryopType::Invert; }
        }
        ASR::ttype_t *operand_type = expr_type(operand);
        asr = ASR::make_UnaryOp_t(al, x.base.base.loc,
                op, operand, operand_type);
    }

    static inline void visit_StrOp(Allocator& al, const AST::StrOp_t &x, ASR::expr_t*& left, 
                                    ASR::expr_t*& right, ASR::asr_t*& asr) { 
        ASR::stropType op;
        switch (x.m_op) {
            case (AST::Concat) :
                op = ASR::Concat;
        }
        ASR::ttype_t *right_type = expr_type(right);
        ASR::ttype_t *dest_type = right_type;
        // TODO: Type check here?
        asr = ASR::make_StrOp_t(al, x.base.base.loc,
                left, op, right, dest_type);
    }        

};

std::map<std::string, std::string> intrinsic_procedures = {
        {"kind", "lfortran_intrinsic_kind"},
        {"selected_int_kind", "lfortran_intrinsic_kind"},
        {"selected_real_kind", "lfortran_intrinsic_kind"},
        {"size", "lfortran_intrinsic_array"},
    };

std::string read_file(const std::string &filename)
{
    std::ifstream ifs(filename.c_str(), std::ios::in | std::ios::binary
            | std::ios::ate);

    std::ifstream::pos_type filesize = ifs.tellg();
    if (filesize < 0) return std::string();

    ifs.seekg(0, std::ios::beg);

    std::vector<char> bytes(filesize);
    ifs.read(&bytes[0], filesize);

    return std::string(&bytes[0], filesize);
}

bool present(Vec<char*> &v, const char* name) {
    for (auto &a : v) {
        if (std::string(a) == std::string(name)) {
            return true;
        }
    }
    return false;
}

ASR::Module_t* extract_module(const ASR::TranslationUnit_t &m) {
    LFORTRAN_ASSERT(m.m_global_scope->scope.size()== 1);
    for (auto &a : m.m_global_scope->scope) {
        LFORTRAN_ASSERT(ASR::is_a<ASR::Module_t>(*a.second));
        return ASR::down_cast<ASR::Module_t>(a.second);
    }
    throw LFortranException("ICE: Module not found");
}

ASR::TranslationUnit_t* find_and_load_module(
        Allocator &al,
        const std::string &msym,
        SymbolTable &symtab, bool intrinsic) {
    std::string modfilename = msym + ".mod";
    if (intrinsic) {
        std::string rl_path = get_runtime_library_dir();
        modfilename = rl_path + "/" + modfilename;
    }
    std::string modfile = read_file(modfilename);
    if (modfile == "") return nullptr;
    ASR::TranslationUnit_t *asr = load_modfile(al, modfile, false,
        symtab);
    return asr;
}

ASR::Module_t* load_module(
    Allocator &al,
    SymbolTable *symtab,
    const std::string &module_name,
    const Location &loc, bool intrinsic) {
    LFORTRAN_ASSERT(symtab);
    if (symtab->scope.find(module_name) != symtab->scope.end()) {
        ASR::symbol_t *m = symtab->scope[module_name];
        if (ASR::is_a<ASR::Module_t>(*m)) {
            return ASR::down_cast<ASR::Module_t>(m);
        } else {
            throw SemanticError("The symbol '" + module_name
                + "' is not a module", loc);
        }
    }
    LFORTRAN_ASSERT(symtab->parent == nullptr);
    ASR::TranslationUnit_t *mod1 = find_and_load_module(al, module_name,
            *symtab, intrinsic);
    if (mod1 == nullptr) {
        throw SemanticError("Module '" + module_name + "' not declared in the current source and the modfile was not found",
            loc);
    }
    ASR::Module_t *mod2 = extract_module(*mod1);
    symtab->scope[module_name] = (ASR::symbol_t*)mod2;
    mod2->m_symtab->parent = symtab;
    mod2->m_loaded_from_mod = true;
    LFORTRAN_ASSERT(symtab->resolve_symbol(module_name));

    // Create a temporary TranslationUnit just for fixing the symbols
    ASR::TranslationUnit_t *tu
        = ASR::down_cast2<ASR::TranslationUnit_t>(ASR::make_TranslationUnit_t(al, loc,
            symtab, nullptr, 0));

    // Load any dependent modules recursively
    bool rerun = true;
    while (rerun) {
        rerun = false;
        std::vector<std::string> modules_list
            = determine_module_dependencies(*tu);
        for (auto &item : modules_list) {
            if (symtab->scope.find(item)
                    == symtab->scope.end()) {
                // A module that was loaded requires to load another
                // module

                // This is not very robust, we should store that information
                // in the ASR itself, or encode in the name in a robust way,
                // such as using `module_name@intrinsic`:
                bool is_intrinsic = startswith(item, "lfortran_intrinsic");
                ASR::TranslationUnit_t *mod1 = find_and_load_module(al,
                        item,
                        *symtab, is_intrinsic);
                if (mod1 == nullptr) {
                    throw SemanticError("Module '" + item + "' modfile was not found",
                        loc);
                }
                ASR::Module_t *mod2 = extract_module(*mod1);
                symtab->scope[item] = (ASR::symbol_t*)mod2;
                mod2->m_symtab->parent = symtab;
                mod2->m_loaded_from_mod = true;
                rerun = true;
            }
        }
    }

    // Check that all modules are included in ASR now
    std::vector<std::string> modules_list
        = determine_module_dependencies(*tu);
    for (auto &item : modules_list) {
        if (symtab->scope.find(item) == symtab->scope.end()) {
            throw SemanticError("ICE: Module '" + item + "' modfile was not found, but should have",
                loc);
        }
    }

    // Fix all external symbols
    fix_external_symbols(*tu, *symtab);
    LFORTRAN_ASSERT(asr_verify(*tu));

    return mod2;
}


class SymbolTableVisitor : public AST::BaseVisitor<SymbolTableVisitor>
{
public:
    ASR::asr_t *asr;
    Allocator &al;
    SymbolTable *current_scope;
    SymbolTable *global_scope;
    std::map<std::string, std::vector<std::string>> generic_procedures;
    ASR::accessType dflt_access = ASR::Public;
    std::map<std::string, ASR::accessType> assgnd_access;
    Vec<char*> current_module_dependencies;
    bool in_module=false;
    bool is_interface=false;
    std::vector<std::string> current_procedure_args;

    SymbolTableVisitor(Allocator &al, SymbolTable *symbol_table)
        : al{al}, current_scope{symbol_table} { }

    ASR::symbol_t* resolve_symbol(const Location &loc, const char* id) {
        SymbolTable *scope = current_scope;
        std::string sub_name = id;
        ASR::symbol_t *sub = scope->resolve_symbol(sub_name);
        if (!sub) {
            throw SemanticError("Symbol '" + sub_name + "' not declared", loc);
        }
        return sub;
    }

    void visit_TranslationUnit(const AST::TranslationUnit_t &x) {
        if (!current_scope) {
            current_scope = al.make_new<SymbolTable>(nullptr);
        }
        LFORTRAN_ASSERT(current_scope != nullptr);
        global_scope = current_scope;
        for (size_t i=0; i<x.n_items; i++) {
            AST::astType t = x.m_items[i]->type;
            if (t != AST::astType::expr && t != AST::astType::stmt) {
                visit_ast(*x.m_items[i]);
            }
        }
        global_scope = nullptr;
        asr = ASR::make_TranslationUnit_t(al, x.base.base.loc,
            current_scope, nullptr, 0);
    }

    void visit_Module(const AST::Module_t &x) {
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        current_module_dependencies.reserve(al, 4);
        in_module = true;
        for (size_t i=0; i<x.n_use; i++) {
            visit_unit_decl1(*x.m_use[i]);
        }
        for (size_t i=0; i<x.n_decl; i++) {
            visit_unit_decl2(*x.m_decl[i]);
        }
        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }
        add_generic_procedures();
        asr = ASR::make_Module_t(
            al, x.base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ x.m_name,
            current_module_dependencies.p,
            current_module_dependencies.n,
            false);
        std::string sym_name = x.m_name;
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            throw SemanticError("Module already defined", asr->loc);
        }
        parent_scope->scope[sym_name] = ASR::down_cast<ASR::symbol_t>(asr);
        current_scope = parent_scope;
        in_module = false;
    }

    void visit_Program(const AST::Program_t &x) {
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        current_module_dependencies.reserve(al, 4);
        for (size_t i=0; i<x.n_use; i++) {
            visit_unit_decl1(*x.m_use[i]);
        }
        for (size_t i=0; i<x.n_decl; i++) {
            visit_unit_decl2(*x.m_decl[i]);
        }
        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }
        asr = ASR::make_Program_t(
            al, x.base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ x.m_name,
            current_module_dependencies.p,
            current_module_dependencies.n,
            /* a_body */ nullptr,
            /* n_body */ 0);
        std::string sym_name = x.m_name;
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            throw SemanticError("Program already defined", asr->loc);
        }
        parent_scope->scope[sym_name] = ASR::down_cast<ASR::symbol_t>(asr);
        current_scope = parent_scope;
    }

    void visit_Subroutine(const AST::Subroutine_t &x) {
        ASR::accessType s_access = dflt_access;
        ASR::deftypeType deftype = ASR::deftypeType::Implementation;
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        for (size_t i=0; i<x.n_args; i++) {
            char *arg=x.m_args[i].m_arg;
            std::string arg_s = arg;
            current_procedure_args.push_back(arg);
        }
        for (size_t i=0; i<x.n_decl; i++) {
            visit_unit_decl2(*x.m_decl[i]);
        }
        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }
        Vec<ASR::expr_t*> args;
        args.reserve(al, x.n_args);
        for (size_t i=0; i<x.n_args; i++) {
            char *arg=x.m_args[i].m_arg;
            std::string arg_s = arg;
            if (current_scope->scope.find(arg_s) == current_scope->scope.end()) {
                throw SemanticError("Dummy argument '" + arg_s + "' not defined", x.base.base.loc);
            }
            ASR::symbol_t *var = current_scope->scope[arg_s];
            args.push_back(al, EXPR(ASR::make_Var_t(al, x.base.base.loc,
                var)));
        }
        std::string sym_name = x.m_name;
        if (assgnd_access.count(sym_name)) {
            s_access = assgnd_access[sym_name];
        }
        if (is_interface){
            deftype = ASR::deftypeType::Interface;
        }
        asr = ASR::make_Subroutine_t(
            al, x.base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ x.m_name,
            /* a_args */ args.p,
            /* n_args */ args.size(),
            /* a_body */ nullptr,
            /* n_body */ 0,
            ASR::abiType::Source,
            s_access, deftype);
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            ASR::symbol_t *f1 = parent_scope->scope[sym_name];
            ASR::Subroutine_t *f2 = ASR::down_cast<ASR::Subroutine_t>(f1);
            if (f2->m_abi == ASR::abiType::Interactive) {
                // Previous declaration will be shadowed
            } else {
                throw SemanticError("Subroutine already defined", asr->loc);
            }
        }
        parent_scope->scope[sym_name] = ASR::down_cast<ASR::symbol_t>(asr);
        current_scope = parent_scope;
        /* FIXME: This can become incorrect/get cleared prematurely, perhaps
           in nested functions, and also in callback.f90 test, but it may not
           matter since we would have already checked the intent */
        current_procedure_args.clear();
    }

    AST::AttrType_t* find_return_type(AST::decl_attribute_t** attributes,
            size_t n, const Location &loc) {
        AST::AttrType_t* r = nullptr;
        bool found = false;
        for (size_t i=0; i<n; i++) {
            if (AST::is_a<AST::AttrType_t>(*attributes[i])) {
                if (found) {
                    throw SemanticError("Return type declared twice", loc);
                } else {
                    r = AST::down_cast<AST::AttrType_t>(attributes[i]);
                    found = true;
                }
            }
        }
        return r;
    }

    void visit_Function(const AST::Function_t &x) {
        // Extract local (including dummy) variables first
        ASR::accessType s_access = dflt_access;
        ASR::deftypeType deftype = ASR::deftypeType::Implementation;
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        for (size_t i=0; i<x.n_args; i++) {
            char *arg=x.m_args[i].m_arg;
            std::string arg_s = arg;
            current_procedure_args.push_back(arg);
        }
        for (size_t i=0; i<x.n_decl; i++) {
            visit_unit_decl2(*x.m_decl[i]);
        }
        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }
        // Convert and check arguments
        Vec<ASR::expr_t*> args;
        args.reserve(al, x.n_args);
        for (size_t i=0; i<x.n_args; i++) {
            char *arg=x.m_args[i].m_arg;
            std::string arg_s = arg;
            if (current_scope->scope.find(arg_s) == current_scope->scope.end()) {
                throw SemanticError("Dummy argument '" + arg_s + "' not defined", x.base.base.loc);
            }
            ASR::symbol_t *var = current_scope->scope[arg_s];
            args.push_back(al, EXPR(ASR::make_Var_t(al, x.base.base.loc,
                var)));
        }

        // Handle the return variable and type
        // First determine the name of the variable: either the function name
        // or result(...)
        char *return_var_name;
        if (x.m_return_var) {
            if (x.m_return_var->type == AST::exprType::Name) {
                return_var_name = ((AST::Name_t*)(x.m_return_var))->m_id;
            } else {
                throw SemanticError("Return variable must be an identifier",
                    x.m_return_var->base.loc);
            }
        } else {
            return_var_name = x.m_name;
        }

        // Determine the type of the variable, the type is either specified as
        //     integer function f()
        // or in local variables as
        //     integer :: f
        ASR::asr_t *return_var;
        AST::AttrType_t *return_type = find_return_type(x.m_attributes,
            x.n_attributes, x.base.base.loc);
        if (current_scope->scope.find(std::string(return_var_name)) == current_scope->scope.end()) {
            // The variable is not defined among local variables, extract the
            // type from "integer function f()" and add the variable.
            if (!return_type) {
                throw SemanticError("Return type not specified",
                        x.base.base.loc);
            }
            ASR::ttype_t *type;
            int a_kind = 4;
            if (return_type->m_kind != nullptr) {
                visit_expr(*return_type->m_kind->m_value);
                ASR::expr_t* kind_expr = EXPR(asr);
                a_kind = HelperMethods::extract_kind(kind_expr, x.base.base.loc);
            }
            switch (return_type->m_type) {
                case (AST::decl_typeType::TypeInteger) : {
                    type = TYPE(ASR::make_Integer_t(al, x.base.base.loc, a_kind, nullptr, 0));
                    break;
                }
                case (AST::decl_typeType::TypeReal) : {
                    type = TYPE(ASR::make_Real_t(al, x.base.base.loc, a_kind, nullptr, 0));
                    break;
                }
                case (AST::decl_typeType::TypeComplex) : {
                    type = TYPE(ASR::make_Complex_t(al, x.base.base.loc, a_kind, nullptr, 0));
                    break;
                }
                case (AST::decl_typeType::TypeLogical) : {
                    type = TYPE(ASR::make_Logical_t(al, x.base.base.loc, 4, nullptr, 0));
                    break;
                }
                default :
                    throw SemanticError("Return type not supported",
                            x.base.base.loc);
            }
            // Add it as a local variable:
            return_var = ASR::make_Variable_t(al, x.base.base.loc,
                current_scope, return_var_name, intent_return_var, nullptr,
                ASR::storage_typeType::Default, type,
                ASR::abiType::Source, ASR::Public);
            current_scope->scope[std::string(return_var_name)]
                = ASR::down_cast<ASR::symbol_t>(return_var);
        } else {
            if (return_type) {
                throw SemanticError("Cannot specify the return type twice",
                    x.base.base.loc);
            }
            // Extract the variable from the local scope
            return_var = (ASR::asr_t*) current_scope->scope[std::string(return_var_name)];
            ASR::down_cast2<ASR::Variable_t>(return_var)->m_intent = intent_return_var;
        }

        ASR::asr_t *return_var_ref = ASR::make_Var_t(al, x.base.base.loc,
            ASR::down_cast<ASR::symbol_t>(return_var));

        // Create and register the function
        std::string sym_name = x.m_name;
        if (assgnd_access.count(sym_name)) {
            s_access = assgnd_access[sym_name];
        }
        if (is_interface) {
            deftype = ASR::deftypeType::Interface;
        }
        asr = ASR::make_Function_t(
            al, x.base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ x.m_name,
            /* a_args */ args.p,
            /* n_args */ args.size(),
            /* a_body */ nullptr,
            /* n_body */ 0,
            /* a_return_var */ EXPR(return_var_ref),
            ASR::abiType::Source, s_access, deftype);
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            ASR::symbol_t *f1 = parent_scope->scope[sym_name];
            ASR::Function_t *f2 = ASR::down_cast<ASR::Function_t>(f1);
            if (f2->m_abi == ASR::abiType::Interactive) {
                // Previous declaration will be shadowed
            } else {
                throw SemanticError("Function already defined", asr->loc);
            }
        }
        parent_scope->scope[sym_name] = ASR::down_cast<ASR::symbol_t>(asr);
        current_scope = parent_scope;
        current_procedure_args.clear();
    }

    void visit_StrOp(const AST::StrOp_t &x) { 
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = EXPR(asr);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = EXPR(asr);
        CommonVisitorMethods::visit_StrOp(al, x, left, right, asr);
    }

    void visit_UnaryOp(const AST::UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        ASR::expr_t *operand = EXPR(asr);
        CommonVisitorMethods::visit_UnaryOp(al, x, operand, asr);
    }

    void visit_BoolOp(const AST::BoolOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = EXPR(asr);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = EXPR(asr);
        CommonVisitorMethods::visit_BoolOp(al, x, left, right, asr);
    }

    void visit_Compare(const AST::Compare_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = EXPR(asr);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = EXPR(asr);
        CommonVisitorMethods::visit_Compare(al, x, left, right, asr);
    }

    void visit_BinOp(const AST::BinOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = EXPR(asr);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = EXPR(asr);
        CommonVisitorMethods::visit_BinOp(al, x, left, right, asr);
    }

    void visit_Str(const AST::Str_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Character_t(al, x.base.base.loc,
                8, nullptr, 0));
        asr = ASR::make_Str_t(al, x.base.base.loc, x.m_s, type);
    }

    void visit_Logical(const AST::Logical_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Logical_t(al, x.base.base.loc,
                4, nullptr, 0));
        asr = ASR::make_ConstantLogical_t(al, x.base.base.loc, x.m_value, type);
    }

    void visit_Complex(const AST::Complex_t &x) {
        this->visit_expr(*x.m_re);
        ASR::expr_t *re = EXPR(asr);
        this->visit_expr(*x.m_im);
        ASR::expr_t *im = EXPR(asr);
        int re_kind = HelperMethods::extract_kind_from_ttype_t(expr_type(re));
        int im_kind = HelperMethods::extract_kind_from_ttype_t(expr_type(im));
        ASR::ttype_t *type = TYPE(ASR::make_Complex_t(al, x.base.base.loc,
                std::max(re_kind, im_kind), nullptr, 0));
        asr = ASR::make_ConstantComplex_t(al, x.base.base.loc,
                re, im, type);
    }

    void process_dims(Allocator &al, Vec<ASR::dimension_t> &dims,
        AST::dimension_t *m_dim, size_t n_dim) {
        LFORTRAN_ASSERT(dims.size() == 0);
        dims.reserve(al, n_dim);
        for (size_t i=0; i<n_dim; i++) {
            ASR::dimension_t dim;
            dim.loc = m_dim[i].loc;
            if (m_dim[i].m_start) {
                this->visit_expr(*m_dim[i].m_start);
                dim.m_start = EXPR(asr);
            } else {
                dim.m_start = nullptr;
            }
            if (m_dim[i].m_end) {
                this->visit_expr(*m_dim[i].m_end);
                dim.m_end = EXPR(asr);
            } else {
                dim.m_end = nullptr;
            }
            dims.push_back(al, dim);
        }
    }

    void visit_Declaration(const AST::Declaration_t &x) {
        if (x.m_vartype == nullptr &&
                x.n_attributes == 1 &&
                AST::is_a<AST::AttrNamelist_t>(*x.m_attributes[0])) {
            //char *name = down_cast<AttrNamelist_t>(x.m_attributes[0])->m_name;
            throw SemanticError("Namelists not implemented yet", x.base.base.loc);
        }
        for (size_t i=0; i<x.n_attributes; i++) {
            if (AST::is_a<AST::AttrType_t>(*x.m_attributes[i])) {
                throw SemanticError("Type must be declared first",
                    x.base.base.loc);
            };
        }
        if (x.m_vartype == nullptr) {
            // Examples:
            // private
            // public
            // private :: x, y, z
            if (x.n_attributes == 0) {
                throw SemanticError("No attribute specified",
                    x.base.base.loc);
            }
            if (x.n_attributes > 1) {
                throw SemanticError("Only one attribute can be specified if type is missing",
                    x.base.base.loc);
            }
            LFORTRAN_ASSERT(x.n_attributes == 1);
            if (AST::is_a<AST::SimpleAttribute_t>(*x.m_attributes[0])) {
                AST::SimpleAttribute_t *sa =
                    AST::down_cast<AST::SimpleAttribute_t>(x.m_attributes[0]);
                if (x.n_syms == 0) {
                    // Example:
                    // private
                    if (sa->m_attr == AST::simple_attributeType
                            ::AttrPrivate) {
                        dflt_access = ASR::accessType::Private;
                    } else if (sa->m_attr == AST::simple_attributeType
                            ::AttrPublic) {
                        // Do nothing (public access is the default)
                        LFORTRAN_ASSERT(dflt_access == ASR::accessType::Public);
                    } else if (sa->m_attr == AST::simple_attributeType
                            ::AttrSave) {
                        if (in_module) {
                            // Do nothing (all variables implicitly have the
                            // save attribute in a module/main program)
                        } else {
                            throw SemanticError("Save Attribute not "
                                    "supported yet", x.base.base.loc);
                        }
                    } else {
                        throw SemanticError("Attribute declaration not "
                                "supported yet", x.base.base.loc);
                    }
                } else {
                    // Example:
                    // private :: x, y, z
                    for (size_t i=0; i<x.n_syms; i++) {
                        AST::var_sym_t &s = x.m_syms[i];
                        std::string sym = s.m_name;
                        if (sa->m_attr == AST::simple_attributeType
                                ::AttrPrivate) {
                            assgnd_access[sym] = ASR::accessType::Private;
                        } else if (sa->m_attr == AST::simple_attributeType
                                ::AttrPublic) {
                            assgnd_access[sym] = ASR::accessType::Public;
                        } else {
                            throw SemanticError("Attribute declaration not "
                                    "supported", x.base.base.loc);
                        }
                    }
                }
            } else {
                throw SemanticError("Attribute declaration not supported",
                    x.base.base.loc);
            }
        } else {
            // Example
            // real(dp), private :: x, y(3), z
            for (size_t i=0; i<x.n_syms; i++) {
                AST::var_sym_t &s = x.m_syms[i];
                std::string sym = s.m_name;
                ASR::accessType s_access = dflt_access;
                AST::AttrType_t *sym_type =
                    AST::down_cast<AST::AttrType_t>(x.m_vartype);
                if (assgnd_access.count(sym)) {
                    s_access = assgnd_access[sym];
                }
                ASR::storage_typeType storage_type =
                        ASR::storage_typeType::Default;
                bool is_pointer = false;
                if (current_scope->scope.find(sym) !=
                        current_scope->scope.end()) {
                    if (current_scope->parent != nullptr) {
                        // re-declaring a global scope variable is allowed
                        // Otherwise raise an error
                        throw SemanticError("Symbol already declared",
                                x.base.base.loc);
                    }
                }
                ASR::intentType s_intent;
                if (std::find(current_procedure_args.begin(), 
                        current_procedure_args.end(), s.m_name) != 
                        current_procedure_args.end()) {
                    s_intent = intent_unspecified;
                } else {
                    s_intent = intent_local;
                }
                Vec<ASR::dimension_t> dims;
                dims.reserve(al, 0);
                if (x.n_attributes > 0) {
                    for (size_t i=0; i < x.n_attributes; i++) {
                        AST::decl_attribute_t *a = x.m_attributes[i];
                        if (AST::is_a<AST::SimpleAttribute_t>(*a)) {
                            AST::SimpleAttribute_t *sa =
                                AST::down_cast<AST::SimpleAttribute_t>(a);
                            if (sa->m_attr == AST::simple_attributeType
                                    ::AttrPrivate) {
                                s_access = ASR::accessType::Private;
                            } else if (sa->m_attr == AST::simple_attributeType
                                    ::AttrPublic) {
                                s_access = ASR::accessType::Public;
                            } else if (sa->m_attr == AST::simple_attributeType
                                    ::AttrParameter) {
                                storage_type = ASR::storage_typeType::Parameter;
                            } else if (sa->m_attr == AST::simple_attributeType
                                    ::AttrPointer) {
                                is_pointer = true;
                            } else if (sa->m_attr == AST::simple_attributeType
                                    ::AttrTarget) {
                                // Do nothing for now
                            } else if (sa->m_attr == AST::simple_attributeType
                                    ::AttrAllocatable) {
                                // TODO
                            } else {
                                throw SemanticError("Attribute type not implemented yet",
                                        x.base.base.loc);
                            }
                        } else if (AST::is_a<AST::AttrIntent_t>(*a)) {
                            AST::AttrIntent_t *ai =
                                AST::down_cast<AST::AttrIntent_t>(a);
                            switch (ai->m_intent) {
                                case (AST::attr_intentType::In) : {
                                    s_intent = intent_in;
                                    break;
                                }
                                case (AST::attr_intentType::Out) : {
                                    s_intent = intent_out;
                                    break;
                                }
                                case (AST::attr_intentType::InOut) : {
                                    s_intent = intent_inout;
                                    break;
                                }
                                default : {
                                    s_intent = intent_unspecified;
                                    break;
                                }
                            }
                        } else if (AST::is_a<AST::AttrDimension_t>(*a)) {
                            AST::AttrDimension_t *ad =
                                AST::down_cast<AST::AttrDimension_t>(a);
                            if (dims.size() > 0) {
                                throw SemanticError("Dimensions specified twice",
                                        x.base.base.loc);
                            }
                            process_dims(al, dims, ad->m_dim, ad->n_dim);
                        } else {
                            throw SemanticError("Attribute type not implemented yet",
                                    x.base.base.loc);
                        }
                    }
                }
                if (s.n_dim > 0) {
                    if (dims.size() > 0) {
                        throw SemanticError("Cannot specify dimensions both ways",
                                x.base.base.loc);
                    }
                    process_dims(al, dims, s.m_dim, s.n_dim);
                }
                ASR::ttype_t *type;
                int a_kind = 4;
                if (sym_type->m_kind != nullptr) {
                    visit_expr(*sym_type->m_kind->m_value);
                    ASR::expr_t* kind_expr = EXPR(asr);
                    a_kind = HelperMethods::extract_kind(kind_expr, x.base.base.loc);
                }
                if (sym_type->m_type == AST::decl_typeType::TypeReal) {
                    if (is_pointer) {
                        type = TYPE(ASR::make_RealPointer_t(al, x.base.base.loc,
                            a_kind, dims.p, dims.size()));
                    } else {
                        type = TYPE(ASR::make_Real_t(al, x.base.base.loc,
                            a_kind, dims.p, dims.size()));
                    }
                } else if (sym_type->m_type == AST::decl_typeType::TypeInteger) {
                    if (is_pointer) {
                        type = TYPE(ASR::make_IntegerPointer_t(al, x.base.base.loc, a_kind, dims.p, dims.size()));
                    } else {
                        type = TYPE(ASR::make_Integer_t(al, x.base.base.loc, a_kind, dims.p, dims.size()));
                    }
                } else if (sym_type->m_type == AST::decl_typeType::TypeLogical) {
                    type = TYPE(ASR::make_Logical_t(al, x.base.base.loc, 4,
                        dims.p, dims.size()));
                } else if (sym_type->m_type == AST::decl_typeType::TypeComplex) {
                    if( is_pointer ) {
                        type = TYPE(ASR::make_ComplexPointer_t(al, x.base.base.loc, a_kind,
                                    dims.p, dims.size()));
                    } else {
                        type = TYPE(ASR::make_Complex_t(al, x.base.base.loc, a_kind,
                                    dims.p, dims.size()));
                    }
                } else if (sym_type->m_type == AST::decl_typeType::TypeCharacter) {
                    type = TYPE(ASR::make_Character_t(al, x.base.base.loc, 4,
                        dims.p, dims.size()));
                } else if (sym_type->m_type == AST::decl_typeType::TypeType) {
                    LFORTRAN_ASSERT(sym_type->m_name);
                    std::string derived_type_name = sym_type->m_name;
                    ASR::symbol_t *v = current_scope->resolve_symbol(derived_type_name);
                    if (!v) {
                        throw SemanticError("Derived type '"
                            + derived_type_name + "' not declared", x.base.base.loc);
                    }
                    type = TYPE(ASR::make_Derived_t(al, x.base.base.loc, v,
                        dims.p, dims.size()));
                } else {
                    throw SemanticError("Type not implemented yet.",
                         x.base.base.loc);
                }
                ASR::expr_t* init_expr = nullptr;
                if (s.m_initializer != nullptr) {
                    this->visit_expr(*s.m_initializer);
                    init_expr = EXPR(asr);
                    ASR::ttype_t *init_type = expr_type(init_expr);
                    ImplicitCastRules::set_converted_value(al, x.base.base.loc, &init_expr, init_type, type);
                }
                ASR::asr_t *v = ASR::make_Variable_t(al, x.base.base.loc, current_scope,
                        s.m_name, s_intent, init_expr, storage_type, type,
                        ASR::abiType::Source, s_access);
                current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(v);
            } // for m_syms
        }
    }

    Vec<ASR::expr_t*> visit_expr_list(AST::fnarg_t *ast_list, size_t n) {
        Vec<ASR::expr_t*> asr_list;
        asr_list.reserve(al, n);
        for (size_t i=0; i<n; i++) {
            visit_expr(*ast_list[i].m_end);
            ASR::expr_t *expr = EXPR(asr);
            asr_list.push_back(al, expr);
        }
        return asr_list;
    }

    void visit_FuncCallOrArray(const AST::FuncCallOrArray_t &x) {
        std::string var_name = x.m_func;
        ASR::symbol_t *v = current_scope->resolve_symbol(var_name);
        if (!v) {
            std::string remote_sym = to_lower(var_name);
            if (intrinsic_procedures.find(remote_sym)
                        != intrinsic_procedures.end()) {
                std::string module_name = intrinsic_procedures[remote_sym];
                ASR::Module_t *m = load_module(al, current_scope->parent,
                    module_name, x.base.base.loc, true);

                ASR::symbol_t *t = m->m_symtab->resolve_symbol(remote_sym);
                if (!t) {
                    throw SemanticError("The symbol '" + remote_sym
                        + "' not found in the module '" + module_name + "'",
                        x.base.base.loc);
                }

                ASR::Function_t *mfn = ASR::down_cast<ASR::Function_t>(t);
                ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                    al, mfn->base.base.loc,
                    /* a_symtab */ current_scope,
                    /* a_name */ mfn->m_name,
                    (ASR::symbol_t*)mfn,
                    m->m_name, mfn->m_name,
                    ASR::accessType::Private
                    );
                std::string sym = mfn->m_name;
                current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(fn);
                v = ASR::down_cast<ASR::symbol_t>(fn);
                // Add the module `m` to current module dependencies
                if (!present(current_module_dependencies, m->m_name)) {
                    current_module_dependencies.push_back(al, m->m_name);
                }
            } else {
                throw SemanticError("Function '" + var_name + "' not found"
                    " or not implemented yet (if it is intrinsic)",
                    x.base.base.loc);
            }
        }
        Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
        ASR::ttype_t *type = EXPR2VAR(ASR::down_cast<ASR::Function_t>(
                symbol_get_past_external(v))
                ->m_return_var)->m_type;
        asr = ASR::make_FunctionCall_t(al, x.base.base.loc, v, nullptr,
            args.p, args.size(), nullptr, 0, type);
    }

    void visit_DerivedType(const AST::DerivedType_t &x) {
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        for (size_t i=0; i<x.n_items; i++) {
            this->visit_unit_decl2(*x.m_items[i]);
        }
        std::string sym_name = x.m_name;
        if (current_scope->scope.find(sym_name) != current_scope->scope.end()) {
            throw SemanticError("DerivedType already defined", x.base.base.loc);
        }
        asr = ASR::make_DerivedType_t(al, x.base.base.loc, current_scope,
            x.m_name, ASR::abiType::Source, dflt_access);
        parent_scope->scope[sym_name] = ASR::down_cast<ASR::symbol_t>(asr);

        current_scope = parent_scope;
    }

    void visit_InterfaceProc(const AST::InterfaceProc_t &x) {
        is_interface = true;
        visit_program_unit(*x.m_proc);
        is_interface = false;
        return;
    }

    void visit_Interface(const AST::Interface_t &x) {
        if (AST::is_a<AST::InterfaceHeader2_t>(*x.m_header)) {
            char *generic_name = AST::down_cast<AST::InterfaceHeader2_t>(x.m_header)->m_name;
            std::vector<std::string> proc_names;
            for (size_t i = 0; i < x.n_items; i++) {
                AST::interface_item_t *item = x.m_items[i];
                if (AST::is_a<AST::InterfaceModuleProcedure_t>(*item)) {
                    AST::InterfaceModuleProcedure_t *proc
                        = AST::down_cast<AST::InterfaceModuleProcedure_t>(item);
                    for (size_t i = 0; i < proc->n_names; i++) {
                        char *proc_name = proc->m_names[i];
                        proc_names.push_back(std::string(proc_name));
                    }
                } else {
                    throw SemanticError("Interface procedure type not imlemented yet", item->base.loc);
                }
            }
            generic_procedures[std::string(generic_name)] = proc_names;
        } else if (AST::is_a<AST::InterfaceHeader1_t>(*x.m_header)) {
            std::vector<std::string> proc_names;
            for (size_t i = 0; i < x.n_items; i++) {
                visit_interface_item(*x.m_items[i]);
            }
        } else {
            throw SemanticError("Interface type not imlemented yet", x.base.base.loc);
        }
    }

    void add_generic_procedures() {
        for (auto &proc : generic_procedures) {
            Location loc;
            loc.first_line = 1;
            loc.last_line = 1;
            loc.first_column = 1;
            loc.last_column = 1;
            Str s;
            s.from_str_view(proc.first);
            char *generic_name = s.c_str(al);
            Vec<ASR::symbol_t*> symbols;
            symbols.reserve(al, proc.second.size());
            for (auto &pname : proc.second) {
                ASR::symbol_t *x;
                Str s;
                s.from_str_view(pname);
                char *name = s.c_str(al);
                x = resolve_symbol(loc, name);
                symbols.push_back(al, x);
            }
            ASR::asr_t *v = ASR::make_GenericProcedure_t(al, loc,
                current_scope,
                generic_name, symbols.p, symbols.size(), ASR::Public);
            current_scope->scope[proc.first] = ASR::down_cast<ASR::symbol_t>(v);
        }
    }

    void visit_Use(const AST::Use_t &x) {
        std::string msym = x.m_module;
        if (!present(current_module_dependencies, x.m_module)) {
            current_module_dependencies.push_back(al, x.m_module);
        }
        ASR::symbol_t *t = current_scope->parent->resolve_symbol(msym);
        if (!t) {
            t = (ASR::symbol_t*)load_module(al, current_scope->parent,
                msym, x.base.base.loc, false);
        }
        if (!ASR::is_a<ASR::Module_t>(*t)) {
            throw SemanticError("The symbol '" + msym + "' must be a module",
                x.base.base.loc);
        }
        ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(t);
        if (x.n_symbols == 0) {
            // Import all symbols from the module, e.g.:
            //     use a
            for (auto &item : m->m_symtab->scope) {
                // TODO: only import "public" symbols from the module
                if (ASR::is_a<ASR::Subroutine_t>(*item.second)) {
                    ASR::Subroutine_t *msub = ASR::down_cast<ASR::Subroutine_t>(item.second);
                    ASR::asr_t *sub = ASR::make_ExternalSymbol_t(
                        al, msub->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ msub->m_name,
                        (ASR::symbol_t*)msub,
                        m->m_name, msub->m_name,
                        dflt_access
                        );
                    std::string sym = msub->m_name;
                    current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(sub);
                } else if (ASR::is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *mfn = ASR::down_cast<ASR::Function_t>(item.second);
                    ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                        al, mfn->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ mfn->m_name,
                        (ASR::symbol_t*)mfn,
                        m->m_name, mfn->m_name,
                        dflt_access
                        );
                    std::string sym = mfn->m_name;
                    current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(fn);
                } else if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                    ASR::Variable_t *mvar = ASR::down_cast<ASR::Variable_t>(item.second);
                    ASR::asr_t *var = ASR::make_ExternalSymbol_t(
                        al, mvar->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ mvar->m_name,
                        (ASR::symbol_t*)mvar,
                        m->m_name, mvar->m_name,
                        dflt_access
                        );
                    std::string sym = mvar->m_name;
                    current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(var);
                } else {
                    throw LFortranException("'" + item.first + "' is not supported yet for declaring with use.");
                }
            }
        } else {
            // Only import individual symbols from the module, e.g.:
            //     use a, only: x, y, z
            for (size_t i = 0; i < x.n_symbols; i++) {
                std::string remote_sym = AST::down_cast<AST::UseSymbol_t>(x.m_symbols[i])->m_sym;
                std::string local_sym;
                if (AST::down_cast<AST::UseSymbol_t>(x.m_symbols[i])->m_rename) {
                    local_sym = AST::down_cast<AST::UseSymbol_t>(x.m_symbols[i])->m_rename;
                } else {
                    local_sym = remote_sym;
                }
                ASR::symbol_t *t = m->m_symtab->resolve_symbol(remote_sym);
                if (!t) {
                    throw SemanticError("The symbol '" + remote_sym + "' not found in the module '" + msym + "'",
                        x.base.base.loc);
                }
                if (ASR::is_a<ASR::Subroutine_t>(*t)) {
                    if (current_scope->scope.find(local_sym) != current_scope->scope.end()) {
                        throw SemanticError("Subroutine already defined",
                            x.base.base.loc);
                    }
                    ASR::Subroutine_t *msub = ASR::down_cast<ASR::Subroutine_t>(t);
                    // `msub` is the Subroutine in a module. Now we construct
                    // an ExternalSymbol that points to
                    // `msub` via the `external` field.
                    Str name;
                    name.from_str(al, local_sym);
                    ASR::asr_t *sub = ASR::make_ExternalSymbol_t(
                        al, msub->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ name.c_str(al),
                        (ASR::symbol_t*)msub,
                        m->m_name, msub->m_name,
                        dflt_access
                        );
                    current_scope->scope[local_sym] = ASR::down_cast<ASR::symbol_t>(sub);
                } else if (ASR::is_a<ASR::GenericProcedure_t>(*t)) {
                    if (current_scope->scope.find(local_sym) != current_scope->scope.end()) {
                        throw SemanticError("Symbol already defined",
                            x.base.base.loc);
                    }
                    ASR::GenericProcedure_t *gp = ASR::down_cast<ASR::GenericProcedure_t>(t);
                    Str name;
                    name.from_str(al, local_sym);
                    char *cname = name.c_str(al);
                    ASR::asr_t *ep = ASR::make_ExternalSymbol_t(
                        al, t->base.loc,
                        current_scope,
                        /* a_name */ cname,
                        t,
                        m->m_name, gp->m_name,
                        dflt_access
                        );
                    current_scope->scope[local_sym] = ASR::down_cast<ASR::symbol_t>(ep);
                } else if (ASR::is_a<ASR::Function_t>(*t)) {
                    if (current_scope->scope.find(local_sym) != current_scope->scope.end()) {
                        throw SemanticError("Function already defined",
                            x.base.base.loc);
                    }
                    ASR::Function_t *mfn = ASR::down_cast<ASR::Function_t>(t);
                    // `mfn` is the Function in a module. Now we construct
                    // an ExternalSymbol that points to it.
                    Str name;
                    name.from_str(al, local_sym);
                    char *cname = name.c_str(al);
                    ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                        al, mfn->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ cname,
                        (ASR::symbol_t*)mfn,
                        m->m_name, mfn->m_name,
                        dflt_access
                        );
                    current_scope->scope[local_sym] = ASR::down_cast<ASR::symbol_t>(fn);
                } else if (ASR::is_a<ASR::Variable_t>(*t)) {
                    if (current_scope->scope.find(local_sym) != current_scope->scope.end()) {
                        throw SemanticError("Variable already defined",
                            x.base.base.loc);
                    }
                    ASR::Variable_t *mv = ASR::down_cast<ASR::Variable_t>(t);
                    // `mv` is the Variable in a module. Now we construct
                    // an ExternalSymbol that points to it.
                    Str name;
                    name.from_str(al, local_sym);
                    char *cname = name.c_str(al);
                    ASR::asr_t *v = ASR::make_ExternalSymbol_t(
                        al, mv->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ cname,
                        (ASR::symbol_t*)mv,
                        m->m_name, mv->m_name,
                        dflt_access
                        );
                    current_scope->scope[local_sym] = ASR::down_cast<ASR::symbol_t>(v);
                } else if( ASR::is_a<ASR::DerivedType_t>(*t) ) {
                    if (current_scope->scope.find(local_sym) != current_scope->scope.end()) {
                        throw SemanticError("Derived type already defined",
                            x.base.base.loc);
                    }
                    ASR::DerivedType_t *mv = ASR::down_cast<ASR::DerivedType_t>(t);
                    // `mv` is the Variable in a module. Now we construct
                    // an ExternalSymbol that points to it.
                    Str name;
                    name.from_str(al, local_sym);
                    char *cname = name.c_str(al);
                    ASR::asr_t *v = ASR::make_ExternalSymbol_t(
                        al, mv->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ cname,
                        (ASR::symbol_t*)mv,
                        m->m_name, mv->m_name,
                        dflt_access
                        );
                    current_scope->scope[local_sym] = ASR::down_cast<ASR::symbol_t>(v);
                } else {
                    throw LFortranException("Only Subroutines, Functions, Variables and Derived supported in 'use'");
                }
            }
        }
    }

    void visit_Real(const AST::Real_t &x) {
        int a_kind = HelperMethods::extract_kind(x.m_n);
        ASR::ttype_t *type = TYPE(ASR::make_Real_t(al, x.base.base.loc,
                a_kind, nullptr, 0));
        asr = ASR::make_ConstantReal_t(al, x.base.base.loc, x.m_n, type);
    }

    ASR::asr_t* resolve_variable(const Location &loc, const char* id) {
        SymbolTable *scope = current_scope;
        std::string var_name = id;
        ASR::symbol_t *v = scope->resolve_symbol(var_name);
        if (!v) {
            throw SemanticError("Variable '" + var_name + "' not declared", loc);
        }
        return ASR::make_Var_t(al, loc, v);
    }

    void visit_Name(const AST::Name_t &x) {
        asr = resolve_variable(x.base.base.loc, x.m_id);
    }

    void visit_Num(const AST::Num_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                4, nullptr, 0));
        asr = ASR::make_ConstantInteger_t(al, x.base.base.loc, x.m_n, type);
    }

};

class BodyVisitor : public AST::BaseVisitor<BodyVisitor>
{
public:
    Allocator &al;
    ASR::asr_t *asr, *tmp;
    SymbolTable *current_scope;
    ASR::Module_t *current_module=nullptr;
    BodyVisitor(Allocator &al, ASR::asr_t *unit) : al{al}, asr{unit} {}

    void visit_TranslationUnit(const AST::TranslationUnit_t &x) {
        ASR::TranslationUnit_t *unit = ASR::down_cast2<ASR::TranslationUnit_t>(asr);
        current_scope = unit->m_global_scope;
        Vec<ASR::asr_t*> items;
        items.reserve(al, x.n_items);
        for (size_t i=0; i<x.n_items; i++) {
            tmp = nullptr;
            visit_ast(*x.m_items[i]);
            if (tmp) {
                items.push_back(al, tmp);
            }
        }
        unit->m_items = items.p;
        unit->n_items = items.size();
    }

    void visit_Declaration(const AST::Declaration_t & /* x */) {
        // This AST node was already visited in SymbolTableVisitor
    }

    void visit_Open(const AST::Open_t& x) {
        ASR::expr_t *a_newunit = nullptr, *a_filename = nullptr, *a_status = nullptr;
        if( x.n_args > 1 ) {
            throw SemanticError("Number of arguments cannot be more than 1 in Open statement.",
                                x.base.base.loc);
        }
        if( x.n_args == 1 ) {
            this->visit_expr(*x.m_args[0]);
            a_newunit = EXPR(tmp);
        }
        for( std::uint32_t i = 0; i < x.n_kwargs; i++ ) {
            AST::keyword_t kwarg = x.m_kwargs[i];
            std::string m_arg_str(kwarg.m_arg);
            if( m_arg_str == std::string("newunit") || 
                m_arg_str == std::string("unit") ) {
                if( a_newunit != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `unit` found, `unit` has already been specified via argument or keyword arguments)""",
                                        x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_newunit = EXPR(tmp);
                ASR::ttype_t* a_newunit_type = expr_type(a_newunit);
                if( ( m_arg_str == std::string("newunit") && 
                      a_newunit->type != ASR::exprType::Var ) || 
                    ( a_newunit_type->type != ASR::ttypeType::Integer && 
                    a_newunit_type->type != ASR::ttypeType::IntegerPointer ) ) {
                        throw SemanticError("`newunit`/`unit` must be a variable of type, Integer or IntegerPointer", x.base.base.loc);
                }
            } else if( m_arg_str == std::string("file") ) {
                if( a_filename != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `file` found, unit has already been specified via arguments or keyword arguments)""",
                                        x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_filename = EXPR(tmp);
                ASR::ttype_t* a_filename_type = expr_type(a_filename);
                if( a_filename_type->type != ASR::ttypeType::Character && 
                    a_filename_type->type != ASR::ttypeType::CharacterPointer ) {
                        throw SemanticError("`file` must be of type, Character or CharacterPointer", x.base.base.loc);
                }
            } else if( m_arg_str == std::string("status") ) {
                if( a_status != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `status` found, unit has already been specified via arguments or keyword arguments)""",
                                        x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_status = EXPR(tmp);
                ASR::ttype_t* a_status_type = expr_type(a_status);
                if( a_status_type->type != ASR::ttypeType::Character && 
                    a_status_type->type != ASR::ttypeType::CharacterPointer ) {
                        throw SemanticError("`status` must be of type, Character or CharacterPointer", x.base.base.loc);
                }
            }
        }
        if( a_newunit == nullptr ) {
            throw SemanticError("`newunit` or `unit` must be specified either in argument or keyword arguments.",
                                x.base.base.loc);
        }
        tmp = ASR::make_Open_t(al, x.base.base.loc, x.m_label, 
                               a_newunit, a_filename, a_status);
    }

    void visit_Close(const AST::Close_t& x) {
        ASR::expr_t *a_unit = nullptr, *a_iostat = nullptr, *a_iomsg = nullptr; 
        ASR::expr_t *a_err = nullptr, *a_status = nullptr;
        if( x.n_args > 1 ) {
            throw SemanticError("Number of arguments cannot be more than 1 in Close statement.",
                        x.base.base.loc);
        }
        if( x.n_args == 1 ) {
            this->visit_expr(*x.m_args[0]);
            a_unit = EXPR(tmp);
        }
        for( std::uint32_t i = 0; i < x.n_kwargs; i++ ) {
            AST::keyword_t kwarg = x.m_kwargs[i];
            std::string m_arg_str(kwarg.m_arg);
            if( m_arg_str == std::string("unit") ) {
                if( a_unit != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `unit` found, `unit` has already been specified via argument or keyword arguments)""",
                                        x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_unit = EXPR(tmp);
                ASR::ttype_t* a_newunit_type = expr_type(a_unit);
                if( a_newunit_type->type != ASR::ttypeType::Integer && 
                    a_newunit_type->type != ASR::ttypeType::IntegerPointer ) {
                        throw SemanticError("`unit` must be of type, Integer or IntegerPointer", x.base.base.loc);
                }
            } else if( m_arg_str == std::string("iostat") ) {
                if( a_iostat != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `iostat` found, unit has already been specified via arguments or keyword arguments)""",
                                        x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_iostat = EXPR(tmp);
                ASR::ttype_t* a_iostat_type = expr_type(a_iostat);
                if( a_iostat->type != ASR::exprType::Var || 
                    ( a_iostat_type->type != ASR::ttypeType::Integer && 
                      a_iostat_type->type != ASR::ttypeType::IntegerPointer ) ) {
                        throw SemanticError("`iostat` must be a variable of type, Integer or IntegerPointer", x.base.base.loc);
                }
            } else if( m_arg_str == std::string("iomsg") ) {
                if( a_iomsg != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `iomsg` found, unit has already been specified via arguments or keyword arguments)""",
                                        x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_iomsg = EXPR(tmp);
                ASR::ttype_t* a_iomsg_type = expr_type(a_iomsg);
                if( a_iomsg->type != ASR::exprType::Var || 
                   ( a_iomsg_type->type != ASR::ttypeType::Character && 
                    a_iomsg_type->type != ASR::ttypeType::CharacterPointer ) ) {
                        throw SemanticError("`iomsg` must be of type, Character or CharacterPointer", x.base.base.loc);
                    }
            } else if( m_arg_str == std::string("status") ) {
                if( a_status != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `status` found, unit has already been specified via arguments or keyword arguments)""",
                                        x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_status = EXPR(tmp);
                ASR::ttype_t* a_status_type = expr_type(a_status);
                if( a_status_type->type != ASR::ttypeType::Character && 
                    a_status_type->type != ASR::ttypeType::CharacterPointer ) {
                        throw SemanticError("`status` must be of type, Character or CharacterPointer", x.base.base.loc);
                }
            } else if( m_arg_str == std::string("err") ) {
                if( a_err != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `err` found, `err` has already been specified via arguments or keyword arguments)""",
                                        x.base.base.loc);
                }
                if( kwarg.m_value->type != AST::exprType::Num ) {
                    throw SemanticError("`err` must be a literal integer", x.base.base.loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_err = EXPR(tmp);
            }
        }
        if( a_unit == nullptr ) {
            throw SemanticError("`newunit` or `unit` must be specified either in argument or keyword arguments.",
                                x.base.base.loc);
        }
        tmp = ASR::make_Close_t(al, x.base.base.loc, x.m_label, a_unit, a_iostat, a_iomsg, a_err, a_status);
    }

    void create_read_write_ASR_node(const AST::stmt_t& read_write_stmt, AST::stmtType _type) {
        int64_t m_label = -1;
        AST::argstar_t* m_args = nullptr; size_t n_args = 0;
        AST::kw_argstar_t* m_kwargs = nullptr; size_t n_kwargs = 0; 
        AST::expr_t** m_values = nullptr; size_t n_values = 0;
        const Location& loc = read_write_stmt.base.loc;
        if( _type == AST::stmtType::Write ) {
            AST::Write_t* w = (AST::Write_t*)(&read_write_stmt);
            m_label = w->m_label;
            m_args = w->m_args; n_args = w->n_args;
            m_kwargs = w->m_kwargs; n_kwargs = w->n_kwargs;
            m_values = w->m_values; n_values = w->n_values;
        } else if( _type == AST::stmtType::Read ) {
            AST::Read_t* r = (AST::Read_t*)(&read_write_stmt);
            m_label = r->m_label;
            m_args = r->m_args; n_args = r->n_args;
            m_kwargs = r->m_kwargs; n_kwargs = r->n_kwargs;
            m_values = r->m_values; n_values = r->n_values;
        }

        ASR::expr_t *a_unit, *a_fmt, *a_iomsg, *a_iostat, *a_id;
        a_unit = a_fmt = a_iomsg = a_iostat = a_id = nullptr;
        Vec<ASR::expr_t*> a_values_vec;
        a_values_vec.reserve(al, n_values); 

        if( n_args > 2 ) {
            throw SemanticError("Number of arguments cannot be more than 2 in Read/Write statement.",
                                loc);
        }
        std::vector<ASR::expr_t**> args = {&a_unit, &a_fmt};
        for( std::uint32_t i = 0; i < n_args; i++ ) {
            if( m_args[i].m_value != nullptr ) {
                this->visit_expr(*m_args[i].m_value);
                *args[i] = EXPR(tmp);
            }
        }
        for( std::uint32_t i = 0; i < n_kwargs; i++ ) {
            AST::kw_argstar_t kwarg = m_kwargs[i];
            std::string m_arg_str(kwarg.m_arg);
            if( m_arg_str == std::string("unit") ) {
                if( a_unit != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `unit` found, `unit` has already been specified via argument or keyword arguments)""",
                                        loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_unit = EXPR(tmp);
                ASR::ttype_t* a_unit_type = expr_type(a_unit);
                if( a_unit_type->type != ASR::ttypeType::Integer && 
                    a_unit_type->type != ASR::ttypeType::IntegerPointer ) {
                        throw SemanticError("`unit` must be of type, Integer or IntegerPointer", loc);
                }
            } else if( m_arg_str == std::string("iostat") ) {
                if( a_iostat != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `iostat` found, unit has already been specified via arguments or keyword arguments)""",
                                        loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_iostat = EXPR(tmp);
                ASR::ttype_t* a_iostat_type = expr_type(a_iostat);
                if( a_iostat->type != ASR::exprType::Var || 
                    ( a_iostat_type->type != ASR::ttypeType::Integer && 
                      a_iostat_type->type != ASR::ttypeType::IntegerPointer ) ) {
                        throw SemanticError("`iostat` must be of type, Integer or IntegerPointer", loc);
                }
            } else if( m_arg_str == std::string("iomsg") ) {
                if( a_iomsg != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `iomsg` found, unit has already been specified via arguments or keyword arguments)""",
                                        loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_iomsg = EXPR(tmp);
                ASR::ttype_t* a_iomsg_type = expr_type(a_iomsg);
                if( a_iomsg->type != ASR::exprType::Var || 
                   ( a_iomsg_type->type != ASR::ttypeType::Character && 
                     a_iomsg_type->type != ASR::ttypeType::CharacterPointer ) ) {
                        throw SemanticError("`iomsg` must be of type, Character or CharacterPointer", loc);
                    }
            } else if( m_arg_str == std::string("id") ) {
                if( a_id != nullptr ) {
                    throw SemanticError(R"""(Duplicate value of `id` found, unit has already been specified via arguments or keyword arguments)""",
                                        loc);
                }
                this->visit_expr(*kwarg.m_value);
                a_id = EXPR(tmp);
                ASR::ttype_t* a_status_type = expr_type(a_id);
                if( a_status_type->type != ASR::ttypeType::Character && 
                    a_status_type->type != ASR::ttypeType::CharacterPointer ) {
                        throw SemanticError("`status` must be of type, Character or CharacterPointer", loc);
                }
            }
        }
        if( a_unit == nullptr && n_args < 1 ) {
            throw SemanticError("`unit` must be specified either in arguments or keyword arguments.",
                                loc);
        }
        if( a_fmt == nullptr && n_args < 2 ) {
            throw SemanticError("`fmt` must be specified either in arguments or keyword arguments.",
                                loc);
        }
        
        for( std::uint32_t i = 0; i < n_values; i++ ) {
            this->visit_expr(*m_values[i]);
            a_values_vec.push_back(al, EXPR(tmp));
        }
        if( _type == AST::stmtType::Write ) {
            tmp = ASR::make_Write_t(al, loc, m_label, a_unit, a_fmt, 
                                    a_iomsg, a_iostat, a_id, a_values_vec.p, n_values);
        } else if( _type == AST::stmtType::Read ) {
            tmp = ASR::make_Read_t(al, loc, m_label, a_unit, a_fmt, 
                                   a_iomsg, a_iostat, a_id, a_values_vec.p, n_values);
        }
    }

    void visit_Write(const AST::Write_t& x) {
        create_read_write_ASR_node(x.base, x.class_type);
    }

    void visit_Read(const AST::Read_t& x) {
        create_read_write_ASR_node(x.base, x.class_type);
    }

    void visit_Associate(const AST::Associate_t& x) {
        this->visit_expr(*(x.m_target));
        ASR::expr_t* target = EXPR(tmp);
        this->visit_expr(*(x.m_value));
        ASR::expr_t* value = EXPR(tmp);
        ASR::ttype_t* target_type = expr_type(target);
        ASR::ttype_t* value_type = expr_type(value);
        bool is_target_pointer = HelperMethods::is_pointer(target_type);
        bool is_value_pointer = HelperMethods::is_pointer(value_type);
        if( !(is_target_pointer && !is_value_pointer) ) {
            throw SemanticError("Only a pointer variable can be associated with a non-pointer variable.", x.base.base.loc);
        }
        if( HelperMethods::is_same_type_pointer(target_type, value_type) ) {
            tmp = ASR::make_Associate_t(al, x.base.base.loc, target, value);
        }
    } 

    void visit_Allocate(const AST::Allocate_t& x) {
        // TODO
        tmp = ASR::make_Allocate_t(al, x.base.base.loc);
    }

    void visit_Deallocate(const AST::Deallocate_t& x) {
        // TODO
        tmp = ASR::make_Deallocate_t(al, x.base.base.loc);
    }

    void visit_Return(const AST::Return_t& x) {
        // TODO
        tmp = ASR::make_Return_t(al, x.base.base.loc);
    }

    void visit_case_stmt(const AST::case_stmt_t& x) {
        switch(x.type) {
            case AST::case_stmtType::CaseStmt: {
                AST::CaseStmt_t* Case_Stmt = (AST::CaseStmt_t*)(&(x.base));
                Vec<ASR::expr_t*> a_test_vec;
                a_test_vec.reserve(al, Case_Stmt->n_test);
                for( std::uint32_t i = 0; i < Case_Stmt->n_test; i++ ) {
                    this->visit_expr(*(Case_Stmt->m_test[i]));
                    ASR::expr_t* m_test_i = EXPR(tmp);
                    if( expr_type(m_test_i)->type != ASR::ttypeType::Integer ) {
                        throw SemanticError(R"""(Expression in Case selector can only be an Integer)""", 
                                            x.base.loc);
                    }
                    a_test_vec.push_back(al, EXPR(tmp));
                }
                Vec<ASR::stmt_t*> case_body_vec;
                case_body_vec.reserve(al, Case_Stmt->n_body);
                for( std::uint32_t i = 0; i < Case_Stmt->n_body; i++ ) {
                    this->visit_stmt(*(Case_Stmt->m_body[i]));
                    if (tmp != nullptr) {
                        case_body_vec.push_back(al, STMT(tmp));
                    }
                }
                tmp = ASR::make_CaseStmt_t(al, x.base.loc, a_test_vec.p, a_test_vec.size(), 
                                     case_body_vec.p, case_body_vec.size());
                break;
            } 
            case AST::case_stmtType::CaseStmt_Range : {
                AST::CaseStmt_Range_t* Case_Stmt = (AST::CaseStmt_Range_t*)(&(x.base));
                ASR::expr_t *m_start = nullptr, *m_end = nullptr;
                if( Case_Stmt->m_start != nullptr ) {
                    this->visit_expr(*(Case_Stmt->m_start));
                    m_start = EXPR(tmp);
                    if( expr_type(m_start)->type != ASR::ttypeType::Integer ) {
                        throw SemanticError(R"""(Expression in Case selector can only be an Integer)""", 
                                            x.base.loc);
                    }
                }
                if( Case_Stmt->m_end != nullptr ) {
                    this->visit_expr(*(Case_Stmt->m_end));
                    m_end = EXPR(tmp);
                    if( expr_type(m_end)->type != ASR::ttypeType::Integer ) {
                        throw SemanticError(R"""(Expression in Case selector can only be an Integer)""", 
                                            x.base.loc);
                    }
                }
                Vec<ASR::stmt_t*> case_body_vec;
                case_body_vec.reserve(al, Case_Stmt->n_body);
                for( std::uint32_t i = 0; i < Case_Stmt->n_body; i++ ) {
                    this->visit_stmt(*(Case_Stmt->m_body[i]));
                    if (tmp != nullptr) {
                        case_body_vec.push_back(al, STMT(tmp));
                    }
                }
                tmp = ASR::make_CaseStmt_Range_t(al, x.base.loc, m_start, m_end, 
                                     case_body_vec.p, case_body_vec.size());
                break;
            }
            default: {
                throw SemanticError(R"""(Case statement can only support a valid expression 
                                    that reduces to a constant or range defined by : separator)""", 
                                    x.base.loc);
            }
        }
    }

    void visit_Select(const AST::Select_t& x) {
        this->visit_expr(*(x.m_test));
        ASR::expr_t* a_test = EXPR(tmp);
        if( expr_type(a_test)->type != ASR::ttypeType::Integer ) {
            throw SemanticError(R"""(Expression in Case selector can only be an Integer)""", x.base.base.loc);
        }
        Vec<ASR::case_stmt_t*> a_body_vec;
        a_body_vec.reserve(al, x.n_body);
        for( std::uint32_t i = 0; i < x.n_body; i++ ) {
            this->visit_case_stmt(*(x.m_body[i]));
            a_body_vec.push_back(al, CASE_STMT(tmp));
        }
        Vec<ASR::stmt_t*> def_body;
        def_body.reserve(al, x.n_default);
        for( std::uint32_t i = 0; i < x.n_default; i++ ) {
            this->visit_stmt(*(x.m_default[i]));
            if (tmp != nullptr) {
                def_body.push_back(al, STMT(tmp));
            }
        }
        tmp = ASR::make_Select_t(al, x.base.base.loc, a_test, a_body_vec.p, 
                           a_body_vec.size(), def_body.p, def_body.size());
    }

    void visit_Module(const AST::Module_t &x) {
        SymbolTable *old_scope = current_scope;
        ASR::symbol_t *t = current_scope->scope[std::string(x.m_name)];
        ASR::Module_t *v = ASR::down_cast<ASR::Module_t>(t);
        current_scope = v->m_symtab;
        current_module = v;

        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }

        current_scope = old_scope;
        current_module = nullptr;
        tmp = nullptr;
    }

    void visit_Program(const AST::Program_t &x) {
        SymbolTable *old_scope = current_scope;
        ASR::symbol_t *t = current_scope->scope[std::string(x.m_name)];
        ASR::Program_t *v = ASR::down_cast<ASR::Program_t>(t);
        current_scope = v->m_symtab;

        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                body.push_back(al, STMT(tmp));
            }
        }
        v->m_body = body.p;
        v->n_body = body.size();

        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }

        current_scope = old_scope;
        tmp = nullptr;
    }

    void visit_Subroutine(const AST::Subroutine_t &x) {
    // TODO: add SymbolTable::lookup_symbol(), which will automatically return
    // an error
    // TODO: add SymbolTable::get_symbol(), which will only check in Debug mode
        SymbolTable *old_scope = current_scope;
        ASR::symbol_t *t = current_scope->scope[std::string(x.m_name)];
        ASR::Subroutine_t *v = ASR::down_cast<ASR::Subroutine_t>(t);
        current_scope = v->m_symtab;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                body.push_back(al, STMT(tmp));
            }
        }
        v->m_body = body.p;
        v->n_body = body.size();

        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }

        current_scope = old_scope;
        tmp = nullptr;
    }

    void visit_Function(const AST::Function_t &x) {
        SymbolTable *old_scope = current_scope;
        ASR::symbol_t *t = current_scope->scope[std::string(x.m_name)];
        ASR::Function_t *v = ASR::down_cast<ASR::Function_t>(t);
        current_scope = v->m_symtab;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                body.push_back(al, STMT(tmp));
            }
        }
        v->m_body = body.p;
        v->n_body = body.size();

        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }

        current_scope = old_scope;
        tmp = nullptr;
    }

    void visit_Assignment(const AST::Assignment_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *target = EXPR(tmp);
        ASR::ttype_t *target_type = expr_type(target);
        if( target->type != ASR::exprType::Var && 
            target->type != ASR::exprType::ArrayRef && 
            target->type != ASR::exprType::DerivedRef )
        {
            throw SemanticError(
                "The LHS of assignment can only be a variable or an array reference",
                x.base.base.loc
            );
        }

        this->visit_expr(*x.m_value);
        ASR::expr_t *value = EXPR(tmp);
        ASR::ttype_t *value_type = expr_type(value);
        if (target->type == ASR::exprType::Var) {

            ImplicitCastRules::set_converted_value(al, x.base.base.loc, &value, 
                                                    value_type, target_type);

        }
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, value);
    }

    Vec<ASR::expr_t*> visit_expr_list(AST::fnarg_t *ast_list, size_t n) {
        Vec<ASR::expr_t*> asr_list;
        asr_list.reserve(al, n);
        for (size_t i=0; i<n; i++) {
            visit_expr(*ast_list[i].m_end);
            ASR::expr_t *expr = EXPR(tmp);
            asr_list.push_back(al, expr);
        }
        return asr_list;
    }

    void visit_SubroutineCall(const AST::SubroutineCall_t &x) {
        std::string sub_name = x.m_name;
        ASR::symbol_t *original_sym = current_scope->resolve_symbol(sub_name);
        if (!original_sym) {
            throw SemanticError("Subroutine '" + sub_name + "' not declared", x.base.base.loc);
        }
        Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
        ASR::symbol_t *final_sym=nullptr;
        switch (original_sym->type) {
            case (ASR::symbolType::Subroutine) : {
                final_sym=original_sym;
                original_sym = nullptr;
                break;
            }
            case (ASR::symbolType::GenericProcedure) : {
                ASR::GenericProcedure_t *p = ASR::down_cast<ASR::GenericProcedure_t>(original_sym);
                int idx = select_generic_procedure(args, *p, x.base.base.loc);
                final_sym = p->m_procs[idx];
                break;
            }
            case (ASR::symbolType::ExternalSymbol) : {
                ASR::ExternalSymbol_t *p = ASR::down_cast<ASR::ExternalSymbol_t>(original_sym);
                final_sym = p->m_external;
                // Enforced by verify(), but we ensure anyway that
                // ExternalSymbols are not chained:
                LFORTRAN_ASSERT(!ASR::is_a<ASR::ExternalSymbol_t>(*final_sym))
                if (ASR::is_a<ASR::GenericProcedure_t>(*final_sym)) {
                    ASR::GenericProcedure_t *g = ASR::down_cast<ASR::GenericProcedure_t>(final_sym);
                    int idx = select_generic_procedure(args, *g, x.base.base.loc);
                    // FIXME
                    // Create ExternalSymbol for the final subroutine here
                    final_sym = g->m_procs[idx];
                    if (!ASR::is_a<ASR::Subroutine_t>(*final_sym)) {
                        throw SemanticError("ExternalSymbol must point to a Subroutine", x.base.base.loc);
                    }
                    // We mangle the new ExternalSymbol's local name as:
                    //   generic_procedure_local_name @
                    //     specific_procedure_remote_name
                    std::string local_sym = std::string(p->m_name) + "@"
                        + symbol_name(final_sym);
                    if (current_scope->scope.find(local_sym)
                        == current_scope->scope.end()) {
                        Str name;
                        name.from_str(al, local_sym);
                        char *cname = name.c_str(al);
                        ASR::asr_t *sub = ASR::make_ExternalSymbol_t(
                            al, p->base.base.loc,
                            /* a_symtab */ current_scope,
                            /* a_name */ cname,
                            final_sym,
                            p->m_module_name, symbol_name(final_sym),
                            ASR::accessType::Private
                            );
                        final_sym = ASR::down_cast<ASR::symbol_t>(sub);
                        current_scope->scope[local_sym] = final_sym;
                    } else {
                        final_sym = current_scope->scope[local_sym];
                    }
                } else {
                    if (!ASR::is_a<ASR::Subroutine_t>(*final_sym)) {
                        throw SemanticError("ExternalSymbol must point to a Subroutine", x.base.base.loc);
                    }
                    final_sym=original_sym;
                    original_sym = nullptr;
                }
                break;
            }
            default : {
                throw SemanticError("Symbol type not supported", x.base.base.loc);
            }
        }
        tmp = ASR::make_SubroutineCall_t(al, x.base.base.loc,
                final_sym, original_sym, args.p, args.size());
    }

    int select_generic_procedure(const Vec<ASR::expr_t*> &args,
            const ASR::GenericProcedure_t &p, Location loc) {
        for (size_t i=0; i < p.n_procs; i++) {
            ASR::Subroutine_t *sub
                = ASR::down_cast<ASR::Subroutine_t>(p.m_procs[i]);
            if (argument_types_match(args, *sub)) {
                return i;
            }
        }
        throw SemanticError("Arguments do not match", loc);
    }

    bool argument_types_match(const Vec<ASR::expr_t*> &args,
            const ASR::Subroutine_t &sub) {
        if (args.size() == sub.n_args) {
            for (size_t i=0; i < args.size(); i++) {
                ASR::Variable_t *v = EXPR2VAR(sub.m_args[i]);
                ASR::ttype_t *arg1 = expr_type(args[i]);
                ASR::ttype_t *arg2 = v->m_type;
                if (!types_equal(*arg1, *arg2)) {
                    return false;
                }
            }
            return true;
        } else {
            return false;
        }
    }

    bool types_equal(const ASR::ttype_t &a, const ASR::ttype_t &b) {
        return (a.type == b.type);
    }

    void visit_Compare(const AST::Compare_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = EXPR(tmp);
        CommonVisitorMethods::visit_Compare(al, x, left, right, tmp);
    }

    void visit_BoolOp(const AST::BoolOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = EXPR(tmp);
        CommonVisitorMethods::visit_BoolOp(al, x, left, right, tmp);
    }

    void visit_BinOp(const AST::BinOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = EXPR(tmp);
        CommonVisitorMethods::visit_BinOp(al, x, left, right, tmp);
    }

    void visit_StrOp(const AST::StrOp_t &x) { 
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = EXPR(tmp);
        CommonVisitorMethods::visit_StrOp(al, x, left, right, tmp);
    }

    void visit_UnaryOp(const AST::UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        ASR::expr_t *operand = EXPR(tmp);
        CommonVisitorMethods::visit_UnaryOp(al, x, operand, tmp);
    }

    ASR::asr_t* resolve_variable(const Location &loc, const char* id) {
        SymbolTable *scope = current_scope;
        std::string var_name = id;
        ASR::symbol_t *v = scope->resolve_symbol(var_name);
        if (!v) {
            throw SemanticError("Variable '" + var_name + "' not declared", loc);
        }
        return ASR::make_Var_t(al, loc, v);
    }

    ASR::asr_t* getDerivedRef_t(const Location& loc, ASR::asr_t* v_var, ASR::symbol_t* member) {
        ASR::Variable_t* member_variable = ((ASR::Variable_t*)(&(member->base)));
        ASR::ttype_t* member_type = member_variable->m_type;
        switch( member_type->type ) {
            case ASR::ttypeType::Derived: {
                ASR::Derived_t* der = (ASR::Derived_t*)(&(member_type->base));
                ASR::DerivedType_t* der_type = (ASR::DerivedType_t*)(&(der->m_derived_type->base));
                if( der_type->m_symtab->counter != current_scope->counter ) {
                    ASR::symbol_t* der_ext;
                    if( current_scope->scope.find(std::string(der_type->m_name)) == current_scope->scope.end() ) {
                        char* module_name = (char*)"nullptr";
                        ASR::symbol_t* m_external = der->m_derived_type;
                        if( m_external->type == ASR::symbolType::ExternalSymbol ) {
                            ASR::ExternalSymbol_t* m_ext = (ASR::ExternalSymbol_t*)(&(m_external->base));
                            m_external = m_ext->m_external;
                            module_name = m_ext->m_module_name;
                        }
                        der_ext = (ASR::symbol_t*)ASR::make_ExternalSymbol_t(al, loc, current_scope, der_type->m_name, m_external, 
                                                                             module_name, der_type->m_name, ASR::accessType::Public);
                        current_scope->scope[std::string(der_type->m_name)] = der_ext;
                    } else {
                        der_ext = current_scope->scope[std::string(der_type->m_name)];
                    }
                    ASR::asr_t* der_new = ASR::make_Derived_t(al, loc, der_ext, der->m_dims, der->n_dims);
                    member_type = (ASR::ttype_t*)(der_new);
                }
                break;
            }
            default : 
                break;
        }
        return ASR::make_DerivedRef_t(al, loc, EXPR(v_var), member, member_type);
    }

    ASR::asr_t* resolve_variable2(const Location &loc, const char* id,
            const char* derived_type_id, SymbolTable*& scope) {
        std::string var_name = id;
        std::string dt_name = derived_type_id;
        ASR::symbol_t *v = scope->resolve_symbol(dt_name);
        if (!v) {
            throw SemanticError("Variable '" + dt_name + "' not declared", loc);
        }
        ASR::Variable_t* v_variable = ((ASR::Variable_t*)(&(v->base)));
        if ( v_variable->m_type->type == ASR::ttypeType::Derived || 
             v_variable->m_type->type == ASR::ttypeType::DerivedPointer ) {
            ASR::ttype_t* v_type = v_variable->m_type;
            ASR::Derived_t* der = (ASR::Derived_t*)(&(v_type->base));
            ASR::DerivedType_t* der_type;
            if( der->m_derived_type->type == ASR::symbolType::ExternalSymbol ) {
                ASR::ExternalSymbol_t* der_ext = (ASR::ExternalSymbol_t*)(&(der->m_derived_type->base));
                ASR::symbol_t* der_sym = der_ext->m_external;
                if( der_sym == nullptr ) {
                    throw SemanticError("'" + std::string(der_ext->m_name) + "' isn't a Derived type.", loc);
                } else {
                    der_type = (ASR::DerivedType_t*)(&(der_sym->base));
                }
            } else {
                der_type = (ASR::DerivedType_t*)(&(der->m_derived_type->base));
            }
            scope = der_type->m_symtab;
            ASR::symbol_t* member = der_type->m_symtab->resolve_symbol(var_name);
            if( member != nullptr ) {
                ASR::asr_t* v_var = ASR::make_Var_t(al, loc, v);
                return getDerivedRef_t(loc, v_var, member);
            } else {
                throw SemanticError("Variable '" + dt_name + "' doesn't have any member named, '" + var_name + "'.", loc);
            }
        } else {
            throw SemanticError("Variable '" + dt_name + "' is not a derived type", loc);
        }
    }

    void visit_Name(const AST::Name_t &x) {
        if (x.n_member == 0) {
            tmp = resolve_variable(x.base.base.loc, x.m_id);
        } else if (x.n_member == 1 && x.m_member[0].n_args == 0) {
            SymbolTable* scope = current_scope;
            tmp = resolve_variable2(x.base.base.loc, x.m_id,
                x.m_member[0].m_name, scope);
        } else {
            SymbolTable* scope = current_scope;
            tmp = resolve_variable2(x.base.base.loc, x.m_member[1].m_name, x.m_member[0].m_name, scope);
            ASR::DerivedRef_t* tmp2;
            std::uint32_t i;
            for( i = 2; i < x.n_member; i++ ) {
                tmp2 = (ASR::DerivedRef_t*)resolve_variable2(x.base.base.loc, 
                                            x.m_member[i].m_name, x.m_member[i - 1].m_name, scope);
                tmp = ASR::make_DerivedRef_t(al, x.base.base.loc, EXPR(tmp), tmp2->m_m, tmp2->m_type);
            }
            i = x.n_member - 1;
            tmp2 = (ASR::DerivedRef_t*)resolve_variable2(x.base.base.loc, x.m_id, x.m_member[i].m_name, scope);
            tmp = ASR::make_DerivedRef_t(al, x.base.base.loc, EXPR(tmp), tmp2->m_m, tmp2->m_type);
        }
    }

    void visit_FuncCallOrArray(const AST::FuncCallOrArray_t &x) {
        std::vector<std::string> all_intrinsics = {
            "sin",  "cos",  "tan",  "sinh",  "cosh",  "tanh",
            "asin", "acos", "atan", "asinh", "acosh", "atanh"};

        SymbolTable *scope = current_scope;
        std::string var_name = x.m_func;
        ASR::symbol_t *v = scope->resolve_symbol(var_name);
        if (!v) {
            std::string remote_sym = to_lower(var_name);
            if (intrinsic_procedures.find(remote_sym)
                        != intrinsic_procedures.end()) {
                std::string module_name = intrinsic_procedures[remote_sym];
                bool shift_scope = false;
                if (current_scope->parent->parent) {
                    current_scope = current_scope->parent;
                    shift_scope = true;
                }
                ASR::Module_t *m = load_module(al, current_scope->parent, module_name,
                    x.base.base.loc, true);
                if (shift_scope) current_scope = scope;

                ASR::symbol_t *t = m->m_symtab->resolve_symbol(remote_sym);
                if (!t) {
                    throw SemanticError("The symbol '" + remote_sym
                        + "' not found in the module '" + module_name + "'",
                        x.base.base.loc);
                }

                ASR::Function_t *mfn = ASR::down_cast<ASR::Function_t>(t);
                ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                    al, mfn->base.base.loc,
                    /* a_symtab */ current_scope,
                    /* a_name */ mfn->m_name,
                    (ASR::symbol_t*)mfn,
                    m->m_name, mfn->m_name,
                    ASR::accessType::Private
                    );
                std::string sym = mfn->m_name;
                current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(fn);
                v = ASR::down_cast<ASR::symbol_t>(fn);
                if (current_module) {
                    // Add the module `m` to current module dependencies
                    Vec<char*> vec;
                    vec.from_pointer_n_copy(al, current_module->m_dependencies,
                                current_module->n_dependencies);
                    if (!present(vec, m->m_name)) {
                        vec.push_back(al, m->m_name);
                        current_module->m_dependencies = vec.p;
                        current_module->n_dependencies = vec.size();
                    }
                }
            } else if (var_name == "present") {
                // Intrinsic function present(), add it to the global scope
                ASR::TranslationUnit_t *unit = (ASR::TranslationUnit_t *)asr;
                const char *fn_name_orig = "present";
                char *fn_name = (char *)fn_name_orig;
                SymbolTable *fn_scope =
                    al.make_new<SymbolTable>(unit->m_global_scope);
                ASR::ttype_t *type;
                type = TYPE(ASR::make_Logical_t(al, x.base.base.loc, 4, nullptr, 0));
                ASR::asr_t *return_var = ASR::make_Variable_t(
                    al, x.base.base.loc, fn_scope, fn_name, intent_return_var,
                    nullptr, ASR::storage_typeType::Default, type,
                    ASR::abiType::Source,
                    ASR::Public);
                fn_scope->scope[std::string(fn_name)] =
                    ASR::down_cast<ASR::symbol_t>(return_var);
                ASR::asr_t *return_var_ref = ASR::make_Var_t(
                    al, x.base.base.loc, ASR::down_cast<ASR::symbol_t>(return_var));
                ASR::asr_t *fn =
                    ASR::make_Function_t(al, x.base.base.loc,
                                       /* a_symtab */ fn_scope,
                                       /* a_name */ fn_name,
                                       /* a_args */ nullptr,
                                       /* n_args */ 0,
                                       /* a_body */ nullptr,
                                       /* n_body */ 0,
                                       /* a_return_var */ EXPR(return_var_ref),
                                       ASR::abiType::Source,
                                       ASR::Public, ASR::deftypeType::Implementation);
                std::string sym_name = fn_name;
                unit->m_global_scope->scope[sym_name] =
                    ASR::down_cast<ASR::symbol_t>(fn);
                v = ASR::down_cast<ASR::symbol_t>(fn);
            } else {
                auto find_intrinsic =
                    std::find(all_intrinsics.begin(), all_intrinsics.end(),
                        to_lower(var_name));
                if (find_intrinsic == all_intrinsics.end()) {
                    throw SemanticError("Function or array '" + var_name +
                                        "' not declared",
                                    x.base.base.loc);
                } else {
                    int intrinsic_index =
                      std::distance(all_intrinsics.begin(), find_intrinsic);
                    // Intrinsic function, add it to the global scope
                    ASR::TranslationUnit_t *unit = (ASR::TranslationUnit_t *)asr;
                    Str s;
                    s.from_str_view(all_intrinsics[intrinsic_index]);
                    char *fn_name = s.c_str(al);
                    SymbolTable *fn_scope =
                        al.make_new<SymbolTable>(unit->m_global_scope);
                    ASR::ttype_t *type;

                    // Arguments
                    Vec<ASR::expr_t *> args;
                    args.reserve(al, 1);
                    type = TYPE(ASR::make_Real_t(al, x.base.base.loc, 4, nullptr, 0));
                    const char *arg0_s_orig = "x";
                    char *arg0_s = (char *)arg0_s_orig;
                    ASR::asr_t *arg0 = ASR::make_Variable_t(
                        al, x.base.base.loc, fn_scope, arg0_s, intent_in, nullptr,
                        ASR::storage_typeType::Default, type,
                        ASR::abiType::Source,
                        ASR::Public);
                    ASR::symbol_t *var = ASR::down_cast<ASR::symbol_t>(arg0);
                    fn_scope->scope[std::string(arg0_s)] = var;
                    args.push_back(al, EXPR(ASR::make_Var_t(al, x.base.base.loc, var)));

                    // Return value
                    type = TYPE(ASR::make_Real_t(al, x.base.base.loc, 4, nullptr, 0));
                    ASR::asr_t *return_var = ASR::make_Variable_t(
                        al, x.base.base.loc, fn_scope, fn_name, intent_return_var,
                        nullptr, ASR::storage_typeType::Default, type,
                        ASR::abiType::Source,
                        ASR::Public);
                    fn_scope->scope[std::string(fn_name)] =
                        ASR::down_cast<ASR::symbol_t>(return_var);
                    ASR::asr_t *return_var_ref = ASR::make_Var_t(
                        al, x.base.base.loc, ASR::down_cast<ASR::symbol_t>(return_var));
                    ASR::asr_t *fn =
                        ASR::make_Function_t(al, x.base.base.loc,
                                             /* a_symtab */ fn_scope,
                                             /* a_name */ fn_name,
                                             /* a_args */ args.p,
                                             /* n_args */ args.n,
                                             /* a_body */ nullptr,
                                             /* n_body */ 0,
                                             /* a_return_var */ EXPR(return_var_ref),
                                             ASR::abiType::Intrinsic,
                                             ASR::Public, ASR::deftypeType::Implementation);
                    std::string sym_name = fn_name;
                    unit->m_global_scope->scope[sym_name] =
                        ASR::down_cast<ASR::symbol_t>(fn);
                    v = ASR::down_cast<ASR::symbol_t>(fn);
                }
            }
        }
        switch (v->type) {
            case (ASR::symbolType::Function) : {
                Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
                ASR::ttype_t *type;
                type = EXPR2VAR(ASR::down_cast<ASR::Function_t>(v)->m_return_var)->m_type;
                tmp = ASR::make_FunctionCall_t(al, x.base.base.loc,
                    v, nullptr, args.p, args.size(), nullptr, 0, type);
                break;
            }
            case (ASR::symbolType::ExternalSymbol) : {
                ASR::symbol_t *f2 = ASR::down_cast<ASR::ExternalSymbol_t>(v)->m_external;
                LFORTRAN_ASSERT(f2);
                if (ASR::is_a<ASR::Function_t>(*f2)) {
                    Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
                    ASR::ttype_t *type;
                    type = EXPR2VAR(ASR::down_cast<ASR::Function_t>(f2)->m_return_var)->m_type;
                    tmp = ASR::make_FunctionCall_t(al, x.base.base.loc,
                        v, nullptr, args.p, args.size(), nullptr, 0, type);
                } else if (ASR::is_a<ASR::Variable_t>(*f2)) {
                    Vec<ASR::array_index_t> args;
                    args.reserve(al, x.n_args);
                    for (size_t i=0; i<x.n_args; i++) {
                        ASR::array_index_t ai;
                        if (x.m_args[i].m_start == nullptr && x.m_args[i].m_end) {
                            visit_expr(*x.m_args[i].m_end);
                            ai.m_left = nullptr;
                            ai.m_right = EXPR(tmp);
                            ai.m_step = nullptr;
                            ai.loc = ai.m_right->base.loc;
                        } else if (x.m_args[i].m_start == nullptr
                                && x.m_args[i].m_end == nullptr) {
                            ai.m_left = nullptr;
                            ai.m_right = nullptr;
                            ai.m_step = nullptr;
                            ai.loc = x.base.base.loc;
                        } else {
                            throw SemanticError("Argument type not implemented yet",
                                x.base.base.loc);
                        }
                        args.push_back(al, ai);
                    }

                    ASR::ttype_t *type;
                    type = ASR::down_cast<ASR::Variable_t>(f2)->m_type;
                    tmp = ASR::make_ArrayRef_t(al, x.base.base.loc,
                        v, args.p, args.size(), type);
                } else {
                    throw SemanticError("Unimplemented", x.base.base.loc);
                }
                break;
            }
            case (ASR::symbolType::Variable) : {
                Vec<ASR::array_index_t> args;
                args.reserve(al, x.n_args);
                for (size_t i=0; i<x.n_args; i++) {
                    ASR::array_index_t ai;
                    if (x.m_args[i].m_start == nullptr && x.m_args[i].m_end) {
                        visit_expr(*x.m_args[i].m_end);
                        ai.m_left = nullptr;
                        ai.m_right = EXPR(tmp);
                        ai.m_step = nullptr;
                        ai.loc = ai.m_right->base.loc;
                    } else if (x.m_args[i].m_start == nullptr
                            && x.m_args[i].m_end == nullptr) {
                        ai.m_left = nullptr;
                        ai.m_right = nullptr;
                        ai.m_step = nullptr;
                        ai.loc = x.base.base.loc;
                    } else {
                        throw SemanticError("Argument type not implemented yet",
                            x.base.base.loc);
                    }
                    args.push_back(al, ai);
                }

                ASR::ttype_t *type;
                type = ASR::down_cast<ASR::Variable_t>(v)->m_type;
                tmp = ASR::make_ArrayRef_t(al, x.base.base.loc,
                    v, args.p, args.size(), type);
                break;
            }
            default : throw SemanticError("Symbol '" + var_name
                    + "' is not a function or an array", x.base.base.loc);
            }
    }

    void visit_Num(const AST::Num_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                4, nullptr, 0));
        tmp = ASR::make_ConstantInteger_t(al, x.base.base.loc, x.m_n, type);
    }

    void visit_Logical(const AST::Logical_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Logical_t(al, x.base.base.loc,
                4, nullptr, 0));
        tmp = ASR::make_ConstantLogical_t(al, x.base.base.loc, x.m_value, type);
    }

    void visit_Str(const AST::Str_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Character_t(al, x.base.base.loc,
                8, nullptr, 0));
        tmp = ASR::make_Str_t(al, x.base.base.loc, x.m_s, type);
    }

    void visit_Real(const AST::Real_t &x) {
        int a_kind = HelperMethods::extract_kind(x.m_n);
        ASR::ttype_t *type = TYPE(ASR::make_Real_t(al, x.base.base.loc,
                a_kind, nullptr, 0));
        tmp = ASR::make_ConstantReal_t(al, x.base.base.loc, x.m_n, type);
    }

    void visit_Complex(const AST::Complex_t &x) {
        this->visit_expr(*x.m_re);
        ASR::expr_t *re = EXPR(tmp);
        int a_kind_r = HelperMethods::extract_kind_from_ttype_t(expr_type(re));
        this->visit_expr(*x.m_im);
        ASR::expr_t *im = EXPR(tmp);
        int a_kind_i = HelperMethods::extract_kind_from_ttype_t(expr_type(im));
        ASR::ttype_t *type = TYPE(ASR::make_Complex_t(al, x.base.base.loc,
                std::max(a_kind_r, a_kind_i), nullptr, 0));
        tmp = ASR::make_ConstantComplex_t(al, x.base.base.loc,
                re, im, type);
    }


    void visit_ArrayInitializer(const AST::ArrayInitializer_t &x) {
        Vec<ASR::expr_t*> body;
        body.reserve(al, x.n_args);
        ASR::ttype_t *type = nullptr;
        for (size_t i=0; i<x.n_args; i++) {
            visit_expr(*x.m_args[i]);
            ASR::expr_t *expr = EXPR(tmp);
            if (type == nullptr) {
                type = expr_type(expr);
            } else {
                if (expr_type(expr)->type != type->type) {
                    throw SemanticError("Type mismatch in array initializer",
                        x.base.base.loc);
                }
            }
            body.push_back(al, expr);
        }
        tmp = ASR::make_ArrayInitializer_t(al, x.base.base.loc, body.p,
            body.size(), type);
    }

    void visit_Print(const AST::Print_t &x) {
        Vec<ASR::expr_t*> body;
        body.reserve(al, x.n_values);
        for (size_t i=0; i<x.n_values; i++) {
            visit_expr(*x.m_values[i]);
            ASR::expr_t *expr = EXPR(tmp);
            body.push_back(al, expr);
        }
        tmp = ASR::make_Print_t(al, x.base.base.loc, nullptr,
            body.p, body.size());
    }

    void visit_If(const AST::If_t &x) {
        visit_expr(*x.m_test);
        ASR::expr_t *test = EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                body.push_back(al, STMT(tmp));
            }
        }
        Vec<ASR::stmt_t*> orelse;
        orelse.reserve(al, x.n_orelse);
        for (size_t i=0; i<x.n_orelse; i++) {
            visit_stmt(*x.m_orelse[i]);
            if (tmp != nullptr) {
                orelse.push_back(al, STMT(tmp));
            }
        }
        tmp = ASR::make_If_t(al, x.base.base.loc, test, body.p,
                body.size(), orelse.p, orelse.size());
    }

    void visit_WhileLoop(const AST::WhileLoop_t &x) {
        visit_expr(*x.m_test);
        ASR::expr_t *test = EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                body.push_back(al, STMT(tmp));
            }
        }
        tmp = ASR::make_WhileLoop_t(al, x.base.base.loc, test, body.p,
                body.size());
    }

    void visit_DoLoop(const AST::DoLoop_t &x) {
        if (! x.m_var) {
            throw SemanticError("Do loop: loop variable is required for now",
                x.base.base.loc);
        }
        if (! x.m_start) {
            throw SemanticError("Do loop: start condition required for now",
                x.base.base.loc);
        }
        if (! x.m_end) {
            throw SemanticError("Do loop: end condition required for now",
                x.base.base.loc);
        }
        ASR::expr_t *var = EXPR(resolve_variable(x.base.base.loc, x.m_var));
        visit_expr(*x.m_start);
        ASR::expr_t *start = EXPR(tmp);
        visit_expr(*x.m_end);
        ASR::expr_t *end = EXPR(tmp);
        ASR::expr_t *increment;
        if (x.m_increment) {
            visit_expr(*x.m_increment);
            increment = EXPR(tmp);
        } else {
            increment = nullptr;
        }

        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                body.push_back(al, STMT(tmp));
            }
        }
        ASR::do_loop_head_t head;
        head.m_v = var;
        head.m_start = start;
        head.m_end = end;
        head.m_increment = increment;
        head.loc = head.m_v->base.loc;
        tmp = ASR::make_DoLoop_t(al, x.base.base.loc, head, body.p,
                body.size());
    }

    void visit_DoConcurrentLoop(const AST::DoConcurrentLoop_t &x) {
        if (x.n_control != 1) {
            throw SemanticError("Do concurrent: exactly one control statement is required for now",
            x.base.base.loc);
        }
        AST::ConcurrentControl_t &h = *(AST::ConcurrentControl_t*) x.m_control[0];
        if (! h.m_var) {
            throw SemanticError("Do loop: loop variable is required for now",
                x.base.base.loc);
        }
        if (! h.m_start) {
            throw SemanticError("Do loop: start condition required for now",
                x.base.base.loc);
        }
        if (! h.m_end) {
            throw SemanticError("Do loop: end condition required for now",
                x.base.base.loc);
        }
        ASR::expr_t *var = EXPR(resolve_variable(x.base.base.loc, h.m_var));
        visit_expr(*h.m_start);
        ASR::expr_t *start = EXPR(tmp);
        visit_expr(*h.m_end);
        ASR::expr_t *end = EXPR(tmp);
        ASR::expr_t *increment;
        if (h.m_increment) {
            visit_expr(*h.m_increment);
            increment = EXPR(tmp);
        } else {
            increment = nullptr;
        }

        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
            if (tmp != nullptr) {
                body.push_back(al, STMT(tmp));
            }
        }
        ASR::do_loop_head_t head;
        head.m_v = var;
        head.m_start = start;
        head.m_end = end;
        head.m_increment = increment;
        head.loc = head.m_v->base.loc;
        tmp = ASR::make_DoConcurrentLoop_t(al, x.base.base.loc, head, body.p,
                body.size());
    }

    void visit_Exit(const AST::Exit_t &x) {
        // TODO: add a check here that we are inside a While loop
        tmp = ASR::make_Exit_t(al, x.base.base.loc);
    }

    void visit_Cycle(const AST::Cycle_t &x) {
        // TODO: add a check here that we are inside a While loop
        tmp = ASR::make_Cycle_t(al, x.base.base.loc);
    }

    void visit_Continue(const AST::Continue_t &/*x*/) {
        // TODO: add a check here that we are inside a While loop
        // Nothing to generate, we return a null pointer
        tmp = nullptr;
    }

    void visit_Stop(const AST::Stop_t &x) {
        ASR::expr_t *code;
        if (x.m_code) {
            visit_expr(*x.m_code);
            code = EXPR(tmp);
        } else {
            code = nullptr;
        }
        tmp = ASR::make_Stop_t(al, x.base.base.loc, code);
    }

    void visit_ErrorStop(const AST::ErrorStop_t &x) {
        ASR::expr_t *code;
        if (x.m_code) {
            visit_expr(*x.m_code);
            code = EXPR(tmp);
        } else {
            code = nullptr;
        }
        tmp = ASR::make_ErrorStop_t(al, x.base.base.loc, code);
    }
};

ASR::TranslationUnit_t *ast_to_asr(Allocator &al, AST::TranslationUnit_t &ast,
        SymbolTable *symbol_table)
{
    SymbolTableVisitor v(al, symbol_table);
    v.visit_TranslationUnit(ast);
    ASR::asr_t *unit = v.asr;

    // Uncomment for debugging the ASR after SymbolTable building:
    // std::cout << pickle(*unit) << std::endl;

    BodyVisitor b(al, unit);
    b.visit_TranslationUnit(ast);
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(unit);
    LFORTRAN_ASSERT(asr_verify(*tu));
    return tu;
}

} // namespace LFortran
