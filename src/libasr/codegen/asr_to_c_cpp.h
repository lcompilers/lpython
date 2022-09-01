#ifndef LFORTRAN_ASR_TO_C_CPP_H
#define LFORTRAN_ASR_TO_C_CPP_H

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
#include <set>

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/codegen/asr_to_c.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/string_utils.h>
#include <libasr/pass/unused_functions.h>


namespace LFortran {

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

        CodeGenError(const std::string &msg, const Location &loc)
            : d{diag::Diagnostic(msg, diag::Level::Error, diag::Stage::CodeGen, {
                diag::Label("", {loc})
            })}
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
    Platform platform;
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
    std::set<std::string> headers;

    SymbolTable* global_scope;
    int64_t lower_bound;

    std::string template_for_Kokkos;
    size_t template_number;
    std::string from_std_vector_helper;

    BaseCCPPVisitor(diag::Diagnostics &diag, Platform &platform,
            bool gen_stdstring, bool gen_stdcomplex, bool is_c,
            int64_t default_lower_bound) : diag{diag},
            platform{platform},
        gen_stdstring{gen_stdstring}, gen_stdcomplex{gen_stdcomplex},
        is_c{is_c}, global_scope{nullptr}, lower_bound{default_lower_bound},
        template_number{0} {}

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        global_scope = x.m_global_scope;
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LFORTRAN_ASSERT(x.n_items == 0);
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

