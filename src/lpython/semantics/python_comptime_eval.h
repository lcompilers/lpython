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
#include <lpython/bigint.h>
#include <libasr/string_utils.h>
#include <lpython/utils.h>
#include <lpython/semantics/semantic_exception.h>
#include <libasr/asr_utils.h>

namespace LCompilers::LPython {

struct ProceduresDatabase {
    std::map<std::string, std::set<std::string>> to_be_ignored;

    ProceduresDatabase() {
        to_be_ignored = {{"numpy", {"empty", "int64", "int32",
                                    "float32", "float64",
                                    "reshape", "array", "int16",
                                    "complex64", "complex128",
                                    "int8", "exp", "exp2",
                                    "uint8", "uint16", "uint32", "uint64",
                                    "size", "bool_"}},
                         {"math", {"sin", "cos", "tan",
                                    "asin", "acos", "atan",
                                    "exp", "exp2", "expm1"}},
                         {"enum", {"Enum"}}
                        };
    }

    bool is_function_to_be_ignored(std::string& module_name,
                                   std::string& function_name) {
        if( to_be_ignored.find(module_name) == to_be_ignored.end() ) {
            return false;
        }
        return to_be_ignored[module_name].find(function_name) != to_be_ignored[module_name].end();
    }

};

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
            // {"abs", {m_builtin, &eval_abs}},
            {"pow", {m_builtin, &eval_pow}},
            {"round", {m_builtin, &eval_round}},
            {"bin", {m_builtin, &eval_bin}},
            {"hex", {m_builtin, &eval_hex}},
            {"oct", {m_builtin, &eval_oct}},
            {"list", {m_builtin, &eval_list}},
            {"complex", {m_builtin, &eval_complex}},
            {"_lpython_imag", {m_builtin, &eval__lpython_imag}},
            {"divmod", {m_builtin, &eval_divmod}},
            {"_lpython_floordiv", {m_builtin, &eval__lpython_floordiv}},
            {"_mod", {m_builtin, &eval__mod}},
            {"max" , {m_builtin , &eval_max}},
            {"min" , {m_builtin , &eval_min}},
            {"sum" , {m_builtin , &not_implemented}},
            // The following functions for string methods are not used
            // for evaluation.
            {"_lpython_str_capitalize", {m_builtin, &not_implemented}},
            {"_lpython_str_count", {m_builtin, &not_implemented}},
            {"_lpython_str_lower", {m_builtin, &not_implemented}},
            {"_lpython_str_upper", {m_builtin, &not_implemented}},
            {"_lpython_str_join", {m_builtin, &not_implemented}},
            {"_lpython_str_find", {m_builtin, &not_implemented}},
            {"_lpython_str_rstrip", {m_builtin, &not_implemented}},
            {"_lpython_str_lstrip", {m_builtin, &not_implemented}},
            {"_lpython_str_strip", {m_builtin, &not_implemented}},
            {"_lpython_str_swapcase", {m_builtin, &not_implemented}},
            {"_lpython_str_startswith", {m_builtin, &not_implemented}},
            {"_lpython_str_endswith", {m_builtin, &not_implemented}},
            {"_lpython_str_partition", {m_builtin, &not_implemented}},
            {"_lpython_str_islower", {m_builtin, &not_implemented}},
            {"_lpython_str_isupper", {m_builtin, &not_implemented}},
            {"_lpython_str_isdecimal", {m_builtin, &not_implemented}},
            {"_lpython_str_isascii", {m_builtin, &not_implemented}},
            {"_lpython_str_isspace", {m_builtin, &not_implemented}}
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
    ASR::expr_t *comptime_eval(std::string name, Allocator &al, const Location &loc, Vec<ASR::call_arg_t> &args) const {
        auto search = comptime_eval_map.find(name);
        if (search != comptime_eval_map.end()) {
            comptime_eval_callback cb = std::get<1>(search->second);
            Vec<ASR::call_arg_t> arg_values = ASRUtils::get_arg_values(al, args);
            if (arg_values.size() != args.size()) {
                // Not all arguments have compile time values; we do not call the callback
                return nullptr;
            }
            Vec<ASR::expr_t*> expr_args;
            expr_args.reserve(al, arg_values.size());
            for( auto& a: arg_values ) {
                expr_args.push_back(al, a.m_value);
            }
            return cb(al, loc, expr_args);
        } else {
            throw SemanticError("Intrinsic function '" + name
                + "' compile time evaluation is not implemented yet",
                loc);
        }
    }

