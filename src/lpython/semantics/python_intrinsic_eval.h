#ifndef LPYTHON_INTRINSIC_EVAL_H
#define LPYTHON_INTRINSIC_EVAL_H


#include <libasr/asr.h>
#include <libasr/string_utils.h>
#include <lpython/utils.h>
#include <lpython/semantics/semantic_exception.h>

namespace LCompilers::LPython {

struct IntrinsicNodeHandler {

    typedef ASR::asr_t* (*intrinsic_eval_callback)(Allocator &, Vec<ASR::call_arg_t>,
                                        const Location &);

    std::map<std::string, intrinsic_eval_callback> intrinsic_map;

    IntrinsicNodeHandler() {
        intrinsic_map = {
            {"int", &handle_intrinsic_int},
            {"float", &handle_intrinsic_float},
            {"str", &handle_intrinsic_str},
            {"bool", &handle_intrinsic_bool},
            {"len", &handle_intrinsic_len},
            {"reshape", &handle_reshape},
            {"ord", &handle_intrinsic_ord},
            {"chr", &handle_intrinsic_chr},
        };
    }

    bool is_present(std::string call_name) {
        return intrinsic_map.find(call_name) != intrinsic_map.end();
    }

    ASR::asr_t* get_intrinsic_node(std::string call_name,
            Allocator &al, const Location &loc, Vec<ASR::call_arg_t> args) {
        auto search = intrinsic_map.find(call_name);
        if (search != intrinsic_map.end()) {
            intrinsic_eval_callback cb = search->second;
            return cb(al, args, loc);
        } else {
            throw SemanticError(call_name + " is not implemented yet",
                loc);
        }
    }

