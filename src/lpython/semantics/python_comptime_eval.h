#ifndef LPYTHON_SEMANTICS_COMPTIME_EVAL_H
#define LPYTHON_SEMANTICS_COMPTIME_EVAL_H

#include <complex>
#include <string>
#include <cstring>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <iterator>

#include <libasr/asr.h>
#include <lpython/ast.h>
#include <lpython/bigint.h>
#include <libasr/string_utils.h>
#include <lpython/utils.h>
#include <lpython/semantics/semantic_exception.h>

namespace LFortran {

struct PythonIntrinsicProcedures {

    const std::string m_builtin = "lpython_builtin";

    typedef ASR::expr_t* (*comptime_eval_callback)(Allocator &, const Location &, Vec<ASR::expr_t*> &);
    // Table of intrinsics
    // The callback is only called if all arguments have compile time `value`
    // which is always one of the `Constant*` expression ASR nodes, so inside
    // the callback one can assume that.
    std::map<std::string, std::tuple<std::string, comptime_eval_callback>> comptime_eval_map;

    PythonIntrinsicProcedures() {
        comptime_eval_map = {
            {"abs", {m_builtin, &eval_abs}},
            {"str", {m_builtin, &eval_str}},
            {"bool", {m_builtin, &eval_bool}},
            {"chr", {m_builtin, &eval_chr}},
            {"ord", {m_builtin, &eval_ord}},
            {"len", {m_builtin, &eval_len}},
            {"pow", {m_builtin, &eval_pow}},
            {"int", {m_builtin, &eval_int}},
            {"float", {m_builtin, &eval_float}},
            {"round", {m_builtin, &eval_round}},
            {"bin", {m_builtin, &eval_bin}},
            {"hex", {m_builtin, &eval_hex}},
            {"oct", {m_builtin, &eval_oct}},
            {"complex", {m_builtin, &eval_complex}},
            {"divmod", {m_builtin, &eval_divmod}},
        };
    }

    // Return `true` if `name` is in the table of intrinsics
    bool is_intrinsic(std::string name) const {
        auto search = comptime_eval_map.find(name);
        if (search != comptime_eval_map.end()) {
            return true;
        } else {
            return false;
        }
    }

    // Looks up `name` in the table of intrinsics and returns the corresponding
    // module name; Otherwise rises an exception
    std::string get_module(std::string name, const Location &loc) const {
        auto search = comptime_eval_map.find(name);
        if (search != comptime_eval_map.end()) {
            std::string module_name = std::get<0>(search->second);
            return module_name;
        } else {
            throw SemanticError("Function '" + name
                + "' not found among intrinsic procedures",
                loc);
        }
    }

    // Evaluates the intrinsic function `name` at compile time
    ASR::expr_t *comptime_eval(std::string name, Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) const {
        auto search = comptime_eval_map.find(name);
        if (search != comptime_eval_map.end()) {
            comptime_eval_callback cb = std::get<1>(search->second);
            Vec<ASR::expr_t*> arg_values = ASRUtils::get_arg_values(al, args);
            if (arg_values.size() != args.size()) {
                // Not all arguments have compile time values; we do not call the callback
                return nullptr;
            }
            return cb(al, loc, arg_values);
        } else {
            throw SemanticError("Intrinsic function '" + name
                + "' compile time evaluation is not implemented yet",
                loc);
        }
    }


