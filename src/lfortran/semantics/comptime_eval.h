#ifndef LFORTRAN_SEMANTICS_AST_COMPTIME_EVAL_H
#define LFORTRAN_SEMANTICS_AST_COMPTIME_EVAL_H

#include <complex>

#include <lfortran/asr.h>
#include <lfortran/ast.h>
#include <lfortran/bigint.h>
#include <lfortran/string_utils.h>
#include <lfortran/utils.h>

namespace LFortran {

struct IntrinsicProcedures {

    const std::string m_kind = "lfortran_intrinsic_kind";
    const std::string m_builtin = "lfortran_intrinsic_builtin";
    const std::string m_trig = "lfortran_intrinsic_trig";
    const std::string m_math = "lfortran_intrinsic_math";
    const std::string m_math2 = "lfortran_intrinsic_math2";
    const std::string m_string = "lfortran_intrinsic_string";
    const std::string m_bit = "lfortran_intrinsic_bit";

    /*
        The last parameter is true if the callback accepts evaluated arguments.

        If true, the arguments are first converted to their compile time
        "values". If not possible, nullptr is returned; otherwise the
        callback is called and it always succeeds to evaluate the result at
        compile time.

        If false, the arguments might not be compile time values. The
        callback can return nullptr if it cannot evaluate itself.
    */

    typedef ASR::expr_t* (*comptime_eval_callback)(Allocator &, const Location &, Vec<ASR::expr_t*> &);
    std::map<std::string, std::tuple<std::string, comptime_eval_callback, bool>> comptime_eval_map;

    IntrinsicProcedures() {
        comptime_eval_map = {
            // Arguments can be evaluated or not
            {"kind", {m_kind, &eval_kind, false}},
            {"tiny", {m_builtin, &eval_tiny, false}},
            {"int", {m_builtin, &eval_int, false}},
            {"real", {m_builtin, &not_implemented, false}}, // Implemented separately

            // Require evaluated arguments
            {"aimag", {m_math, &eval_aimag, true}},
            {"char", {m_builtin, &eval_char, true}},
            {"floor", {m_math2, &eval_floor, true}},
            {"nint", {m_math2, &eval_nint, true}},
            {"mod", {m_math2, &eval_mod, true}},
            {"modulo", {m_math2, &eval_modulo, true}},
            {"min", {m_math2, &eval_min, true}},
            {"max", {m_math2, &eval_max, true}},
            {"selected_int_kind", {m_kind, &eval_selected_int_kind, true}},
            {"selected_real_kind", {m_kind, &eval_selected_real_kind, true}},
            {"selected_char_kind", {m_kind, &eval_selected_char_kind, true}},
            {"exp", {m_math, &eval_exp, true}},
            {"log", {m_math, &eval_log, true}},
            {"erf", {m_math, &eval_erf, true}},
            {"erfc", {m_math, &eval_erfc, true}},
            {"abs", {m_math, &eval_abs, true}},
            {"sqrt", {m_math, &eval_sqrt, true}},
            {"gamma", {m_math, &eval_gamma, true}},
            {"log_gamma", {m_math, &eval_log_gamma, true}},
            {"log10", {m_math, &eval_log10, true}},

            //{"sin", {m_trig, &eval_sin, true}},
            {"sin", {m_math, &eval_sin, true}},
            {"cos", {m_math, &eval_cos, true}},
            {"tan", {m_math, &eval_tan, true}},

            {"asin", {m_math, &eval_asin, true}},
            {"acos", {m_math, &eval_acos, true}},
            {"atan", {m_math, &eval_atan, true}},

            {"sinh", {m_math, &eval_sinh, true}},
            {"cosh", {m_math, &eval_cosh, true}},
            {"tanh", {m_math, &eval_tanh, true}},

            {"asinh", {m_math, &eval_asinh, true}},
            {"acosh", {m_math, &eval_acosh, true}},
            {"atanh", {m_math, &eval_atanh, true}},

            {"atan2", {m_math, &eval_atan2, true}},

            {"iand", {m_bit, &not_implemented, false}},
            {"ior", {m_bit, &not_implemented, false}},
            {"ieor", {m_bit, &eval_ieor, true}},
            {"ibclr", {m_bit, &eval_ibclr, true}},
            {"ibset", {m_bit, &eval_ibset, true}},
            {"btest", {m_bit, &not_implemented, false}},

            // These will fail if used in symbol table visitor, but will be
            // left unevaluated in body visitor
            {"trim", {m_string, &not_implemented, false}},
            {"len_trim", {m_string, &not_implemented, false}},

            // Subroutines
            {"cpu_time", {m_math, &not_implemented, false}},
            {"bit_size", {m_builtin, &not_implemented, false}},
            {"not", {m_builtin, &not_implemented, false}},
            {"iachar",  {m_builtin, &not_implemented, false}},
            {"achar", {m_builtin, &eval_achar, false}},
            {"len", {m_builtin, &not_implemented, false}},
            {"size", {m_builtin, &not_implemented, false}},
            {"present", {m_builtin, &not_implemented, false}},
            {"lbound", {m_builtin, &not_implemented, false}},
            {"ubound", {m_builtin, &not_implemented, false}},
            {"allocated", {m_builtin, &not_implemented, false}},
            {"minval", {m_builtin, &not_implemented, false}},
            {"maxval", {m_builtin, &not_implemented, false}},
            {"sum", {m_builtin, &not_implemented, false}},
            {"bit_size", {m_builtin, &not_implemented, false}},
            {"not", {m_builtin, &not_implemented, false}},
            {"index", {m_string, &not_implemented, false}},
        };
    }

