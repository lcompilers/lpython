#ifndef LFORTRAN_SEMANTICS_AST_COMPTIME_EVAL_H
#define LFORTRAN_SEMANTICS_AST_COMPTIME_EVAL_H

#include <lfortran/asr.h>
#include <lfortran/ast.h>
#include <lfortran/bigint.h>
#include <lfortran/string_utils.h>
#include <lfortran/utils.h>

namespace LFortran {

struct ComptimeEval {

    static ASR::expr_t *eval(std::string name, Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        typedef ASR::expr_t* (*const comptime_eval_callback)(Allocator &, const Location &, Vec<ASR::expr_t*> &);
        /*
            The last parameter is true if the callback accepts evaluated arguments.

            If true, the arguments are first converted to their compile time
            "values". If not possible, nullptr is returned; otherwise the
            callback is called and it always succeeds to evaluate the result at
            compile time.

            If false, the arguments might not be compile time values. The
            callback can return nullptr if it cannot evaluate itself.
        */
        const static std::map<std::string, std::pair<comptime_eval_callback, bool>> comptime_eval_map = {
            // Arguments can be evaluated or not
            {"kind", {&eval_kind, false}},
            {"tiny", {&eval_tiny, false}},
            {"int", {&eval_int, false}},

            // Require evalated arguments
            {"char", {&eval_char, true}},
            {"floor", {&eval_floor, true}},
            {"sin", {&eval_sin, true}},
            {"selected_int_kind", {&eval_selected_int_kind, true}},
            {"selected_real_kind", {&eval_selected_real_kind, true}},

            // These will fail if used in symbol table visitor, but will be
            // left unevaluated in body visitor
            {"trim", {&not_implemented, false}},
            {"len_trim", {&not_implemented, false}},
            {"len", {&not_implemented, false}},
            {"size", {&not_implemented, false}},
            {"present", {&not_implemented, false}},
            {"min", {&not_implemented, false}},
            {"max", {&not_implemented, false}},
            {"lbound", {&not_implemented, false}},
            {"ubound", {&not_implemented, false}},
        };

        auto search = comptime_eval_map.find(name);
        if (search != comptime_eval_map.end()) {
            comptime_eval_callback cb = search->second.first;
            bool eval_args = search->second.second;
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

    static ASR::expr_t *eval_sin(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
        // TODO: this is already double precision --- possibly
        // pass the original GenericProcedure
        if (args.size() != 1) {
            throw SemanticError("Intrinsic function sin accepts exactly 1 argument", loc);
        }
        ASR::expr_t* sin_arg = args[0];
        ASR::ttype_t* t = LFortran::ASRUtils::expr_type(args[0]);
        int k = ASRUtils::extract_kind_from_ttype_t(t);
        if (LFortran::ASR::is_a<LFortran::ASR::Real_t>(*t)) {
            if (k == 4) {
                float rv = ASR::down_cast<ASR::ConstantReal_t>(sin_arg)->m_r;
                float val = sin(rv);
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, loc, val, t));
            } else {
                double rv = ASR::down_cast<ASR::ConstantReal_t>(sin_arg)->m_r;
                double val = sin(rv);
                return ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, loc, val, t));
            }
        } else {
            throw SemanticError("Argument for sin must be Real", loc);
        }
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
