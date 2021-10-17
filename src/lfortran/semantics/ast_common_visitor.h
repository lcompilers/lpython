#ifndef LFORTRAN_SEMANTICS_AST_COMMON_VISITOR_H
#define LFORTRAN_SEMANTICS_AST_COMMON_VISITOR_H

#include <lfortran/asr.h>
#include <lfortran/ast.h>
#include <lfortran/bigint.h>
#include <lfortran/string_utils.h>
#include <lfortran/utils.h>
#include <lfortran/semantics/comptime_eval.h>

namespace LFortran {

#define LFORTRAN_STMT_LABEL_TYPE(x) \
        case AST::stmtType::x: { return AST::down_cast<AST::x##_t>(f)->m_label; }

static inline int64_t stmt_label(AST::stmt_t *f)
{
    switch (f->type) {
        LFORTRAN_STMT_LABEL_TYPE(Allocate)
        LFORTRAN_STMT_LABEL_TYPE(Assign)
        LFORTRAN_STMT_LABEL_TYPE(Assignment)
        LFORTRAN_STMT_LABEL_TYPE(Associate)
        LFORTRAN_STMT_LABEL_TYPE(Backspace)
        LFORTRAN_STMT_LABEL_TYPE(Close)
        LFORTRAN_STMT_LABEL_TYPE(Continue)
        LFORTRAN_STMT_LABEL_TYPE(Cycle)
        LFORTRAN_STMT_LABEL_TYPE(Deallocate)
        LFORTRAN_STMT_LABEL_TYPE(Endfile)
        LFORTRAN_STMT_LABEL_TYPE(Entry)
        LFORTRAN_STMT_LABEL_TYPE(ErrorStop)
        LFORTRAN_STMT_LABEL_TYPE(EventPost)
        LFORTRAN_STMT_LABEL_TYPE(EventWait)
        LFORTRAN_STMT_LABEL_TYPE(Exit)
        LFORTRAN_STMT_LABEL_TYPE(Flush)
        LFORTRAN_STMT_LABEL_TYPE(ForAllSingle)
        LFORTRAN_STMT_LABEL_TYPE(Format)
        LFORTRAN_STMT_LABEL_TYPE(FormTeam)
        LFORTRAN_STMT_LABEL_TYPE(GoTo)
        LFORTRAN_STMT_LABEL_TYPE(Inquire)
        LFORTRAN_STMT_LABEL_TYPE(Nullify)
        LFORTRAN_STMT_LABEL_TYPE(Open)
        LFORTRAN_STMT_LABEL_TYPE(Return)
        LFORTRAN_STMT_LABEL_TYPE(Print)
        LFORTRAN_STMT_LABEL_TYPE(Read)
        LFORTRAN_STMT_LABEL_TYPE(Rewind)
        LFORTRAN_STMT_LABEL_TYPE(Stop)
        LFORTRAN_STMT_LABEL_TYPE(SubroutineCall)
        LFORTRAN_STMT_LABEL_TYPE(SyncAll)
        LFORTRAN_STMT_LABEL_TYPE(SyncImages)
        LFORTRAN_STMT_LABEL_TYPE(SyncMemory)
        LFORTRAN_STMT_LABEL_TYPE(SyncTeam)
        LFORTRAN_STMT_LABEL_TYPE(Write)
        LFORTRAN_STMT_LABEL_TYPE(AssociateBlock)
        LFORTRAN_STMT_LABEL_TYPE(Block)
        LFORTRAN_STMT_LABEL_TYPE(ChangeTeam)
        LFORTRAN_STMT_LABEL_TYPE(Critical)
        LFORTRAN_STMT_LABEL_TYPE(DoConcurrentLoop)
        LFORTRAN_STMT_LABEL_TYPE(DoLoop)
        LFORTRAN_STMT_LABEL_TYPE(ForAll)
        LFORTRAN_STMT_LABEL_TYPE(If)
        LFORTRAN_STMT_LABEL_TYPE(IfArithmetic)
        LFORTRAN_STMT_LABEL_TYPE(Select)
        LFORTRAN_STMT_LABEL_TYPE(SelectRank)
        LFORTRAN_STMT_LABEL_TYPE(SelectType)
        LFORTRAN_STMT_LABEL_TYPE(Where)
        LFORTRAN_STMT_LABEL_TYPE(WhileLoop)
        default : throw LFortranException("Not implemented");
    }
}


class CommonVisitorMethods {
public:

inline static void visit_BinOp(Allocator &al, const AST::BinOp_t &x,
                                ASR::expr_t *&left, ASR::expr_t *&right,
                                ASR::asr_t *&asr, std::string& intrinsic_op_name,
                                SymbolTable* curr_scope) {
    ASR::binopType op;
    switch (x.m_op) {
        case (AST::Add):
            op = ASR::Add;
            break;
        case (AST::Sub):
            op = ASR::Sub;
            break;
        case (AST::Mul):
            op = ASR::Mul;
            break;
        case (AST::Div):
            op = ASR::Div;
            break;
        case (AST::Pow):
            op = ASR::Pow;
            break;
        // Fix compiler warning:
        default: {
            LFORTRAN_ASSERT(false);
            op = ASR::binopType::Pow;
        }
    }

    // Cast LHS or RHS if necessary
    ASR::ttype_t *left_type = LFortran::ASRUtils::expr_type(left);
    ASR::ttype_t *right_type = LFortran::ASRUtils::expr_type(right);
    ASR::expr_t *overloaded = nullptr;
    if( LFortran::ASRUtils::use_overloaded(left, right, op,
        intrinsic_op_name, curr_scope, asr, al,
        x.base.base.loc) ) {
        overloaded = LFortran::ASRUtils::EXPR(asr);
    }

    ASR::expr_t **conversion_cand = &left;
    ASR::ttype_t *source_type = left_type;
    ASR::ttype_t *dest_type = right_type;

    ImplicitCastRules::find_conversion_candidate(&left, &right, left_type,
                                                 right_type, conversion_cand,
                                                 &source_type, &dest_type);
    ImplicitCastRules::set_converted_value(al, x.base.base.loc, conversion_cand,
                                           source_type, dest_type);

    LFORTRAN_ASSERT(
        ASRUtils::check_equal_type(LFortran::ASRUtils::expr_type(left),
                                   LFortran::ASRUtils::expr_type(right)));
    ASR::expr_t *value = nullptr;
    // Assign evaluation to `value` if possible, otherwise leave nullptr
    if (LFortran::ASRUtils::expr_value(left) != nullptr &&
                LFortran::ASRUtils::expr_value(right) != nullptr) {
        if (ASR::is_a<LFortran::ASR::Integer_t>(*dest_type)) {
            int64_t left_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                    LFortran::ASRUtils::expr_value(left))
                                    ->m_n;
            int64_t right_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                    LFortran::ASRUtils::expr_value(right))
                                    ->m_n;
            int64_t result;
            switch (op) {
                case (ASR::Add):
                    result = left_value + right_value;
                    break;
                case (ASR::Sub):
                    result = left_value - right_value;
                    break;
                case (ASR::Mul):
                    result = left_value * right_value;
                    break;
                case (ASR::Div):
                    result = left_value / right_value;
                    break;
                case (ASR::Pow):
                    result = std::pow(left_value, right_value);
                    break;
                // Reconsider
                default: {
                    LFORTRAN_ASSERT(false);
                    op = ASR::binopType::Pow;
                }
            }
            value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(
                al, x.base.base.loc, result, dest_type));
        } else if (ASR::is_a<LFortran::ASR::Real_t>(*dest_type)) {
            double left_value = ASR::down_cast<ASR::ConstantReal_t>(
                                    LFortran::ASRUtils::expr_value(left))
                                    ->m_r;
            double right_value = ASR::down_cast<ASR::ConstantReal_t>(
                                    LFortran::ASRUtils::expr_value(right))
                                    ->m_r;
            double result;
            switch (op) {
                case (ASR::Add):
                    result = left_value + right_value;
                    break;
                case (ASR::Sub):
                    result = left_value - right_value;
                    break;
                case (ASR::Mul):
                    result = left_value * right_value;
                    break;
                case (ASR::Div):
                    result = left_value / right_value;
                    break;
                case (ASR::Pow):
                    result = std::pow(left_value, right_value);
                    break;
                // Reconsider
                default: {
                    LFORTRAN_ASSERT(false);
                    op = ASR::binopType::Pow;
                }
            }
            value = ASR::down_cast<ASR::expr_t>(
                ASR::make_ConstantReal_t(al, x.base.base.loc, result, dest_type));
        } else if (ASR::is_a<LFortran::ASR::Complex_t>(*dest_type)) {
            ASR::ConstantComplex_t *left0
                = ASR::down_cast<ASR::ConstantComplex_t>(
                        LFortran::ASRUtils::expr_value(left));
            ASR::ConstantComplex_t *right0
                = ASR::down_cast<ASR::ConstantComplex_t>(
                        LFortran::ASRUtils::expr_value(right));
            std::complex<double> left_value(left0->m_re, left0->m_im);
            std::complex<double> right_value(right0->m_re, right0->m_im);
            std::complex<double> result;
            switch (op) {
                case (ASR::Add):
                    result = left_value + right_value;
                    break;
                case (ASR::Sub):
                    result = left_value - right_value;
                    break;
                case (ASR::Mul):
                    result = left_value * right_value;
                    break;
                case (ASR::Div):
                    result = left_value / right_value;
                    break;
                case (ASR::Pow):
                    result = std::pow(left_value, right_value);
                    break;
                // Reconsider
                default: {
                    LFORTRAN_ASSERT(false);
                    op = ASR::binopType::Pow;
                }
            }
            value = ASR::down_cast<ASR::expr_t>(
                ASR::make_ConstantComplex_t(al, x.base.base.loc,
                    std::real(result), std::imag(result), dest_type));
        }
    }
    asr = ASR::make_BinOp_t(al, x.base.base.loc, left, op, right, dest_type,
                            value, overloaded);
  }

  inline static void visit_Compare(Allocator &al, const AST::Compare_t &x,
                                   ASR::expr_t *&left, ASR::expr_t *&right,
                                   ASR::asr_t *&asr, std::string& intrinsic_op_name,
                                   SymbolTable* curr_scope) {
    ASR::cmpopType asr_op;
    switch (x.m_op) {
        case (AST::cmpopType::Eq): {
        asr_op = ASR::cmpopType::Eq;
        break;
        }
        case (AST::cmpopType::Gt): {
        asr_op = ASR::cmpopType::Gt;
        break;
        }
        case (AST::cmpopType::GtE): {
        asr_op = ASR::cmpopType::GtE;
        break;
        }
        case (AST::cmpopType::Lt): {
        asr_op = ASR::cmpopType::Lt;
        break;
        }
        case (AST::cmpopType::LtE): {
        asr_op = ASR::cmpopType::LtE;
        break;
        }
        case (AST::cmpopType::NotEq): {
        asr_op = ASR::cmpopType::NotEq;
        break;
        }
        default: {
        throw SemanticError("Comparison operator not implemented",
                            x.base.base.loc);
        }
    }
    // Cast LHS or RHS if necessary
    ASR::ttype_t *left_type = LFortran::ASRUtils::expr_type(left);
    ASR::ttype_t *right_type = LFortran::ASRUtils::expr_type(right);

    ASR::expr_t *overloaded = nullptr;
    if( LFortran::ASRUtils::use_overloaded(left, right, asr_op,
        intrinsic_op_name, curr_scope, asr, al,
        x.base.base.loc) ) {
        overloaded = LFortran::ASRUtils::EXPR(asr);
    }

    if (((left_type->type != ASR::ttypeType::Real &&
         left_type->type != ASR::ttypeType::Integer) &&
        (right_type->type != ASR::ttypeType::Real &&
         right_type->type != ASR::ttypeType::Integer) &&
        ((left_type->type != ASR::ttypeType::Complex ||
          right_type->type != ASR::ttypeType::Complex) &&
         x.m_op != AST::cmpopType::Eq && x.m_op != AST::cmpopType::NotEq) &&
         (left_type->type != ASR::ttypeType::Character ||
          right_type->type != ASR::ttypeType::Character))
         && overloaded == nullptr) {
      throw SemanticError(
          "Compare: only Integer or Real can be on the LHS and RHS. "
          "If operator is .eq. or .neq. then Complex type is also acceptable",
          x.base.base.loc);
    } else {
      ASR::expr_t **conversion_cand = &left;
      ASR::ttype_t *dest_type = right_type;
      ASR::ttype_t *source_type = left_type;
      ImplicitCastRules::find_conversion_candidate(&left, &right, left_type,
                                                   right_type, conversion_cand,
                                                   &source_type, &dest_type);

      ImplicitCastRules::set_converted_value(
          al, x.base.base.loc, conversion_cand, source_type, dest_type);
    }

    LFORTRAN_ASSERT(
        ASRUtils::check_equal_type(LFortran::ASRUtils::expr_type(left),
                                   LFortran::ASRUtils::expr_type(right)));
    ASR::ttype_t *type = LFortran::ASRUtils::TYPE(
        ASR::make_Logical_t(al, x.base.base.loc, 4, nullptr, 0));

    ASR::expr_t *value = nullptr;
    ASR::ttype_t *source_type = left_type;
    // Assign evaluation to `value` if possible, otherwise leave nullptr
    if (LFortran::ASRUtils::expr_value(left) != nullptr &&
        LFortran::ASRUtils::expr_value(right) != nullptr) {
      if (ASR::is_a<LFortran::ASR::Integer_t>(*source_type)) {
        int64_t left_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                 LFortran::ASRUtils::expr_value(left))
                                 ->m_n;
        int64_t right_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                  LFortran::ASRUtils::expr_value(right))
                                  ->m_n;
        bool result;
        switch (asr_op) {
            case (ASR::cmpopType::Eq): {
                result = left_value == right_value;
                break;
            }
            case (ASR::cmpopType::Gt): {
                result = left_value > right_value;
                break;
            }
            case (ASR::cmpopType::GtE): {
                result = left_value >= right_value;
                break;
            }
            case (ASR::cmpopType::Lt): {
                result = left_value < right_value;
                break;
            }
            case (ASR::cmpopType::LtE): {
                result = left_value <= right_value;
                break;
            }
            case (ASR::cmpopType::NotEq): {
                result = left_value != right_value;
                break;
            }
            default: {
                throw SemanticError("Comparison operator not implemented",
                                    x.base.base.loc);
            }
        }
        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantLogical_t(
            al, x.base.base.loc, result, source_type));
      } else if (ASR::is_a<LFortran::ASR::Real_t>(*source_type)) {
        double left_value = ASR::down_cast<ASR::ConstantReal_t>(
                                LFortran::ASRUtils::expr_value(left))
                                ->m_r;
        double right_value = ASR::down_cast<ASR::ConstantReal_t>(
                                 LFortran::ASRUtils::expr_value(right))
                                 ->m_r;
        bool result;
        switch (asr_op) {
            case (ASR::cmpopType::Eq): {
                result = left_value == right_value;
                break;
            }
            case (ASR::cmpopType::Gt): {
                result = left_value > right_value;
                break;
            }
            case (ASR::cmpopType::GtE): {
                result = left_value >= right_value;
                break;
            }
            case (ASR::cmpopType::Lt): {
                result = left_value < right_value;
                break;
            }
            case (ASR::cmpopType::LtE): {
                result = left_value <= right_value;
                break;
            }
            case (ASR::cmpopType::NotEq): {
                result = left_value != right_value;
                break;
            }
            default: {
                throw SemanticError("Comparison operator not implemented",
                                    x.base.base.loc);
            }
        }
        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantLogical_t(
            al, x.base.base.loc, result, source_type));
      }
    }
    asr = ASR::make_Compare_t(al, x.base.base.loc, left, asr_op, right, type,
                              value, overloaded);
  }

  inline static void visit_BoolOp(Allocator &al, const AST::BoolOp_t &x,
                                  ASR::expr_t *&left, ASR::expr_t *&right,
                                  ASR::asr_t *&asr) {
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
    ASR::ttype_t *left_type = LFortran::ASRUtils::expr_type(left);
    ASR::ttype_t *right_type = LFortran::ASRUtils::expr_type(right);
    ASR::expr_t **conversion_cand = &left;
    ASR::ttype_t *source_type = left_type;
    ASR::ttype_t *dest_type = right_type;

    ImplicitCastRules::find_conversion_candidate(&left, &right, left_type,
                                                 right_type, conversion_cand,
                                                 &source_type, &dest_type);
    ImplicitCastRules::set_converted_value(al, x.base.base.loc, conversion_cand,
                                           source_type, dest_type);

    LFORTRAN_ASSERT(
        ASRUtils::check_equal_type(LFortran::ASRUtils::expr_type(left),
                                   LFortran::ASRUtils::expr_type(right)));

    ASR::expr_t *value = nullptr;
    // Assign evaluation to `value` if possible, otherwise leave nullptr
    if (LFortran::ASRUtils::expr_value(left) != nullptr &&
        LFortran::ASRUtils::expr_value(right) != nullptr) {
        LFORTRAN_ASSERT(ASR::is_a<LFortran::ASR::Logical_t>(*dest_type))

        bool left_value = ASR::down_cast<ASR::ConstantLogical_t>(
                                 LFortran::ASRUtils::expr_value(left))
                                 ->m_value;
        bool right_value = ASR::down_cast<ASR::ConstantLogical_t>(
                                  LFortran::ASRUtils::expr_value(right))
                                  ->m_value;
        bool result;
        switch (op) {
            case (ASR::And):
                result = left_value && right_value;
                break;
            case (ASR::Or):
                result = left_value || right_value;
                break;
            case (ASR::NEqv):
                result = left_value != right_value;
                break;
            case (ASR::Eqv):
                result = left_value == right_value;
                break;
            default:
                throw SemanticError(R"""(Only .and., .or., .neqv., .eqv.
                                                implemented for logical type operands.)""",
                                    x.base.base.loc);
        }
        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantLogical_t(
            al, x.base.base.loc, result, dest_type));
    }
    asr = ASR::make_BoolOp_t(al, x.base.base.loc, left, op, right, dest_type,
                             value);
  }

  inline static void visit_UnaryOp(Allocator &al, const AST::UnaryOp_t &x,
                                   ASR::expr_t *&operand, ASR::asr_t *&asr) {
    ASR::unaryopType op;
    switch (x.m_op) {
    case (AST::unaryopType::Invert):
      op = ASR::unaryopType::Invert;
      break;
    case (AST::unaryopType::Not):
      op = ASR::unaryopType::Not;
      break;
    case (AST::unaryopType::UAdd):
      op = ASR::unaryopType::UAdd;
      break;
    case (AST::unaryopType::USub):
      op = ASR::unaryopType::USub;
      break;
    // Fix compiler warning:
    default: {
      LFORTRAN_ASSERT(false);
      op = ASR::unaryopType::Invert;
    }
    }
    ASR::ttype_t *operand_type = LFortran::ASRUtils::expr_type(operand);
    ASR::expr_t *value = nullptr;
    // Assign evaluation to `value` if possible, otherwise leave nullptr
    if (LFortran::ASRUtils::expr_value(operand) != nullptr) {
      if (ASR::is_a<LFortran::ASR::Integer_t>(*operand_type)) {
        int64_t op_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                  LFortran::ASRUtils::expr_value(operand))
                                  ->m_n;
        int64_t result;
        switch (op) {
        case (ASR::unaryopType::UAdd):
          result = op_value;
          break;
        case (ASR::unaryopType::USub):
          result = -op_value;
          break;
        default: {
            throw SemanticError("Unary operator not implemented yet for compile time evaluation",
                x.base.base.loc);
        }
        }
        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(
            al, x.base.base.loc, result, operand_type));
      } else if (ASR::is_a<LFortran::ASR::Real_t>(*operand_type)) {
        double op_value = ASR::down_cast<ASR::ConstantReal_t>(
                                LFortran::ASRUtils::expr_value(operand))
                                ->m_r;
        double result;
        switch (op) {
        case (ASR::unaryopType::UAdd):
          result = op_value;
          break;
        case (ASR::unaryopType::USub):
          result = -op_value;
          break;
        default: {
            throw SemanticError("Unary operator not implemented yet for compile time evaluation",
                x.base.base.loc);
        }
        }
        value = ASR::down_cast<ASR::expr_t>(
            ASR::make_ConstantReal_t(al, x.base.base.loc, result, operand_type));
      }
    }
    asr = ASR::make_UnaryOp_t(al, x.base.base.loc, op, operand, operand_type,
                              value);
  }

  static inline void visit_StrOp(Allocator &al, const AST::StrOp_t &x,
                                 ASR::expr_t *&left, ASR::expr_t *&right,
                                 ASR::asr_t *&asr) {
    ASR::stropType op;
    LFORTRAN_ASSERT(x.m_op == AST::Concat)
    switch (x.m_op) {
    case (AST::Concat):
      op = ASR::Concat;
    }
    ASR::ttype_t *left_type = LFortran::ASRUtils::expr_type(left);
    ASR::ttype_t *right_type = LFortran::ASRUtils::expr_type(right);
    LFORTRAN_ASSERT(ASR::is_a<ASR::Character_t>(*left_type))
    LFORTRAN_ASSERT(ASR::is_a<ASR::Character_t>(*right_type))
    ASR::Character_t *left_type2 = ASR::down_cast<ASR::Character_t>(left_type);
    ASR::Character_t *right_type2 = ASR::down_cast<ASR::Character_t>(right_type);
    LFORTRAN_ASSERT(left_type2->n_dims == 0);
    LFORTRAN_ASSERT(right_type2->n_dims == 0);
    ASR::ttype_t *dest_type = ASR::down_cast<ASR::ttype_t>(ASR::make_Character_t(al, x.base.base.loc, left_type2->m_kind,
        left_type2->m_len + right_type2->m_len, nullptr, nullptr, 0));

    ASR::expr_t *value = nullptr;
    // Assign evaluation to `value` if possible, otherwise leave nullptr
    if (LFortran::ASRUtils::expr_value(left) != nullptr &&
        LFortran::ASRUtils::expr_value(right) != nullptr) {
        char* left_value = ASR::down_cast<ASR::ConstantString_t>(
                                 LFortran::ASRUtils::expr_value(left))
                                 ->m_s;
        char* right_value = ASR::down_cast<ASR::ConstantString_t>(
                                  LFortran::ASRUtils::expr_value(right))
                                  ->m_s;
        char* result;
        std::string result_s = std::string(left_value)+std::string(right_value);
        Str s; s.from_str_view(result_s);
        result = s.c_str(al);
        LFORTRAN_ASSERT((int64_t)strlen(result) == ASR::down_cast<ASR::Character_t>(dest_type)->m_len)
        value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantString_t(
            al, x.base.base.loc, result, dest_type));
      }
    asr = ASR::make_StrOp_t(al, x.base.base.loc, left, op, right, dest_type,
                            value);
  }