    bool is_intrinsic(std::string name) const {
        auto search = comptime_eval_map.find(name);
        if (search != comptime_eval_map.end()) {
            return true;
        } else {
            return false;
        }
    }

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

    ASR::expr_t *comptime_eval(std::string name, Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) const {
        auto search = comptime_eval_map.find(name);
        if (search != comptime_eval_map.end()) {
            comptime_eval_callback cb = std::get<1>(search->second);
            bool eval_args = std::get<2>(search->second);
            if (eval_args) {
                Vec<ASR::expr_t*> arg_values = ASRUtils::get_arg_values(al, args);
                if (arg_values.size() != args.size()) return nullptr;
                return cb(al, loc, arg_values);
            } else {
                return cb(al, loc, args);
            }
        } else {
            throw SemanticError("Intrinsic function '" + name
                + "' compile time evaluation is not implemented yet",
                loc);
        }
    }

    static ASR::expr_t *eval_kind(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        // TODO: Refactor to allow early return
        // kind_num --> value {4, 8, etc.}
        int64_t kind_num = 4; // Default
        ASR::expr_t* kind_expr = args[0];
        // TODO: Check that the expression reduces to a valid constant expression (10.1.12)
        switch( kind_expr->type ) {
            case ASR::exprType::ConstantInteger: {
                kind_num = ASR::down_cast<ASR::Integer_t>(ASR::down_cast<ASR::ConstantInteger_t>(kind_expr)->m_type)->m_kind;
                break;
            }
            case ASR::exprType::ConstantReal:{
                kind_num = ASR::down_cast<ASR::Real_t>(ASR::down_cast<ASR::ConstantReal_t>(kind_expr)->m_type)->m_kind;
                break;
            }
            case ASR::exprType::ConstantLogical:{
                kind_num = ASR::down_cast<ASR::Logical_t>(ASR::down_cast<ASR::ConstantLogical_t>(kind_expr)->m_type)->m_kind;
                break;
            }
            case ASR::exprType::Var : {
                kind_num = ASRUtils::extract_kind(kind_expr, loc);
                break;
            }
            default: {
                std::string msg = R"""(Only Integer literals or expressions which reduce to constant Integer are accepted as kind parameters.)""";
                throw SemanticError(msg, loc);
                break;
            }
        }
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(
                ASR::make_Integer_t(al, loc,
                    4, nullptr, 0));
        return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc,
                kind_num, type));
    }

    static ASR::expr_t *eval_tiny(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        // We assume the input is valid
        // ASR::expr_t* tiny_expr = args[0];
        ASR::ttype_t* tiny_type = LFortran::ASRUtils::expr_type(args[0]);
        // TODO: Arrays of reals are a valid argument for tiny
        if (LFortran::ASRUtils::is_array(tiny_type)){
            throw SemanticError("Array values not implemented yet",
                                loc);
        }
        // TODO: Figure out how to deal with higher precision later
        if (ASR::is_a<LFortran::ASR::Real_t>(*tiny_type)) {
            // We don't actually need the value yet, it is enough to know it is a double
            // but it might provide further information later (precision)
            // double tiny_val = ASR::down_cast<ASR::ConstantReal_t>(LFortran::ASRUtils::expr_value(tiny_expr))->m_r;
            int tiny_kind = LFortran::ASRUtils::extract_kind_from_ttype_t(tiny_type);
            if (tiny_kind == 4){
                float low_val = std::numeric_limits<float>::min();
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, loc,
                                                                                low_val, // value
                                                                                tiny_type));
            } else {
                double low_val = std::numeric_limits<double>::min();
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, loc,
                                                                                low_val, // value
                                                                                tiny_type));
                    }
        }
        else {
            throw SemanticError("Argument for tiny must be Real",
                                loc);
        }
    }

    static ASR::expr_t *eval_floor(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        // TODO: Implement optional kind; J3/18-007r1 --> FLOOR(A, [KIND])
        ASR::expr_t* func_expr = args[0];
        ASR::ttype_t* func_type = LFortran::ASRUtils::expr_type(func_expr);
        if (LFortran::ASR::is_a<LFortran::ASR::Real_t>(*func_type)) {
            double rv = ASR::down_cast<ASR::ConstantReal_t>(func_expr)->m_r;
            int64_t ival = floor(rv);
            ASR::ttype_t *type = LFortran::ASRUtils::TYPE(
                    ASR::make_Integer_t(al, loc, 4, nullptr, 0));
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, ival, type));
        } else {
            throw SemanticError("floor must have one real argument", loc);
        }
    }

    static ASR::expr_t *eval_nint(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::expr_t* func_expr = args[0];
        ASR::ttype_t* func_type = LFortran::ASRUtils::expr_type(func_expr);
        if (LFortran::ASR::is_a<LFortran::ASR::Real_t>(*func_type)) {
            double rv = ASR::down_cast<ASR::ConstantReal_t>(func_expr)->m_r;
            int64_t ival = round(rv);
            ASR::ttype_t *type = LFortran::ASRUtils::TYPE(
                    ASR::make_Integer_t(al, loc, 4, nullptr, 0));
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, ival, type));
        } else {
            throw SemanticError("nint must have one real argument", loc);
        }
    }

    typedef double (*trig_eval_callback_double)(double);
    typedef std::complex<double> (*trig_eval_callback_complex_double)(std::complex<double>);
    static ASR::expr_t *eval_trig(Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args,
            trig_eval_callback_double trig_double,
            trig_eval_callback_complex_double trig_complex_double
            ) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 1) {
            throw SemanticError("Intrinsic trig function accepts exactly 1 argument", loc);
        }
        ASR::expr_t* trig_arg = args[0];
        ASR::ttype_t* t = LFortran::ASRUtils::expr_type(args[0]);
        if (LFortran::ASR::is_a<LFortran::ASR::Real_t>(*t)) {
            double rv = ASR::down_cast<ASR::ConstantReal_t>(trig_arg)->m_r;
            double val = trig_double(rv);
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, loc, val, t));
        } else if (LFortran::ASR::is_a<LFortran::ASR::Complex_t>(*t)) {
            if (trig_complex_double) {
                double re = ASR::down_cast<ASR::ConstantComplex_t>(trig_arg)->m_re;
                double im = ASR::down_cast<ASR::ConstantComplex_t>(trig_arg)->m_im;
                std::complex<double> x(re, im);
                std::complex<double> result = trig_complex_double(x);
                re = std::real(result);
                im = std::imag(result);
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantComplex_t(al, loc, re, im, t));
            } else {
                return nullptr;
            }
        } else {
            throw SemanticError("Argument for trig function must be Real or Complex", loc);
        }
    }

    typedef double (*eval2_callback_double)(double, double);
    static ASR::expr_t *eval_2args(Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args,
            eval2_callback_double eval2_double
            ) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 2) {
            throw SemanticError("This intrinsic function accepts exactly 2 arguments", loc);
        }
        ASR::expr_t* trig_arg1 = args[0];
        ASR::ttype_t* t1 = LFortran::ASRUtils::expr_type(args[0]);
        ASR::expr_t* trig_arg2 = args[1];
        ASR::ttype_t* t2 = LFortran::ASRUtils::expr_type(args[1]);
        if (ASR::is_a<LFortran::ASR::Real_t>(*t1) && ASR::is_a<LFortran::ASR::Real_t>(*t2)) {
            double rv1 = ASR::down_cast<ASR::ConstantReal_t>(trig_arg1)->m_r;
            double rv2 = ASR::down_cast<ASR::ConstantReal_t>(trig_arg2)->m_r;
            double val = eval2_double(rv1, rv2);
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, loc, val, t1));
        } else {
            throw SemanticError("Arguments for this intrinsic function must be Real", loc);
        }
    }

    typedef int64_t (*eval2_callback_int)(int64_t, int64_t);
    static ASR::expr_t *eval_2args_ri(Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args,
            eval2_callback_double eval2_double,
            eval2_callback_int eval2_int
            ) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 2) {
            throw SemanticError("This intrinsic function accepts exactly 2 arguments", loc);
        }
        ASR::expr_t* trig_arg1 = args[0];
        ASR::ttype_t* t1 = LFortran::ASRUtils::expr_type(args[0]);
        ASR::expr_t* trig_arg2 = args[1];
        ASR::ttype_t* t2 = LFortran::ASRUtils::expr_type(args[1]);
        if (ASR::is_a<LFortran::ASR::Real_t>(*t1) && ASR::is_a<LFortran::ASR::Real_t>(*t2)) {
            double rv1 = ASR::down_cast<ASR::ConstantReal_t>(trig_arg1)->m_r;
            double rv2 = ASR::down_cast<ASR::ConstantReal_t>(trig_arg2)->m_r;
            double val = eval2_double(rv1, rv2);
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, loc, val, t1));
        } else if (ASR::is_a<LFortran::ASR::Integer_t>(*t1) && ASR::is_a<LFortran::ASR::Integer_t>(*t2)) {
            int64_t rv1 = ASR::down_cast<ASR::ConstantInteger_t>(trig_arg1)->m_n;
            int64_t rv2 = ASR::down_cast<ASR::ConstantInteger_t>(trig_arg2)->m_n;
            int64_t val = eval2_int(rv1, rv2);
            ASR::ttype_t *type = LFortran::ASRUtils::TYPE(
                    ASR::make_Integer_t(al, loc, 4, nullptr, 0));
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, val, type));
        } else {
            throw SemanticError("Arguments for this intrinsic function must be Real or Integer", loc);
        }
    }

