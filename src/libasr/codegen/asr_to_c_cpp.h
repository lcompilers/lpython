#ifndef LCOMPILERS_ASR_TO_C_CPP_H
#define LCOMPILERS_ASR_TO_C_CPP_H

/*
 * Common code to be used in both of:
 *
 * * asr_to_cpp.cpp
 * * asr_to_c.cpp
 *
 * In particular, a common base class visitor with visitors that are identical
 * for both C and C++ code generation.
 */

#include <iostream>
#include <memory>

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/codegen/asr_to_c.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/string_utils.h>
#include <libasr/pass/unused_functions.h>


namespace LCompilers {

namespace {

    // Local exception that is only used in this file to exit the visitor
    // pattern and caught later (not propagated outside)
    class CodeGenError
    {
    public:
        diag::Diagnostic d;
    public:
        CodeGenError(const std::string &msg)
            : d{diag::Diagnostic(msg, diag::Level::Error, diag::Stage::CodeGen)}
        { }
    };

    class Abort {};

}

// Platform dependent fast unique hash:
static uint64_t get_hash(ASR::asr_t *node)
{
    return (uint64_t)node;
}

struct SymbolInfo
{
    bool needs_declaration = true;
    bool intrinsic_function = false;
};

template <class Derived>
class BaseCCPPVisitor : public ASR::BaseVisitor<Derived>
{
private:
    Derived& self() { return static_cast<Derived&>(*this); }
public:
    diag::Diagnostics &diag;
    std::string src;
    int indentation_level;
    int indentation_spaces;
    // The precedence of the last expression, using the table:
    // https://en.cppreference.com/w/cpp/language/operator_precedence
    int last_expr_precedence;
    bool intrinsic_module = false;
    const ASR::Function_t *current_function = nullptr;
    std::map<uint64_t, SymbolInfo> sym_info;

    // Output configuration:
    // Use std::string or char*
    bool gen_stdstring;
    // Use std::complex<float/double> or float/double complex
    bool gen_stdcomplex;
    bool is_c;

    BaseCCPPVisitor(diag::Diagnostics &diag,
            bool gen_stdstring, bool gen_stdcomplex, bool is_c) : diag{diag},
        gen_stdstring{gen_stdstring}, gen_stdcomplex{gen_stdcomplex},
        is_c{is_c} {}

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LCOMPILERS_ASSERT(x.n_items == 0);
        std::string unit_src = "";
        indentation_level = 0;
        indentation_spaces = 4;

        std::string headers =
R"(#include <stdio.h>
#include <assert.h>
#include <complex.h>
#include <lfortran_intrinsics.h>
)";
        unit_src += headers;


        // TODO: We need to pre-declare all functions first, then generate code
        // Otherwise some function might not be found.

        {
            // Process intrinsic modules in the right order
            std::vector<std::string> build_order
                = LCompilers::ASRUtils::determine_module_dependencies(x);
            for (auto &item : build_order) {
                LCOMPILERS_ASSERT(x.m_global_scope->get_scope().find(item)
                    != x.m_global_scope->get_scope().end());
                if (startswith(item, "lfortran_intrinsic")) {
                    ASR::symbol_t *mod = x.m_global_scope->get_symbol(item);
                    self().visit_symbol(*mod);
                    unit_src += src;
                }
            }
        }

