#include <libasr/casting_utils.h>
#include <libasr/asr_utils.h>

#include <map>


namespace LCompilers::CastingUtil {

    // Data structure which contains priorities for
    // intrinsic types defined in ASR
    const std::map<ASR::ttypeType, int>& type2weight = {
        {ASR::ttypeType::Complex, 4},
        {ASR::ttypeType::Real, 3},
        {ASR::ttypeType::Integer, 2},
        {ASR::ttypeType::Logical, 1}
    };

    // Data structure which contains casting rules for non-equal
    // intrinsic types defined in ASR
    const std::map<std::pair<ASR::ttypeType, ASR::ttypeType>, ASR::cast_kindType>& type_rules = {
        {std::make_pair(ASR::ttypeType::Complex, ASR::ttypeType::Real), ASR::cast_kindType::ComplexToReal},
        {std::make_pair(ASR::ttypeType::Complex, ASR::ttypeType::Logical), ASR::cast_kindType::ComplexToLogical},
        {std::make_pair(ASR::ttypeType::Real, ASR::ttypeType::Complex), ASR::cast_kindType::RealToComplex},
        {std::make_pair(ASR::ttypeType::Real, ASR::ttypeType::Integer), ASR::cast_kindType::RealToInteger},
        {std::make_pair(ASR::ttypeType::Real, ASR::ttypeType::Logical), ASR::cast_kindType::RealToLogical},
        {std::make_pair(ASR::ttypeType::Real, ASR::ttypeType::UnsignedInteger), ASR::cast_kindType::RealToUnsignedInteger},
        {std::make_pair(ASR::ttypeType::Integer, ASR::ttypeType::Complex), ASR::cast_kindType::IntegerToComplex},
        {std::make_pair(ASR::ttypeType::Integer, ASR::ttypeType::Real), ASR::cast_kindType::IntegerToReal},
        {std::make_pair(ASR::ttypeType::Integer, ASR::ttypeType::Logical), ASR::cast_kindType::IntegerToLogical},
        {std::make_pair(ASR::ttypeType::Character, ASR::ttypeType::Integer), ASR::cast_kindType::CharacterToInteger},
        {std::make_pair(ASR::ttypeType::Integer, ASR::ttypeType::UnsignedInteger), ASR::cast_kindType::IntegerToUnsignedInteger},
        {std::make_pair(ASR::ttypeType::Logical, ASR::ttypeType::Real), ASR::cast_kindType::LogicalToReal},
        {std::make_pair(ASR::ttypeType::Logical, ASR::ttypeType::Integer), ASR::cast_kindType::LogicalToInteger},
        {std::make_pair(ASR::ttypeType::UnsignedInteger, ASR::ttypeType::Integer), ASR::cast_kindType::UnsignedIntegerToInteger},
        {std::make_pair(ASR::ttypeType::UnsignedInteger, ASR::ttypeType::Real), ASR::cast_kindType::UnsignedIntegerToReal},
        {std::make_pair(ASR::ttypeType::Integer, ASR::ttypeType::SymbolicExpression), ASR::cast_kindType::IntegerToSymbolicExpression}
    };

    // Data structure which contains casting rules for equal intrinsic
    // types but with different kinds.
    const std::map<ASR::ttypeType, ASR::cast_kindType>& kind_rules = {
        {ASR::ttypeType::Complex, ASR::cast_kindType::ComplexToComplex},
        {ASR::ttypeType::Real, ASR::cast_kindType::RealToReal},
        {ASR::ttypeType::Integer, ASR::cast_kindType::IntegerToInteger},
        {ASR::ttypeType::UnsignedInteger, ASR::cast_kindType::UnsignedIntegerToUnsignedInteger}
    };

    int get_type_priority(ASR::ttypeType type) {
        if( type2weight.find(type) == type2weight.end() ) {
            return -1;
        }

        return type2weight.at(type);
    }