#define TRIG_CB(X) static std::complex<double> lfortran_z##X(std::complex<double> x) { return std::X(x); }
#define TRIG_CB2(X) static ASR::expr_t *eval_##X(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) { \
        return eval_trig(al, loc, args, &X, &lfortran_z##X); \
    }
#define TRIG(X) TRIG_CB(X) \
    TRIG_CB2(X)

TRIG(sin)
TRIG(cos)
TRIG(tan)
TRIG(asin)
TRIG(acos)
TRIG(atan)
TRIG(sinh)
TRIG(cosh)
TRIG(tanh)
TRIG(asinh)
TRIG(acosh)
TRIG(atanh)

TRIG(exp)
TRIG(log)
TRIG(sqrt)

    static ASR::expr_t *eval_erf(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &erf, nullptr);
    }
    static ASR::expr_t *eval_erfc(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &erfc, nullptr);
    }
    static ASR::expr_t *eval_gamma(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &tgamma, nullptr);
    }
    static ASR::expr_t *eval_log_gamma(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &lgamma, nullptr);
    }
    static ASR::expr_t *eval_log10(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &log10, nullptr);
    }
    static ASR::expr_t *eval_atan2(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_2args(al, loc, args, &atan2);
    }

    static double lfortran_modulo(double x, double y) {
        if (x > 0 && y > 0) {
            return std::fmod(x, y);
        } else if (x < 0 && y < 0) {
            return -std::fmod(-x, -y);
        } else {
            return std::remainder(x, y);
        }
    }

    static int64_t lfortran_modulo_i(int64_t x, int64_t y) {
        if (x > 0 && y > 0) {
            return std::fmod(x, y);
        } else if (x < 0 && y < 0) {
            return -std::fmod(-x, -y);
        } else {
            return std::remainder(x, y);
        }
    }

    static double lfortran_mod(double x, double y) {
        return std::fmod(x, y);
    }

    static int64_t lfortran_mod_i(int64_t x, int64_t y) {
        return std::fmod(x, y);
    }

    static ASR::expr_t *eval_modulo(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_2args_ri(al, loc, args,
            &IntrinsicProcedures::lfortran_modulo,
            &IntrinsicProcedures::lfortran_modulo_i);
    }

    static ASR::expr_t *eval_mod(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_2args_ri(al, loc, args,
            &IntrinsicProcedures::lfortran_mod,
            &IntrinsicProcedures::lfortran_mod_i);
    }

    static double lfortran_min(double x, double y) {
        return std::fmin(x, y);
    }

    static int64_t lfortran_min_i(int64_t x, int64_t y) {
        return std::fmin(x, y);
    }

    static ASR::expr_t *eval_min(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_2args_ri(al, loc, args,
            &IntrinsicProcedures::lfortran_min,
            &IntrinsicProcedures::lfortran_min_i);
    }

    static double lfortran_max(double x, double y) {
        return std::fmax(x, y);
    }

    static int64_t lfortran_max_i(int64_t x, int64_t y) {
        return std::fmax(x, y);
    }

    static ASR::expr_t *eval_max(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_2args_ri(al, loc, args,
            &IntrinsicProcedures::lfortran_max,
            &IntrinsicProcedures::lfortran_max_i);
    }

    static ASR::expr_t *eval_abs(Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args
            ) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 1) {
            throw SemanticError("Intrinsic trig function accepts exactly 1 argument", loc);
        }
        ASR::expr_t* trig_arg = args[0];
        ASR::ttype_t* t = LFortran::ASRUtils::expr_type(args[0]);
        if (LFortran::ASR::is_a<LFortran::ASR::Real_t>(*t)) {
            double rv = ASR::down_cast<ASR::ConstantReal_t>(trig_arg)->m_r;
            double val = std::abs(rv);
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, loc, val, t));
        } else if (LFortran::ASR::is_a<LFortran::ASR::Integer_t>(*t)) {
            int64_t rv = ASR::down_cast<ASR::ConstantInteger_t>(trig_arg)->m_n;
            int64_t val = std::abs(rv);
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, val, t));
        } else if (LFortran::ASR::is_a<LFortran::ASR::Complex_t>(*t)) {
            double re = ASR::down_cast<ASR::ConstantComplex_t>(trig_arg)->m_re;
            double im = ASR::down_cast<ASR::ConstantComplex_t>(trig_arg)->m_im;
            std::complex<double> x(re, im);
            double result = std::abs(x);
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, loc, result, t));
        } else {
            throw SemanticError("Argument of the abs function must be Integer, Real or Complex", loc);
        }
    }

    static ASR::expr_t *eval_aimag(Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args
            ) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 1) {
            throw SemanticError("Intrinsic trig function accepts exactly 1 argument", loc);
        }
        ASR::expr_t* trig_arg = args[0];
        ASR::ttype_t* t = LFortran::ASRUtils::expr_type(args[0]);
        if (LFortran::ASR::is_a<LFortran::ASR::Complex_t>(*t)) {
            double im = ASR::down_cast<ASR::ConstantComplex_t>(trig_arg)->m_im;
            double result = im;
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, loc, result, t));
        } else {
            throw SemanticError("Argument of the aimag() function must be Complex", loc);
        }
    }

    static ASR::expr_t *eval_int(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        ASR::expr_t* int_expr = args[0];
        if( int_expr->type == ASR::exprType::BOZ ) {
            ASR::BOZ_t *boz_expr = ASR::down_cast<ASR::BOZ_t>(int_expr);
            ASR::ttype_t* tmp_int_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, boz_expr->m_v, tmp_int_type));;
        }
        ASR::ttype_t* int_type = LFortran::ASRUtils::expr_type(int_expr);
        int int_kind = ASRUtils::extract_kind_from_ttype_t(int_type);
        if (LFortran::ASR::is_a<LFortran::ASR::Integer_t>(*int_type)) {
            if (int_kind == 4){
                int64_t ival = ASR::down_cast<ASR::ConstantInteger_t>(LFortran::ASRUtils::expr_value(int_expr))->m_n;
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, ival, int_type));
            } else {
                int64_t ival = ASR::down_cast<ASR::ConstantInteger_t>(LFortran::ASRUtils::expr_value(int_expr))->m_n;
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, ival, int_type));
            }
        } else if (LFortran::ASR::is_a<LFortran::ASR::Real_t>(*int_type)) {
            if (int_kind == 4){
                float rv = ASR::down_cast<ASR::ConstantReal_t>(
                    LFortran::ASRUtils::expr_value(int_expr))->m_r;
                int64_t ival = static_cast<int64_t>(rv);
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, ival, int_type));
            } else {
                double rv = ASR::down_cast<ASR::ConstantReal_t>(LFortran::ASRUtils::expr_value(int_expr))->m_r;
                int64_t ival = static_cast<int64_t>(rv);
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, ival, int_type));
            }
        } else {
            throw SemanticError("int must have only one argument", loc);
        }
    }

    static ASR::expr_t *not_implemented(Allocator &/*al*/, const Location &/*loc*/, Vec<ASR::expr_t*> &/*args*/) {
        // This intrinsic is not evaluated at compile time yet
        return nullptr;
    }

    static ASR::expr_t *eval_char(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::expr_t* real_expr = args[0];
        ASR::ttype_t* real_type = LFortran::ASRUtils::expr_type(real_expr);
        if (LFortran::ASR::is_a<LFortran::ASR::Integer_t>(*real_type)) {
            int64_t c = ASR::down_cast<ASR::ConstantInteger_t>(real_expr)->m_n;
            ASR::ttype_t* str_type =
                LFortran::ASRUtils::TYPE(ASR::make_Character_t(al,
                loc, 1, 1, nullptr, nullptr, 0));
            if (! (c >= 0 && c <= 127) ) {
                throw SemanticError("The argument 'x' in char(x) must be in the range 0 <= x <= 127.", loc);
            }
            char cc = c;
            std::string svalue;
            svalue += cc;
            Str s;
            s.from_str_view(svalue);
            char *str_val = s.c_str(al);
            return ASR::down_cast<ASR::expr_t>(
                ASR::make_ConstantString_t(al, loc,
                str_val, str_type));
        } else {
            throw SemanticError("char() must have one integer argument", loc);
        }
    }

    static ASR::expr_t *eval_achar(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::expr_t* int_expr = args[0];
        ASR::ttype_t* int_type = LFortran::ASRUtils::expr_type(int_expr);
        if (LFortran::ASR::is_a<LFortran::ASR::Integer_t>(*int_type)) {
            int64_t c = ASR::down_cast<ASR::ConstantInteger_t>(LFortran::ASRUtils::expr_value(int_expr))->m_n;
            ASR::ttype_t* str_type =
                LFortran::ASRUtils::TYPE(ASR::make_Character_t(al,
                loc, 1, 1, nullptr, nullptr, 0));
            char cc = c;
            std::string svalue;
            svalue += cc;
            Str s;
            s.from_str_view(svalue);
            char *str_val = s.c_str(al);
            return ASR::down_cast<ASR::expr_t>(
                ASR::make_ConstantString_t(al, loc,
                str_val, str_type));
        } else {
            throw SemanticError("achar() must have one integer argument", loc);
        }
    }

    static ASR::expr_t *eval_selected_int_kind(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::expr_t* real_expr = args[0];
        ASR::ttype_t* real_type = LFortran::ASRUtils::expr_type(real_expr);
        if (LFortran::ASR::is_a<LFortran::ASR::Integer_t>(*real_type)) {
            int64_t R = ASR::down_cast<ASR::ConstantInteger_t>(
                LFortran::ASRUtils::expr_value(real_expr))->m_n;
            int a_kind = 4;
            if (R < 10) {
                a_kind = 4;
            } else {
                a_kind = 8;
            }
            ASR::ttype_t *type = LFortran::ASRUtils::TYPE(
                    ASR::make_Integer_t(al, loc,
                        4, nullptr, 0));
            return ASR::down_cast<ASR::expr_t>(
                ASR::make_ConstantInteger_t(al, loc,
                a_kind, type));
        } else {
            throw SemanticError("integer_int_kind() must have one integer argument", loc);
        }
    }
    static ASR::expr_t *eval_selected_real_kind(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        // TODO: Be more standards compliant 16.9.170
        // e.g. selected_real_kind(6, 70)
        ASR::expr_t* real_expr = args[0];
        ASR::ttype_t* real_type = LFortran::ASRUtils::expr_type(real_expr);
        if (LFortran::ASR::is_a<LFortran::ASR::Integer_t>(*real_type)) {
            int64_t R = ASR::down_cast<ASR::ConstantInteger_t>(
                LFortran::ASRUtils::expr_value(real_expr))->m_n;
            int a_kind = 4;
            if (R < 7) {
                a_kind = 4;
            } else {
                a_kind = 8;
            }
            ASR::ttype_t *type = LFortran::ASRUtils::TYPE(
                    ASR::make_Integer_t(al, loc,
                        4, nullptr, 0));
            return ASR::down_cast<ASR::expr_t>(
                ASR::make_ConstantInteger_t(al, loc,
                a_kind, type));
        } else {
            throw SemanticError("integer_real_kind() must have one integer argument", loc);
        }
    }
    static ASR::expr_t *eval_selected_char_kind(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        ASR::expr_t* real_expr = args[0];
        ASR::ttype_t* real_type = LFortran::ASRUtils::expr_type(real_expr);
        if (LFortran::ASR::is_a<LFortran::ASR::Character_t>(*real_type)) {
            ASR::ttype_t *type = LFortran::ASRUtils::TYPE(
                    ASR::make_Integer_t(al, loc,
                        4, nullptr, 0));
            return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc,
                    1, type));
        } else {
            throw SemanticError("integer_char_kind() must have one character argument", loc);
        }
    }

    static ASR::expr_t *eval_ibclr(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 2) {
            throw SemanticError("The ibclr intrinsic function accepts exactly 2 arguments", loc);
        }
        ASR::expr_t* arg1 = args[0];
        ASR::expr_t* arg2 = args[1];
        ASR::ttype_t* t1 = LFortran::ASRUtils::expr_type(arg1);
        ASR::ttype_t* t2 = LFortran::ASRUtils::expr_type(arg2);
        if (ASR::is_a<LFortran::ASR::Integer_t>(*t1) && ASR::is_a<LFortran::ASR::Integer_t>(*t2)) {
            int pos = ASR::down_cast<ASR::ConstantInteger_t>(arg2)->m_n;
            int t1_kind = ASRUtils::extract_kind_from_ttype_t(t1);
            if (t1_kind == 4 && pos >= 0 && pos < 32) {
                int32_t i = ASR::down_cast<ASR::ConstantInteger_t>(arg1)->m_n;
                int32_t val = IntrinsicProcedures::lfortran_ibclr32(i, pos);
                ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc, t1_kind, nullptr, 0));
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, val, type));
            } else if (t1_kind == 8 && pos >= 0 && pos < 64) {
                int64_t i = ASR::down_cast<ASR::ConstantInteger_t>(arg1)->m_n;
                int64_t val = IntrinsicProcedures::lfortran_ibclr64(i, pos);
                ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc, t1_kind, nullptr, 0));
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, val, type));
            } else {
                throw SemanticError("ibclr(i, pos) for pos < 0 or pos >= bit_size(i) is not allowed", loc);
            }
        } else {
            throw SemanticError("Arguments for this intrinsic function must be Integer", loc);
        }
    }

    static ASR::expr_t *eval_ibset(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 2) {
            throw SemanticError("The ibset intrinsic function accepts exactly 2 arguments", loc);
        }
        ASR::expr_t* arg1 = args[0];
        ASR::expr_t* arg2 = args[1];
        ASR::ttype_t* t1 = LFortran::ASRUtils::expr_type(arg1);
        ASR::ttype_t* t2 = LFortran::ASRUtils::expr_type(arg2);
        if (ASR::is_a<LFortran::ASR::Integer_t>(*t1) && ASR::is_a<LFortran::ASR::Integer_t>(*t2)) {
            int pos = ASR::down_cast<ASR::ConstantInteger_t>(arg2)->m_n;
            int t1_kind = ASRUtils::extract_kind_from_ttype_t(t1);
            if (t1_kind == 4 && pos >= 0 && pos < 32) {
                int32_t i = ASR::down_cast<ASR::ConstantInteger_t>(arg1)->m_n;
                int32_t val = IntrinsicProcedures::lfortran_ibset32(i, pos);
                ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc, t1_kind, nullptr, 0));
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, val, type));
            } else if (t1_kind == 8 && pos >= 0 && pos < 64) {
                int64_t i = ASR::down_cast<ASR::ConstantInteger_t>(arg1)->m_n;
                int64_t val = IntrinsicProcedures::lfortran_ibset64(i, pos);
                ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc, t1_kind, nullptr, 0));
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, val, type));
            } else {
                throw SemanticError("ibset(i, pos) for pos < 0 or pos >= bit_size(i) is not allowed", loc);
            }
        } else {
            throw SemanticError("Arguments for this intrinsic function must be Integer", loc);
        }
    }

    static ASR::expr_t *eval_ieor(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        if (args.size() != 2) {
            throw SemanticError("The ieor intrinsic function accepts exactly 2 arguments", loc);
        }
        ASR::expr_t* arg1 = args[0];
        ASR::expr_t* arg2 = args[1];
        ASR::ttype_t* t1 = LFortran::ASRUtils::expr_type(arg1);
        ASR::ttype_t* t2 = LFortran::ASRUtils::expr_type(arg2);
        if (ASR::is_a<LFortran::ASR::Integer_t>(*t1) && ASR::is_a<LFortran::ASR::Integer_t>(*t2)) {
            int t1_kind = ASRUtils::extract_kind_from_ttype_t(t1);
            int t2_kind = ASRUtils::extract_kind_from_ttype_t(t2);
            if (t1_kind == 4 && t2_kind == 4) {
                int32_t x = ASR::down_cast<ASR::ConstantInteger_t>(arg1)->m_n;
                int32_t y = ASR::down_cast<ASR::ConstantInteger_t>(arg2)->m_n;
                int32_t val = IntrinsicProcedures::lfortran_ieor32(x, y);
                ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc, t1_kind, nullptr, 0));
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, val, type));
            } else if (t1_kind == 8 && t2_kind == 8) {
                int64_t x = ASR::down_cast<ASR::ConstantInteger_t>(arg1)->m_n;
                int64_t y = ASR::down_cast<ASR::ConstantInteger_t>(arg2)->m_n;
                int64_t val = IntrinsicProcedures::lfortran_ieor64(x, y);
                ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc, t1_kind, nullptr, 0));
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, val, type));
            } else {
                throw SemanticError("ieor(x, y): x and y should have the same kind type", loc);
            }
        } else {
            throw SemanticError("Arguments for this intrinsic function must be Integer", loc);
        }
    }

    static int32_t lfortran_ibclr32(int32_t i, int pos) {
        return i & ~(1 << pos);
    }

    static int64_t lfortran_ibclr64(int64_t i, int pos) {
        return i & ~(1LL << pos);
    }

    static int32_t lfortran_ibset32(int32_t i, int pos) {
        return i | (1 << pos);
    }

    static int64_t lfortran_ibset64(int64_t i, int pos) {
        return i | (1LL << pos);
    }

    static int32_t lfortran_ieor32(int32_t x, int32_t y) {
        return x ^ y;
    }

    static int64_t lfortran_ieor64(int64_t x, int64_t y) {
        return x ^ y;
    }

}; // ComptimeEval

} // namespace LFortran

#endif /* LFORTRAN_SEMANTICS_AST_COMPTIME_EVAL_H */
