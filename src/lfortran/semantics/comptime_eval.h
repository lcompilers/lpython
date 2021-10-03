#ifndef LFORTRAN_SEMANTICS_AST_COMPTIME_EVAL_H
#define LFORTRAN_SEMANTICS_AST_COMPTIME_EVAL_H

#include <lfortran/asr.h>
#include <lfortran/ast.h>
#include <lfortran/bigint.h>
#include <lfortran/string_utils.h>
#include <lfortran/utils.h>

namespace LFortran {

struct IntrinsicProcedures {

    const std::string m_kind = "lfortran_intrinsic_kind";
    const std::string m_array = "lfortran_intrinsic_array";
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
            {"tiny", {m_array, &eval_tiny, false}},
            {"int", {m_array, &eval_int, false}},
            {"real", {m_array, &not_implemented, false}}, // Implemented separately

            // Require evaluated arguments
            {"char", {m_array, &eval_char, true}},
            {"floor", {m_array, &eval_floor, true}},
            {"selected_int_kind", {m_kind, &eval_selected_int_kind, true}},
            {"selected_real_kind", {m_kind, &eval_selected_real_kind, true}},
            {"exp", {m_math, &not_implemented, false}},
            {"log", {m_math, &not_implemented, false}},
            {"erf", {m_math, &not_implemented, false}},

            {"sin", {m_trig, &eval_sin, true}},
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

            {"atan2", {m_math, &not_implemented, false}},
            {"sqrt", {m_math2, &not_implemented, false}},
            {"iand", {m_bit, &not_implemented, false}},
            {"ior", {m_bit, &not_implemented, false}},

            // These will fail if used in symbol table visitor, but will be
            // left unevaluated in body visitor
            {"trim", {m_string, &not_implemented, false}},
            {"len_trim", {m_string, &not_implemented, false}},
            {"len", {m_array, &not_implemented, false}},
            {"size", {m_array, &not_implemented, false}},
            {"present", {m_array, &not_implemented, false}},
            {"min", {m_array, &not_implemented, false}},
            {"max", {m_array, &not_implemented, false}},
            {"lbound", {m_array, &not_implemented, false}},
            {"ubound", {m_array, &not_implemented, false}},
            {"allocated", {"m_array", &not_implemented, false}},
            {"minval", {m_array, &not_implemented, false}},
            {"maxval", {m_array, &not_implemented, false}},
            {"sum", {m_array, &not_implemented, false}},
            {"abs", {m_math2, &not_implemented, false}},
            {"aimag", {m_math2, &not_implemented, false}},
            {"modulo", {m_math2, &not_implemented, false}},
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
        // TODO: Rip out switch to work with optional arguments
        ASR::expr_t* func_expr = args[0];
        ASR::ttype_t* func_type = LFortran::ASRUtils::expr_type(func_expr);
        int func_kind = ASRUtils::extract_kind_from_ttype_t(func_type);
        int64_t ival {0};
        if (LFortran::ASR::is_a<LFortran::ASR::Real_t>(*func_type)) {
            if (func_kind == 4){
                float rv = ASR::down_cast<ASR::ConstantReal_t>(func_expr)->m_r;
                if (rv<0) {
                    // negative number
                    // floor -> integer(|x|+1)
                    ival = static_cast<int64_t>(rv-1);
                } else {
                        // positive, floor -> integer(x)
                        ival = static_cast<int64_t>(rv);
                    }
                    return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, ival,func_type));
                } else {
                    double rv = ASR::down_cast<ASR::ConstantReal_t>(func_expr)->m_r;
                    int64_t ival = static_cast<int64_t>(rv);
                    if (rv<0) {
                        // negative number
                        // floor -> integer(x+1)
                        ival = static_cast<int64_t>(rv+1);
                    } else {
                        // positive, floor -> integer(x)
                        ival = static_cast<int64_t>(rv);
                    }
                    return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, ival,func_type));
            }
        } else {
            throw SemanticError("floor must have one real argument", loc);
        }
    }


    typedef float (*trig_eval_callback_single)(float);
    typedef double (*trig_eval_callback_double)(double);
    static ASR::expr_t *eval_trig(Allocator &al, const Location &loc,
            Vec<ASR::expr_t*> &args,
            trig_eval_callback_single /*trig_single*/,
            trig_eval_callback_double trig_double
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
            return nullptr;
        } else {
            throw SemanticError("Argument for trig function must be Real or Complex", loc);
        }
    }

    static ASR::expr_t *eval_sin(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &sinf, &sin);
    }
    static ASR::expr_t *eval_cos(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &cosf, &cos);
    }
    static ASR::expr_t *eval_tan(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &tanf, &tan);
    }

    static ASR::expr_t *eval_asin(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &asinf, &asin);
    }
    static ASR::expr_t *eval_acos(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &acosf, &acos);
    }
    static ASR::expr_t *eval_atan(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &atanf, &atan);
    }

    static ASR::expr_t *eval_sinh(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &sinhf, &sinh);
    }
    static ASR::expr_t *eval_cosh(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &coshf, &cosh);
    }
    static ASR::expr_t *eval_tanh(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &tanhf, &tanh);
    }

    static ASR::expr_t *eval_asinh(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &asinhf, &asinh);
    }
    static ASR::expr_t *eval_acosh(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &acoshf, &acosh);
    }
    static ASR::expr_t *eval_atanh(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        return eval_trig(al, loc, args, &atanhf, &atanh);
    }

    static ASR::expr_t *eval_int(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        ASR::expr_t* int_expr = args[0];
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
            return ASR::down_cast<ASR::expr_t>(
                ASR::make_ConstantInteger_t(al, loc,
                a_kind, real_type));
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
            return ASR::down_cast<ASR::expr_t>(
                ASR::make_ConstantInteger_t(al, loc,
                a_kind, real_type));
        } else {
            throw SemanticError("integer_real_kind() must have one integer argument", loc);
        }
    }

}; // ComptimeEval

} // namespace LFortran

#endif /* LFORTRAN_SEMANTICS_AST_COMPTIME_EVAL_H */