        {
            // Process intrinsic modules in the right order
            std::vector<std::string> build_order
                = LFortran::ASRUtils::determine_module_dependencies(x);
            for (auto &item : build_order) {
                LFORTRAN_ASSERT(x.m_global_scope->get_scope().find(item)
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
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                self().visit_symbol(*item.second);
                unit_src += src;
            }
        }

        // Then do all the modules in the right order
        std::vector<std::string> build_order
            = LFortran::ASRUtils::determine_module_dependencies(x);
        for (auto &item : build_order) {
            LFORTRAN_ASSERT(x.m_global_scope->get_scope().find(item)
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

        std::string contains;

        // Generate the bodies of subroutines
        for (auto &item : x.m_symtab->get_scope()) {
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

    void visit_BlockCall(const ASR::BlockCall_t &x) {
        LFORTRAN_ASSERT(ASR::is_a<ASR::Block_t>(*x.m_m));
        ASR::Block_t* block = ASR::down_cast<ASR::Block_t>(x.m_m);
        std::string decl, body;
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string open_paranthesis = indent + "{\n";
        std::string close_paranthesis = indent + "}\n";
        indent += std::string(indentation_spaces, ' ');
        indentation_level += 1;
        for (auto &item : block->m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
                decl += indent + self().convert_variable_decl(*v) + ";\n";
            }
        }
        for (size_t i=0; i<block->n_body; i++) {
            self().visit_stmt(*block->m_body[i]);
            body += src;
        }
        src = open_paranthesis + decl + body + close_paranthesis;
        indentation_level -= 1;
    }

    // Returns the declaration, no semi colon at the end
    std::string get_function_declaration(const ASR::Function_t &x) {
        template_for_Kokkos.clear();
        template_number = 0;
        std::string sub, inl;
        if (x.m_inline) {
            inl = "inline __attribute__((always_inline)) ";
        }
        if (x.m_return_var) {
            ASR::Variable_t *return_var = LFortran::ASRUtils::EXPR2VAR(x.m_return_var);
            if (ASRUtils::is_integer(*return_var->m_type)) {
                int kind = ASR::down_cast<ASR::Integer_t>(return_var->m_type)->m_kind;
                switch (kind) {
                    case (1) : sub = "int8_t "; break;
                    case (2) : sub = "int16_t "; break;
                    case (4) : sub = "int32_t "; break;
                    case (8) : sub = "int64_t "; break;
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
            } else if (ASR::is_a<ASR::CPtr_t>(*return_var->m_type)) {
                sub = "void* ";
            } else {
                throw CodeGenError("Return type not supported in function '" +
                    std::string(x.m_name) +
                    + "'", return_var->base.base.loc);
            }
        } else {
            sub = "void ";
        }
        std::string sym_name = x.m_name;
        if (sym_name == "main") {
            sym_name = "_xx_lcompilers_changed_main_xx";
        }
        if (sym_name == "exit") {
            sym_name = "_xx_lcompilers_changed_exit_xx";
        }
        std::string func = inl + sub + sym_name + "(";
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(x.m_args[i]);
            LFORTRAN_ASSERT(LFortran::ASRUtils::is_arg_dummy(arg->m_intent));
            if( is_c ) {
                func += self().convert_variable_decl(*arg, false);
            } else {
                func += self().convert_variable_decl(*arg, false, true);
            }
            if (i < x.n_args-1) func += ", ";
        }
        func += ")";
        if( is_c || template_for_Kokkos.empty() ) {
            return func;
        }

        template_for_Kokkos.pop_back();
        template_for_Kokkos.pop_back();
        return "\ntemplate <" + template_for_Kokkos + ">\n" + func;
    }

    std::string declare_all_functions(const SymbolTable &scope) {
        std::string code;
        for (auto &item : scope.get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                code += get_function_declaration(*s) + ";\n";
            }
        }
        return code;
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
        std::string sub = get_function_declaration(x);
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
                    if (v->m_intent == LFortran::ASRUtils::intent_local || v->m_intent == LFortran::ASRUtils::intent_return_var) {
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

            if (!visited_return && x.m_return_var) {
                body += indent + "return "
                    + LFortran::ASRUtils::EXPR2VAR(x.m_return_var)->m_name
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
            LFortran::ASRUtils::symbol_get_past_external(x.m_name));
        std::string fn_name = fn->m_name;
        if (sym_info[get_hash((ASR::asr_t*)fn)].intrinsic_function) {
            if (fn_name == "size") {
                LFORTRAN_ASSERT(x.n_args > 0);
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
                LFORTRAN_ASSERT(x.n_args > 0);
                self().visit_expr(*x.m_args[0].m_value);
                src = "(int)" + src;
            } else if (fn_name == "not") {
                LFORTRAN_ASSERT(x.n_args > 0);
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
        src = var_name + "->data->extent(" + args + ")";
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        std::string target;
        if (ASR::is_a<ASR::Var_t>(*x.m_target)) {
            visit_Var(*ASR::down_cast<ASR::Var_t>(x.m_target));
            target = src;
            if (!is_c && ASRUtils::is_array(ASRUtils::expr_type(x.m_target))) {
                target += "->data";
            }
        } else if (ASR::is_a<ASR::ArrayItem_t>(*x.m_target)) {
            self().visit_ArrayItem(*ASR::down_cast<ASR::ArrayItem_t>(x.m_target));
            target = src;
        } else if (ASR::is_a<ASR::DerivedRef_t>(*x.m_target)) {
            visit_DerivedRef(*ASR::down_cast<ASR::DerivedRef_t>(x.m_target));
            target = src;
        } else {
            LFORTRAN_ASSERT(false)
        }
        from_std_vector_helper.clear();
        self().visit_expr(*x.m_value);
        std::string value = src;
        std::string indent(indentation_level*indentation_spaces, ' ');
        if( !from_std_vector_helper.empty() ) {
            src = from_std_vector_helper;
        } else {
            src.clear();
        }
        src += indent + target + " = " + value + ";\n";
        from_std_vector_helper.clear();
    }

    void visit_IntegerConstant(const ASR::IntegerConstant_t &x) {
        src = std::to_string(x.m_n);
        last_expr_precedence = 2;
    }

    void visit_RealConstant(const ASR::RealConstant_t &x) {
        src = double_to_scientific(x.m_r);
        last_expr_precedence = 2;
    }


    void visit_StringConstant(const ASR::StringConstant_t &x) {
        src = "\"";
        std::string s = x.m_s;
        for (size_t idx=0; idx < s.size(); idx++) {
            if (s[idx] == '\n') {
                src += "\\n";
            } else if (s[idx] == '\\') {
                src += "\\\\";
            } else if (s[idx] == '\"') {
                src += "\\\"";
            } else {
                src += s[idx];
            }
        }
        src += "\"";
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
        ASR::Variable_t* sv = ASR::down_cast<ASR::Variable_t>(s);
        if( (sv->m_intent == ASRUtils::intent_in ||
            sv->m_intent == ASRUtils::intent_inout) &&
            is_c && ASRUtils::is_array(sv->m_type) &&
            ASRUtils::is_pointer(sv->m_type)) {
            src = "(*" + std::string(ASR::down_cast<ASR::Variable_t>(s)->m_name) + ")";
        } else {
            src = std::string(ASR::down_cast<ASR::Variable_t>(s)->m_name);
        }
        last_expr_precedence = 2;
    }

    void visit_DerivedRef(const ASR::DerivedRef_t& x) {
        std::string der_expr, member;
        this->visit_expr(*x.m_v);
        der_expr = std::move(src);
        member = ASRUtils::symbol_name(x.m_m);
        if( ASR::is_a<ASR::ArrayItem_t>(*x.m_v) ) {
            src = der_expr + "." + member;
        } else {
            src = der_expr + "->" + member;
        }
    }

    void visit_Cast(const ASR::Cast_t &x) {
        self().visit_expr(*x.m_arg);
        switch (x.m_kind) {
            case (ASR::cast_kindType::IntegerToReal) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 4: src = "(float)(" + src + ")"; break;
                    case 8: src = "(double)(" + src + ")"; break;
                    default: throw CodeGenError("Cast IntegerToReal: Unsupported Kind " + std::to_string(dest_kind));
                }
                break;
            }
            case (ASR::cast_kindType::RealToInteger) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                src = "(int" + std::to_string(dest_kind * 8) + "_t)(" + src + ")";
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
            case (ASR::cast_kindType::IntegerToComplex) : {
                if (is_c) {
                    headers.insert("complex");
                    src = "CMPLX(" + src + ", 0)";
                } else {
                    src = "std::complex<double>(" + src + ")";
                }
                break;
            }
            case (ASR::cast_kindType::ComplexToReal) : {
                if (is_c) {
                    headers.insert("complex");
                    src = "creal(" + src + ")";
                } else {
                    src = "std::real(" + src + ")";
                }
                break;
            }
            case (ASR::cast_kindType::RealToComplex) : {
                if (is_c) {
                    headers.insert("complex");
                    src = "CMPLX(" + src + ", 0.0)";
                } else {
                    src = "std::complex<double>(" + src + ")";
                }
                break;
            }
            case (ASR::cast_kindType::LogicalToInteger) : {
                src = "(int)(" + src + ")";
                break;
            }
            case (ASR::cast_kindType::IntegerToLogical) : {
                src = "(bool)(" + src + ")";
                break;
            }
            default : throw CodeGenError("Cast kind " + std::to_string(x.m_kind) + " not implemented",
                x.base.base.loc);
        }
        last_expr_precedence = 2;
    }

    void visit_IntegerCompare(const ASR::IntegerCompare_t &x) {
        handle_Compare(x);
    }

    void visit_RealCompare(const ASR::RealCompare_t &x) {
        handle_Compare(x);
    }

    void visit_ComplexCompare(const ASR::ComplexCompare_t &x) {
        handle_Compare(x);
    }

    void visit_LogicalCompare(const ASR::LogicalCompare_t &x) {
        handle_Compare(x);
    }

    void visit_StringCompare(const ASR::StringCompare_t &x) {
        handle_Compare(x);
    }

    template<typename T>
    void handle_Compare(const T &x) {
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
            default : LFORTRAN_ASSERT(false); // should never happen
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

    std::string get_c_type_from_ttype_t(ASR::ttype_t* t) {
        int kind = ASRUtils::extract_kind_from_ttype_t(t);
        std::string type_src = "";
        switch( t->type ) {
            case ASR::ttypeType::Integer: {
                type_src = "int" + std::to_string(kind * 8) + "_t";
                break;
            }
            case ASR::ttypeType::Pointer: {
                ASR::Pointer_t* ptr_type = ASR::down_cast<ASR::Pointer_t>(t);
                type_src = get_c_type_from_ttype_t(ptr_type->m_type) + "*";
                break;
            }
            case ASR::ttypeType::CPtr: {
                type_src = "void*";
                break;
            }
            case ASR::ttypeType::Derived: {
                ASR::Derived_t* der_type = ASR::down_cast<ASR::Derived_t>(t);
                type_src = std::string("struct ") + ASRUtils::symbol_name(der_type->m_derived_type);
                break;
            }
            default: {
                throw CodeGenError("Type " + ASRUtils::type_to_str_python(t) + " not supported yet.");
            }
        }
        return type_src;
    }

    void visit_GetPointer(const ASR::GetPointer_t& x) {
        self().visit_expr(*x.m_arg);
        std::string arg_src = std::move(src);
        std::string addr_prefix = "&";
        if( ASRUtils::is_array(ASRUtils::expr_type(x.m_arg)) ||
            ASR::is_a<ASR::Derived_t>(*ASRUtils::expr_type(x.m_arg)) ) {
            addr_prefix.clear();
        }
        src = addr_prefix + arg_src;
    }

    void visit_PointerToCPtr(const ASR::PointerToCPtr_t& x) {
        self().visit_expr(*x.m_arg);
        std::string arg_src = std::move(src);
        if( ASRUtils::is_array(ASRUtils::expr_type(x.m_arg)) ) {
            arg_src += "->data";
        }
        std::string type_src = get_c_type_from_ttype_t(x.m_type);
        src = "(" + type_src + ") " + arg_src;
    }

    void visit_CPtrToPointer(const ASR::CPtrToPointer_t& x) {
        self().visit_expr(*x.m_cptr);
        std::string source_src = std::move(src);
        self().visit_expr(*x.m_ptr);
        std::string dest_src = std::move(src);
        if( ASRUtils::is_array(ASRUtils::expr_type(x.m_ptr)) ) {
            dest_src += "->data";
        }
        std::string type_src = get_c_type_from_ttype_t(ASRUtils::expr_type(x.m_ptr));
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + dest_src + " = (" + type_src + ") " + source_src + ";\n";
    }

    void visit_IntegerBinOp(const ASR::IntegerBinOp_t &x) {
        handle_BinOp(x);
    }

    void visit_RealBinOp(const ASR::RealBinOp_t &x) {
        handle_BinOp(x);
    }

    void visit_ComplexBinOp(const ASR::ComplexBinOp_t &x) {
        handle_BinOp(x);
    }

    template <typename T>
    void handle_BinOp(const T &x) {
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
            case (ASR::binopType::BitAnd) : { last_expr_precedence = 11; break; }
            case (ASR::binopType::BitOr) : { last_expr_precedence = 13; break; }
            case (ASR::binopType::BitXor) : { last_expr_precedence = 12; break; }
            case (ASR::binopType::BitLShift) : { last_expr_precedence = 7; break; }
            case (ASR::binopType::BitRShift) : { last_expr_precedence = 7; break; }
            case (ASR::binopType::Pow) : {
                src = "pow(" + left + ", " + right + ")";
                if (is_c) {
                    headers.insert("math");
                } else {
                    src = "std::" + src;
                }
                return;
            }
            default: throw CodeGenError("BinOp: " + std::to_string(x.m_op) + " operator not implemented yet");
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
        src += ASRUtils::binop_to_str_python(x.m_op);
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

    void visit_LogicalBinOp(const ASR::LogicalBinOp_t &x) {
        self().visit_expr(*x.m_left);
        std::string left = std::move(src);
        int left_precedence = last_expr_precedence;
        self().visit_expr(*x.m_right);
        std::string right = std::move(src);
        int right_precedence = last_expr_precedence;
        switch (x.m_op) {
            case (ASR::logicalbinopType::And): {
                last_expr_precedence = 14;
                break;
            }
            case (ASR::logicalbinopType::Or): {
                last_expr_precedence = 15;
                break;
            }
            case (ASR::logicalbinopType::NEqv): {
                last_expr_precedence = 10;
                break;
            }
            case (ASR::logicalbinopType::Eqv): {
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
        src += ASRUtils::logicalbinop_to_str_python(x.m_op);
        if (right_precedence <= last_expr_precedence) {
            src += right;
        } else {
            src += "(" + right + ")";
        }
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
        if (current_function && current_function->m_return_var) {
            src = indent + "return "
                + LFortran::ASRUtils::EXPR2VAR(current_function->m_return_var)->m_name
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

    void visit_ErrorStop(const ASR::ErrorStop_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        if (is_c) {
            src = indent + "fprintf(stderr, \"ERROR STOP\");\n";
        } else {
            src = indent + "std::cerr << \"ERROR STOP\" << std::endl;\n";
        }
        src += indent + "exit(1);\n";
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
        ASR::Variable_t *loop_var = LFortran::ASRUtils::EXPR2VAR(x.m_head.m_v);
        std::string lvname=loop_var->m_name;
        ASR::expr_t *a=x.m_head.m_start;
        ASR::expr_t *b=x.m_head.m_end;
        ASR::expr_t *c=x.m_head.m_increment;
        LFORTRAN_ASSERT(a);
        LFORTRAN_ASSERT(b);
        int increment;
        if (!c) {
            increment = 1;
        } else {
            if (c->type == ASR::exprType::IntegerConstant) {
                increment = ASR::down_cast<ASR::IntegerConstant_t>(c)->m_n;
            } else if (c->type == ASR::exprType::IntegerUnaryMinus) {
                ASR::IntegerUnaryMinus_t *ium = ASR::down_cast<ASR::IntegerUnaryMinus_t>(c);
                increment = - ASR::down_cast<ASR::IntegerConstant_t>(ium->m_arg)->m_n;
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
        ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(
            LFortran::ASRUtils::symbol_get_past_external(x.m_name));
        // TODO: use a mapping with a hash(s) instead:
        std::string sym_name = s->m_name;
        if (sym_name == "exit") {
            sym_name = "_xx_lcompilers_changed_exit_xx";
        }
        std::string out = indent + sym_name + "(";
        for (size_t i=0; i<x.n_args; i++) {
            if (ASR::is_a<ASR::Var_t>(*x.m_args[i].m_value)) {
                ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(x.m_args[i].m_value);
                std::string arg_name = arg->m_name;
                if( ASRUtils::is_array(arg->m_type) &&
                    ASRUtils::is_pointer(arg->m_type) ) {
                    out += "&" + arg_name;
                } else {
                    out += arg_name;
                }
            } else {
                self().visit_expr(*x.m_args[i].m_value);
                if( ASR::is_a<ASR::ArrayItem_t>(*x.m_args[i].m_value) &&
                    ASR::is_a<ASR::Derived_t>(*ASRUtils::expr_type(x.m_args[i].m_value)) ) {
                    out += "&" + src;
                } else {
                    out += src;
                }
            }
            if (i < x.n_args-1) out += ", ";
        }
        out += ");\n";
        src = out;
    }

};

} // namespace LFortran

#endif // LFORTRAN_ASR_TO_C_CPP_H