static ASR::asr_t* comptime_intrinsic_real(ASR::expr_t *A,
        ASR::expr_t * kind,
        Allocator &al, const Location &loc) {
    int kind_int = 4;
    if (kind) {
        ASR::expr_t* kind_value = LFortran::ASRUtils::expr_value(kind);
        if (kind_value) {
            if (ASR::is_a<ASR::ConstantInteger_t>(*kind_value)) {
                kind_int = ASR::down_cast<ASR::ConstantInteger_t>(kind_value)->m_n;
            } else {
                throw SemanticError("kind argument to real(a, kind) is not a constant integer", loc);
            }
        } else {
            throw SemanticError("kind argument to real(a, kind) is not constant", loc);
        }
    }
    ASR::expr_t *result = A;
    ASR::ttype_t *dest_type = LFortran::ASRUtils::TYPE(ASR::make_Real_t(al, loc,
                kind_int, nullptr, 0));
    ASR::ttype_t *source_type = LFortran::ASRUtils::expr_type(A);

    // TODO: this is explicit cast, use ExplicitCast
    ImplicitCastRules::set_converted_value(al, loc, &result,
                                           source_type, dest_type);
    return (ASR::asr_t*)result;
}

}; // class CommonVisitorMethods


template <class Derived>
class CommonVisitor : public AST::BaseVisitor<Derived> {
public:
    std::map<AST::operatorType, std::string> binop2str = {
        {AST::operatorType::Mul, "~mul"},
        {AST::operatorType::Add, "~add"},
    };

