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
            "values". If not possible, an error is produced in symtab mode (and
            left unevaluated in body mode); otherwise the callback is called and
            it always succeeds to evaluate the result at compile time.

            If false, the arguments might not be compile time values. The
            callback can return nullptr. If nullptr and symtab mode, an error is
            produced. Otherwise the function is left unevaluated.
        */
        const static std::map<std::string, std::pair<comptime_eval_callback, bool>> comptime_eval_map = {
            {"kind", {&eval_kind, false}},
            {"floor", {&eval_floor, true}},
        };

        auto search = comptime_eval_map.find(name);
        if (search != comptime_eval_map.end()) {
            comptime_eval_callback cb = search->second.first;
            //bool eval_args = search->second.second;
            return cb(al, loc, args);
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

    static ASR::expr_t *eval_floor(Allocator &al, const Location &loc, Vec<ASR::expr_t*> &args) {
        // TODO: Implement optional kind; J3/18-007r1 --> FLOOR(A, [KIND])
        // TODO: Rip out switch to work with optional arguments
        ASR::expr_t* func_expr = args[0];
        ASR::ttype_t* func_type = LFortran::ASRUtils::expr_type(func_expr);
        int func_kind = ASRUtils::extract_kind_from_ttype_t(func_type);
        int64_t ival {0};
        if (LFortran::ASR::is_a<LFortran::ASR::Real_t>(*func_type)) {
            if (func_kind == 4){
                float rv = ASR::down_cast<ASR::ConstantReal_t>(
                    LFortran::ASRUtils::expr_value(func_expr))->m_r;
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
                    double rv = ASR::down_cast<ASR::ConstantReal_t>(LFortran::ASRUtils::expr_value(func_expr))->m_r;
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

}; // ComptimeEval

} // namespace LFortran

#endif /* LFORTRAN_SEMANTICS_AST_COMPTIME_EVAL_H */
