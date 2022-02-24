#ifndef LPYTHON_SEMANTICS_COMPTIME_EVAL_H
#define LPYTHON_SEMANTICS_COMPTIME_EVAL_H

#include <complex>
#include <string>
#include <cstring>

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
            double rv = ASR::down_cast<ASR::ConstantReal_t>(arg)->m_r;
            result = rv ? true : false;
        } else if (ASRUtils::is_integer(*t)) {
            int64_t rv = ASR::down_cast<ASR::ConstantInteger_t>(arg)->m_n;
            result = rv ? true : false;
        } else if (ASRUtils::is_complex(*t)) {
            double re = ASR::down_cast<ASR::ConstantComplex_t>(arg)->m_re;
            double im = ASR::down_cast<ASR::ConstantComplex_t>(arg)->m_im;
            std::complex<double> c(re, im);
            result = (re || im) ? true : false;
        } else if (ASRUtils::is_logical(*t)) {
            bool rv = ASR::down_cast<ASR::ConstantLogical_t>(arg)->m_value;
            result = rv;
        } else if (ASRUtils::is_character(*t)) {
            char* c = ASR::down_cast<ASR::ConstantString_t>(ASRUtils::expr_value(arg))->m_s;
            result = strlen(s2c(al, std::string(c))) ? true : false;
        } else {
            throw SemanticError("bool() must have one real, integer, character,"
                " complex, or logical argument", loc);
        }
        return ASR::down_cast<ASR::expr_t>(make_ConstantLogical_t(al, loc, result, type));
    }

}; // ComptimeEval

} // namespace LFortran

#endif /* LPYTHON_SEMANTICS_COMPTIME_EVAL_H */
