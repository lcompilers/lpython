#ifndef LFORTRAN_SEMANTICS_AST_COMMON_VISITOR_H
#define LFORTRAN_SEMANTICS_AST_COMMON_VISITOR_H

#include <lfortran/asr.h>
#include <lfortran/ast.h>

namespace LFortran {
class CommonVisitorMethods {
public:
  inline static void visit_BinOp(Allocator &al, const AST::BinOp_t &x,
                                 ASR::expr_t *&left, ASR::expr_t *&right,
                                 ASR::asr_t *&asr) {
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
      }
    }
    asr = ASR::make_BinOp_t(al, x.base.base.loc, left, op, right, dest_type,
                            value);
  }

  inline static void visit_Compare(Allocator &al, const AST::Compare_t &x,
                                   ASR::expr_t *&left, ASR::expr_t *&right,
                                   ASR::asr_t *&asr) {
    // Cast LHS or RHS if necessary
    ASR::ttype_t *left_type = LFortran::ASRUtils::expr_type(left);
    ASR::ttype_t *right_type = LFortran::ASRUtils::expr_type(right);
    if ((left_type->type != ASR::ttypeType::Real &&
         left_type->type != ASR::ttypeType::Integer) &&
        (right_type->type != ASR::ttypeType::Real &&
         right_type->type != ASR::ttypeType::Integer) &&
        ((left_type->type != ASR::ttypeType::Complex ||
          right_type->type != ASR::ttypeType::Complex) &&
         x.m_op != AST::cmpopType::Eq && x.m_op != AST::cmpopType::NotEq)) {
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
                              value);
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
        left_type2->m_len + right_type2->m_len, nullptr, 0));

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

} // namespace LFortran

#endif /* LFORTRAN_SEMANTICS_AST_COMMON_VISITOR_H */