    std::map<AST::cmpopType, std::string> cmpop2str = {
        {AST::cmpopType::Eq, "~eq"},
        {AST::cmpopType::NotEq, "~noteq"},
        {AST::cmpopType::Lt, "~lt"},
        {AST::cmpopType::LtE, "~lte"},
        {AST::cmpopType::Gt, "~gt"},
        {AST::cmpopType::GtE, "~gte"}
    };


    ASR::asr_t *tmp;
    Allocator &al;
    SymbolTable *current_scope;
    ASR::Module_t *current_module = nullptr;
    Vec<char *> current_module_dependencies;
    IntrinsicProcedures intrinsic_procedures;

    CommonVisitor(Allocator &al, SymbolTable *symbol_table) : al{al}, current_scope{symbol_table} {
        current_module_dependencies.reserve(al, 4);
    }

    ASR::asr_t* resolve_variable(const Location &loc, const std::string &var_name) {
        SymbolTable *scope = current_scope;
        ASR::symbol_t *v = scope->resolve_symbol(var_name);
        if (!v) {
            diag::Span s;
            s.loc = loc;
            diag::Label l;
            l.primary = true;
            l.message = "'" + var_name + "' is undeclared";
            l.spans.push_back(s);
            diag::Diagnostic d;
            d.level = diag::Level::Error;
            d.stage = diag::Stage::Semantic;
            d.message = "Variable '" + var_name + "' is not declared";
            d.labels.push_back(l);
            throw SemanticError(d);
            //throw SemanticError("Variable '" + var_name + "' not declared", loc);
        }
        if( v->type == ASR::symbolType::Variable ) {
            ASR::Variable_t* v_var = ASR::down_cast<ASR::Variable_t>(v);
            if( v_var->m_type == nullptr &&
                v_var->m_intent == ASR::intentType::AssociateBlock ) {
                return (ASR::asr_t*)(v_var->m_symbolic_value);
            }
        }
        return ASR::make_Var_t(al, loc, v);
    }

