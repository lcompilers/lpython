#ifndef LIBASR_PASS_INTRINSIC_FUNC_REG_UTIL_H
#define LIBASR_PASS_INTRINSIC_FUNC_REG_UTIL_H

#include <libasr/pass/intrinsic_functions.h>

namespace LCompilers {

namespace ASRUtils {

namespace Kind {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Kind expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_integer(*arg_type0)) || (is_real(*arg_type0)) || (is_logical(*arg_type0)) || (is_character(*arg_type0)) || (is_complex(*arg_type0)), "Unexpected args, Kind expects (int) or (real) or (bool) or (char) or (complex) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Kind takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
        ASRUtils::require_impl(is_integer(*x.m_type), "Unexpected return type, Kind expects `int` as return type", x.base.base.loc, diagnostics);
    }

    static inline ASR::asr_t* create_Kind(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_integer(*arg_type0)) || (is_real(*arg_type0)) || (is_logical(*arg_type0)) || (is_character(*arg_type0)) || (is_complex(*arg_type0)))) {
                append_error(diag, "Unexpected args, Kind expects (int) or (real) or (bool) or (char) or (complex) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Kind takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Kind(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Kind), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace FMA {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 3)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for FMA expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[2]));
            ASRUtils::require_impl((is_real(*arg_type0) && is_real(*arg_type1) && is_real(*arg_type2)), "Unexpected args, FMA expects (real, real, real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, FMA takes 3 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_FMA(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 3)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[2]));
            if(!((is_real(*arg_type0) && is_real(*arg_type1) && is_real(*arg_type2)))) {
                append_error(diag, "Unexpected args, FMA expects (real, real, real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, FMA takes 3 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 3);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        m_args.push_back(al, args[2]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 3);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            args_values.push_back(al, expr_value(m_args[2]));
            m_value = eval_FMA(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::FMA), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace FlipSign {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for FlipSign expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_real(*arg_type1)), "Unexpected args, FlipSign expects (int, real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, FlipSign takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_FlipSign(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_real(*arg_type1)))) {
                append_error(diag, "Unexpected args, FlipSign expects (int, real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, FlipSign takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[1]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_FlipSign(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::FlipSign), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace FloorDiv {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for FloorDiv expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)) || (is_unsigned_integer(*arg_type0) && is_unsigned_integer(*arg_type1)) || (is_real(*arg_type0) && is_real(*arg_type1)) || (is_logical(*arg_type0) && is_logical(*arg_type1)), "Unexpected args, FloorDiv expects (int, int) or (uint, uint) or (real, real) or (bool, bool) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, FloorDiv takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_FloorDiv(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)) || (is_unsigned_integer(*arg_type0) && is_unsigned_integer(*arg_type1)) || (is_real(*arg_type0) && is_real(*arg_type1)) || (is_logical(*arg_type0) && is_logical(*arg_type1)))) {
                append_error(diag, "Unexpected args, FloorDiv expects (int, int) or (uint, uint) or (real, real) or (bool, bool) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, FloorDiv takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_FloorDiv(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::FloorDiv), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Mod {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Mod expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)) || (is_real(*arg_type0) && is_real(*arg_type1)), "Unexpected args, Mod expects (int, int) or (real, real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Mod takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Mod(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)) || (is_real(*arg_type0) && is_real(*arg_type1)))) {
                append_error(diag, "Unexpected args, Mod expects (int, int) or (real, real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Mod takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Mod(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Mod), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Trailz {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Trailz expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_integer(*arg_type0)), "Unexpected args, Trailz expects (int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Trailz takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Trailz(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_integer(*arg_type0)))) {
                append_error(diag, "Unexpected args, Trailz expects (int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Trailz takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Trailz(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Trailz), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace BesselJ0 {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for BesselJ0 expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, BesselJ0 expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, BesselJ0 takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_BesselJ0(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, BesselJ0 expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, BesselJ0 takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_BesselJ0(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::BesselJ0), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace BesselJ1 {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for BesselJ1 expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, BesselJ1 expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, BesselJ1 takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_BesselJ1(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, BesselJ1 expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, BesselJ1 takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_BesselJ1(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::BesselJ1), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace BesselY0 {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for BesselY0 expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, BesselY0 expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, BesselY0 takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_BesselY0(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, BesselY0 expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, BesselY0 takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_BesselY0(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::BesselY0), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Mvbits {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 5)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Mvbits expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[2]));
            ASR::ttype_t *arg_type3 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[3]));
            ASR::ttype_t *arg_type4 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[4]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1) && is_integer(*arg_type2) && is_integer(*arg_type3) && is_integer(*arg_type4)), "Unexpected args, Mvbits expects (int, int, int, int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Mvbits takes 5 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Mvbits(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 5)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[2]));
            ASR::ttype_t *arg_type3 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[3]));
            ASR::ttype_t *arg_type4 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[4]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1) && is_integer(*arg_type2) && is_integer(*arg_type3) && is_integer(*arg_type4)))) {
                append_error(diag, "Unexpected args, Mvbits expects (int, int, int, int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Mvbits takes 5 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[3]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 5);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        m_args.push_back(al, args[2]);
        m_args.push_back(al, args[3]);
        m_args.push_back(al, args[4]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 5);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            args_values.push_back(al, expr_value(m_args[2]));
            args_values.push_back(al, expr_value(m_args[3]));
            args_values.push_back(al, expr_value(m_args[4]));
            m_value = eval_Mvbits(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Mvbits), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Leadz {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Leadz expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_integer(*arg_type0)), "Unexpected args, Leadz expects (int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Leadz takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Leadz(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_integer(*arg_type0)))) {
                append_error(diag, "Unexpected args, Leadz expects (int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Leadz takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Leadz(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Leadz), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace ToLowerCase {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for ToLowerCase expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_character(*arg_type0)), "Unexpected args, ToLowerCase expects (char) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, ToLowerCase takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_ToLowerCase(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_character(*arg_type0)))) {
                append_error(diag, "Unexpected args, ToLowerCase expects (char) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, ToLowerCase takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_ToLowerCase(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::ToLowerCase), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Hypot {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Hypot expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_real(*arg_type0) && is_real(*arg_type1)), "Unexpected args, Hypot expects (real, real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Hypot takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Hypot(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_real(*arg_type0) && is_real(*arg_type1)))) {
                append_error(diag, "Unexpected args, Hypot expects (real, real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Hypot takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Hypot(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Hypot), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace SelectedIntKind {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for SelectedIntKind expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_integer(*arg_type0)), "Unexpected args, SelectedIntKind expects (int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, SelectedIntKind takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_SelectedIntKind(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_integer(*arg_type0)))) {
                append_error(diag, "Unexpected args, SelectedIntKind expects (int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, SelectedIntKind takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_SelectedIntKind(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::SelectedIntKind), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace SelectedRealKind {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 3)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for SelectedRealKind expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[2]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1) && is_integer(*arg_type2)), "Unexpected args, SelectedRealKind expects (int, int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, SelectedRealKind takes 3 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_SelectedRealKind(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 3)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[2]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1) && is_integer(*arg_type2)))) {
                append_error(diag, "Unexpected args, SelectedRealKind expects (int, int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, SelectedRealKind takes 3 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 3);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        m_args.push_back(al, args[2]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 3);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            args_values.push_back(al, expr_value(m_args[2]));
            m_value = eval_SelectedRealKind(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::SelectedRealKind), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace SelectedCharKind {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for SelectedCharKind expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_character(*arg_type0)), "Unexpected args, SelectedCharKind expects (char) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, SelectedCharKind takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_SelectedCharKind(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_character(*arg_type0)))) {
                append_error(diag, "Unexpected args, SelectedCharKind expects (char) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, SelectedCharKind takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_SelectedCharKind(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::SelectedCharKind), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Digits {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Digits expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_integer(*arg_type0)) || (is_real(*arg_type0)), "Unexpected args, Digits expects (int) or (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Digits takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Digits(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_integer(*arg_type0)) || (is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Digits expects (int) or (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Digits takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Digits(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Digits), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Repeat {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Repeat expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_character(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Repeat expects (char, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Repeat takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Repeat(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_character(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Repeat expects (char, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Repeat takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Repeat(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Repeat), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace StringContainsSet {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 4)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for StringContainsSet expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[2]));
            ASR::ttype_t *arg_type3 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[3]));
            ASRUtils::require_impl((is_character(*arg_type0) && is_character(*arg_type1) && is_logical(*arg_type2) && is_integer(*arg_type3)), "Unexpected args, StringContainsSet expects (char, char, bool, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, StringContainsSet takes 4 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_StringContainsSet(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 4)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[2]));
            ASR::ttype_t *arg_type3 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[3]));
            if(!((is_character(*arg_type0) && is_character(*arg_type1) && is_logical(*arg_type2) && is_integer(*arg_type3)))) {
                append_error(diag, "Unexpected args, StringContainsSet expects (char, char, bool, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, StringContainsSet takes 4 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[3]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 4);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        m_args.push_back(al, args[2]);
        m_args.push_back(al, args[3]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 4);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            args_values.push_back(al, expr_value(m_args[2]));
            args_values.push_back(al, expr_value(m_args[3]));
            m_value = eval_StringContainsSet(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::StringContainsSet), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace StringFindSet {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 4)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for StringFindSet expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[2]));
            ASR::ttype_t *arg_type3 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[3]));
            ASRUtils::require_impl((is_character(*arg_type0) && is_character(*arg_type1) && is_logical(*arg_type2) && is_integer(*arg_type3)), "Unexpected args, StringFindSet expects (char, char, bool, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, StringFindSet takes 4 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_StringFindSet(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 4)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[2]));
            ASR::ttype_t *arg_type3 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[3]));
            if(!((is_character(*arg_type0) && is_character(*arg_type1) && is_logical(*arg_type2) && is_integer(*arg_type3)))) {
                append_error(diag, "Unexpected args, StringFindSet expects (char, char, bool, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, StringFindSet takes 4 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[3]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 4);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        m_args.push_back(al, args[2]);
        m_args.push_back(al, args[3]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 4);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            args_values.push_back(al, expr_value(m_args[2]));
            args_values.push_back(al, expr_value(m_args[3]));
            m_value = eval_StringFindSet(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::StringFindSet), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace SubstrIndex {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 4)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for SubstrIndex expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[2]));
            ASR::ttype_t *arg_type3 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[3]));
            ASRUtils::require_impl((is_character(*arg_type0) && is_character(*arg_type1) && is_logical(*arg_type2) && is_integer(*arg_type3)), "Unexpected args, SubstrIndex expects (char, char, bool, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, SubstrIndex takes 4 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_SubstrIndex(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 4)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[2]));
            ASR::ttype_t *arg_type3 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[3]));
            if(!((is_character(*arg_type0) && is_character(*arg_type1) && is_logical(*arg_type2) && is_integer(*arg_type3)))) {
                append_error(diag, "Unexpected args, SubstrIndex expects (char, char, bool, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, SubstrIndex takes 4 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[3]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 4);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        m_args.push_back(al, args[2]);
        m_args.push_back(al, args[3]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 4);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            args_values.push_back(al, expr_value(m_args[2]));
            args_values.push_back(al, expr_value(m_args[3]));
            m_value = eval_SubstrIndex(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::SubstrIndex), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace MinExponent {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for MinExponent expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, MinExponent expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, MinExponent takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_MinExponent(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, MinExponent expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, MinExponent takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_MinExponent(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::MinExponent), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace MaxExponent {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for MaxExponent expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, MaxExponent expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, MaxExponent takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_MaxExponent(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, MaxExponent expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, MaxExponent takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_MaxExponent(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::MaxExponent), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Partition {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Partition expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_character(*arg_type0) && is_character(*arg_type1)), "Unexpected args, Partition expects (char, char) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Partition takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
        ASRUtils::require_impl(ASR::is_a<ASR::Tuple_t>(*x.m_type), "Unexpected return type, Partition expects `tuple` as return type", x.base.base.loc, diagnostics);
    }

}

namespace ListReverse {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for ListReverse expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((ASR::is_a<ASR::List_t>(*arg_type0)), "Unexpected args, ListReverse expects (list) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, ListReverse takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
        ASRUtils::require_impl(x.m_type == nullptr, "Unexpected return type, ListReverse expects `null` as return type", x.base.base.loc, diagnostics);
    }

    static inline ASR::asr_t* create_ListReverse(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((ASR::is_a<ASR::List_t>(*arg_type0)))) {
                append_error(diag, "Unexpected args, ListReverse expects (list) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, ListReverse takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = nullptr;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_ListReverse(al, loc, return_type, args_values, diag);
        }
        return ASR::make_Expr_t(al, loc, ASRUtils::EXPR(ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::ListReverse), m_args.p, m_args.n, 0, return_type, m_value)));
    }
}

namespace ListReserve {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for ListReserve expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((ASR::is_a<ASR::List_t>(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, ListReserve expects (list, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, ListReserve takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
        ASRUtils::require_impl(x.m_type == nullptr, "Unexpected return type, ListReserve expects `null` as return type", x.base.base.loc, diagnostics);
    }

    static inline ASR::asr_t* create_ListReserve(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((ASR::is_a<ASR::List_t>(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, ListReserve expects (list, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, ListReserve takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = nullptr;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_ListReserve(al, loc, return_type, args_values, diag);
        }
        return ASR::make_Expr_t(al, loc, ASRUtils::EXPR(ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::ListReserve), m_args.p, m_args.n, 0, return_type, m_value)));
    }
}

namespace Sign {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Sign expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)) || (is_real(*arg_type0) && is_real(*arg_type1)), "Unexpected args, Sign expects (int, int) or (real, real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Sign takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Sign(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)) || (is_real(*arg_type0) && is_real(*arg_type1)))) {
                append_error(diag, "Unexpected args, Sign expects (int, int) or (real, real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Sign takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Sign(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Sign), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Radix {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Radix expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_integer(*arg_type0)) || (is_real(*arg_type0)), "Unexpected args, Radix expects (int) or (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Radix takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
        ASRUtils::require_impl(x.m_value, "Missing compile time value, `Radix` intrinsic output must be computed during compile time", x.base.base.loc, diagnostics);
        ASRUtils::require_impl(is_integer(*x.m_type), "Unexpected return type, Radix expects `int` as return type", x.base.base.loc, diagnostics);
    }

    static inline ASR::asr_t* create_Radix(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_integer(*arg_type0)) || (is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Radix expects (int) or (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Radix takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        return_type = ASRUtils::extract_type(return_type);
        m_value = eval_Radix(al, loc, return_type, args, diag);
        return ASR::make_TypeInquiry_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Radix), ASRUtils::expr_type(m_args[0]), m_args[0], return_type, m_value);
    }
}

namespace Adjustl {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Adjustl expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_character(*arg_type0)), "Unexpected args, Adjustl expects (char) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Adjustl takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Adjustl(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_character(*arg_type0)))) {
                append_error(diag, "Unexpected args, Adjustl expects (char) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Adjustl takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = character(-1);
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Adjustl(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Adjustl), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Adjustr {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Adjustr expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_character(*arg_type0)), "Unexpected args, Adjustr expects (char) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Adjustr takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Adjustr(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_character(*arg_type0)))) {
                append_error(diag, "Unexpected args, Adjustr expects (char) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Adjustr takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = character(-1);
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Adjustr(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Adjustr), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Aint {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Aint expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, Aint expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Aint takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Aint(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Aint expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Aint takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        if ( args[1] != nullptr ) {
            int kind = -1;
            if (!ASR::is_a<ASR::Integer_t>(*expr_type(args[1])) || !extract_value(args[1], kind)) {
                append_error(diag, "`kind` argument of the `Aint` function must be a scalar Integer constant", args[1]->base.loc);
                return nullptr;
            }
            set_kind_to_ttype_t(return_type, kind);
        }
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Aint(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Aint), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Nint {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Nint expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, Nint expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Nint takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Nint(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Nint expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Nint takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        if ( args[1] != nullptr ) {
            int kind = -1;
            if (!ASR::is_a<ASR::Integer_t>(*expr_type(args[1])) || !extract_value(args[1], kind)) {
                append_error(diag, "`kind` argument of the `Nint` function must be a scalar Integer constant", args[1]->base.loc);
                return nullptr;
            }
            set_kind_to_ttype_t(return_type, kind);
        }
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Nint(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Nint), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Anint {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Anint expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, Anint expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Anint takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Anint(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Anint expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Anint takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        if ( args[1] != nullptr ) {
            int kind = -1;
            if (!ASR::is_a<ASR::Integer_t>(*expr_type(args[1])) || !extract_value(args[1], kind)) {
                append_error(diag, "`kind` argument of the `Anint` function must be a scalar Integer constant", args[1]->base.loc);
                return nullptr;
            }
            set_kind_to_ttype_t(return_type, kind);
        }
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Anint(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Anint), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Floor {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Floor expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, Floor expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Floor takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Floor(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Floor expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Floor takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        if ( args[1] != nullptr ) {
            int kind = -1;
            if (!ASR::is_a<ASR::Integer_t>(*expr_type(args[1])) || !extract_value(args[1], kind)) {
                append_error(diag, "`kind` argument of the `Floor` function must be a scalar Integer constant", args[1]->base.loc);
                return nullptr;
            }
            set_kind_to_ttype_t(return_type, kind);
        }
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Floor(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Floor), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Ceiling {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Ceiling expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, Ceiling expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Ceiling takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Ceiling(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Ceiling expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Ceiling takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        if ( args[1] != nullptr ) {
            int kind = -1;
            if (!ASR::is_a<ASR::Integer_t>(*expr_type(args[1])) || !extract_value(args[1], kind)) {
                append_error(diag, "`kind` argument of the `Ceiling` function must be a scalar Integer constant", args[1]->base.loc);
                return nullptr;
            }
            set_kind_to_ttype_t(return_type, kind);
        }
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Ceiling(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Ceiling), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Sqrt {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Sqrt expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)) || (is_complex(*arg_type0)), "Unexpected args, Sqrt expects (real) or (complex) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Sqrt takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Sqrt(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)) || (is_complex(*arg_type0)))) {
                append_error(diag, "Unexpected args, Sqrt expects (real) or (complex) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Sqrt takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Sqrt(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Sqrt), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Sngl {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Sngl expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, Sngl expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Sngl takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Sngl(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Sngl expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Sngl takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = real32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Sngl(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Sngl), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace SignFromValue {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for SignFromValue expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)) || (is_real(*arg_type0) && is_real(*arg_type1)), "Unexpected args, SignFromValue expects (int, int) or (real, real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, SignFromValue takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_SignFromValue(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)) || (is_real(*arg_type0) && is_real(*arg_type1)))) {
                append_error(diag, "Unexpected args, SignFromValue expects (int, int) or (real, real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, SignFromValue takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_SignFromValue(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::SignFromValue), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Ifix {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Ifix expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, Ifix expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Ifix takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Ifix(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Ifix expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Ifix takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Ifix(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Ifix), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Idint {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Idint expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, Idint expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Idint takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Idint(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Idint expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Idint takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Idint(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Idint), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Ishft {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Ishft expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Ishft expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Ishft takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Ishft(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Ishft expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Ishft takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Ishft(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Ishft), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Bgt {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Bgt expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Bgt expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Bgt takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Bgt(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Bgt expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Bgt takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = logical;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Bgt(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Bgt), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Blt {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Blt expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Blt expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Blt takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Blt(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Blt expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Blt takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = logical;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Blt(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Blt), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Bge {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Bge expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Bge expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Bge takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Bge(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Bge expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Bge takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = logical;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Bge(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Bge), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Ble {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Ble expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Ble expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Ble takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Ble(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Ble expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Ble takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = logical;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Ble(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Ble), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Lgt {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Lgt expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_character(*arg_type0) && is_character(*arg_type1)), "Unexpected args, Lgt expects (char, char) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Lgt takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Lgt(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_character(*arg_type0) && is_character(*arg_type1)))) {
                append_error(diag, "Unexpected args, Lgt expects (char, char) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Lgt takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = logical;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Lgt(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Lgt), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Llt {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Llt expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_character(*arg_type0) && is_character(*arg_type1)), "Unexpected args, Llt expects (char, char) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Llt takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Llt(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_character(*arg_type0) && is_character(*arg_type1)))) {
                append_error(diag, "Unexpected args, Llt expects (char, char) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Llt takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = logical;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Llt(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Llt), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Lge {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Lge expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_character(*arg_type0) && is_character(*arg_type1)), "Unexpected args, Lge expects (char, char) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Lge takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Lge(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_character(*arg_type0) && is_character(*arg_type1)))) {
                append_error(diag, "Unexpected args, Lge expects (char, char) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Lge takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = logical;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Lge(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Lge), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Lle {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Lle expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_character(*arg_type0) && is_character(*arg_type1)), "Unexpected args, Lle expects (char, char) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Lle takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Lle(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_character(*arg_type0) && is_character(*arg_type1)))) {
                append_error(diag, "Unexpected args, Lle expects (char, char) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Lle takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = logical;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Lle(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Lle), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Not {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Not expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_integer(*arg_type0)), "Unexpected args, Not expects (int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Not takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Not(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_integer(*arg_type0)))) {
                append_error(diag, "Unexpected args, Not expects (int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Not takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Not(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Not), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Iand {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Iand expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Iand expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Iand takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Iand(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Iand expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Iand takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Iand(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Iand), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Ior {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Ior expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Ior expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Ior takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Ior(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Ior expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Ior takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Ior(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Ior), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Ieor {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Ieor expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Ieor expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Ieor takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Ieor(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Ieor expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Ieor takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Ieor(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Ieor), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Ibclr {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Ibclr expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Ibclr expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Ibclr takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Ibclr(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Ibclr expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Ibclr takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Ibclr(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Ibclr), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Ibset {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Ibset expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Ibset expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Ibset takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Ibset(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Ibset expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Ibset takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Ibset(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Ibset), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Btest {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Btest expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Btest expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Btest takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Btest(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Btest expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Btest takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = logical;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Btest(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Btest), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Ibits {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 3)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Ibits expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[2]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1) && is_integer(*arg_type2)), "Unexpected args, Ibits expects (int, int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Ibits takes 3 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Ibits(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 3)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[2]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1) && is_integer(*arg_type2)))) {
                append_error(diag, "Unexpected args, Ibits expects (int, int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Ibits takes 3 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 3);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        m_args.push_back(al, args[2]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 3);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            args_values.push_back(al, expr_value(m_args[2]));
            m_value = eval_Ibits(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Ibits), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Shiftr {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Shiftr expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Shiftr expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Shiftr takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Shiftr(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Shiftr expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Shiftr takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Shiftr(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Shiftr), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Rshift {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Rshift expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Rshift expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Rshift takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Rshift(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Rshift expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Rshift takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Rshift(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Rshift), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Shiftl {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Shiftl expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Shiftl expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Shiftl takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Shiftl(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Shiftl expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Shiftl takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Shiftl(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Shiftl), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Aimag {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Aimag expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_complex(*arg_type0)), "Unexpected args, Aimag expects (complex) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Aimag takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Aimag(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_complex(*arg_type0)))) {
                append_error(diag, "Unexpected args, Aimag expects (complex) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Aimag takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = real32;
        if ( args[1] != nullptr ) {
            int kind = -1;
            if (!ASR::is_a<ASR::Integer_t>(*expr_type(args[1])) || !extract_value(args[1], kind)) {
                append_error(diag, "`kind` argument of the `Aimag` function must be a scalar Integer constant", args[1]->base.loc);
                return nullptr;
            }
            set_kind_to_ttype_t(return_type, kind);
        }
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Aimag(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Aimag), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Rank {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Rank expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((!ASR::is_a<ASR::TypeParameter_t>(*arg_type0)), "Unexpected args, Rank expects (any) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Rank takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
        ASRUtils::require_impl(x.m_value, "Missing compile time value, `Rank` intrinsic output must be computed during compile time", x.base.base.loc, diagnostics);
    }

    static inline ASR::asr_t* create_Rank(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((!ASR::is_a<ASR::TypeParameter_t>(*arg_type0)))) {
                append_error(diag, "Unexpected args, Rank expects (any) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Rank takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        return_type = ASRUtils::extract_type(return_type);
        m_value = eval_Rank(al, loc, return_type, args, diag);
        return ASR::make_TypeInquiry_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Rank), ASRUtils::expr_type(m_args[0]), m_args[0], return_type, m_value);
    }
}

namespace Range {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Range expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_integer(*arg_type0)) || (is_real(*arg_type0)) || (is_complex(*arg_type0)), "Unexpected args, Range expects (int) or (real) or (complex) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Range takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
        ASRUtils::require_impl(x.m_value, "Missing compile time value, `Range` intrinsic output must be computed during compile time", x.base.base.loc, diagnostics);
    }

    static inline ASR::asr_t* create_Range(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_integer(*arg_type0)) || (is_real(*arg_type0)) || (is_complex(*arg_type0)))) {
                append_error(diag, "Unexpected args, Range expects (int) or (real) or (complex) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Range takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        return_type = ASRUtils::extract_type(return_type);
        m_value = eval_Range(al, loc, return_type, args, diag);
        return ASR::make_TypeInquiry_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Range), ASRUtils::expr_type(m_args[0]), m_args[0], return_type, m_value);
    }
}

namespace Epsilon {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Epsilon expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, Epsilon expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Epsilon takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
        ASRUtils::require_impl(x.m_value, "Missing compile time value, `Epsilon` intrinsic output must be computed during compile time", x.base.base.loc, diagnostics);
    }

    static inline ASR::asr_t* create_Epsilon(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Epsilon expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Epsilon takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        return_type = ASRUtils::extract_type(return_type);
        m_value = eval_Epsilon(al, loc, return_type, args, diag);
        return ASR::make_TypeInquiry_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Epsilon), ASRUtils::expr_type(m_args[0]), m_args[0], return_type, m_value);
    }
}

namespace Precision {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Precision expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)) || (is_complex(*arg_type0)), "Unexpected args, Precision expects (real) or (complex) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Precision takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
        ASRUtils::require_impl(x.m_value, "Missing compile time value, `Precision` intrinsic output must be computed during compile time", x.base.base.loc, diagnostics);
    }

    static inline ASR::asr_t* create_Precision(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)) || (is_complex(*arg_type0)))) {
                append_error(diag, "Unexpected args, Precision expects (real) or (complex) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Precision takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        return_type = ASRUtils::extract_type(return_type);
        m_value = eval_Precision(al, loc, return_type, args, diag);
        return ASR::make_TypeInquiry_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Precision), ASRUtils::expr_type(m_args[0]), m_args[0], return_type, m_value);
    }
}

namespace Tiny {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Tiny expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, Tiny expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Tiny takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
        ASRUtils::require_impl(x.m_value, "Missing compile time value, `Tiny` intrinsic output must be computed during compile time", x.base.base.loc, diagnostics);
    }

    static inline ASR::asr_t* create_Tiny(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Tiny expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Tiny takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        return_type = ASRUtils::extract_type(return_type);
        m_value = eval_Tiny(al, loc, return_type, args, diag);
        return ASR::make_TypeInquiry_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Tiny), ASRUtils::expr_type(m_args[0]), m_args[0], return_type, m_value);
    }
}

namespace Conjg {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Conjg expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_complex(*arg_type0)), "Unexpected args, Conjg expects (complex) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Conjg takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Conjg(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_complex(*arg_type0)))) {
                append_error(diag, "Unexpected args, Conjg expects (complex) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Conjg takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Conjg(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Conjg), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Scale {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Scale expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_real(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Scale expects (real, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Scale takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Scale(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_real(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Scale expects (real, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Scale takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Scale(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Scale), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Huge {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Huge expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_integer(*arg_type0)) || (is_real(*arg_type0)), "Unexpected args, Huge expects (int) or (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Huge takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
        ASRUtils::require_impl(x.m_value, "Missing compile time value, `Huge` intrinsic output must be computed during compile time", x.base.base.loc, diagnostics);
    }

    static inline ASR::asr_t* create_Huge(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_integer(*arg_type0)) || (is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Huge expects (int) or (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Huge takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        return_type = ASRUtils::extract_type(return_type);
        m_value = eval_Huge(al, loc, return_type, args, diag);
        return ASR::make_TypeInquiry_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Huge), ASRUtils::expr_type(m_args[0]), m_args[0], return_type, m_value);
    }
}

namespace Dprod {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Dprod expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_real(*arg_type0) && is_real(*arg_type1)), "Unexpected args, Dprod expects (real, real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Dprod takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Dprod(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_real(*arg_type0) && is_real(*arg_type1)))) {
                append_error(diag, "Unexpected args, Dprod expects (real, real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Dprod takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = real64;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Dprod(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Dprod), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Dim {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Dim expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)) || (is_real(*arg_type0) && is_real(*arg_type1)), "Unexpected args, Dim expects (int, int) or (real, real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Dim takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Dim(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)) || (is_real(*arg_type0) && is_real(*arg_type1)))) {
                append_error(diag, "Unexpected args, Dim expects (int, int) or (real, real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Dim takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Dim(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Dim), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Maskl {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Maskl expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_integer(*arg_type0)), "Unexpected args, Maskl expects (int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Maskl takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Maskl(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_integer(*arg_type0)))) {
                append_error(diag, "Unexpected args, Maskl expects (int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Maskl takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        if ( args[1] != nullptr ) {
            int kind = -1;
            if (!ASR::is_a<ASR::Integer_t>(*expr_type(args[1])) || !extract_value(args[1], kind)) {
                append_error(diag, "`kind` argument of the `Maskl` function must be a scalar Integer constant", args[1]->base.loc);
                return nullptr;
            }
            set_kind_to_ttype_t(return_type, kind);
        }
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Maskl(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Maskl), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Maskr {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Maskr expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_integer(*arg_type0)), "Unexpected args, Maskr expects (int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Maskr takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Maskr(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_integer(*arg_type0)))) {
                append_error(diag, "Unexpected args, Maskr expects (int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Maskr takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        if ( args[1] != nullptr ) {
            int kind = -1;
            if (!ASR::is_a<ASR::Integer_t>(*expr_type(args[1])) || !extract_value(args[1], kind)) {
                append_error(diag, "`kind` argument of the `Maskr` function must be a scalar Integer constant", args[1]->base.loc);
                return nullptr;
            }
            set_kind_to_ttype_t(return_type, kind);
        }
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Maskr(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Maskr), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Ishftc {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Ishftc expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, Ishftc expects (int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Ishftc takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Ishftc(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, Ishftc expects (int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Ishftc takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_Ishftc(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Ishftc), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Ichar {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Ichar expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_character(*arg_type0)), "Unexpected args, Ichar expects (char) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Ichar takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Ichar(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_character(*arg_type0)))) {
                append_error(diag, "Unexpected args, Ichar expects (char) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Ichar takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        if ( args[1] != nullptr ) {
            int kind = -1;
            if (!ASR::is_a<ASR::Integer_t>(*expr_type(args[1])) || !extract_value(args[1], kind)) {
                append_error(diag, "`kind` argument of the `Ichar` function must be a scalar Integer constant", args[1]->base.loc);
                return nullptr;
            }
            set_kind_to_ttype_t(return_type, kind);
        }
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Ichar(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Ichar), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Char {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Char expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_integer(*arg_type0)), "Unexpected args, Char expects (int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Char takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Char(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_integer(*arg_type0)))) {
                append_error(diag, "Unexpected args, Char expects (int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Char takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = character(1);
        if ( args[1] != nullptr ) {
            int kind = -1;
            if (!ASR::is_a<ASR::Integer_t>(*expr_type(args[1])) || !extract_value(args[1], kind)) {
                append_error(diag, "`kind` argument of the `Char` function must be a scalar Integer constant", args[1]->base.loc);
                return nullptr;
            }
            set_kind_to_ttype_t(return_type, kind);
        }
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Char(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Char), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Exponent {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Exponent expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, Exponent expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Exponent takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Exponent(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Exponent expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Exponent takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Exponent(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Exponent), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Fraction {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Fraction expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, Fraction expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Fraction takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Fraction(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Fraction expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Fraction takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Fraction(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Fraction), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace SetExponent {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 2)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for SetExponent expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASRUtils::require_impl((is_real(*arg_type0) && is_integer(*arg_type1)), "Unexpected args, SetExponent expects (real, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, SetExponent takes 2 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_SetExponent(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 2)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            if(!((is_real(*arg_type0) && is_integer(*arg_type1)))) {
                append_error(diag, "Unexpected args, SetExponent expects (real, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, SetExponent takes 2 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 2);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 2);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            m_value = eval_SetExponent(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::SetExponent), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Rrspacing {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Rrspacing expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_real(*arg_type0)), "Unexpected args, Rrspacing expects (real) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Rrspacing takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Rrspacing(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_real(*arg_type0)))) {
                append_error(diag, "Unexpected args, Rrspacing expects (real) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Rrspacing takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Rrspacing(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Rrspacing), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Dshiftl {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 3)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Dshiftl expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[2]));
            ASRUtils::require_impl((is_integer(*arg_type0) && is_integer(*arg_type1) && is_integer(*arg_type2)), "Unexpected args, Dshiftl expects (int, int, int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Dshiftl takes 3 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Dshiftl(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 3)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            ASR::ttype_t *arg_type1 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[1]));
            ASR::ttype_t *arg_type2 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[2]));
            if(!((is_integer(*arg_type0) && is_integer(*arg_type1) && is_integer(*arg_type2)))) {
                append_error(diag, "Unexpected args, Dshiftl expects (int, int, int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Dshiftl takes 3 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[0]));
        ASR::ttype_t *return_type = type_;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 3);
        m_args.push_back(al, args[0]);
        m_args.push_back(al, args[1]);
        m_args.push_back(al, args[2]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 3);
            args_values.push_back(al, expr_value(m_args[0]));
            args_values.push_back(al, expr_value(m_args[1]));
            args_values.push_back(al, expr_value(m_args[2]));
            m_value = eval_Dshiftl(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Dshiftl), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Popcnt {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Popcnt expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_integer(*arg_type0)), "Unexpected args, Popcnt expects (int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Popcnt takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Popcnt(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_integer(*arg_type0)))) {
                append_error(diag, "Unexpected args, Popcnt expects (int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Popcnt takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Popcnt(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Popcnt), m_args.p, m_args.n, 0, return_type, m_value);
    }
}

namespace Poppar {

    static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {
        if (x.n_args == 1)  {
            ASRUtils::require_impl(x.m_overload_id == 0, "Overload Id for Poppar expected to be 0, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(x.m_args[0]));
            ASRUtils::require_impl((is_integer(*arg_type0)), "Unexpected args, Poppar expects (int) as arguments", x.base.base.loc, diagnostics);
        }
        else {
            ASRUtils::require_impl(false, "Unexpected number of args, Poppar takes 1 arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);
        }
    }

    static inline ASR::asr_t* create_Poppar(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) {
        if (args.size() == 1)  {
            ASR::ttype_t *arg_type0 = ASRUtils::type_get_past_const(ASRUtils::expr_type(args[0]));
            if(!((is_integer(*arg_type0)))) {
                append_error(diag, "Unexpected args, Poppar expects (int) as arguments", loc);
                return nullptr;
            }
        }
        else {
            append_error(diag, "Unexpected number of args, Poppar takes 1 arguments, found " + std::to_string(args.size()), loc);
            return nullptr;
        }
        ASR::ttype_t *return_type = int32;
        ASR::expr_t *m_value = nullptr;
        Vec<ASR::expr_t*> m_args; m_args.reserve(al, 1);
        m_args.push_back(al, args[0]);
        if (all_args_evaluated(m_args)) {
            Vec<ASR::expr_t*> args_values; args_values.reserve(al, 1);
            args_values.push_back(al, expr_value(m_args[0]));
            m_value = eval_Poppar(al, loc, return_type, args_values, diag);
        }
        return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::Poppar), m_args.p, m_args.n, 0, return_type, m_value);
    }
}


} // namespace ASRUtil

} // namespace LCompilers

#endif // LIBASR_PASS_INTRINSIC_FUNC_REG_UTIL_H
