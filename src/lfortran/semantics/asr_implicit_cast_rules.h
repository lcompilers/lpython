#ifndef LFORTRAN_SEMANTICS_ASR_IMPLICIT_CAST_RULES_H
#define LFORTRAN_SEMANTICS_ASR_IMPLICIT_CAST_RULES_H

#include <lfortran/asr.h>
#include <lfortran/ast.h>

#define num_types 12

namespace LFortran {
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
  static constexpr const char *type_names[num_types][2] = {
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
      {"Derived Pointer", "Integer"}};

  /*
   * Rule map for performing implicit cast represented by a 2D integer array.
   *
   * Key is the pair of indices with row index denoting the source type
   * and column index denoting the destination type.
   */
  static constexpr const int rule_map[num_types / 2][num_types] = {

      {integer_to_integer, integer_to_real, integer_to_complex, error_case,
       integer_to_logical, error_case, integer_to_integer, integer_to_real,
       integer_to_complex, error_case, integer_to_logical, error_case},
      {real_to_integer, real_to_real, real_to_complex, default_case,
       default_case, default_case, real_to_integer, real_to_real,
       real_to_complex, default_case, default_case, default_case},
      {default_case, default_case, complex_to_complex, default_case,
       default_case, default_case, default_case, default_case,
       complex_to_complex, default_case, default_case, default_case},
      {default_case, default_case, default_case, default_case, default_case,
       default_case, default_case, default_case, default_case, default_case,
       default_case, default_case},
      {default_case, default_case, default_case, default_case, default_case,
       default_case, default_case, default_case, default_case, default_case,
       default_case, default_case},
      {default_case, default_case, default_case, default_case, default_case,
       default_case, default_case, default_case, default_case, default_case,
       default_case, default_case}

  };

  /*
   * Priority of different types to be used in conversion
   * when source and destination are directly not deducible.
   */
  static constexpr const int type_priority[num_types / 2] = {
      4,  // Integer or IntegerPointer
      5,  // Real or RealPointer
      6,  // Complex or ComplexPointer
      -1, // Character or CharacterPointer
      -1, // Logical or LogicalPointer
      -1  // Derived or DerivedPointer
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
  static void set_converted_value(Allocator &al, const Location &a_loc,
                                  ASR::expr_t **convert_can,
                                  ASR::ttype_t *source_type,
                                  ASR::ttype_t *dest_type) {
    if (source_type->type == dest_type->type ||
        ASRUtils::is_same_type_pointer(source_type, dest_type)) {
      bool is_source_pointer = ASRUtils::is_pointer(source_type);
      bool is_dest_pointer = ASRUtils::is_pointer(dest_type);
      if (is_source_pointer && !is_dest_pointer) {
        ASR::ttype_t *temp = source_type;
        source_type = dest_type;
        dest_type = temp;
      }
      int source_kind = 0, dest_kind = 1;
      source_kind = LFortran::ASRUtils::extract_kind_from_ttype_t(source_type);
      dest_kind = LFortran::ASRUtils::extract_kind_from_ttype_t(dest_type);
      if (source_kind == dest_kind) {
        return;
      }
    }
    int cast_kind =
        rule_map[source_type->type % (num_types / 2)][dest_type->type];
    if (cast_kind == error_case) {
      std::string allowed_types_str = type_names[dest_type->type][1];
      std::string dest_type_str = type_names[dest_type->type][0];
      std::string error_msg =
          "Only " + allowed_types_str + " can be assigned to " + dest_type_str;
      throw SemanticError(error_msg, a_loc);
    } else if (cast_kind != default_case) {
      *convert_can = (ASR::expr_t *)ASR::make_ImplicitCast_t(
          al, a_loc, *convert_can, (ASR::cast_kindType)cast_kind, dest_type,
          nullptr);
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
  static void find_conversion_candidate(ASR::expr_t **left, ASR::expr_t **right,
                                        ASR::ttype_t *left_type,
                                        ASR::ttype_t *right_type,
                                        ASR::expr_t **&conversion_cand,
                                        ASR::ttype_t **source_type,
                                        ASR::ttype_t **dest_type) {

    int left_type_p = type_priority[left_type->type % (num_types / 2)];
    int right_type_p = type_priority[right_type->type % (num_types / 2)];
    if (left_type_p >= right_type_p) {
      conversion_cand = right;
      *source_type = right_type;
      *dest_type = left_type;
    } else {
      conversion_cand = left;
      *source_type = left_type;
      *dest_type = right_type;
    }
  }
};
} // namespace LFortran

#endif /* LFORTRAN_SEMANTICS_ASR_IMPLICIT_CAST_RULES_H */