        // Process procedures first:
        for (auto &item : x.m_global_scope->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)
                || ASR::is_a<ASR::Subroutine_t>(*item.second)) {
                self().visit_symbol(*item.second);
                unit_src += src;
            }
        }

        // Then do all the modules in the right order
        std::vector<std::string> build_order
            = LCompilers::ASRUtils::determine_module_dependencies(x);
        for (auto &item : build_order) {
            LCOMPILERS_ASSERT(x.m_global_scope->get_scope().find(item)
                != x.m_global_scope->get_scope().end());
            if (!startswith(item, "lfortran_intrinsic")) {
                ASR::symbol_t *mod = x.m_global_scope->get_symbol(item);
                self().visit_symbol(*mod);
                unit_src += src;
            }
        }

        // Then the main program:
        for (auto &item : x.m_global_scope->get_scope()) {
            if (ASR::is_a<ASR::Program_t>(*item.second)) {
                self().visit_symbol(*item.second);
                unit_src += src;
            }
        }

        src = unit_src;
    }

    void visit_Module(const ASR::Module_t &x) {
        if (startswith(x.m_name, "lfortran_intrinsic_")) {
            intrinsic_module = true;
        } else {
            intrinsic_module = false;
        }
        // Generate code for nested subroutines and functions first:
        std::string contains;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Subroutine_t>(*item.second)) {
                ASR::Subroutine_t *s = ASR::down_cast<ASR::Subroutine_t>(item.second);
                self().visit_Subroutine(*s);
                contains += src;
            }
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                self().visit_Function(*s);
                contains += src;
            }
        }
        src = contains;
        intrinsic_module = false;
    }

    void visit_Program(const ASR::Program_t &x) {
        // Generate code for nested subroutines and functions first:
        std::string contains;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Subroutine_t>(*item.second)) {
                ASR::Subroutine_t *s = ASR::down_cast<ASR::Subroutine_t>(item.second);
                visit_Subroutine(*s);
                contains += src;
            }
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
                contains += src;
            }
        }

        // Generate code for the main program
        indentation_level += 1;
        std::string indent1(indentation_level*indentation_spaces, ' ');
        indentation_level += 1;
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string decl;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
                decl += self().convert_variable_decl(*v) + ";\n";
            }
        }

        std::string body;
        for (size_t i=0; i<x.n_body; i++) {
            self().visit_stmt(*x.m_body[i]);
            body += src;
        }

        src = contains
                + "int main(int argc, char* argv[])\n{\n"
                + decl + body
                + indent1 + "return 0;\n}\n";
        indentation_level -= 2;
    }

    void visit_Subroutine(const ASR::Subroutine_t &x) {
        indentation_level += 1;
        std::string sym_name = x.m_name;
        if (sym_name == "main") {
            sym_name = "_xx_lcompilers_changed_main_xx";
        }
        std::string sub = "void " + sym_name + "(";
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = LCompilers::ASRUtils::EXPR2VAR(x.m_args[i]);
            LCOMPILERS_ASSERT(ASRUtils::is_arg_dummy(arg->m_intent));
            sub += self().convert_variable_decl(*arg);
            if (i < x.n_args-1) sub += ", ";
        }
        sub += ")";
        if (x.m_abi == ASR::abiType::BindC
                && x.m_deftype == ASR::deftypeType::Interface) {
            sub += ";\n";
        } else {
            sub += "\n";

            for (auto &item : x.m_symtab->get_scope()) {
                if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
                    if (v->m_intent == LCompilers::ASRUtils::intent_local) {
                        SymbolInfo s;
                        s.needs_declaration = true;
                        sym_info[get_hash((ASR::asr_t*)v)] = s;
                    }
                }
            }

            std::string body;
            for (size_t i=0; i<x.n_body; i++) {
                self().visit_stmt(*x.m_body[i]);
                body += src;
            }

            std::string decl;
            for (auto &item : x.m_symtab->get_scope()) {
                if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
                    if (v->m_intent == LCompilers::ASRUtils::intent_local) {
                        if (sym_info[get_hash((ASR::asr_t*) v)].needs_declaration) {
                            std::string indent(indentation_level*indentation_spaces, ' ');
                            decl += indent;
                            decl += self().convert_variable_decl(*v) + ";\n";
                        }
                    }
                }
            }

            sub += "{\n" + decl + body + "}\n";
        }
        sub += "\n";
        src = sub;
        indentation_level -= 1;
    }

    void visit_Function(const ASR::Function_t &x) {
        if (std::string(x.m_name) == "size" && intrinsic_module ) {
            // Intrinsic function `size`
            SymbolInfo s;
            s.intrinsic_function = true;
            sym_info[get_hash((ASR::asr_t*)&x)] = s;
            src = "";
            return;
        } else if ((
                std::string(x.m_name) == "int" ||
                std::string(x.m_name) == "char" ||
                std::string(x.m_name) == "present" ||
                std::string(x.m_name) == "len" ||
                std::string(x.m_name) == "not"
                ) && intrinsic_module) {
            // Intrinsic function `int`
            SymbolInfo s;
            s.intrinsic_function = true;
            sym_info[get_hash((ASR::asr_t*)&x)] = s;
            src = "";
            return;
        } else {
            SymbolInfo s;
            s.intrinsic_function = false;
            sym_info[get_hash((ASR::asr_t*)&x)] = s;
        }
        std::string sub;
        ASR::Variable_t *return_var = LCompilers::ASRUtils::EXPR2VAR(x.m_return_var);
        if (ASRUtils::is_integer(*return_var->m_type)) {
            bool is_int = ASR::down_cast<ASR::Integer_t>(return_var->m_type)->m_kind == 4;
            if (is_int) {
                sub = "int ";
            } else {
                sub = "long long ";
            }
        } else if (ASRUtils::is_real(*return_var->m_type)) {
            bool is_float = ASR::down_cast<ASR::Real_t>(return_var->m_type)->m_kind == 4;
            if (is_float) {
                sub = "float ";
            } else {
                sub = "double ";
            }
        } else if (ASRUtils::is_logical(*return_var->m_type)) {
            sub = "bool ";
        } else if (ASRUtils::is_character(*return_var->m_type)) {
            if (gen_stdstring) {
                sub = "std::string ";
            } else {
                sub = "char* ";
            }
        } else if (ASRUtils::is_complex(*return_var->m_type)) {
            bool is_float = ASR::down_cast<ASR::Complex_t>(return_var->m_type)->m_kind == 4;
            if (is_float) {
                if (gen_stdcomplex) {
                    sub = "std::complex<float> ";
                } else {
                    sub = "float complex ";
                }
            } else {
                if (gen_stdcomplex) {
                    sub = "std::complex<double> ";
                } else {
                    sub = "double complex ";
                }
            }
        } else {
            throw CodeGenError("Return type not supported");
        }
        std::string sym_name = x.m_name;
        if (sym_name == "main") {
            sym_name = "_xx_lcompilers_changed_main_xx";
        }
        sub = sub + sym_name + "(";
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = LCompilers::ASRUtils::EXPR2VAR(x.m_args[i]);
            LCOMPILERS_ASSERT(LCompilers::ASRUtils::is_arg_dummy(arg->m_intent));
            sub += self().convert_variable_decl(*arg);
            if (i < x.n_args-1) sub += ", ";
        }
        sub += ")";
        if (x.m_abi == ASR::abiType::BindC
                && x.m_deftype == ASR::deftypeType::Interface) {
            sub += ";\n";
        } else {
            sub += "\n";

            indentation_level += 1;
            std::string indent(indentation_level*indentation_spaces, ' ');
            std::string decl;
            for (auto &item : x.m_symtab->get_scope()) {
                if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
                    if (v->m_intent == LCompilers::ASRUtils::intent_local || v->m_intent == LCompilers::ASRUtils::intent_return_var) {
                    decl += indent + self().convert_variable_decl(*v) + ";\n";
                    }
                }
            }

            current_function = &x;
            std::string body;

            for (size_t i=0; i<x.n_body; i++) {
                self().visit_stmt(*x.m_body[i]);
                body += src;
            }
            current_function = nullptr;
            bool visited_return = false;

            if (x.n_body > 0 && ASR::is_a<ASR::Return_t>(*x.m_body[x.n_body-1])) {
                visited_return = true;
            }

            if(!visited_return) {
                body += indent + "return "
                    + LCompilers::ASRUtils::EXPR2VAR(x.m_return_var)->m_name
                    + ";\n";
            }

            if (decl.size() > 0 || body.size() > 0) {
                sub += "{\n" + decl + body + "}\n";
            } else {
                sub[sub.size()-1] = ';';
                sub += "\n";
            }
            indentation_level -= 1;
        }
        sub += "\n";
        src = sub;
    }

    void visit_FunctionCall(const ASR::FunctionCall_t &x) {
        ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(
            LCompilers::ASRUtils::symbol_get_past_external(x.m_name));
        std::string fn_name = fn->m_name;
        if (sym_info[get_hash((ASR::asr_t*)fn)].intrinsic_function) {
            if (fn_name == "size") {
                LCOMPILERS_ASSERT(x.n_args > 0);
                self().visit_expr(*x.m_args[0].m_value);
                std::string var_name = src;
                std::string args;
                if (x.n_args == 1) {
                    args = "0";
                } else {
                    for (size_t i=1; i<x.n_args; i++) {
                        self().visit_expr(*x.m_args[i].m_value);
                        args += src + "-1";
                        if (i < x.n_args-1) args += ", ";
                    }
                }
                src = var_name + ".extent(" + args + ")";
            } else if (fn_name == "int") {
                LCOMPILERS_ASSERT(x.n_args > 0);
                self().visit_expr(*x.m_args[0].m_value);
                src = "(int)" + src;
            } else if (fn_name == "not") {
                LCOMPILERS_ASSERT(x.n_args > 0);
                self().visit_expr(*x.m_args[0].m_value);
                src = "!(" + src + ")";
            } else {
                throw CodeGenError("Intrinsic function '" + fn_name
                        + "' not implemented");
            }
        } else {
            std::string args;
            for (size_t i=0; i<x.n_args; i++) {
                self().visit_expr(*x.m_args[i].m_value);
                args += src;
                if (i < x.n_args-1) args += ", ";
            }
            src = fn_name + "(" + args + ")";
        }
        last_expr_precedence = 2;
    }

    void visit_ArraySize(const ASR::ArraySize_t& x) {
        self().visit_expr(*x.m_v);
        std::string var_name = src;
        std::string args = "";
        if (x.m_dim == nullptr) {
            // TODO: return the product of all dimensions:
            args = "0";
        } else {
            if( x.m_dim ) {
                self().visit_expr(*x.m_dim);
                args += src + "-1";
                args += ", ";
            }
            args += std::to_string(ASRUtils::extract_kind_from_ttype_t(x.m_type)) + "-1";
        }
        src = var_name + ".extent(" + args + ")";
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        std::string target;
        if (ASR::is_a<ASR::Var_t>(*x.m_target)) {
            target = LCompilers::ASRUtils::EXPR2VAR(x.m_target)->m_name;
        } else if (ASR::is_a<ASR::ArrayRef_t>(*x.m_target)) {
            visit_ArrayRef(*ASR::down_cast<ASR::ArrayRef_t>(x.m_target));
            target = src;
        } else {
            LCOMPILERS_ASSERT(false)
        }
        self().visit_expr(*x.m_value);
        std::string value = src;
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + target + " = " + value + ";\n";
    }

    void visit_IntegerConstant(const ASR::IntegerConstant_t &x) {
        src = std::to_string(x.m_n);
        last_expr_precedence = 2;
    }

    void visit_RealConstant(const ASR::RealConstant_t &x) {
        src = std::to_string(x.m_r);
        last_expr_precedence = 2;
    }


    void visit_StringConstant(const ASR::StringConstant_t &x) {
        src = "\"" + std::string(x.m_s) + "\"";
        last_expr_precedence = 2;
    }

    void visit_LogicalConstant(const ASR::LogicalConstant_t &x) {
        if (x.m_value == true) {
            src = "true";
        } else {
            src = "false";
        }
        last_expr_precedence = 2;
    }

    void visit_Var(const ASR::Var_t &x) {
        const ASR::symbol_t *s = ASRUtils::symbol_get_past_external(x.m_v);
        src = ASR::down_cast<ASR::Variable_t>(s)->m_name;
        last_expr_precedence = 2;
    }

    void visit_ArrayRef(const ASR::ArrayRef_t &x) {
        const ASR::symbol_t *s = ASRUtils::symbol_get_past_external(x.m_v);
        std::string out = ASR::down_cast<ASR::Variable_t>(s)->m_name;
        out += "[";
        for (size_t i=0; i<x.n_args; i++) {
            if (x.m_args[i].m_right) {
                self().visit_expr(*x.m_args[i].m_right);
            } else {
                src = "/* FIXME right index */";
            }
            out += src;
            if (i < x.n_args-1) out += ", ";
        }
        out += "-1]";
        last_expr_precedence = 2;
        src = out;
    }

    void visit_Cast(const ASR::Cast_t &x) {
        self().visit_expr(*x.m_arg);
        switch (x.m_kind) {
            case (ASR::cast_kindType::IntegerToReal) : {
                src = "(float)(" + src + ")";
                break;
            }
            case (ASR::cast_kindType::RealToInteger) : {
                src = "(int)(" + src + ")";
                break;
            }
            case (ASR::cast_kindType::RealToReal) : {
                // In C++, we do not need to cast float to float explicitly:
                // src = src;
                break;
            }
            case (ASR::cast_kindType::IntegerToInteger) : {
                // In C++, we do not need to cast int <-> long long explicitly:
                // src = src;
                break;
            }
            case (ASR::cast_kindType::ComplexToComplex) : {
                break;
            }
            case (ASR::cast_kindType::ComplexToReal) : {
                src = "creal(" + src + ")";
                break;
            }
            case (ASR::cast_kindType::LogicalToInteger) : {
                src = "(int)(" + src + ")";
                break;
            }
            default : throw CodeGenError("Cast kind " + std::to_string(x.m_kind) + " not implemented");
        }
        last_expr_precedence = 2;
    }

    void visit_Compare(const ASR::Compare_t &x) {
        self().visit_expr(*x.m_left);
        std::string left = std::move(src);
        int left_precedence = last_expr_precedence;
        self().visit_expr(*x.m_right);
        std::string right = std::move(src);
        int right_precedence = last_expr_precedence;
        switch (x.m_op) {
            case (ASR::cmpopType::Eq) : { last_expr_precedence = 10; break; }
            case (ASR::cmpopType::Gt) : { last_expr_precedence = 9;  break; }
            case (ASR::cmpopType::GtE) : { last_expr_precedence = 9; break; }
            case (ASR::cmpopType::Lt) : { last_expr_precedence = 9;  break; }
            case (ASR::cmpopType::LtE) : { last_expr_precedence = 9; break; }
            case (ASR::cmpopType::NotEq): { last_expr_precedence = 10; break; }
            default : LCOMPILERS_ASSERT(false); // should never happen
        }
        if (left_precedence <= last_expr_precedence) {
            src += left;
        } else {
            src += "(" + left + ")";
        }
        src += ASRUtils::cmpop_to_str(x.m_op);
        if (right_precedence <= last_expr_precedence) {
            src += right;
        } else {
            src += "(" + right + ")";
        }
    }

    void visit_UnaryOp(const ASR::UnaryOp_t &x) {
        self().visit_expr(*x.m_operand);
        int expr_precedence = last_expr_precedence;
        if (x.m_type->type == ASR::ttypeType::Integer) {
            if (x.m_op == ASR::unaryopType::UAdd) {
                // src = src;
                // Skip unary plus, keep the previous precedence

            } else if (x.m_op == ASR::unaryopType::Not) {
                last_expr_precedence = 3;
                if (expr_precedence <= last_expr_precedence) {
                    src = "!" + src;
                } else {
                    src = "!(" + src + ")";
                }
            } else {
                throw CodeGenError("Unary type not implemented yet for Integer");
            }
            return;
        } else if (x.m_type->type == ASR::ttypeType::Real) {
            if (x.m_op == ASR::unaryopType::UAdd) {
                // src = src;
                // Skip unary plus, keep the previous precedence
            } else if (x.m_op == ASR::unaryopType::Not) {
                last_expr_precedence = 3;
                if (expr_precedence <= last_expr_precedence) {
                    src = "!" + src;
                } else {
                    src = "!(" + src + ")";
                }
            } else {
                throw CodeGenError("Unary type not implemented yet for Real");
            }
            return;
        } else {
            throw CodeGenError("UnaryOp: type not supported yet");
        }
    }

    void visit_IntegerBitNot(const ASR::IntegerBitNot_t& x) {
        self().visit_expr(*x.m_arg);
        int expr_precedence = last_expr_precedence;
        last_expr_precedence = 3;
        if (expr_precedence <= last_expr_precedence) {
            src = "~" + src;
        } else {
            src = "~(" + src + ")";
        }
    }

    void visit_IntegerUnaryMinus(const ASR::IntegerUnaryMinus_t &x) {
        handle_UnaryMinus(x);
    }

    void visit_RealUnaryMinus(const ASR::RealUnaryMinus_t &x) {
        handle_UnaryMinus(x);
    }

    template <typename T>
    void handle_UnaryMinus(const T &x) {
        self().visit_expr(*x.m_arg);
        int expr_precedence = last_expr_precedence;
        last_expr_precedence = 3;
        if (expr_precedence <= last_expr_precedence) {
            src = "-" + src;
        } else {
            src = "-(" + src + ")";
        }
    }

    void visit_LogicalNot(const ASR::LogicalNot_t &x) {
        self().visit_expr(*x.m_arg);
        int expr_precedence = last_expr_precedence;
        last_expr_precedence = 3;
        if (expr_precedence <= last_expr_precedence) {
            src = "!" + src;
        } else {
            src = "!(" + src + ")";
        }
    }

    void visit_BinOp(const ASR::BinOp_t &x) {
        self().visit_expr(*x.m_left);
        std::string left = std::move(src);
        int left_precedence = last_expr_precedence;
        self().visit_expr(*x.m_right);
        std::string right = std::move(src);
        int right_precedence = last_expr_precedence;
        switch (x.m_op) {
            case (ASR::binopType::Add) : { last_expr_precedence = 6; break; }
            case (ASR::binopType::Sub) : { last_expr_precedence = 6; break; }
            case (ASR::binopType::Mul) : { last_expr_precedence = 5; break; }
            case (ASR::binopType::Div) : { last_expr_precedence = 5; break; }
            case (ASR::binopType::Pow) : {
                src = "pow(" + left + ", " + right + ")";
                if (!is_c) src = "std::" + src;
                return;
            }
            default: throw CodeGenError("BinOp: operator not implemented yet");
        }
        src = "";
        if (left_precedence == 3) {
            src += "(" + left + ")";
        } else {
            if (left_precedence <= last_expr_precedence) {
                src += left;
            } else {
                src += "(" + left + ")";
            }
        }
        src += ASRUtils::binop_to_str(x.m_op);
        if (right_precedence == 3) {
            src += "(" + right + ")";
        } else if (x.m_op == ASR::binopType::Sub) {
            if (right_precedence < last_expr_precedence) {
                src += right;
            } else {
                src += "(" + right + ")";
            }
        } else {
            if (right_precedence <= last_expr_precedence) {
                src += right;
            } else {
                src += "(" + right + ")";
            }
        }
    }

    void visit_BoolOp(const ASR::BoolOp_t &x) {
        self().visit_expr(*x.m_left);
        std::string left = std::move(src);
        int left_precedence = last_expr_precedence;
        self().visit_expr(*x.m_right);
        std::string right = std::move(src);
        int right_precedence = last_expr_precedence;
        switch (x.m_op) {
            case (ASR::boolopType::And): {
                last_expr_precedence = 14;
                break;
            }
            case (ASR::boolopType::Or): {
                last_expr_precedence = 15;
                break;
            }
            case (ASR::boolopType::NEqv): {
                last_expr_precedence = 10;
                break;
            }
            case (ASR::boolopType::Eqv): {
                last_expr_precedence = 10;
                break;
            }
            default : throw CodeGenError("Unhandled switch case");
        }

        if (left_precedence <= last_expr_precedence) {
            src += left;
        } else {
            src += "(" + left + ")";
        }
        src += ASRUtils::boolop_to_str(x.m_op);
        if (right_precedence <= last_expr_precedence) {
            src += right;
        } else {
            src += "(" + right + ")";
        }
    }

    std::string get_print_type(ASR::ttype_t *t) {
        switch (t->type) {
            case ASR::ttypeType::Integer: {
                ASR::Integer_t *i = (ASR::Integer_t*)t;
                switch (i->m_kind) {
                    case 1: { return "%d"; }
                    case 2: { return "%d"; }
                    case 4: { return "%d"; }
                    case 8: { return "%lli"; }
                    default: { throw LCompilersException("Integer kind not supported"); }
                }
            }
            case ASR::ttypeType::Real: {
                ASR::Real_t *r = (ASR::Real_t*)t;
                switch (r->m_kind) {
                    case 4: { return "%f"; }
                    case 8: { return "%lf"; }
                    default: { throw LCompilersException("Float kind not supported"); }
                }
            }
            case ASR::ttypeType::Logical: {
                return "%d";
            }
            case ASR::ttypeType::Character: {
                return "%s";
            }
            default : throw LCompilersException("Not implemented");
        }
    }

    void visit_Print(const ASR::Print_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "printf(\"";
        std::vector<std::string> v;
        for (size_t i=0; i<x.n_values; i++) {
            self().visit_expr(*x.m_values[i]);
            out += get_print_type(ASRUtils::expr_type(x.m_values[i]));
            if (i+1!=x.n_values) {
                out += " ";
            }
            v.push_back(src);
        }
        out += "\\n\"";
        if (!v.empty()) {
            for (auto s: v) {
                out += ", " + s;
            }
        }
        out += ");\n";
        src = out;
    }

    void visit_Allocate(const ASR::Allocate_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "// FIXME: allocate(";
        for (size_t i=0; i<x.n_args; i++) {
            ASR::symbol_t* a = x.m_args[i].m_a;
            //ASR::dimension_t* dims = x.m_args[i].m_dims;
            //size_t n_dims = x.m_args[i].n_dims;
            out += std::string(ASRUtils::symbol_name(a)) + ", ";
        }
        out += ");\n";
        src = out;
    }

    void visit_Assert(const ASR::Assert_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent;
        if (x.m_msg) {
            out += "assert ((";
            self().visit_expr(*x.m_msg);
            out += src + ", ";
            self().visit_expr(*x.m_test);
            out += src + "));\n";
        } else {
            out += "assert (";
            self().visit_expr(*x.m_test);
            out += src + ");\n";
        }
        src = out;
    }

    void visit_ExplicitDeallocate(const ASR::ExplicitDeallocate_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "// FIXME: deallocate(";
        for (size_t i=0; i<x.n_vars; i++) {
            out += std::string(ASRUtils::symbol_name(x.m_vars[i])) + ", ";
        }
        out += ");\n";
        src = out;
    }

    void visit_ImplicitDeallocate(const ASR::ImplicitDeallocate_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "// FIXME: implicit deallocate(";
        for (size_t i=0; i<x.n_vars; i++) {
            out += std::string(ASRUtils::symbol_name(x.m_vars[i])) + ", ";
        }
        out += ");\n";
        src = out;
    }

    void visit_Select(const ASR::Select_t &/*x*/) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "// FIXME: select case()\n";
        src = out;
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "while (";
        self().visit_expr(*x.m_test);
        out += src + ") {\n";
        indentation_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            self().visit_stmt(*x.m_body[i]);
            out += src;
        }
        out += indent + "}\n";
        indentation_level -= 1;
        src = out;
    }

    void visit_Exit(const ASR::Exit_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "break;\n";
    }

    void visit_Cycle(const ASR::Cycle_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "continue;\n";
    }

    void visit_Return(const ASR::Return_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        if (current_function) {
            src = indent + "return "
                + LCompilers::ASRUtils::EXPR2VAR(current_function->m_return_var)->m_name
                + ";\n";
        } else {
            src = indent + "return;\n";
        }
    }

    void visit_GoToTarget(const ASR::GoToTarget_t & /* x */) {
        // Ignore for now
        src = "";
    }

    void visit_Stop(const ASR::Stop_t &x) {
        if (x.m_code) {
            self().visit_expr(*x.m_code);
        } else {
            src = "0";
        }
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "exit(" + src + ");\n";
    }

    void visit_ImpliedDoLoop(const ASR::ImpliedDoLoop_t &/*x*/) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + " /* FIXME: implied do loop */ ";
        src = out;
        last_expr_precedence = 2;
    }

    void visit_DoLoop(const ASR::DoLoop_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "for (";
        ASR::Variable_t *loop_var = LCompilers::ASRUtils::EXPR2VAR(x.m_head.m_v);
        std::string lvname=loop_var->m_name;
        ASR::expr_t *a=x.m_head.m_start;
        ASR::expr_t *b=x.m_head.m_end;
        ASR::expr_t *c=x.m_head.m_increment;
        LCOMPILERS_ASSERT(a);
        LCOMPILERS_ASSERT(b);
        int increment;
        if (!c) {
            increment = 1;
        } else {
            if (c->type == ASR::exprType::IntegerConstant) {
                increment = ASR::down_cast<ASR::IntegerConstant_t>(c)->m_n;
            } else if (c->type == ASR::exprType::UnaryOp) {
                ASR::UnaryOp_t *u = ASR::down_cast<ASR::UnaryOp_t>(c);
                LCOMPILERS_ASSERT(u->m_op == ASR::unaryopType::USub);
                LCOMPILERS_ASSERT(u->m_operand->type == ASR::exprType::IntegerConstant);
                increment = - ASR::down_cast<ASR::IntegerConstant_t>(u->m_operand)->m_n;
            } else {
                throw CodeGenError("Do loop increment type not supported");
            }
        }
        std::string cmp_op;
        if (increment > 0) {
            cmp_op = "<=";
        } else {
            cmp_op = ">=";
        }

        out += lvname + "=";
        self().visit_expr(*a);
        out += src + "; " + lvname + cmp_op;
        self().visit_expr(*b);
        out += src + "; " + lvname;
        if (increment == 1) {
            out += "++";
        } else if (increment == -1) {
            out += "--";
        } else {
            out += "+=" + std::to_string(increment);
        }
        out += ") {\n";
        indentation_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            self().visit_stmt(*x.m_body[i]);
            out += src;
        }
        out += indent + "}\n";
        indentation_level -= 1;
        src = out;
    }

    void visit_ErrorStop(const ASR::ErrorStop_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "fprintf(stderr, \"ERROR STOP\");\n";
        src += indent + "exit(1);\n";
    }

    void visit_If(const ASR::If_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "if (";
        self().visit_expr(*x.m_test);
        out += src + ") {\n";
        indentation_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            self().visit_stmt(*x.m_body[i]);
            out += src;
        }
        out += indent + "}";
        if (x.n_orelse == 0) {
            out += "\n";
        } else {
            out += " else {\n";
            for (size_t i=0; i<x.n_orelse; i++) {
                self().visit_stmt(*x.m_orelse[i]);
                out += src;
            }
            out += indent + "}\n";
        }
        indentation_level -= 1;
        src = out;
    }

    void visit_IfExp(const ASR::IfExp_t &x) {
        // IfExp is like a ternary operator in c++
        // test ? body : orelse;
        std::string out = "(";
        self().visit_expr(*x.m_test);
        out += src + ") ? (";
        self().visit_expr(*x.m_body);
        out += src + ") : (";
        self().visit_expr(*x.m_orelse);
        out += src + ")";
        src = out;
        last_expr_precedence = 16;
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        ASR::Subroutine_t *s = ASR::down_cast<ASR::Subroutine_t>(
            LCompilers::ASRUtils::symbol_get_past_external(x.m_name));
        std::string out = indent + s->m_name + "(";
        for (size_t i=0; i<x.n_args; i++) {
            if (ASR::is_a<ASR::Var_t>(*x.m_args[i].m_value)) {
                ASR::Variable_t *arg = LCompilers::ASRUtils::EXPR2VAR(x.m_args[i].m_value);
                std::string arg_name = arg->m_name;
                out += arg_name;
            } else {
                self().visit_expr(*x.m_args[i].m_value);
                out += src;
            }
            if (i < x.n_args-1) out += ", ";
        }
        out += ");\n";
        src = out;
    }

};

} // namespace LCompilers

#endif // LCOMPILERS_ASR_TO_C_CPP_H
