#ifndef LFORTRAN_SEMANTICS_ASR_COMMON_VISITOR_H
#define LFORTRAN_SEMANTICS_ASR_COMMON_VISITOR_H

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
    asr = ASR::make_Compare_t(al, x.base.base.loc, left, asr_op, right, type,
                              nullptr);
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
    asr = ASR::make_BoolOp_t(al, x.base.base.loc, left, op, right, dest_type,
                             nullptr);
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
    asr = ASR::make_UnaryOp_t(al, x.base.base.loc, op, operand, operand_type,
                              nullptr);
  }

  static inline void visit_StrOp(Allocator &al, const AST::StrOp_t &x,
                                 ASR::expr_t *&left, ASR::expr_t *&right,
                                 ASR::asr_t *&asr) {
    ASR::stropType op;
    switch (x.m_op) {
    case (AST::Concat):
      op = ASR::Concat;
    }
    ASR::ttype_t *right_type = LFortran::ASRUtils::expr_type(right);
    ASR::ttype_t *dest_type = right_type;
    // TODO: Type check here?
    asr = ASR::make_StrOp_t(al, x.base.base.loc, left, op, right, dest_type,
                            nullptr);
  }
};
} // namespace LFortran

#endif /* LFORTRAN_SEMANTICS_ASR_COMMON_VISITOR_H */