    ASR::asr_t* create_DerivedTypeConstructor(const Location &loc,
            AST::fnarg_t* m_args, size_t n_args, ASR::symbol_t *v) {
        Vec<ASR::expr_t*> vals = visit_expr_list(m_args, n_args);
        ASR::ttype_t* der = LFortran::ASRUtils::TYPE(
                            ASR::make_Derived_t(al, loc, v,
                                                nullptr, 0));
        return ASR::make_DerivedTypeConstructor_t(al, loc,
                v, vals.p, vals.size(), der);
    }

    ASR::asr_t* create_ArrayRef(const Location &loc,
                AST::fnarg_t* m_args, size_t n_args,
                    ASR::symbol_t *v,
                    ASR::symbol_t *f2) {
        Vec<ASR::array_index_t> args;
        args.reserve(al, n_args);
        for (size_t i=0; i<n_args; i++) {
            ASR::array_index_t ai;
            ai.loc = loc;
            ASR::expr_t *m_start, *m_end, *m_step;
            m_start = m_end = m_step = nullptr;
            if (m_args[i].m_start != nullptr) {
                this->visit_expr(*(m_args[i].m_start));
                m_start = LFortran::ASRUtils::EXPR(tmp);
                ai.loc = m_start->base.loc;
            }
            if (m_args[i].m_end != nullptr) {
                this->visit_expr(*(m_args[i].m_end));
                m_end = LFortran::ASRUtils::EXPR(tmp);
                ai.loc = m_end->base.loc;
            }
            if (m_args[i].m_step != nullptr) {
                this->visit_expr(*(m_args[i].m_step));
                m_step = LFortran::ASRUtils::EXPR(tmp);
                ai.loc = m_step->base.loc;
            }
            ai.m_left = m_start;
            ai.m_right = m_end;
            ai.m_step = m_step;
            args.push_back(al, ai);
        }

        ASR::ttype_t *type;
        type = ASR::down_cast<ASR::Variable_t>(f2)->m_type;
        ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(f2);
        if( var->m_type == nullptr &&
            var->m_intent == ASR::intentType::AssociateBlock ) {
            ASR::expr_t* orig_expr = var->m_symbolic_value;
            ASR::Var_t* orig_Var = ASR::down_cast<ASR::Var_t>(orig_expr);
            v = orig_Var->m_v;
            type = ASR::down_cast<ASR::Variable_t>(v)->m_type;
        }
        return ASR::make_ArrayRef_t(al, loc,
            v, args.p, args.size(), type, nullptr);
    }