    static ASR::asr_t* handle_intrinsic_int(Allocator &al, Vec<ASR::call_arg_t> args,
                                        const Location &loc) {
        ASR::expr_t *arg = nullptr, *value = nullptr;
        ASR::ttype_t *type = nullptr;
        if (args.size() > 1) {
            throw SemanticError("Either 0 or 1 argument is expected in 'int()'",
                    loc);
        }
        if (args.size() > 0) {
            arg = args[0].m_value;
            type = ASRUtils::expr_type(arg);
        }
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 8));
        if (!arg) {
            return ASR::make_IntegerConstant_t(al, loc, 0, to_type);
        }
        if (ASRUtils::is_real(*type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                int32_t ival = ASR::down_cast<ASR::RealConstant_t>(
                                        ASRUtils::expr_value(arg))->m_r;
                value =  ASR::down_cast<ASR::expr_t>(make_IntegerConstant_t(al,
                                loc, ival, to_type));
            }
            return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                al, loc, arg, ASR::cast_kindType::RealToInteger,
                to_type, value));
        } else if (ASRUtils::is_character(*type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                char *c = ASR::down_cast<ASR::StringConstant_t>(
                                    ASRUtils::expr_value(arg))->m_s;
                int ival = 0;
                char *ch = c;
                if (*ch == '-') {
                    ch++;
                }
                while (*ch) {
                    if (*ch == '.') {
                        throw SemanticError("invalid literal for int() with base 10: '"+ std::string(c) + "'", arg->base.loc);
                    }
                    if (*ch < '0' || *ch > '9') {
                        throw SemanticError("invalid literal for int() with base 10: '"+ std::string(c) + "'", arg->base.loc);
                    }
                    ch++;
                }
                ival = std::stoi(c);
                return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(al,
                                loc, ival, to_type));
            }
            return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                al, loc, arg, ASR::cast_kindType::CharacterToInteger,
                to_type, value));
        } else if (ASRUtils::is_logical(*type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                int32_t ival = ASR::down_cast<ASR::LogicalConstant_t>(
                                        ASRUtils::expr_value(arg))->m_value;
                value =  ASR::down_cast<ASR::expr_t>(make_IntegerConstant_t(al,
                                loc, ival, to_type));
            }
            return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                al, loc, arg, ASR::cast_kindType::LogicalToInteger,
                to_type, value));
        } else if (ASRUtils::is_integer(*type)) {
            // int() returns a 64-bit integer
            if (ASRUtils::extract_kind_from_ttype_t(type) != 8) {
                if (ASRUtils::expr_value(arg) != nullptr) {
                    int64_t ival = ASR::down_cast<ASR::IntegerConstant_t>(
                                            ASRUtils::expr_value(arg))->m_n;
                    value =  ASR::down_cast<ASR::expr_t>(make_IntegerConstant_t(al,
                                    loc, ival, to_type));
                }
                return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                    al, loc, arg, ASR::cast_kindType::IntegerToInteger,
                    to_type, value));
            }
            return (ASR::asr_t *)arg;
        } else {
            std::string stype = ASRUtils::type_to_str_python(type);
            throw SemanticError(
                "Conversion of '" + stype + "' to integer is not Implemented",
                loc);
        }
        // TODO: Make this work if the argument is, let's say, a class.
        return nullptr;
    }


    static ASR::asr_t* handle_intrinsic_float(Allocator &al, Vec<ASR::call_arg_t> args,
                                        const Location &loc) {
        ASR::expr_t *arg = nullptr, *value = nullptr;
        ASR::ttype_t *type = nullptr;
        if (args.size() > 1) {
            throw SemanticError("Either 0 or 1 argument is expected in 'float()'",
                    loc);
        }
        if (args.size() > 0) {
            arg = args[0].m_value;
            type = ASRUtils::expr_type(arg);
        }
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, 8));
        if (!arg) {
            return ASR::make_RealConstant_t(al, loc, 0.0, to_type);
        }
        if (ASRUtils::is_integer(*type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                double dval = ASR::down_cast<ASR::IntegerConstant_t>(
                                        ASRUtils::expr_value(arg))->m_n;
                value =  ASR::down_cast<ASR::expr_t>(make_RealConstant_t(al,
                                loc, dval, to_type));
            }
            return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                al, loc, arg, ASR::cast_kindType::IntegerToReal,
                to_type, value));
        } else if (ASRUtils::is_logical(*type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                double dval = ASR::down_cast<ASR::LogicalConstant_t>(
                                        ASRUtils::expr_value(arg))->m_value;
                value =  ASR::down_cast<ASR::expr_t>(make_RealConstant_t(al,
                                loc, dval, to_type));
            }
            return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                al, loc, arg, ASR::cast_kindType::LogicalToReal,
                to_type, value));
        } else if (ASRUtils::is_real(*type)) {
            // float() always returns 64-bit floating point numbers.
            if (ASRUtils::extract_kind_from_ttype_t(type) != 8) {
                return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                    al, loc, arg, ASR::cast_kindType::RealToReal,
                    to_type, value));
            }
            return (ASR::asr_t *)arg;
        } else {
            std::string stype = ASRUtils::type_to_str_python(type);
            throw SemanticError(
                "Conversion of '" + stype + "' to float is not Implemented",
                loc);
        }
        // TODO: Make this work if the argument is, let's say, a class.
        return nullptr;
    }

    static ASR::asr_t* handle_intrinsic_bool(Allocator &al, Vec<ASR::call_arg_t> args,
                                        const Location &loc) {
        if (args.size() > 1) {
            throw SemanticError("Either 0 or 1 argument is expected in 'bool()'",
                    loc);
        }
        ASR::expr_t *arg = nullptr, *value = nullptr;
        ASR::ttype_t *type = nullptr;
        if (args.size() > 0) {
            arg = args[0].m_value;
            type = ASRUtils::expr_type(arg);
        }
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4));
        if (!arg) {
            return ASR::make_LogicalConstant_t(al, loc, false, to_type);
        }
        if (ASRUtils::is_integer(*type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                bool b = ASR::down_cast<ASR::IntegerConstant_t>(
                                        ASRUtils::expr_value(arg))->m_n;
                value = ASR::down_cast<ASR::expr_t>(make_LogicalConstant_t(al,
                                loc, b, to_type));
            }
            return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                al, loc, arg, ASR::cast_kindType::IntegerToLogical, to_type, value));

        } else if (ASRUtils::is_real(*type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                bool b = ASR::down_cast<ASR::RealConstant_t>(
                                        ASRUtils::expr_value(arg))->m_r;
                value = ASR::down_cast<ASR::expr_t>(make_LogicalConstant_t(al,
                                loc, b, to_type));
            }
            return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                al, loc, arg, ASR::cast_kindType::RealToLogical, to_type, value));

        } else if (ASRUtils::is_character(*type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                char *c = ASR::down_cast<ASR::StringConstant_t>(
                                        ASRUtils::expr_value(arg))->m_s;
                value = ASR::down_cast<ASR::expr_t>(make_LogicalConstant_t(al,
                                loc, std::string(c) != "", to_type));
            }
            return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                al, loc, arg, ASR::cast_kindType::CharacterToLogical, to_type, value));

        } else if (ASRUtils::is_complex(*type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                ASR::ComplexConstant_t *c_arg = ASR::down_cast<ASR::ComplexConstant_t>(
                                                ASRUtils::expr_value(arg));
                std::complex<double> c_value(c_arg->m_re, c_arg->m_im);
                value = ASR::down_cast<ASR::expr_t>(make_LogicalConstant_t(al,
                                loc, c_value.real() != 0.0 || c_value.imag() != 0.0, to_type));
            }
            return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                al, loc, arg, ASR::cast_kindType::ComplexToLogical, to_type, value));

        } else if (ASRUtils::is_logical(*type)) {
            return (ASR::asr_t *)arg;
        } else {
            std::string stype = ASRUtils::type_to_str_python(type);
            throw SemanticError(
                "Conversion of '" + stype + "' to logical is not Implemented",
                loc);
        }
        // TODO: Make this work if the argument is, let's say, a class.
        return nullptr;
    }


    static ASR::asr_t* handle_intrinsic_str(Allocator &al, Vec<ASR::call_arg_t> args,
                                        const Location &loc) {
        if (args.size() > 1) {
            throw SemanticError("Either 0 or 1 argument is expected in 'str()'",
                    loc);
        }
        ASR::expr_t *arg = nullptr;
        ASR::expr_t *res_value = nullptr;
        ASR::ttype_t *arg_type = nullptr;
        if (args.size() > 0) {
            arg = args[0].m_value;
            arg_type = ASRUtils::expr_type(arg);
        }
        ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, -2, nullptr));
        if (!arg) {
            ASR::ttype_t *res_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, 0, nullptr));
            return ASR::make_StringConstant_t(al, loc, s2c(al, ""), res_type);
        }
        if (ASRUtils::is_real(*arg_type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                double ival = ASR::down_cast<ASR::RealConstant_t>(
                                        ASRUtils::expr_value(arg))->m_r;
                // stringstream for exponential notation formatting.
                std::stringstream sm;
                sm << ival;
                std::string value_str = sm.str();
                sm.clear();
                ASR::ttype_t *res_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                                            1, value_str.size(), nullptr));
                res_value =  ASR::down_cast<ASR::expr_t>(ASR::make_StringConstant_t(al,
                                loc, s2c(al, value_str), res_type));
            }
            return ASR::make_Cast_t(al, loc, arg, ASR::cast_kindType::RealToCharacter,
                str_type, res_value);
        } else if (ASRUtils::is_integer(*arg_type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                int64_t number = ASR::down_cast<ASR::IntegerConstant_t>(
                                        ASRUtils::expr_value(arg))->m_n;
                std::string value_str = std::to_string(number);
                ASR::ttype_t *res_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                                            1, value_str.size(), nullptr));
                res_value = ASR::down_cast<ASR::expr_t>(ASR::make_StringConstant_t(al,
                                loc, s2c(al, value_str), res_type));
            }
            return ASR::make_Cast_t(al, loc, arg, ASR::cast_kindType::IntegerToCharacter,
                str_type, res_value);
        } else if (ASRUtils::is_logical(*arg_type)) {
            if(ASRUtils::expr_value(arg) != nullptr) {
                bool bool_number = ASR::down_cast<ASR::LogicalConstant_t>(
                                        ASRUtils::expr_value(arg))->m_value;
                std::string value_str = (bool_number)? "True" : "False";
                ASR::ttype_t *res_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                                            1, value_str.size(), nullptr));
                res_value = ASR::down_cast<ASR::expr_t>(ASR::make_StringConstant_t(al,
                                loc, s2c(al, value_str), res_type));
            }
            return ASR::make_Cast_t(al, loc, arg, ASR::cast_kindType::LogicalToCharacter,
                str_type, res_value);

        } else if (ASRUtils::is_character(*arg_type)) {
            return (ASR::asr_t *)arg;
        } else {
            std::string stype = ASRUtils::type_to_str_python(arg_type);
            throw SemanticError("Conversion of '" + stype + "' to string is not Implemented", loc);
        }
    }

    static ASR::asr_t* handle_intrinsic_len(Allocator &al, Vec<ASR::call_arg_t> args,
                                        const Location &loc) {
        if (args.size() != 1) {
            throw SemanticError("len() takes exactly one argument (" +
                std::to_string(args.size()) + " given)", loc);
        }
        ASR::expr_t *arg = args[0].m_value;
        ASR::ttype_t *type = ASRUtils::expr_type(arg);
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
        ASR::expr_t *value = nullptr;
        if (ASRUtils::is_character(*type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                char* c = ASR::down_cast<ASR::StringConstant_t>(
                                        ASRUtils::expr_value(arg))->m_s;
                int64_t ival = std::string(c).size();
                value = ASR::down_cast<ASR::expr_t>(make_IntegerConstant_t(al,
                                loc, ival, to_type));
            }
            return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(
                    ASR::make_StringLen_t(al, loc, arg, to_type, value));
        } else if (ASR::is_a<ASR::Set_t>(*type)) {
            // Skip for set as they can have multiple same values
            // which shouldn't account for len.
            // Example: {1, 1, 1, 3} and the length should be 2.
            // So it should be handled in the backend.
            return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(
                    ASR::make_SetLen_t(al, loc, arg, to_type, value));
        } else if (ASR::is_a<ASR::Tuple_t>(*type)) {
            value = ASR::down_cast<ASR::expr_t>(make_IntegerConstant_t(al,
                    loc, ASR::down_cast<ASR::Tuple_t>(type)->n_type, to_type));
            return ASR::make_TupleLen_t(al, loc, arg, to_type, value);
        } else if (ASR::is_a<ASR::List_t>(*type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                int64_t ival = (int64_t)ASR::down_cast<ASR::ListConstant_t>(
                                        ASRUtils::expr_value(arg))->n_args;
                value = ASR::down_cast<ASR::expr_t>(make_IntegerConstant_t(al,
                                loc, ival, to_type));
            }
            return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(
                    ASR::make_ListLen_t(al, loc, arg, to_type, value));
        } else if (ASR::is_a<ASR::Dict_t>(*type)) {
            // Skip for dictionary as they can have multiple same keys
            // which shouldn't account for len.
            // Example: {1: 2, 1: 3, 1: 4} and the length should be 1.
            // So it should be handled in the backend.
            return (ASR::asr_t *)ASR::down_cast<ASR::expr_t>(
                    ASR::make_DictLen_t(al, loc, arg, to_type, value));
        }
        throw SemanticError("len() is only supported for `str`, `set`, `dict`, `list` and `tuple`", loc);
    }

    static ASR::asr_t* handle_reshape(Allocator &al, Vec<ASR::call_arg_t> args,
                               const Location &loc) {
        if( args.size() != 2 ) {
            throw SemanticError("reshape accepts only 2 arguments, got " +
                                std::to_string(args.size()) + " arguments instead.",
                                loc);
        }
        ASR::expr_t* array = args[0].m_value;
        ASR::expr_t* newshape = args[1].m_value;
        if( !ASRUtils::is_array(ASRUtils::expr_type(newshape)) ) {
            throw SemanticError("reshape only accept arrays for shape "
                                "arguments, found " +
                                ASRUtils::type_to_str_python(ASRUtils::expr_type(newshape)) +
                                " instead.",
                                loc);
        }
        Vec<ASR::dimension_t> dims;
        dims.reserve(al, 1);
        ASR::dimension_t newdim;
        newdim.loc = loc;
        newdim.m_start = nullptr, newdim.m_length = nullptr;
        dims.push_back(al, newdim);
        ASR::ttype_t* empty_type = ASRUtils::duplicate_type(al, ASRUtils::expr_type(array), &dims);
        ASR::array_physical_typeType array_physical_type = ASRUtils::extract_physical_type(
                                                                ASRUtils::expr_type(array));
        if( array_physical_type == ASR::array_physical_typeType::FixedSizeArray ) {
        empty_type = ASRUtils::duplicate_type(al, ASRUtils::expr_type(array),
                        &dims, array_physical_type, true);
        } else {
        empty_type = ASRUtils::duplicate_type(al, ASRUtils::expr_type(array), &dims);
        }
        newshape = ASRUtils::cast_to_descriptor(al, newshape);
        return ASR::make_ArrayReshape_t(al, loc, array, newshape, empty_type, nullptr);
    }

    static ASR::asr_t* handle_intrinsic_ord(Allocator &al, Vec<ASR::call_arg_t> args,
                                        const Location &loc) {
        if (args.size() != 1) {
            throw SemanticError("ord() takes exactly one argument (" +
                std::to_string(args.size()) + " given)", loc);
        }
        ASR::expr_t *arg = args[0].m_value;
        ASR::ttype_t *type = ASRUtils::expr_type(arg);
        ASR::ttype_t *to_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
        ASR::expr_t *value = nullptr;
        if (ASRUtils::is_character(*type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                char* c = ASR::down_cast<ASR::StringConstant_t>(ASRUtils::expr_value(arg))->m_s;
                if (std::string(c).size() != 1) {
                    throw SemanticError("ord() is only supported for `str` of length 1", arg->base.loc);
                }
                value = ASR::down_cast<ASR::expr_t>(
                    ASR::make_IntegerConstant_t(al, loc, c[0], to_type));
            }
            return ASR::make_StringOrd_t(al, loc, arg, to_type, value);
        } else {
            throw SemanticError("ord() expected string of length 1, but " + ASRUtils::type_to_str_python(type) + " found",
                arg->base.loc);
        }
    }

    static ASR::asr_t* handle_intrinsic_chr(Allocator &al, Vec<ASR::call_arg_t> args,
                                        const Location &loc) {
        if (args.size() != 1) {
            throw SemanticError("chr() takes exactly one argument (" +
                std::to_string(args.size()) + " given)", loc);
        }
        ASR::expr_t *arg = args[0].m_value;
        ASR::ttype_t *type = ASRUtils::expr_type(arg);
        ASR::ttype_t* str_type = ASRUtils::TYPE(ASR::make_Character_t(al,
            loc, 1, 1, nullptr));
        ASR::expr_t *value = nullptr;
        if (ASRUtils::is_integer(*type)) {
            if (ASRUtils::expr_value(arg) != nullptr) {
                int64_t c = ASR::down_cast<ASR::IntegerConstant_t>(arg)->m_n;
                if (! (c >= 0 && c <= 127) ) {
                    throw SemanticError("The argument 'x' in chr(x) must be in the range 0 <= x <= 127.", loc);
                }
                char cc = c;
                std::string svalue;
                svalue += cc;
                value = ASR::down_cast<ASR::expr_t>(
                    ASR::make_StringConstant_t(al, loc, s2c(al, svalue), str_type));
            }
            return ASR::make_StringChr_t(al, loc, arg, str_type, value);
        } else {
            throw SemanticError("'" + ASRUtils::type_to_str_python(type) + "' object cannot be interpreted as an integer",
                arg->base.loc);
        }
    }

}; // IntrinsicNodeHandler

} // namespace LCompilers::LPython

#endif /* LPYTHON_INTRINSIC_EVAL_H */