    int get_src_dest(ASR::expr_t* left_expr, ASR::expr_t* right_expr,
                      ASR::expr_t*& src_expr, ASR::expr_t*& dest_expr,
                      ASR::ttype_t*& src_type, ASR::ttype_t*& dest_type,
                      bool is_assign) {
        ASR::ttype_t* left_type = ASRUtils::expr_type(left_expr);
        ASR::ttype_t* right_type = ASRUtils::expr_type(right_expr);
        if( ASR::is_a<ASR::Const_t>(*left_type) ) {
            left_type = ASRUtils::get_contained_type(left_type);
        }
        if( ASR::is_a<ASR::Const_t>(*right_type) ) {
            right_type = ASRUtils::get_contained_type(right_type);
        }
        left_type = ASRUtils::type_get_past_pointer(left_type);
        right_type = ASRUtils::type_get_past_pointer(right_type);
        if( ASRUtils::check_equal_type(left_type, right_type) ||
            ASRUtils::is_character(*left_type) || ASRUtils::is_character(*right_type) ) {
            return 2;
        }
        if( is_assign ) {
            if( ASRUtils::is_real(*left_type) && ASRUtils::is_integer(*right_type)) {
                throw SemanticError("Assigning integer to float is not supported",
                                    right_expr->base.loc);
            }
            if ( ASRUtils::is_complex(*left_type) && !ASRUtils::is_complex(*right_type)) {
                throw SemanticError("Assigning non-complex to complex is not supported",
                        right_expr->base.loc);
            }
            dest_expr = left_expr, dest_type = left_type;
            src_expr = right_expr, src_type = right_type;
            return 1;
        }

        int casted_expr_signal = 2;
        ASR::ttypeType left_Type = left_type->type, right_Type = right_type->type;
        int left_kind = ASRUtils::extract_kind_from_ttype_t(left_type);
        int right_kind = ASRUtils::extract_kind_from_ttype_t(right_type);
        int left_priority = get_type_priority(left_Type);
        int right_priority = get_type_priority(right_Type);
        if( left_priority > right_priority ) {
            src_expr = right_expr, src_type = right_type;
            dest_expr = left_expr, dest_type = left_type;
            casted_expr_signal = 1;
        } else if( left_priority < right_priority ) {
            src_expr = left_expr, src_type = left_type;
            dest_expr = right_expr, dest_type = right_type;
            casted_expr_signal = 0;
        } else {
            if( left_kind > right_kind ) {
                src_expr = right_expr, src_type = right_type;
                dest_expr = left_expr, dest_type = left_type;
                casted_expr_signal = 1;
            } else if( left_kind < right_kind ) {
                src_expr = left_expr, src_type = left_type;
                dest_expr = right_expr, dest_type = right_type;
                casted_expr_signal = 0;
            } else {
                return 2;
            }
        }

        return casted_expr_signal;
    }

    ASR::expr_t* perform_casting(ASR::expr_t* expr, ASR::ttype_t* src,
                                 ASR::ttype_t* dest, Allocator& al,
                                 const Location& loc) {
        ASR::ttypeType src_type = src->type;
        ASR::ttypeType dest_type = dest->type;
        ASR::cast_kindType cast_kind;
        if( src_type == dest_type ) {
            if( kind_rules.find(src_type) == kind_rules.end() ) {
                return expr;
            }
            cast_kind = kind_rules.at(src_type);
        } else {
            std::pair<ASR::ttypeType, ASR::ttypeType> cast_key = std::make_pair(src_type, dest_type);
            if( type_rules.find(cast_key) == type_rules.end() ) {
                return expr;
            }
            cast_kind = type_rules.at(cast_key);
        }
        if( ASRUtils::check_equal_type(src, dest, true) ) {
            return expr;
        }
        // TODO: Fix loc
        return ASRUtils::EXPR(ASRUtils::make_Cast_t_value(al, loc, expr,
                                                          cast_kind, dest));
    }
}