    ASR::ttype_t* handle_character_return(ASR::ttype_t *return_type, const Location &loc) {
        // Rebuild the return type if needed and make FunctionCalls use ExternalSymbol
        ASR::Character_t *t = ASR::down_cast<ASR::Character_t>(return_type);
        if (t->m_len_expr) {
            if (ASR::is_a<ASR::FunctionCall_t>(*t->m_len_expr)) {
                ASR::FunctionCall_t *fc = ASR::down_cast<ASR::FunctionCall_t>(t->m_len_expr);
                if (ASR::is_a<ASR::Function_t>(*fc->m_name)) {
                    ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(fc->m_name);
                    ASR::Module_t *m = ASR::down_cast2<ASR::Module_t>(f->m_symtab->parent->asr_owner);
                    char *modname = m->m_name;
                    ASR::symbol_t *new_es;
                    std::string unique_name = current_scope->get_unique_name(f->m_name);
                    Str s; s.from_str_view(unique_name);
                    char *unique_name_c = s.c_str(al);
                    LFORTRAN_ASSERT(current_scope->scope.find(unique_name) == current_scope->scope.end());
                    new_es = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
                        al, f->base.base.loc,
                        /* a_symtab */ current_scope,
                        /* a_name */ unique_name_c,
                        (ASR::symbol_t*)f,
                        modname, nullptr, 0,
                        f->m_name,
                        ASR::accessType::Private
                        ));
                    current_scope->scope[unique_name] = new_es;
                    Vec<ASR::expr_t*> args;
                    args.reserve(al, fc->n_args);
                    for (size_t i=0; i < fc->n_args; i++) {
                        ASR::expr_t *arg = fc->m_args[i];
                        if (ASR::is_a<ASR::Var_t>(*arg)) {
                            ASR::Var_t *var = ASR::down_cast<ASR::Var_t>(arg);
                            if (ASR::is_a<ASR::Variable_t>(*var->m_v)) {
                                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(var->m_v);
                                ASR::symbol_t *new_v;
                                std::string unique_name = current_scope->get_unique_name(v->m_name);
                                Str s; s.from_str_view(unique_name);
                                char *unique_name_c = s.c_str(al);
                                LFORTRAN_ASSERT(current_scope->scope.find(unique_name) == current_scope->scope.end());

                                Vec<char*> scope_names0 = ASRUtils::get_scope_names(al, v->m_parent_symtab);
                                LFORTRAN_ASSERT(scope_names0.size() >= 1)
                                char *modname = scope_names0[scope_names0.size()-1];
                                Vec<char*>  scope_names;
                                scope_names.reserve(al, scope_names0.size()-1);
                                for (size_t i=0; i < scope_names0.size()-1; i++) {
                                    scope_names.push_back(al, scope_names0[scope_names0.size()-i-2]);
                                }
                                new_v = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
                                    al, v->base.base.loc,
                                    /* a_symtab */ current_scope,
                                    /* a_name */ unique_name_c,
                                    (ASR::symbol_t*)v,
                                    modname, scope_names.p, scope_names.size(),
                                    v->m_name,
                                    ASR::accessType::Private
                                    ));
                                current_scope->scope[unique_name] = new_v;
                                arg = ASR::down_cast<ASR::expr_t>(ASR::make_Var_t(al, arg->base.loc, new_v));
                            }
                        }
                        args.push_back(al, arg);
                    }
                    ASR::expr_t *new_len_expr = ASR::down_cast<ASR::expr_t>(ASR::make_FunctionCall_t(
                        al, fc->base.base.loc, new_es, nullptr, args.p, args.n, fc->m_keywords, fc->n_keywords, fc->m_type, fc->m_value, fc->m_dt));
                    return ASR::down_cast<ASR::ttype_t>(
                        ASR::make_Character_t(al, t->base.base.loc,
                            t->m_kind, t->m_len, new_len_expr, t->m_dims, t->n_dims)
                    );
                }
            } else {
                throw SemanticError("Currently only FunctionCall is supported in character's len expression in ExternalSymbol", loc);
            }
        }
        return return_type;
    }

    ASR::asr_t* symbol_resolve_external_generic_procedure(
                const Location &loc,
                ASR::symbol_t *v,
                Vec<ASR::expr_t*> args) {
        ASR::ExternalSymbol_t *p = ASR::down_cast<ASR::ExternalSymbol_t>(v);
        ASR::symbol_t *f2 = ASR::down_cast<ASR::ExternalSymbol_t>(v)->m_external;
        ASR::GenericProcedure_t *g = ASR::down_cast<ASR::GenericProcedure_t>(f2);
        int idx = select_generic_procedure(args, *g, loc);
        ASR::symbol_t *final_sym;
        final_sym = g->m_procs[idx];
        if (!ASR::is_a<ASR::Function_t>(*final_sym)) {
            throw SemanticError("ExternalSymbol must point to a Function", loc);
        }
        ASR::ttype_t *return_type = LFortran::ASRUtils::EXPR2VAR(ASR::down_cast<ASR::Function_t>(final_sym)->m_return_var)->m_type;
        // Create ExternalSymbol for the final subroutine:
        // We mangle the new ExternalSymbol's local name as:
        //   generic_procedure_local_name @
        //     specific_procedure_remote_name
        std::string local_sym = std::string(p->m_name) + "@"
            + LFortran::ASRUtils::symbol_name(final_sym);
        if (current_scope->scope.find(local_sym)
            == current_scope->scope.end()) {
            Str name;
            name.from_str(al, local_sym);
            char *cname = name.c_str(al);
            ASR::asr_t *sub = ASR::make_ExternalSymbol_t(
                al, g->base.base.loc,
                /* a_symtab */ current_scope,
                /* a_name */ cname,
                final_sym,
                p->m_module_name, nullptr, 0, LFortran::ASRUtils::symbol_name(final_sym),
                ASR::accessType::Private
                );
            final_sym = ASR::down_cast<ASR::symbol_t>(sub);
            current_scope->scope[local_sym] = final_sym;
        } else {
            final_sym = current_scope->scope[local_sym];
        }
        ASR::expr_t *value = nullptr;
        ASR::symbol_t* final_sym2 = LFortran::ASRUtils::symbol_get_past_external(final_sym);
        if (ASR::is_a<ASR::Function_t>(*final_sym2)) {
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(final_sym2);
            if (ASRUtils::is_intrinsic_function(f)) {
                ASR::symbol_t* v2 = LFortran::ASRUtils::symbol_get_past_external(v);
                ASR::GenericProcedure_t *gp = ASR::down_cast<ASR::GenericProcedure_t>(v2);

                ASR::asr_t *result = intrinsic_function_transformation(al, loc, gp->m_name, args);
                if (result) {
                    return result;
                } else {
                    value = intrinsic_procedures.comptime_eval(gp->m_name, al, loc, args);
                }
            }
        }
        return ASR::make_FunctionCall_t(al, loc,
            final_sym, v, args.p, args.size(), nullptr, 0, return_type,
            value, nullptr);
    }

    ASR::asr_t* create_ClassProcedure(const Location &loc,
                AST::fnarg_t* m_args, size_t n_args,
                    ASR::symbol_t *v,
                    ASR::expr_t *v_expr) {
        Vec<ASR::expr_t*> args = visit_expr_list(m_args, n_args);
        ASR::ttype_t *type = nullptr;
        ASR::ClassProcedure_t *v_class_proc = ASR::down_cast<ASR::ClassProcedure_t>(v);
        type = LFortran::ASRUtils::EXPR2VAR(ASR::down_cast<ASR::Function_t>(v_class_proc->m_proc)->m_return_var)->m_type;
        return ASR::make_FunctionCall_t(al, loc,
                v, nullptr, args.p, args.size(), nullptr, 0, type, nullptr,
                v_expr);
    }

    ASR::asr_t* create_GenericProcedure(const Location &loc,
                Vec<ASR::expr_t*> args,
                    ASR::symbol_t *v) {
        if (ASR::is_a<ASR::ExternalSymbol_t>(*v)) {
            return symbol_resolve_external_generic_procedure(loc, v,
                    args);
        } else {
            ASR::GenericProcedure_t *p = ASR::down_cast<ASR::GenericProcedure_t>(v);
            int idx = select_generic_procedure(args, *p, loc);
            ASR::symbol_t *final_sym = p->m_procs[idx];

            ASR::ttype_t *type;
            type = LFortran::ASRUtils::EXPR2VAR(ASR::down_cast<ASR::Function_t>(final_sym)->m_return_var)->m_type;
            return ASR::make_FunctionCall_t(al, loc,
                final_sym, v, args.p, args.size(), nullptr, 0, type, nullptr,
                nullptr);
        }
    }

    ASR::asr_t* create_Function(const Location &loc,
                Vec<ASR::expr_t*> args,
                    ASR::symbol_t *v) {
        ASR::symbol_t *f2 = ASRUtils::symbol_get_past_external(v);
        ASR::ttype_t *return_type = ASRUtils::EXPR2VAR(ASR::down_cast<ASR::Function_t>(f2)->m_return_var)->m_type;
        ASR::expr_t* value = nullptr;
        if (ASR::is_a<ASR::ExternalSymbol_t>(*v)) {
            if (ASR::is_a<ASR::Character_t>(*return_type)) {
                return_type = handle_character_return(return_type, loc);
            }

            // Populate value
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(f2);
            if (ASRUtils::is_intrinsic_function(f)) {
                ASR::asr_t* result = intrinsic_function_transformation(al, loc, f->m_name, args);
                if (result) {
                    return result;
                } else {
                    value = intrinsic_procedures.comptime_eval(f->m_name, al, loc, args);
                }
            }
        }
        return ASR::make_FunctionCall_t(al, loc, v, nullptr,
            args.p, args.size(), nullptr, 0, return_type, value, nullptr);
    }

    // `fn` is a local Function or GenericProcedure (that resolves to a
    // Function), or an ExternalSymbol that points to a Function or
    // GenericProcedure (that resolves to a Function). This function resolves
    // the GeneralProcedure (based on `args`) and repacks the final function
    // into an ExternalSymbol if needed. It returns a FunctionCall ASR node.
    //
    // `args` are arguments of the function call as a list of `expr` nodes.
    //
    // If `fn` is intrinsic, it will also try to evaluate it into the `value`
    // member of the returned `FunctionCall`.
    ASR::asr_t* create_FunctionCall(const Location &loc,
                ASR::symbol_t *v, Vec<ASR::expr_t*> args) {
        ASR::symbol_t *f2 = ASRUtils::symbol_get_past_external(v);
        if (ASR::is_a<ASR::Function_t>(*f2)) {
            return create_Function(loc, args, v);
        } else {
            LFORTRAN_ASSERT(ASR::is_a<ASR::GenericProcedure_t>(*f2))
            return create_GenericProcedure(loc, args, v);
        }
    }

    ASR::asr_t* resolve_variable2(const Location &loc, const std::string &var_name,
            const std::string &dt_name, SymbolTable*& scope) {
        ASR::symbol_t *v = scope->resolve_symbol(dt_name);
        if (!v) {
            throw SemanticError("Variable '" + dt_name + "' not declared", loc);
        }
        ASR::Variable_t* v_variable = ASR::down_cast<ASR::Variable_t>(v);
        if (ASR::is_a<ASR::Derived_t>(*v_variable->m_type) ||
                ASR::is_a<ASR::DerivedPointer_t>(*v_variable->m_type) ||
                ASR::is_a<ASR::Class_t>(*v_variable->m_type)) {
            ASR::ttype_t* v_type = v_variable->m_type;
            ASR::symbol_t *derived_type = nullptr;
            if (ASR::is_a<ASR::Derived_t>(*v_type)) {
                derived_type = ASR::down_cast<ASR::Derived_t>(v_type)->m_derived_type;
            } else if (ASR::is_a<ASR::DerivedPointer_t>(*v_type)) {
                derived_type = ASR::down_cast<ASR::DerivedPointer_t>(v_type)->m_derived_type;
            } else if (ASR::is_a<ASR::Class_t>(*v_type)) {
                derived_type = ASR::down_cast<ASR::Class_t>(v_type)->m_class_type;
            }
            ASR::DerivedType_t *der_type;
            if (ASR::is_a<ASR::ExternalSymbol_t>(*derived_type)) {
                ASR::ExternalSymbol_t* der_ext = ASR::down_cast<ASR::ExternalSymbol_t>(derived_type);
                ASR::symbol_t* der_sym = der_ext->m_external;
                if (der_sym == nullptr) {
                    throw SemanticError("'" + std::string(der_ext->m_name) + "' isn't a Derived type.", loc);
                } else {
                    der_type = ASR::down_cast<ASR::DerivedType_t>(der_sym);
                }
            } else {
                der_type = ASR::down_cast<ASR::DerivedType_t>(derived_type);
            }
            ASR::DerivedType_t *par_der_type = der_type;
            // scope = der_type->m_symtab;
            // ASR::symbol_t* member = der_type->m_symtab->resolve_symbol(var_name);
            ASR::symbol_t* member = nullptr;
            while( par_der_type != nullptr && member == nullptr ) {
                scope = par_der_type->m_symtab;
                member = par_der_type->m_symtab->resolve_symbol(var_name);
                if( par_der_type->m_parent != nullptr ) {
                    par_der_type = ASR::down_cast<ASR::DerivedType_t>(LFortran::ASRUtils::symbol_get_past_external(par_der_type->m_parent));
                } else {
                    par_der_type = nullptr;
                }
            }
            if( member != nullptr ) {
                ASR::asr_t* v_var = ASR::make_Var_t(al, loc, v);
                return ASRUtils::getDerivedRef_t(al, loc, v_var, member, current_scope);
            } else {
                throw SemanticError("Variable '" + dt_name + "' doesn't have any member named, '" + var_name + "'.", loc);
            }
        } else if (ASR::is_a<ASR::Complex_t>(*v_variable->m_type)) {
            if (var_name == "re") {
                ASR::expr_t *val = ASR::down_cast<ASR::expr_t>(ASR::make_Var_t(al, loc, v));
                int kind = ASRUtils::extract_kind_from_ttype_t(v_variable->m_type);
                ASR::ttype_t *dest_type = ASR::down_cast<ASR::ttype_t>(ASR::make_Real_t(al, loc, kind, nullptr, 0));
                ImplicitCastRules::set_converted_value(
                    al, loc, &val, v_variable->m_type, dest_type);
                return (ASR::asr_t*)val;
            } else if (var_name == "im") {
                ASR::expr_t *val = ASR::down_cast<ASR::expr_t>(ASR::make_Var_t(al, loc, v));
                ASR::symbol_t *fn_aimag = resolve_intrinsic_function(loc, "aimag");
                Vec<ASR::expr_t*> args;
                args.reserve(al, 1);
                args.push_back(al, val);
                ASR::asr_t *result = create_FunctionCall(loc, fn_aimag, args);
                return result;
            } else {
                throw SemanticError("Complex variable '" + dt_name + "' only has %re and %im members, not '" + var_name + "'", loc);
            }
        } else {
            throw SemanticError("Variable '" + dt_name + "' is not a derived type", loc);
        }
    }

    ASR::symbol_t* resolve_deriv_type_proc(const Location &loc, const std::string &var_name,
            const std::string &dt_name, SymbolTable*& scope) {
        ASR::symbol_t *v = scope->resolve_symbol(dt_name);
        if (!v) {
            throw SemanticError("Variable '" + dt_name + "' not declared", loc);
        }
        ASR::Variable_t* v_variable = ((ASR::Variable_t*)(&(v->base)));
        if ( v_variable->m_type->type == ASR::ttypeType::Derived ||
             v_variable->m_type->type == ASR::ttypeType::DerivedPointer ||
             v_variable->m_type->type == ASR::ttypeType::Class ) {
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
                return member;
            } else {
                throw SemanticError("Variable '" + dt_name + "' doesn't have any member named, '" + var_name + "'.", loc);
            }
        } else {
            throw SemanticError("Variable '" + dt_name + "' is not a derived type", loc);
        }
    }

    void visit_FuncCallOrArray(const AST::FuncCallOrArray_t &x) {
        SymbolTable *scope = current_scope;
        std::string var_name = to_lower(x.m_func);
        ASR::symbol_t *v = nullptr;
        ASR::expr_t *v_expr = nullptr;
        // If this is a type bound procedure (in a class) it won't be in the
        // main symbol table. Need to check n_member.
        if (x.n_member == 1) {
            ASR::symbol_t *obj = current_scope->resolve_symbol(x.m_member[0].m_name);
            ASR::asr_t *obj_var = ASR::make_Var_t(al, x.base.base.loc, obj);
            v_expr = LFortran::ASRUtils::EXPR(obj_var);
            v = resolve_deriv_type_proc(x.base.base.loc, var_name,
                x.m_member[0].m_name, scope);
        } else {
            v = current_scope->resolve_symbol(var_name);
        }
        if (!v) {
            v = resolve_intrinsic_function(x.base.base.loc, var_name);
        }
        ASR::symbol_t *f2 = ASRUtils::symbol_get_past_external(v);
        if (ASR::is_a<ASR::Function_t>(*f2) || ASR::is_a<ASR::GenericProcedure_t>(*f2)) {
            Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
            tmp = create_FunctionCall(x.base.base.loc, v, args);
        } else {
            switch (f2->type) {
            case(ASR::symbolType::Variable):
                tmp = create_ArrayRef(x.base.base.loc, x.m_args, x.n_args, v, f2); break;
            case(ASR::symbolType::DerivedType):
                tmp = create_DerivedTypeConstructor(x.base.base.loc, x.m_args, x.n_args, v); break;
            case(ASR::symbolType::ClassProcedure):
                tmp = create_ClassProcedure(x.base.base.loc, x.m_args, x.n_args, v, v_expr); break;
            default: throw SemanticError("Symbol '" + var_name
                        + "' is not a function or an array", x.base.base.loc);
            }
        }
    }

    ASR::symbol_t* resolve_intrinsic_function(const Location &loc, const std::string &remote_sym) {
        if (!intrinsic_procedures.is_intrinsic(remote_sym)) {
            throw SemanticError("Function '" + remote_sym + "' not found"
                " or not implemented yet (if it is intrinsic)",
                loc);
        }
        std::string module_name = intrinsic_procedures.get_module(remote_sym, loc);

        SymbolTable *tu_symtab = ASRUtils::get_tu_symtab(current_scope);
        ASR::Module_t *m = ASRUtils::load_module(al, tu_symtab, module_name,
                loc, true);

        ASR::symbol_t *t = m->m_symtab->resolve_symbol(remote_sym);
        if (!t) {
            throw SemanticError("The symbol '" + remote_sym
                + "' not found in the module '" + module_name + "'",
                loc);
        } else if (! (ASR::is_a<ASR::GenericProcedure_t>(*t)
                    || ASR::is_a<ASR::Function_t>(*t))) {
            throw SemanticError("The symbol '" + remote_sym
                + "' found in the module '" + module_name + "', "
                + "but it is not a function or a generic function.",
                loc);
        }
        char *fn_name = ASRUtils::symbol_name(t);
        ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
            al, t->base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ fn_name,
            t,
            m->m_name, nullptr, 0, fn_name,
            ASR::accessType::Private
            );
        std::string sym = fn_name;

        current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(fn);
        ASR::symbol_t *v = ASR::down_cast<ASR::symbol_t>(fn);
        if (current_module) {
            // We are in body visitor
            // Add the module `m` to current module dependencies
            Vec<char*> vec;
            vec.from_pointer_n_copy(al, current_module->m_dependencies,
                        current_module->n_dependencies);
            if (!present(vec, m->m_name)) {
                vec.push_back(al, m->m_name);
                current_module->m_dependencies = vec.p;
                current_module->n_dependencies = vec.size();
            }
        } else {
            // We are in the symtab visitor or body visitor (the
            // current_module_dependencies is not used in body visitor)
            if (!present(current_module_dependencies, m->m_name)) {
                current_module_dependencies.push_back(al, m->m_name);
            }
        }
        return v;
    }

    void visit_BinOp(const AST::BinOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(tmp);
        CommonVisitorMethods::visit_BinOp(al, x, left, right, tmp, binop2str[x.m_op], current_scope);
    }

    void visit_BoolOp(const AST::BoolOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(tmp);
        CommonVisitorMethods::visit_BoolOp(al, x, left, right, tmp);
    }

    void visit_StrOp(const AST::StrOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(tmp);
        CommonVisitorMethods::visit_StrOp(al, x, left, right, tmp);
    }

    void visit_UnaryOp(const AST::UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        ASR::expr_t *operand = LFortran::ASRUtils::EXPR(tmp);
        CommonVisitorMethods::visit_UnaryOp(al, x, operand, tmp);
    }

    void visit_Compare(const AST::Compare_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(tmp);
        CommonVisitorMethods::visit_Compare(al, x, left, right, tmp,
                                            cmpop2str[x.m_op], current_scope);
    }

    void visit_Parenthesis(const AST::Parenthesis_t &x) {
        this->visit_expr(*x.m_operand);
    }

    void visit_Logical(const AST::Logical_t &x) {
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc,
                4, nullptr, 0));
        tmp = ASR::make_ConstantLogical_t(al, x.base.base.loc, x.m_value, type);
    }

    void visit_String(const AST::String_t &x) {
        int s_len = strlen(x.m_s);
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                1, s_len, nullptr, nullptr, 0));
        tmp = ASR::make_ConstantString_t(al, x.base.base.loc, x.m_s, type);
    }

    void visit_Num(const AST::Num_t &x) {
        int ikind = 4;
        if (x.m_kind) {
            ikind = std::atoi(x.m_kind);
            if (ikind == 0) {
                std::string var_name = x.m_kind;
                ASR::symbol_t *v = current_scope->resolve_symbol(var_name);
                if (v) {
                    const ASR::symbol_t *v3 = LFortran::ASRUtils::symbol_get_past_external(v);
                    if (ASR::is_a<ASR::Variable_t>(*v3)) {
                        ASR::Variable_t *v2 = ASR::down_cast<ASR::Variable_t>(v3);
                        if (v2->m_value) {
                            if (ASR::is_a<ASR::ConstantInteger_t>(*v2->m_value)) {
                                ikind = ASR::down_cast<ASR::ConstantInteger_t>(v2->m_value)->m_n;
                            } else {
                                throw SemanticError("Variable '" + var_name + "' is constant but not an integer",
                                    x.base.base.loc);
                            }
                        } else {
                            throw SemanticError("Variable '" + var_name + "' is not constant",
                                x.base.base.loc);
                        }
                    } else {
                        throw SemanticError("Symbol '" + var_name + "' is not a variable",
                            x.base.base.loc);
                    }
                } else {
                    throw SemanticError("Variable '" + var_name + "' not declared",
                        x.base.base.loc);
                }
            }
        }
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al,
                x.base.base.loc, ikind, nullptr, 0));
        if (BigInt::is_int_ptr(x.m_n)) {
            throw SemanticError("Integer constants larger than 2^62-1 are not implemented yet", x.base.base.loc);
        } else {
            LFORTRAN_ASSERT(!BigInt::is_int_ptr(x.m_n));
            tmp = ASR::make_ConstantInteger_t(al, x.base.base.loc, x.m_n, type);
        }
    }

    void visit_Real(const AST::Real_t &x) {
        double r = ASRUtils::extract_real(x.m_n);
        char* s_kind;
        int r_kind = ASRUtils::extract_kind_str(x.m_n, s_kind);
        if (r_kind == 0) {
            std::string var_name = s_kind;
            ASR::symbol_t *v = current_scope->resolve_symbol(var_name);
            if (v) {
                const ASR::symbol_t *v3 = LFortran::ASRUtils::symbol_get_past_external(v);
                if (ASR::is_a<ASR::Variable_t>(*v3)) {
                    ASR::Variable_t *v2 = ASR::down_cast<ASR::Variable_t>(v3);
                    if (v2->m_value) {
                        if (ASR::is_a<ASR::ConstantInteger_t>(*v2->m_value)) {
                            r_kind = ASR::down_cast<ASR::ConstantInteger_t>(v2->m_value)->m_n;
                        } else {
                            throw SemanticError("Variable '" + var_name + "' is constant but not an integer",
                                x.base.base.loc);
                        }
                    } else {
                        throw SemanticError("Variable '" + var_name + "' is not constant",
                            x.base.base.loc);
                    }
                } else {
                    throw SemanticError("Symbol '" + var_name + "' is not a variable",
                        x.base.base.loc);
                }
            } else {
                throw SemanticError("Variable '" + var_name + "' not declared",
                    x.base.base.loc);
            }
        }
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Real_t(al, x.base.base.loc,
                r_kind, nullptr, 0));
        tmp = ASR::make_ConstantReal_t(al, x.base.base.loc, r, type);
    }

    void visit_Complex(const AST::Complex_t &x) {
        this->visit_expr(*x.m_re);
        ASR::expr_t *re = ASRUtils::EXPR(tmp);
        ASR::expr_t *re_value = ASRUtils::expr_value(re);
        int a_kind_r = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(re));
        this->visit_expr(*x.m_im);
        ASR::expr_t *im = ASRUtils::EXPR(tmp);
        ASR::expr_t *im_value = ASRUtils::expr_value(im);
        int a_kind_i = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(im));
        // TODO: Add semantic checks what type are allowed
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Complex_t(al, x.base.base.loc,
                std::max(a_kind_r, a_kind_i), nullptr, 0));
        ASR::expr_t *value = nullptr;
        if (re_value && im_value) {
            double re_double;
            if (ASR::is_a<ASR::ConstantReal_t>(*re_value)) {
                re_double = ASR::down_cast<ASR::ConstantReal_t>(re_value)->m_r;
            } else if (ASR::is_a<ASR::ConstantInteger_t>(*re_value)) {
                re_double = ASR::down_cast<ASR::ConstantInteger_t>(re_value)->m_n;
            } else {
                throw SemanticError("Argument `a` in a ComplexConstructor `(a,b)` must be either Real or Integer", x.base.base.loc);
            }
            double im_double;
            if (ASR::is_a<ASR::ConstantReal_t>(*im_value)) {
                im_double = ASR::down_cast<ASR::ConstantReal_t>(im_value)->m_r;
            } else if (ASR::is_a<ASR::ConstantInteger_t>(*im_value)) {
                im_double = ASR::down_cast<ASR::ConstantInteger_t>(im_value)->m_n;
            } else {
                throw SemanticError("Argument `b` in a ComplexConstructor `(a,b)` must be either Real or Integer", x.base.base.loc);
            }
            value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantComplex_t(al, x.base.base.loc, re_double, im_double, type));
        }
        tmp = ASR::make_ComplexConstructor_t(al, x.base.base.loc,
                re, im, type, value);
    }

    Vec<ASR::expr_t*> visit_expr_list(AST::fnarg_t *ast_list, size_t n) {
        Vec<ASR::expr_t*> asr_list;
        asr_list.reserve(al, n);
        for (size_t i=0; i<n; i++) {
            LFORTRAN_ASSERT(ast_list[i].m_end != nullptr);
            this->visit_expr(*ast_list[i].m_end);
            ASR::expr_t *expr = LFortran::ASRUtils::EXPR(tmp);
            asr_list.push_back(al, expr);
        }
        return asr_list;
    }

    void visit_Name(const AST::Name_t &x) {
        if (x.n_member == 0) {
            tmp = resolve_variable(x.base.base.loc, to_lower(x.m_id));
        } else if (x.n_member == 1) {
            if (x.m_member[0].n_args == 0) {
                SymbolTable* scope = current_scope;
                tmp = this->resolve_variable2(x.base.base.loc, to_lower(x.m_id),
                    to_lower(x.m_member[0].m_name), scope);
            } else {
                // TODO: incorporate m_args
                SymbolTable* scope = current_scope;
                tmp = this->resolve_variable2(x.base.base.loc, to_lower(x.m_id),
                    to_lower(x.m_member[0].m_name), scope);
            }
        } else {
            SymbolTable* scope = current_scope;
            tmp = this->resolve_variable2(x.base.base.loc, to_lower(x.m_member[1].m_name), to_lower(x.m_member[0].m_name), scope);
            ASR::DerivedRef_t* tmp2;
            std::uint32_t i;
            for( i = 2; i < x.n_member; i++ ) {
                tmp2 = (ASR::DerivedRef_t*)this->resolve_variable2(x.base.base.loc,
                                            to_lower(x.m_member[i].m_name), to_lower(x.m_member[i - 1].m_name), scope);
                tmp = ASR::make_DerivedRef_t(al, x.base.base.loc, LFortran::ASRUtils::EXPR(tmp), tmp2->m_m, tmp2->m_type, nullptr);
            }
            i = x.n_member - 1;
            tmp2 = (ASR::DerivedRef_t*)this->resolve_variable2(x.base.base.loc, to_lower(x.m_id), to_lower(x.m_member[i].m_name), scope);
            tmp = ASR::make_DerivedRef_t(al, x.base.base.loc, LFortran::ASRUtils::EXPR(tmp), tmp2->m_m, tmp2->m_type, nullptr);
        }
    }

    // Transforms intrinsics real(),int() to ImplicitCast. Return true if `f` is
    // real/int (result in `tmp`), false otherwise (`tmp` unchanged)
    ASR::asr_t* intrinsic_function_transformation(Allocator &al, const Location &loc,
            const std::string &fn_name, Vec<ASR::expr_t*> &args) {
        if (fn_name == "real") {
            // real(), int() are represented using ExplicitCast
            // (for now we use ImplicitCast) in ASR, so we save them
            // to tmp and exit:
            ASR::expr_t *arg1;
            if (args.size() == 1) {
                arg1 = nullptr;
            } else if (args.size() == 2) {
                arg1 = args[1];
            } else {
                throw SemanticError("real(...) must have 1 or 2 arguments", loc);
            }
            return CommonVisitorMethods::comptime_intrinsic_real(args[0], arg1, al, loc);
        } else {
            return nullptr;
        }
    }

    bool select_func_subrout(const ASR::symbol_t* proc, const Vec<ASR::expr_t*> &args,
                             Location& loc) {
        bool result = false;
        if (ASR::is_a<ASR::Subroutine_t>(*proc)) {
            ASR::Subroutine_t *sub
                = ASR::down_cast<ASR::Subroutine_t>(proc);
            if (argument_types_match(args, *sub)) {
                result = true;
            }
        } else if (ASR::is_a<ASR::Function_t>(*proc)) {
            ASR::Function_t *fn
                = ASR::down_cast<ASR::Function_t>(proc);
            if (argument_types_match(args, *fn)) {
                result = true;
            }
        } else {
            throw SemanticError("Only Subroutine and Function supported in generic procedure", loc);
        }
        return result;
    }


    int select_generic_procedure(const Vec<ASR::expr_t*> &args,
            const ASR::GenericProcedure_t &p, Location loc) {
        for (size_t i=0; i < p.n_procs; i++) {
            if( ASR::is_a<ASR::ClassProcedure_t>(*p.m_procs[i]) ) {
                ASR::ClassProcedure_t *clss_fn 
                    = ASR::down_cast<ASR::ClassProcedure_t>(p.m_procs[i]);
                const ASR::symbol_t *proc = ASRUtils::symbol_get_past_external(clss_fn->m_proc);
                if( select_func_subrout(proc, args, loc) ) {
                    return i;
                }
            } else {
                if( select_func_subrout(p.m_procs[i], args, loc) ) {
                    return i;
                }
            }
        }
        throw SemanticError("Arguments do not match for any generic procedure", loc);
    }

    template <typename T>
    bool argument_types_match(const Vec<ASR::expr_t*> &args,
            const T &sub) {
        if (args.size() == sub.n_args) {
            for (size_t i=0; i < args.size(); i++) {
                ASR::Variable_t *v = LFortran::ASRUtils::EXPR2VAR(sub.m_args[i]);
                ASR::ttype_t *arg1 = LFortran::ASRUtils::expr_type(args[i]);
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
        if ((a.type == ASR::ttypeType::Derived || a.type == ASR::ttypeType::Class) &&
            (b.type == ASR::ttypeType::Derived || b.type == ASR::ttypeType::Class)) {
            return true;
        }
        if (a.type == b.type) {
            // TODO: check dims
            // TODO: check all types
            switch (a.type) {
                case (ASR::ttypeType::Integer) : {
                    ASR::Integer_t *a2 = ASR::down_cast<ASR::Integer_t>(&a);
                    ASR::Integer_t *b2 = ASR::down_cast<ASR::Integer_t>(&b);
                    if (a2->m_kind == b2->m_kind) {
                        return true;
                    } else {
                        return false;
                    }
                    break;
                }
                case (ASR::ttypeType::Real) : {
                    ASR::Real_t *a2 = ASR::down_cast<ASR::Real_t>(&a);
                    ASR::Real_t *b2 = ASR::down_cast<ASR::Real_t>(&b);
                    if (a2->m_kind == b2->m_kind) {
                        return true;
                    } else {
                        return false;
                    }
                    break;
                }
                case (ASR::ttypeType::Complex) : {
                    ASR::Complex_t *a2 = ASR::down_cast<ASR::Complex_t>(&a);
                    ASR::Complex_t *b2 = ASR::down_cast<ASR::Complex_t>(&b);
                    if (a2->m_kind == b2->m_kind) {
                        return true;
                    } else {
                        return false;
                    }
                    break;
                }
                default : return false;
            }
        }
        return false;
    }

};

} // namespace LFortran

#endif /* LFORTRAN_SEMANTICS_AST_COMMON_VISITOR_H */