    static ASR::expr_t *not_implemented(Allocator &/*al*/, const Location &/*loc*/,
            Vec<ASR::expr_t*> &/*args*/) {
        // This intrinsic is not evaluated at compile time yet.
        return nullptr;
    }

    static ASR::expr_t *eval_str(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() == 0) { // create an empty string
            ASR::ttype_t* str_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, 0, nullptr));
            return ASR::down_cast<ASR::expr_t>(ASR::make_StringConstant_t(al, loc, s2c(al, ""), str_type));
        }
        std::string s = "";
        ASR::expr_t* arg = args[0];
        ASR::ttype_t* arg_type = ASRUtils::expr_type(arg);
        if (ASRUtils::is_integer(*arg_type)) {
            int64_t ival = ASR::down_cast<ASR::IntegerConstant_t>(arg)->m_n;
            s = std::to_string(ival);
        } else if (ASRUtils::is_real(*arg_type)) {
            double rval = ASR::down_cast<ASR::RealConstant_t>(arg)->m_r;
            s = std::to_string(rval);
        } else if (ASRUtils::is_logical(*arg_type)) {
            bool rv = ASR::down_cast<ASR::LogicalConstant_t>(arg)->m_value;
            s = rv ? "True" : "False";
        } else if (ASRUtils::is_character(*arg_type)) {
            char* c = ASR::down_cast<ASR::StringConstant_t>(arg)->m_s;
            s = std::string(c);
        } else {
            throw SemanticError("str() argument must be real, integer, logical, or a string, not '" +
                ASRUtils::type_to_str_python(arg_type) + "'", loc);
        }
        ASR::ttype_t* str_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, s.size(), nullptr));
        return ASR::down_cast<ASR::expr_t>(ASR::make_StringConstant_t(al, loc, s2c(al, s), str_type));
    }

    static ASR::expr_t *eval__mod(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 2) {
            throw SemanticError("_mod() must have two integer/real arguments.", loc);
        }
        ASR::expr_t* arg1 = args[0], *arg2 = args[1];
        LCOMPILERS_ASSERT(ASRUtils::check_equal_type(ASRUtils::expr_type(arg1),
                                    ASRUtils::expr_type(arg2)));
        ASR::ttype_t* type = ASRUtils::expr_type(arg1);
        if (ASRUtils::is_integer(*type)) {
            int64_t a = ASR::down_cast<ASR::IntegerConstant_t>(arg1)->m_n;
            int64_t b = ASR::down_cast<ASR::IntegerConstant_t>(arg2)->m_n;
            if(b == 0) { // Zero Division
                throw SemanticError("Integer division or modulo by zero",loc);
            }

            // Refer the following link to understand how modulo in C++ is modified to behave like Python.
            // https://stackoverflow.com/questions/1907565/c-and-python-different-behaviour-of-the-modulo-operation
            return ASR::down_cast<ASR::expr_t>(
                ASR::make_IntegerConstant_t(al, loc, ((a%b)+b)%b, type));
        } else if (ASRUtils::is_real(*type)) {
            double a = ASR::down_cast<ASR::RealConstant_t>(arg1)->m_r;
            double b = ASR::down_cast<ASR::RealConstant_t>(arg2)->m_r;
            if (b == 0) { // Zero Division
                throw SemanticError("Float division or modulo by zero", loc);
            }

            // https://stackoverflow.com/questions/1907565/c-and-python-different-behaviour-of-the-modulo-operation
            return ASR::down_cast<ASR::expr_t>(
                ASR::make_RealConstant_t(al, loc, std::fmod(std::fmod(a, b) + b, b), type));
        } else {
            throw SemanticError("_mod() must have both integer or both real arguments.", loc);
        }
    }

    static ASR::expr_t *eval_pow(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::expr_t* arg1 = args[0];
        ASR::expr_t* arg2 = args[1];
        ASR::ttype_t* arg1_type = ASRUtils::expr_type(arg1);
        ASR::ttype_t* arg2_type = ASRUtils::expr_type(arg2);
        int64_t mod_by = -1;
        if (args.size() == 3) {
            ASR::expr_t* arg3 = args[2];
            ASR::ttype_t* arg3_type = ASRUtils::expr_type(arg3);
            if (!ASRUtils::is_integer(*arg3_type) ) { // Zero Division
                throw SemanticError("Third argument must be an integer. Found: " + \
                        ASRUtils::type_to_str_python(arg3_type), loc);
            }
            mod_by = ASR::down_cast<ASR::IntegerConstant_t>(arg3)->m_n;
        }
        ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
        ASR::ttype_t *real_type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, 8));
        ASR::ttype_t *complex_type = ASRUtils::TYPE(ASR::make_Complex_t(al, loc, 8));
        if (ASRUtils::is_integer(*arg1_type) && ASRUtils::is_integer(*arg2_type)) {
            int64_t a = ASR::down_cast<ASR::IntegerConstant_t>(arg1)->m_n;
            int64_t b = ASR::down_cast<ASR::IntegerConstant_t>(arg2)->m_n;
            if (a == 0 && b < 0) { // Zero Division
                throw SemanticError("0.0 cannot be raised to a negative power.", loc);
            }
            if (b < 0) // Negative power
                return ASR::down_cast<ASR::expr_t>(make_RealConstant_t(al, loc,
                    pow(a, b), real_type));
            else {// Positive power
                if (mod_by == -1)
                    return ASR::down_cast<ASR::expr_t>(make_RealConstant_t(al, loc,
                        pow(a, b), real_type));
                else {
                    int64_t res = (int64_t)pow(a, b);
                    return ASR::down_cast<ASR::expr_t>(make_RealConstant_t(al, loc,
                        (double) (res % mod_by), real_type));
                }
            }

        } else if (ASRUtils::is_real(*arg1_type) && ASRUtils::is_real(*arg2_type)) {
            double a = ASR::down_cast<ASR::RealConstant_t>(arg1)->m_r;
            double b = ASR::down_cast<ASR::RealConstant_t>(arg2)->m_r;
            if (a == 0.0 && b < 0.0) { // Zero Division
                throw SemanticError("0.0 cannot be raised to a negative power.", loc);
            }
            return ASR::down_cast<ASR::expr_t>(make_RealConstant_t(al, loc,
                pow(a, b), real_type));

        } else if (ASRUtils::is_integer(*arg1_type) && ASRUtils::is_real(*arg2_type)) {
            int64_t a = ASR::down_cast<ASR::IntegerConstant_t>(arg1)->m_n;
            double b = ASR::down_cast<ASR::RealConstant_t>(arg2)->m_r;
            if (a == 0 && b < 0.0) { // Zero Division
                throw SemanticError("0.0 cannot be raised to a negative power.", loc);
            }
            return ASR::down_cast<ASR::expr_t>(make_RealConstant_t(al, loc,
                pow(a, b), real_type));

        } else if (ASRUtils::is_real(*arg1_type) && ASRUtils::is_integer(*arg2_type)) {
            double a = ASR::down_cast<ASR::RealConstant_t>(arg1)->m_r;
            int64_t b = ASR::down_cast<ASR::IntegerConstant_t>(arg2)->m_n;
            if (a == 0.0 && b < 0) { // Zero Division
                throw SemanticError("0.0 cannot be raised to a negative power.", loc);
            }
            return ASR::down_cast<ASR::expr_t>(make_RealConstant_t(al, loc,
                pow(a, b), real_type));

        } else if (ASRUtils::is_logical(*arg1_type) && ASRUtils::is_logical(*arg2_type)) {
            bool a = ASR::down_cast<ASR::LogicalConstant_t>(arg1)->m_value;
            bool b = ASR::down_cast<ASR::LogicalConstant_t>(arg2)->m_value;
            return ASR::down_cast<ASR::expr_t>(make_IntegerConstant_t(al, loc,
                pow(a, b), int_type));

        } else if (ASRUtils::is_complex(*arg1_type) && ASRUtils::is_integer(*arg2_type)) {
            double re = ASR::down_cast<ASR::ComplexConstant_t>(arg1)->m_re;
            double im = ASR::down_cast<ASR::ComplexConstant_t>(arg1)->m_im;
            std::complex<double> x(re, im);
            int64_t b = ASR::down_cast<ASR::IntegerConstant_t>(arg2)->m_n;
            std::complex<double> y = pow(x, b);
            return ASR::down_cast<ASR::expr_t>(make_ComplexConstant_t(al, loc,
                y.real(), y.imag(), complex_type));

        } else {
            throw SemanticError("pow() only works on integer, real, logical, and complex types", loc);
        }
    }

    static ASR::expr_t *eval_bin(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 1) {
            throw SemanticError("bin() takes exactly one argument (" +
                std::to_string(args.size()) + " given)", loc);
        }
        ASR::expr_t* expr = args[0];
        ASR::ttype_t* type = ASRUtils::expr_type(expr);
        if (ASRUtils::is_integer(*type)) {
            int64_t n = ASR::down_cast<ASR::IntegerConstant_t>(expr)->m_n;
            std::string str, prefix;
            prefix = n > 0 ? "0b" : "-0b";
            str += std::bitset<64>(std::abs(n)).to_string();
            str.erase(0, str.find_first_not_of('0'));
            str.insert(0, prefix);
            ASR::ttype_t* str_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, str.size(), nullptr));
            return ASR::down_cast<ASR::expr_t>(make_StringConstant_t(al, loc, s2c(al, str), str_type));
        } else {
            throw SemanticError("bin() argument must be an integer, not '" +
                ASRUtils::type_to_str_python(type) + "'", loc);
        }
    }

    static ASR::expr_t *eval_hex(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 1) {
            throw SemanticError("hex() takes exactly one argument (" +
                std::to_string(args.size()) + " given)", loc);
        }
        ASR::expr_t* expr = args[0];
        ASR::ttype_t* type = ASRUtils::expr_type(expr);
        if (ASRUtils::is_integer(*type)) {
            int64_t n = ASR::down_cast<ASR::IntegerConstant_t>(expr)->m_n;
            std::string str, prefix;
            std::stringstream ss;
            prefix = n > 0 ? "0x" : "-0x";
            ss << std::hex << std::abs(n);
            str += ss.str();
            str.insert(0, prefix);
            ASR::ttype_t* str_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, str.size(), nullptr));
            return ASR::down_cast<ASR::expr_t>(make_StringConstant_t(al, loc, s2c(al, str), str_type));
        } else {
            throw SemanticError("hex() argument must be an integer, not '" +
                ASRUtils::type_to_str_python(type) + "'", loc);
        }
    }

    static ASR::expr_t *eval_oct(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 1) {
            throw SemanticError("oct() takes exactly one argument (" +
                std::to_string(args.size()) + " given)", loc);
        }
        ASR::expr_t* expr = args[0];
        ASR::ttype_t* type = ASRUtils::expr_type(expr);
        if (ASRUtils::is_integer(*type)) {
            int64_t n = ASR::down_cast<ASR::IntegerConstant_t>(expr)->m_n;
            std::string str, prefix;
            std::stringstream ss;
            prefix = n > 0 ? "0o" : "-0o";
            ss << std::oct << std::abs(n);
            str += ss.str();
            str.insert(0, prefix);
            ASR::ttype_t* str_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, str.size(), nullptr));
            return ASR::down_cast<ASR::expr_t>(make_StringConstant_t(al, loc, s2c(al, str), str_type));
        } else {
            throw SemanticError("oct() argument must be an integer, not '" +
                ASRUtils::type_to_str_python(type) + "'", loc);
        }
    }

    static ASR::expr_t *eval_list(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() > 1) {
            throw SemanticError("list() takes 0 or 1 argument (" +
                std::to_string(args.size()) + " given)", loc);
        }
        LCOMPILERS_ASSERT(args.size()==1);
        ASR::expr_t *arg = args[0];
        ASR::ttype_t *type = ASRUtils::expr_type(arg);
        ASR::ttype_t* str_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, 1, nullptr));
        if (ASRUtils::is_integer(*type) || ASRUtils::is_real(*type)
                || ASRUtils::is_complex(*type) || ASRUtils::is_logical(*type)) {
            throw SemanticError("Integer, Real, Complex and Boolean are not iterable "
                "and cannot be converted to List", loc);
        } else if (ASR::is_a<ASR::List_t>(*type)) {
            return arg;
        } else if (ASRUtils::is_character(*type)) {
            ASR::ttype_t *list_type = ASRUtils::TYPE(ASR::make_List_t(al, loc, str_type));
            LCOMPILERS_ASSERT(ASRUtils::expr_value(arg) != nullptr)
            std::string c = ASR::down_cast<ASR::StringConstant_t>(arg)->m_s;
            Vec<ASR::expr_t*> list;
            list.reserve(al, c.length());
            std::string r;
            for (size_t i=0; i<c.length(); i++) {
                r.push_back(char(c[i]));
                list.push_back(al, ASR::down_cast<ASR::expr_t>(
                    ASR::make_StringConstant_t(al, loc, s2c(al, r),
                            str_type)));
                r.pop_back();
            }
            return ASR::down_cast<ASR::expr_t>(ASR::make_ListConstant_t(al, loc, list.p,
                list.size(), list_type));
        } else {
            throw SemanticError("'" + ASRUtils::type_to_str_python(type) +
                "' object conversion to List is not implemented ",
                arg->base.loc);
        }
    }

    static ASR::expr_t *eval_round(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
        if (args.size() != 1) {
            throw SemanticError("round() missing required argument 'number' (pos 1)", loc);
        }
        ASR::expr_t* expr = args[0];
        ASR::ttype_t* t = ASRUtils::expr_type(expr);
        if (ASRUtils::is_real(*t)) {
            double rv = ASR::down_cast<ASR::RealConstant_t>(expr)->m_r;
            int64_t rounded = round(rv);
            if (fabs(rv-rounded) == 0.5)
                rounded = 2.0*round(rv/2.0);
            return ASR::down_cast<ASR::expr_t>(make_IntegerConstant_t(al, loc, rounded, type));
        } else if (ASRUtils::is_integer(*t)) {
            int64_t rv = ASR::down_cast<ASR::IntegerConstant_t>(expr)->m_n;
            return ASR::down_cast<ASR::expr_t>(make_IntegerConstant_t(al, loc, rv, type));
        } else if (ASRUtils::is_logical(*t)) {
            int64_t rv = ASR::down_cast<ASR::LogicalConstant_t>(expr)->m_value;
            return ASR::down_cast<ASR::expr_t>(make_IntegerConstant_t(al, loc, rv, type));
        } else {
            throw SemanticError("round() argument must be float, integer, or logical for now, not '" +
                ASRUtils::type_to_str_python(t) + "'", loc);
        }
    }

    static ASR::expr_t *eval_complex(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        int16_t n_args = args.size();
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Complex_t(al, loc, 8));
        if( n_args > 2 || n_args < 0 ) { // n_args shouldn't be less than 0 but added this check for safety
            throw SemanticError("Only constant integer or real values are supported as "
                "the (at most two) arguments of complex()", loc);
        }
        double c1 = 0.0, c2 = 0.0; // Default if n_args = 0
        if (n_args >= 1) { // Handles both n_args = 1 and n_args = 2
            if (ASR::is_a<ASR::IntegerConstant_t>(*args[0])) {
                c1 = ASR::down_cast<ASR::IntegerConstant_t>(args[0])->m_n;
            } else if (ASR::is_a<ASR::RealConstant_t>(*args[0])) {
                c1 = ASR::down_cast<ASR::RealConstant_t>(args[0])->m_r;
            }
        }
        if (n_args == 2) { // Extracts imaginary component if n_args = 2
            if (ASR::is_a<ASR::IntegerConstant_t>(*args[1])) {
                c2 = ASR::down_cast<ASR::IntegerConstant_t>(args[1])->m_n;
            } else if (ASR::is_a<ASR::RealConstant_t>(*args[1])) {
                c2 = ASR::down_cast<ASR::RealConstant_t>(args[1])->m_r;
            }
        }
        return ASR::down_cast<ASR::expr_t>(make_ComplexConstant_t(al, loc, c1, c2, type));
    }

    static ASR::expr_t *eval__lpython_imag(Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args
            ) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 1) {
            throw SemanticError("Intrinsic _lpython_imag function accepts exactly 1 argument", loc);
        }
        ASR::expr_t* imag_arg = args[0];
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, 8));
        if (ASR::is_a<ASR::Complex_t>(*ASRUtils::expr_type(imag_arg))) {
            double im = ASR::down_cast<ASR::ComplexConstant_t>(imag_arg)->m_im;
            double result = im;
            return ASR::down_cast<ASR::expr_t>(ASR::make_RealConstant_t(al, loc, result, type));
        } else {
            throw SemanticError("Argument of the _lpython_imag() function must be Complex", loc);
        }
    }

    static ASR::expr_t *eval__lpython_floordiv(Allocator &al, const Location &loc, Vec<ASR::expr_t *> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 2) {
            throw SemanticError("_lpython_floordiv() takes exactly two arguments (" +
                std::to_string(args.size()) + " given)", loc);
        }
        ASR::expr_t *arg1 = args[0];
        ASR::expr_t *arg2 = args[1];
        ASR::ttype_t *arg1_type = ASRUtils::expr_type(arg1);
        ASR::ttype_t *arg2_type = ASRUtils::expr_type(arg2);
        if (ASRUtils::is_real(*arg1_type) && ASRUtils::is_real(*arg2_type)) {
            int kind = ASRUtils::extract_kind_from_ttype_t(arg1_type);
            ASR::ttype_t *type = nullptr;
            if (kind == 8) {
                type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, 8));
            } else {
                type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, 4));
            }
            double n = ASR::down_cast<ASR::RealConstant_t>(arg1)->m_r;
            double d = ASR::down_cast<ASR::RealConstant_t>(arg2)->m_r;
            double r = n/d, res = 0.0;
            int64_t ival = (int64_t)r;
            if (r > 0 || ival == r) {
                res = ival;
            } else {
                res = ival-1;
            }
            return ASR::down_cast<ASR::expr_t>(make_RealConstant_t(al, loc, res, type));
        } else if (ASRUtils::is_integer(*arg1_type) && ASRUtils::is_integer(*arg2_type)) {
            int kind = ASRUtils::extract_kind_from_ttype_t(arg1_type);
            ASR::ttype_t *type = nullptr;
            if (kind == 8) {
                type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 8));
            } else {
                type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
            }
            int64_t n = ASR::down_cast<ASR::IntegerConstant_t>(arg1)->m_n;
            int64_t d = ASR::down_cast<ASR::IntegerConstant_t>(arg2)->m_n;
            int64_t res = 0;
            double r = 1.0*n/d;
            int64_t ival = (int64_t)r;
            if (r > 0 || ival == r) {
                res = ival;
            } else {
                res = ival-1;
            }
            return ASR::down_cast<ASR::expr_t>(make_IntegerConstant_t(al, loc, res, type));
        } else if (ASRUtils::is_logical(*arg1_type) && ASRUtils::is_logical(*arg2_type)) {
            ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 1));
            bool n = false, d = false;
            ASRUtils::extract_value(arg1, n);
            ASRUtils::extract_value(arg2, d);
            if( !d ) {
                throw SemanticError("Denominator cannot be False or 0.", arg2->base.loc);
            }
            return ASR::down_cast<ASR::expr_t>(make_LogicalConstant_t(al, loc, n, type));
        } else {
            throw SemanticError("Only real/integers/logical arguments are expected.", loc);
        }
    }

    static ASR::expr_t *eval_divmod(Allocator &al, const Location &loc, Vec<ASR::expr_t *> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
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
            int64_t ival1 = ASR::down_cast<ASR::IntegerConstant_t>(arg1)->m_n;
            int64_t ival2 = ASR::down_cast<ASR::IntegerConstant_t>(arg2)->m_n;
            if (ival2 == 0) {
                throw SemanticError("Integer division or modulo by zero not possible", loc);
            } else {
                int64_t div = ival1 / ival2;
                int64_t mod = ival1 % ival2;
                tuple.push_back(al, ASRUtils::EXPR(
                                ASR::make_IntegerConstant_t(al, loc, div, arg1_type)));
                tuple.push_back(al, ASRUtils::EXPR(
                                ASR::make_IntegerConstant_t(al, loc, mod, arg1_type)));
                tuple_type_vec.push_back(al, arg1_type);
                tuple_type_vec.push_back(al, arg2_type);
                ASR::ttype_t *tuple_type = ASRUtils::TYPE(ASR::make_Tuple_t(al, loc,
                                                          tuple_type_vec.p, tuple_type_vec.n));
                return ASR::down_cast<ASR::expr_t>(make_TupleConstant_t(al, loc, tuple.p, tuple.size(), tuple_type));
            }
        } else {
            throw SemanticError("Both arguments of divmod() must be integers for now, not '" +
                ASRUtils::type_to_str_python(arg1_type) + "' and '" + ASRUtils::type_to_str_python(arg2_type) + "'", loc);
        }
    }

    static ASR::expr_t *eval_max(Allocator &/*al*/, const Location &loc, Vec<ASR::expr_t *> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        bool semantic_error_flag = args.size() != 0;
        std::string msg = "max() takes many arguments to comparing";
        ASR::expr_t *first_element = args[0];
        ASR::ttype_t *first_element_type = ASRUtils::expr_type(first_element);
        semantic_error_flag &= ASRUtils::is_integer(*first_element_type)
                               || ASRUtils::is_real(*first_element_type)
                               || ASRUtils::is_character(*first_element_type);
        int32_t biggest_ind = 0;
        if (semantic_error_flag) {
            if (ASRUtils::is_integer(*first_element_type)) {
                int32_t biggest = 0;
                for (size_t i = 0; i < args.size() && semantic_error_flag; i++) {
                    ASR::expr_t *current_arg = args[i];
                    ASR::ttype_t *current_arg_type = ASRUtils::expr_type(current_arg);
                    semantic_error_flag &= current_arg_type->type == first_element_type->type;
                    if (!semantic_error_flag) {
                        msg = "type of arg in index [" + std::to_string(i) = "] is not comparable";
                        break;
                    }
                    int32_t current_val = ASR::down_cast<ASR::IntegerConstant_t>(current_arg)->m_n;
                    if (i == 0) {
                        biggest = current_val;
                        biggest_ind = 0;
                    } else {
                        if (current_val > biggest) {
                            biggest = current_val;
                            biggest_ind = i;
                        }
                    }
                }
                if (semantic_error_flag) {
                    return args[biggest_ind];
                }
            } else if (ASRUtils::is_real(*first_element_type)) {
                double_t biggest = 0;
                for (size_t i = 0; i < args.size() && semantic_error_flag; i++) {
                    ASR::expr_t *current_arg = args[i];
                    ASR::ttype_t *current_arg_type = ASRUtils::expr_type(current_arg);
                    semantic_error_flag &= current_arg_type->type == first_element_type->type;
                    if (!semantic_error_flag) {
                        msg = "type of arg in index [" + std::to_string(i) = "] is not comparable";
                        break;
                    }
                    double_t current_val = ASR::down_cast<ASR::RealConstant_t>(current_arg)->m_r;
                    if (i == 0) {
                        biggest = current_val;
                        biggest_ind = 0;
                    } else {
                        if (current_val - biggest > 1e-6) {
                            biggest = current_val;
                            biggest_ind = i;
                        }
                    }
                }
                if (semantic_error_flag) {
                    return args[biggest_ind];
                }
            }
        }
        throw SemanticError(msg, loc);
    }

    static ASR::expr_t *eval_min(Allocator &/*al*/, const Location &loc, Vec<ASR::expr_t *> &args) {
        LCOMPILERS_ASSERT(ASRUtils::all_args_evaluated(args));
        bool semantic_error_flag = args.size() != 0;
        std::string msg = "min() takes many arguments to comparing";
        ASR::expr_t *first_element = args[0];
        ASR::ttype_t *first_element_type = ASRUtils::expr_type(first_element);
        semantic_error_flag &= ASRUtils::is_integer(*first_element_type)
                               || ASRUtils::is_real(*first_element_type)
                               || ASRUtils::is_character(*first_element_type);
        int32_t smallest_ind = 0;
        if (semantic_error_flag) {
            if (ASRUtils::is_integer(*first_element_type)) {
                int32_t smallest = 0;
                for (size_t i = 0; i < args.size() && semantic_error_flag; i++) {
                    ASR::expr_t *current_arg = args[i];
                    ASR::ttype_t *current_arg_type = ASRUtils::expr_type(current_arg);
                    semantic_error_flag &= current_arg_type->type == first_element_type->type;
                    if (!semantic_error_flag) {
                        msg = "type of arg in index [" + std::to_string(i) = "] is not comparable";
                        break;
                    }
                    int32_t current_val = ASR::down_cast<ASR::IntegerConstant_t>(current_arg)->m_n;
                    if (i == 0) {
                        smallest = current_val;
                        smallest_ind = 0;
                    } else {
                        if (current_val < smallest) {
                            smallest = current_val;
                            smallest_ind = i;
                        }
                    }
                }
                if (semantic_error_flag) {
                    return args[smallest_ind];
                }
            } else if (ASRUtils::is_real(*first_element_type)) {
                double_t smallest = 0;
                for (size_t i = 0; i < args.size() && semantic_error_flag; i++) {
                    ASR::expr_t *current_arg = args[i];
                    ASR::ttype_t *current_arg_type = ASRUtils::expr_type(current_arg);
                    semantic_error_flag &= current_arg_type->type == first_element_type->type;
                    if (!semantic_error_flag) {
                        msg = "type of arg in index [" + std::to_string(i) = "] is not comparable";
                        break;
                    }
                    double_t current_val = ASR::down_cast<ASR::RealConstant_t>(current_arg)->m_r;
                    if (i == 0) {
                        smallest = current_val;
                        smallest_ind = 0;
                    } else {
                        if (smallest - current_val > 1e-6) {
                            smallest = current_val;
                            smallest_ind = i;
                        }
                    }
                }
                if (semantic_error_flag) {
                    return args[smallest_ind];
                }
            }
        }
        throw SemanticError(msg, loc);

    }

}; // ComptimeEval

} // namespace LCompilers::LPython

#endif /* LPYTHON_SEMANTICS_COMPTIME_EVAL_H */