    static ASR::expr_t *eval_abs(Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args
            ) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 1) {
            throw SemanticError("Intrinsic abs function accepts exactly 1 argument", loc);
        }
        ASR::expr_t* trig_arg = args[0];
        ASR::ttype_t* t = ASRUtils::expr_type(args[0]);
        if (ASR::is_a<ASR::Real_t>(*t)) {
            double rv = ASR::down_cast<ASR::ConstantReal_t>(trig_arg)->m_r;
            double val = std::abs(rv);
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, loc, val, t));
        } else if (ASR::is_a<ASR::Integer_t>(*t)) {
            int64_t rv = ASR::down_cast<ASR::ConstantInteger_t>(trig_arg)->m_n;
            int64_t val = std::abs(rv);
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, val, t));
        } else if (ASR::is_a<ASR::Complex_t>(*t)) {
            double re = ASR::down_cast<ASR::ConstantComplex_t>(trig_arg)->m_re;
            double im = ASR::down_cast<ASR::ConstantComplex_t>(trig_arg)->m_im;
            std::complex<double> x(re, im);
            double result = std::abs(x);
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, loc, result, t));
        } else {
            throw SemanticError("Argument of the abs function must be Integer, Real or Complex", loc);
        }
    }

    static ASR::expr_t *eval_str(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        ASR::ttype_t* str_type = ASRUtils::TYPE(ASR::make_Character_t(al,
            loc, 1, 1, nullptr, nullptr, 0));
        if (args.size() == 0) { // create an empty string
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantString_t(al, loc, s2c(al, ""), str_type));
        }
        ASR::expr_t* arg = ASRUtils::expr_value(args[0]);
        ASR::ttype_t* arg_type = ASRUtils::expr_type(arg);
        if (ASRUtils::is_integer(*arg_type)) {
            int64_t ival = ASR::down_cast<ASR::ConstantInteger_t>(arg)->m_n;
            std::string s = std::to_string(ival);
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantString_t(al, loc, s2c(al, s), str_type));
        } else if (ASRUtils::is_real(*arg_type)) {
            double rval = ASR::down_cast<ASR::ConstantReal_t>(arg)->m_r;
            std::string s = std::to_string(rval);
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantString_t(al, loc, s2c(al, s), str_type));
        } else if (ASRUtils::is_logical(*arg_type)) {
            bool rv = ASR::down_cast<ASR::ConstantLogical_t>(arg)->m_value;
            std::string s = rv ? "True" : "False";
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantString_t(al, loc, s2c(al, s), str_type));
        } else if (ASRUtils::is_character(*arg_type)) {
            char* c = ASR::down_cast<ASR::ConstantString_t>(arg)->m_s;
            std::string s = std::string(c);
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantString_t(al, loc, s2c(al, s), str_type));
        } else {
            throw SemanticError("str() argument must be real, integer, logical, or a string, not '" +
                ASRUtils::type_to_str(arg_type) + "'", loc);
        }
    }

    static ASR::expr_t *eval_bool(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        if (args.size() != 1) {
            throw SemanticError("bool() takes exactly one argument (" +
                std::to_string(args.size()) + " given)", loc);
        }
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc,
            1, nullptr, 0));
        ASR::expr_t* arg = ASRUtils::expr_value(args[0]);
        ASR::ttype_t* t = ASRUtils::expr_type(arg);
        bool result;
        if (ASRUtils::is_real(*t)) {
            result = ASR::down_cast<ASR::ConstantReal_t>(arg)->m_r;
        } else if (ASRUtils::is_integer(*t)) {
            result = ASR::down_cast<ASR::ConstantInteger_t>(arg)->m_n;
        } else if (ASRUtils::is_complex(*t)) {
            double re = ASR::down_cast<ASR::ConstantComplex_t>(arg)->m_re;
            double im = ASR::down_cast<ASR::ConstantComplex_t>(arg)->m_im;
            std::complex<double> c(re, im);
            result = (re || im);
        } else if (ASRUtils::is_logical(*t)) {
            result = ASR::down_cast<ASR::ConstantLogical_t>(arg)->m_value;
        } else if (ASRUtils::is_character(*t)) {
            char* c = ASR::down_cast<ASR::ConstantString_t>(ASRUtils::expr_value(arg))->m_s;
            result = strlen(s2c(al, std::string(c)));
        } else {
            throw SemanticError("bool() must have one real, integer, character,"
                " complex, or logical argument, not '" + ASRUtils::type_to_str(t) + "'", loc);
        }
        return ASR::down_cast<ASR::expr_t>(make_ConstantLogical_t(al, loc, result, type));
    }

    static ASR::expr_t *eval_chr(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::expr_t* arg = args[0];
        ASR::ttype_t* type = ASRUtils::expr_type(arg);
        if (ASR::is_a<ASR::Integer_t>(*type)) {
            int64_t c = ASR::down_cast<ASR::ConstantInteger_t>(arg)->m_n;
            ASR::ttype_t* str_type =
                ASRUtils::TYPE(ASR::make_Character_t(al,
                loc, 1, 1, nullptr, nullptr, 0));
            if (! (c >= 0 && c <= 127) ) {
                throw SemanticError("The argument 'x' in chr(x) must be in the range 0 <= x <= 127.", loc);
            }
            char cc = c;
            std::string svalue;
            svalue += cc;
            return ASR::down_cast<ASR::expr_t>(
                ASR::make_ConstantString_t(al, loc, s2c(al, svalue), str_type));
        } else {
            throw SemanticError("chr() must have one integer argument.", loc);
        }
    }

    static ASR::expr_t *eval_ord(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::expr_t* char_expr = args[0];
        ASR::ttype_t* char_type = ASRUtils::expr_type(char_expr);
        if (ASRUtils::is_character(*char_type)) {
            char* c = ASR::down_cast<ASR::ConstantString_t>(ASRUtils::expr_value(char_expr))->m_s;
            ASR::ttype_t* int_type =
                ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
            return ASR::down_cast<ASR::expr_t>(
                ASR::make_ConstantInteger_t(al, loc, c[0], int_type));
        } else {
            throw SemanticError("ord() must have one character argument", loc);
        }
    }

    static ASR::expr_t *eval_len(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        if (args.size() != 1) {
            throw SemanticError("len() takes exactly one argument (" +
                std::to_string(args.size()) + " given)", loc);
        }
        ASR::expr_t *arg = ASRUtils::expr_value(args[0]);
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
        if (arg->type == ASR::exprType::ConstantString) {
            char* str_value = ASR::down_cast<ASR::ConstantString_t>(arg)->m_s;
            return ASR::down_cast<ASR::expr_t>(make_ConstantInteger_t(al, loc,
                (int64_t)strlen(s2c(al, std::string(str_value))), type));
        } else if (arg->type == ASR::exprType::ConstantArray) {
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc,
                (int64_t)ASR::down_cast<ASR::ConstantArray_t>(arg)->n_args, type));
        } else if (arg->type == ASR::exprType::ConstantTuple) {
            return ASR::down_cast<ASR::expr_t>(make_ConstantInteger_t(al, loc,
                (int64_t)ASR::down_cast<ASR::ConstantTuple_t>(arg)->n_elements, type));
        } else if (arg->type == ASR::exprType::ConstantDictionary) {
            return ASR::down_cast<ASR::expr_t>(make_ConstantInteger_t(al, loc,
                (int64_t)ASR::down_cast<ASR::ConstantDictionary_t>(arg)->n_keys, type));
        } else if (arg->type == ASR::exprType::ConstantSet) {
            return ASR::down_cast<ASR::expr_t>(make_ConstantInteger_t(al, loc,
                (int64_t)ASR::down_cast<ASR::ConstantSet_t>(arg)->n_elements, type));
        } else {
            throw SemanticError("len() only works on strings, lists, tuples, dictionaries and sets",
                loc);
        }
    }

    static ASR::expr_t *eval_pow(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::expr_t* arg1 = ASRUtils::expr_value(args[0]);
        ASR::expr_t* arg2 = ASRUtils::expr_value(args[1]);
        ASR::ttype_t* arg1_type = ASRUtils::expr_type(arg1);
        ASR::ttype_t* arg2_type = ASRUtils::expr_type(arg2);
        ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
        ASR::ttype_t *real_type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, 8, nullptr, 0));
        if (!ASRUtils::check_equal_type(arg1_type, arg2_type)) {
            throw SemanticError("The arguments to pow() must have the same type.", loc);
        }
        if (ASRUtils::is_integer(*arg1_type) && ASRUtils::is_integer(*arg2_type)) {
            int64_t a = ASR::down_cast<ASR::ConstantInteger_t>(arg1)->m_n;
            int64_t b = ASR::down_cast<ASR::ConstantInteger_t>(arg2)->m_n;
            if (a == 0 && b < 0) { // Zero Division
                throw SemanticError("0.0 cannot be raised to a negative power.", loc);
            }
            if (b < 0) // Negative power
                return ASR::down_cast<ASR::expr_t>(make_ConstantReal_t(al, loc,
                    pow(a, b), real_type));
            else // Positive power
                return ASR::down_cast<ASR::expr_t>(make_ConstantInteger_t(al, loc,
                    (int64_t)pow(a, b), int_type));

        } else {
            throw SemanticError("The arguments to pow() must be of type integers for now.", loc);
        }
    }

    static ASR::expr_t *eval_int(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
        if (args.size() == 0) {
            return ASR::down_cast<ASR::expr_t>(make_ConstantInteger_t(al, loc, 0, type));
        }
        ASR::expr_t* int_expr = ASRUtils::expr_value(args[0]);
        ASR::ttype_t* int_type = ASRUtils::expr_type(int_expr);
        if (ASRUtils::is_integer(*int_type)) {
            int64_t ival = ASR::down_cast<ASR::ConstantInteger_t>(int_expr)->m_n;
            return ASR::down_cast<ASR::expr_t>(make_ConstantInteger_t(al, loc, ival, type));

        } else if (ASRUtils::is_character(*int_type)) {
            // convert a string to an int
            char* c = ASR::down_cast<ASR::ConstantString_t>(int_expr)->m_s;
            std::string str = std::string(c);
            int64_t ival = std::stoll(str);
            return ASR::down_cast<ASR::expr_t>(make_ConstantInteger_t(al, loc, ival, type));

        } else if (ASRUtils::is_real(*int_type)) {
            int64_t ival = ASR::down_cast<ASR::ConstantReal_t>(int_expr)->m_r;
            return ASR::down_cast<ASR::expr_t>(make_ConstantInteger_t(al, loc, ival, type));

        } else if (ASRUtils::is_logical(*int_type)) {
            bool rv = ASR::down_cast<ASR::ConstantLogical_t>(int_expr)->m_value;
            int8_t val = rv ? 1 : 0;
            return ASR::down_cast<ASR::expr_t>(make_ConstantInteger_t(al, loc, val, type));

        } else {
            throw SemanticError("int() argument must be real, integer, logical, or a string, not '" +
                ASRUtils::type_to_str(int_type) + "'", loc);
        }
    }

    static ASR::expr_t *eval_float(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        ASR::ttype_t* type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, 8, nullptr, 0));
        if (args.size() == 0) {
            return ASR::down_cast<ASR::expr_t>(make_ConstantReal_t(al, loc, 0.0, type));
        }
        ASR::expr_t* expr = ASRUtils::expr_value(args[0]);
        ASR::ttype_t* float_type = ASRUtils::expr_type(expr);
        if (ASRUtils::is_real(*float_type)) {
            float rv = ASR::down_cast<ASR::ConstantReal_t>(expr)->m_r;
            return ASR::down_cast<ASR::expr_t>(make_ConstantReal_t(al, loc, rv, type));
        } else if (ASRUtils::is_integer(*float_type)) {
            double rv = ASR::down_cast<ASR::ConstantInteger_t>(expr)->m_n;
            return ASR::down_cast<ASR::expr_t>(make_ConstantReal_t(al, loc, rv, type));
        } else if (ASRUtils::is_logical(*float_type)) {
            bool rv = ASR::down_cast<ASR::ConstantLogical_t>(expr)->m_value;
            float val = rv ? 1.0 : 0.0;
            return ASR::down_cast<ASR::expr_t>(make_ConstantReal_t(al, loc, val, type));
        } else if (ASRUtils::is_character(*float_type)) {
            // convert a string to a float
            char* c = ASR::down_cast<ASR::ConstantString_t>(expr)->m_s;
            std::string str = std::string(c);
            float rv = std::stof(str);
            return ASR::down_cast<ASR::expr_t>(make_ConstantReal_t(al, loc, rv, type));
        } else {
            throw SemanticError("float() argument must be real, integer, logical, or a string, not '" +
                ASRUtils::type_to_str(float_type) + "'", loc);
        }
    }

    static ASR::expr_t *eval_bin(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        if (args.size() != 1) {
            throw SemanticError("bin() takes exactly one argument (" +
                std::to_string(args.size()) + " given)", loc);
        }
        ASR::expr_t* expr = ASRUtils::expr_value(args[0]);
        ASR::ttype_t* type = ASRUtils::expr_type(expr);
        if (ASRUtils::is_integer(*type)) {
            int64_t n = ASR::down_cast<ASR::ConstantInteger_t>(expr)->m_n;
            ASR::ttype_t* str_type = ASRUtils::TYPE(ASR::make_Character_t(al,
                loc, 1, 1, nullptr, nullptr, 0));
            std::string str, prefix;
            prefix = n > 0 ? "0b" : "-0b";
            str += std::bitset<64>(std::abs(n)).to_string();
            str.erase(0, str.find_first_not_of('0'));
            str.insert(0, prefix);
            return ASR::down_cast<ASR::expr_t>(make_ConstantString_t(al, loc, s2c(al, str), str_type));
        } else {
            throw SemanticError("bin() argument must be an integer, not '" +
                ASRUtils::type_to_str(type) + "'", loc);
        }
    }

    static ASR::expr_t *eval_hex(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        if (args.size() != 1) {
            throw SemanticError("hex() takes exactly one argument (" +
                std::to_string(args.size()) + " given)", loc);
        }
        ASR::expr_t* expr = ASRUtils::expr_value(args[0]);
        ASR::ttype_t* type = ASRUtils::expr_type(expr);
        if (ASRUtils::is_integer(*type)) {
            int64_t n = ASR::down_cast<ASR::ConstantInteger_t>(expr)->m_n;
            ASR::ttype_t* str_type = ASRUtils::TYPE(ASR::make_Character_t(al,
                loc, 1, 1, nullptr, nullptr, 0));
            std::string str, prefix;
            std::stringstream ss;
            prefix = n > 0 ? "0x" : "-0x";
            ss << std::hex << std::abs(n);
            str += ss.str();
            str.insert(0, prefix);
            return ASR::down_cast<ASR::expr_t>(make_ConstantString_t(al, loc, s2c(al, str), str_type));
        } else {
            throw SemanticError("hex() argument must be an integer, not '" +
                ASRUtils::type_to_str(type) + "'", loc);
        }
    }

    static ASR::expr_t *eval_oct(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        if (args.size() != 1) {
            throw SemanticError("oct() takes exactly one argument (" +
                std::to_string(args.size()) + " given)", loc);
        }
        ASR::expr_t* expr = ASRUtils::expr_value(args[0]);
        ASR::ttype_t* type = ASRUtils::expr_type(expr);
        if (ASRUtils::is_integer(*type)) {
            int64_t n = ASR::down_cast<ASR::ConstantInteger_t>(expr)->m_n;
            ASR::ttype_t* str_type = ASRUtils::TYPE(ASR::make_Character_t(al,
                loc, 1, 1, nullptr, nullptr, 0));
            std::string str, prefix;
            std::stringstream ss;
            prefix = n > 0 ? "0o" : "-0o";
            ss << std::oct << std::abs(n);
            str += ss.str();
            str.insert(0, prefix);
            return ASR::down_cast<ASR::expr_t>(make_ConstantString_t(al, loc, s2c(al, str), str_type));
        } else {
            throw SemanticError("oct() argument must be an integer, not '" +
                ASRUtils::type_to_str(type) + "'", loc);
        }
    }

    static ASR::expr_t *eval_round(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
        if (args.size() != 1) {
            throw SemanticError("round() missing required argument 'number' (pos 1)", loc);
        }
        ASR::expr_t* expr = args[0];
        ASR::ttype_t* t = ASRUtils::expr_type(expr);
        if (ASRUtils::is_real(*t)) {
            double rv = ASR::down_cast<ASR::ConstantReal_t>(expr)->m_r;
            int64_t rounded = round(rv);
            if (fabs(rv-rounded) == 0.5)
                rounded = 2.0*round(rv/2.0);
            return ASR::down_cast<ASR::expr_t>(make_ConstantInteger_t(al, loc, rounded, type));
        } else {
            throw SemanticError("round() argument must be float for now, not '" +
                ASRUtils::type_to_str(t) + "'", loc);
        }
    }

    static ASR::expr_t *eval_complex(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        int16_t n_args = args.size();
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Complex_t(al, loc, 8, nullptr, 0));
        if( n_args > 2 || n_args < 0 ) { // n_args shouldn't be less than 0 but added this check for safety
            throw SemanticError("Only constant integer or real values are supported as "
                "the (at most two) arguments of complex()", loc);
        }
        double c1 = 0.0, c2 = 0.0; // Default if n_args = 0
        if (n_args >= 1) { // Handles both n_args = 1 and n_args = 2
            if (ASR::is_a<ASR::ConstantInteger_t>(*args[0])) {
                c1 = ASR::down_cast<ASR::ConstantInteger_t>(args[0])->m_n;
            } else if (ASR::is_a<ASR::ConstantReal_t>(*args[0])) {
                c1 = ASR::down_cast<ASR::ConstantReal_t>(ASRUtils::expr_value(args[0]))->m_r;
            }
        }
        if (n_args == 2) { // Extracts imaginary component if n_args = 2
            if (ASR::is_a<ASR::ConstantInteger_t>(*args[1])) {
                c2 = ASR::down_cast<ASR::ConstantInteger_t>(args[1])->m_n;
            } else if (ASR::is_a<ASR::ConstantReal_t>(*args[1])) {
                c2 = ASR::down_cast<ASR::ConstantReal_t>(ASRUtils::expr_value(args[1]))->m_r;
            }
        }
        return ASR::down_cast<ASR::expr_t>(make_ConstantComplex_t(al, loc, c1, c2, type));
    }

    static ASR::expr_t *eval_divmod(Allocator &al, const Location &loc, Vec<ASR::expr_t *> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 2) {
            throw SemanticError("divmod() takes exactly two arguments (" +
                std::to_string(args.size()) + " given)", loc);
        }
        ASR::expr_t *arg1 = args[0];
        ASR::expr_t *arg2 = args[1];
        ASR::ttype_t *arg1_type = ASRUtils::expr_type(arg1);
        ASR::ttype_t *arg2_type = ASRUtils::expr_type(arg2);
        Vec<ASR::expr_t *> tuple; // pair consisting of quotient and remainder
        tuple.reserve(al, 2);
        Vec<ASR::ttype_t *> tuple_type_vec;
        tuple_type_vec.reserve(al, 2);
        if (ASRUtils::is_integer(*arg1_type) && ASRUtils::is_integer(*arg2_type)) {
            int64_t ival1 = ASR::down_cast<ASR::ConstantInteger_t>(arg1)->m_n;
            int64_t ival2 = ASR::down_cast<ASR::ConstantInteger_t>(arg2)->m_n;
            if (ival2 == 0) {
                throw SemanticError("Integer division or modulo by zero not possible", loc);
            } else {
                int64_t div = ival1 / ival2;
                int64_t mod = ival1 % ival2;
                tuple.push_back(al, ASRUtils::EXPR(
                                ASR::make_ConstantInteger_t(al, loc, div, arg1_type)));
                tuple.push_back(al, ASRUtils::EXPR(
                                ASR::make_ConstantInteger_t(al, loc, mod, arg1_type)));
                tuple_type_vec.push_back(al, arg1_type);
                tuple_type_vec.push_back(al, arg2_type);
                ASR::ttype_t *tuple_type = ASRUtils::TYPE(ASR::make_Tuple_t(al, loc,
                                                          tuple_type_vec.p, tuple_type_vec.n));
                return ASR::down_cast<ASR::expr_t>(make_ConstantTuple_t(al, loc, tuple.p, tuple.size(), tuple_type));
            }
        } else {
            throw SemanticError("Both arguments of divmod() must be integers for now, not '" +
                ASRUtils::type_to_str(arg1_type) + "' and '" + ASRUtils::type_to_str(arg2_type) + "'", loc);
        }
    }

}; // ComptimeEval

} // namespace LFortran

#endif /* LPYTHON_SEMANTICS_COMPTIME_EVAL_H */
