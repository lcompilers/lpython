#include <iostream>
#include <memory>

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/codegen/asr_to_cpp.h>
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
    };

    class Abort {};

}

// Platform dependent fast unique hash:
uint64_t get_hash(ASR::asr_t *node)
{
    return (uint64_t)node;
}

struct SymbolInfo
{
    bool needs_declaration = true;
    bool intrinsic_function = false;
};

std::string convert_dims(size_t n_dims, ASR::dimension_t *m_dims)
{
    std::string dims;
    for (size_t i=0; i<n_dims; i++) {
        ASR::expr_t *start = m_dims[i].m_start;
        ASR::expr_t *end = m_dims[i].m_end;
        if (!start && !end) {
            dims += "*";
        } else if (start && end) {
            if (ASR::is_a<ASR::ConstantInteger_t>(*start) && ASR::is_a<ASR::ConstantInteger_t>(*end)) {
                ASR::ConstantInteger_t *s = ASR::down_cast<ASR::ConstantInteger_t>(start);
                ASR::ConstantInteger_t *e = ASR::down_cast<ASR::ConstantInteger_t>(end);
                if (s->m_n == 1) {
                    dims += "[" + std::to_string(e->m_n) + "]";
                } else {
                    throw CodeGenError("Lower dimension must be 1 for now");
                }
            } else {
                dims += "[ /* FIXME symbolic dimensions */ ]";
            }
        } else {
            throw CodeGenError("Dimension type not supported");
        }
    }
    return dims;
}

std::string format_type(const std::string &dims, const std::string &type,
        const std::string &name, bool use_ref, bool dummy)
{
    std::string fmt;
    if (dims.size() == 0) {
        std::string ref;
        if (use_ref) ref = "&";
        fmt = type + " " + ref + name;
    } else {
        if (dummy) {
            std::string c;
            if (!use_ref) c = "const ";
            fmt = "const Kokkos::View<" + c + type + dims + "> &" + name;
        } else {
            fmt = "Kokkos::View<" + type + dims + "> " + name
                + "(\"" + name + "\")";
        }
    }
    return fmt;
}

class ASRToCPPVisitor : public ASR::BaseVisitor<ASRToCPPVisitor>
{
public:
    diag::Diagnostics &diag;
    std::map<uint64_t, SymbolInfo> sym_info;
    std::string src;
    int indentation_level;
    int indentation_spaces;
    // The precedence of the last expression, using the table:
    // https://en.cppreference.com/w/cpp/language/operator_precedence
    int last_expr_precedence;
    bool intrinsic_module = false;
    const ASR::Function_t *current_function = nullptr;

    ASRToCPPVisitor(diag::Diagnostics &diag) : diag{diag} {}

    std::string convert_variable_decl(const ASR::Variable_t &v)
    {
        std::string sub;
        bool use_ref = (v.m_intent == LFortran::ASRUtils::intent_out || v.m_intent == LFortran::ASRUtils::intent_inout);
        bool dummy = LFortran::ASRUtils::is_arg_dummy(v.m_intent);
        if (ASRUtils::is_pointer(v.m_type)) {
            ASR::ttype_t *t2 = ASR::down_cast<ASR::Pointer_t>(v.m_type)->m_type;
            if (ASRUtils::is_integer(*t2)) {
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(t2);
                std::string dims = convert_dims(t->n_dims, t->m_dims);
                sub = format_type(dims, "int *", v.m_name, use_ref, dummy);
            } else {
                diag.codegen_error_label("Type number '"
                    + std::to_string(v.m_type->type)
                    + "' not supported", {v.base.base.loc}, "");
                throw Abort();
            }
        } else {
            if (ASRUtils::is_integer(*v.m_type)) {
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(v.m_type);
                std::string dims = convert_dims(t->n_dims, t->m_dims);
                std::string type_name = "int";
                if (t->m_kind == 8) type_name = "long long";
                sub = format_type(dims, type_name, v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_real(*v.m_type)) {
                ASR::Real_t *t = ASR::down_cast<ASR::Real_t>(v.m_type);
                std::string dims = convert_dims(t->n_dims, t->m_dims);
                std::string type_name = "float";
                if (t->m_kind == 8) type_name = "double";
                sub = format_type(dims, type_name, v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_complex(*v.m_type)) {
                ASR::Complex_t *t = ASR::down_cast<ASR::Complex_t>(v.m_type);
                std::string dims = convert_dims(t->n_dims, t->m_dims);
                sub = format_type(dims, "std::complex<float>", v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_logical(*v.m_type)) {
                ASR::Logical_t *t = ASR::down_cast<ASR::Logical_t>(v.m_type);
                std::string dims = convert_dims(t->n_dims, t->m_dims);
                sub = format_type(dims, "bool", v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_character(*v.m_type)) {
                ASR::Character_t *t = ASR::down_cast<ASR::Character_t>(v.m_type);
                std::string dims = convert_dims(t->n_dims, t->m_dims);
                sub = format_type(dims, "std::string", v.m_name, use_ref, dummy);
                if (v.m_symbolic_value) {
                    this->visit_expr(*v.m_symbolic_value);
                    std::string init = src;
                    sub += "=" + init;
                }
            } else if (ASR::is_a<ASR::Derived_t>(*v.m_type)) {
                ASR::Derived_t *t = ASR::down_cast<ASR::Derived_t>(v.m_type);
                std::string dims = convert_dims(t->n_dims, t->m_dims);
                sub = format_type(dims, "struct", v.m_name, use_ref, dummy);
                if (v.m_symbolic_value) {
                    this->visit_expr(*v.m_symbolic_value);
                    std::string init = src;
                    sub += "=" + init;
                }
            } else {
                diag.codegen_error_label("Type number '"
                    + std::to_string(v.m_type->type)
                    + "' not supported", {v.base.base.loc}, "");
                throw Abort();
            }
        }
        return sub;
    }


    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LFORTRAN_ASSERT(x.n_items == 0);
        std::string unit_src = "";
        indentation_level = 0;
        indentation_spaces = 4;

        std::string headers =
R"(#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <cmath>
#include <Kokkos_Core.hpp>
#include <lfortran_intrinsics.h>

template <typename T>
Kokkos::View<T*> from_std_vector(const std::vector<T> &v)
{
    Kokkos::View<T*> r("r", v.size());
    for (size_t i=0; i < v.size(); i++) {
        r(i) = v[i];
    }
    return r;
}

)";
        unit_src += headers;


        // TODO: We need to pre-declare all functions first, then generate code
        // Otherwise some function might not be found.

        {
            // Process intrinsic modules in the right order
            std::vector<std::string> build_order
                = LFortran::ASRUtils::determine_module_dependencies(x);
            for (auto &item : build_order) {
                LFORTRAN_ASSERT(x.m_global_scope->scope.find(item)
                    != x.m_global_scope->scope.end());
                if (startswith(item, "lfortran_intrinsic")) {
                    ASR::symbol_t *mod = x.m_global_scope->scope[item];
                    visit_symbol(*mod);
                    unit_src += src;
                }
            }
        }

        // Process procedures first:
        for (auto &item : x.m_global_scope->scope) {
            if (ASR::is_a<ASR::Function_t>(*item.second)
                || ASR::is_a<ASR::Subroutine_t>(*item.second)) {
                visit_symbol(*item.second);
                unit_src += src;
            }
        }

        // Then do all the modules in the right order
        std::vector<std::string> build_order
            = LFortran::ASRUtils::determine_module_dependencies(x);
        for (auto &item : build_order) {
            LFORTRAN_ASSERT(x.m_global_scope->scope.find(item)
                != x.m_global_scope->scope.end());
            if (!startswith(item, "lfortran_intrinsic")) {
                ASR::symbol_t *mod = x.m_global_scope->scope[item];
                visit_symbol(*mod);
                unit_src += src;
            }
        }

        // Then the main program:
        for (auto &item : x.m_global_scope->scope) {
            if (ASR::is_a<ASR::Program_t>(*item.second)) {
                visit_symbol(*item.second);
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
        for (auto &item : x.m_symtab->scope) {
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
        src = contains;
        intrinsic_module = false;
    }

    void visit_Program(const ASR::Program_t &x) {
        // Generate code for nested subroutines and functions first:
        std::string contains;
        for (auto &item : x.m_symtab->scope) {
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
        for (auto &item : x.m_symtab->scope) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
                decl += indent;
                decl += convert_variable_decl(*v) + ";\n";
            }
        }

        std::string body;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            body += src;
        }

        src = "namespace {\n"
                + contains
                + "\nvoid main2() {\n"
                + decl + body
                + "}\n\n"
                + "}\n"
                + "int main(int argc, char* argv[])\n{\n"
                + indent1 + "Kokkos::initialize(argc, argv);\n"
                + indent1 + "main2();\n"
                + indent1 + "Kokkos::finalize();\n"
                + indent1 + "return 0;\n}\n";
        indentation_level -= 2;
    }

    void visit_Subroutine(const ASR::Subroutine_t &x) {
        indentation_level += 1;
        std::string sub = "void " + std::string(x.m_name) + "(";
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(x.m_args[i]);
            LFORTRAN_ASSERT(ASRUtils::is_arg_dummy(arg->m_intent));
            sub += convert_variable_decl(*arg);
            if (i < x.n_args-1) sub += ", ";
        }
        sub += ")\n";

        for (auto &item : x.m_symtab->scope) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
                if (v->m_intent == LFortran::ASRUtils::intent_local) {
                    SymbolInfo s;
                    s.needs_declaration = true;
                    sym_info[get_hash((ASR::asr_t*)v)] = s;
                }
            }
        }

        std::string body;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            body += src;
        }

        std::string decl;
        for (auto &item : x.m_symtab->scope) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
                if (v->m_intent == LFortran::ASRUtils::intent_local) {
                    if (sym_info[get_hash((ASR::asr_t*) v)].needs_declaration) {
                        std::string indent(indentation_level*indentation_spaces, ' ');
                        decl += indent;
                        decl += convert_variable_decl(*v) + ";\n";
                    }
                }
            }
        }

        sub += "{\n" + decl + body + "}\n\n";
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
        ASR::Variable_t *return_var = LFortran::ASRUtils::EXPR2VAR(x.m_return_var);
        if (ASRUtils::is_integer(*return_var->m_type)) {
            bool is_int = ASR::down_cast<ASR::Integer_t>(return_var->m_type)->m_kind == 4;
            if (is_int) {
                sub = "int ";
            } else {
                sub = "long long ";
            }
        } else if (ASRUtils::is_real(*return_var->m_type)) {
            sub = "float ";
        } else if (ASRUtils::is_logical(*return_var->m_type)) {
            sub = "bool ";
        } else if (ASRUtils::is_character(*return_var->m_type)) {
            sub = "std::string ";
        } else if (ASRUtils::is_complex(*return_var->m_type)) {
            sub = "std::complex<float> ";
        } else {
            throw CodeGenError("Return type not supported");
        }
        sub = sub + std::string(x.m_name) + "(";
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(x.m_args[i]);
            LFORTRAN_ASSERT(LFortran::ASRUtils::is_arg_dummy(arg->m_intent));
            sub += convert_variable_decl(*arg);
            if (i < x.n_args-1) sub += ", ";
        }
        sub += ")\n";

        indentation_level += 1;
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string decl;
        for (auto &item : x.m_symtab->scope) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
                if (v->m_intent == LFortran::ASRUtils::intent_local || v->m_intent == LFortran::ASRUtils::intent_return_var) {
                   decl += indent + convert_variable_decl(*v) + ";\n";
                }
            }
        }

        current_function = &x;
        std::string body;

        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            body += src;
        }
        current_function = nullptr;
        bool visited_return = false;

        if (x.n_body > 0 && ASR::is_a<ASR::Return_t>(*x.m_body[x.n_body-1])) {
            visited_return = true;
        }

        if(!visited_return) {
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
        sub += "\n";
        src = sub;
        indentation_level -= 1;
    }

    void visit_FunctionCall(const ASR::FunctionCall_t &x) {
        ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(
            LFortran::ASRUtils::symbol_get_past_external(x.m_name));
        std::string fn_name = fn->m_name;
        if (sym_info[get_hash((ASR::asr_t*)fn)].intrinsic_function) {
            if (fn_name == "size") {
                LFORTRAN_ASSERT(x.n_args > 0);
                visit_expr(*x.m_args[0].m_value);
                std::string var_name = src;
                std::string args;
                if (x.n_args == 1) {
                    args = "0";
                } else {
                    for (size_t i=1; i<x.n_args; i++) {
                        visit_expr(*x.m_args[i].m_value);
                        args += src + "-1";
                        if (i < x.n_args-1) args += ", ";
                    }
                }
                src = var_name + ".extent(" + args + ")";
            } else if (fn_name == "int") {
                LFORTRAN_ASSERT(x.n_args > 0);
                visit_expr(*x.m_args[0].m_value);
                src = "(int)" + src;
            } else if (fn_name == "not") {
                LFORTRAN_ASSERT(x.n_args > 0);
                visit_expr(*x.m_args[0].m_value);
                src = "!(" + src + ")";
            } else if (fn_name == "len") {
                LFORTRAN_ASSERT(x.n_args > 0);
                visit_expr(*x.m_args[0].m_value);
                src = "(" + src + ").size()";
            } else {
                throw CodeGenError("Intrinsic function '" + fn_name
                        + "' not implemented");
            }
        } else {
            std::string args;
            for (size_t i=0; i<x.n_args; i++) {
                visit_expr(*x.m_args[i].m_value);
                args += src;
                if (i < x.n_args-1) args += ", ";
            }
            src = fn_name + "(" + args + ")";
        }
        last_expr_precedence = 2;
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        std::string target;
        if (ASR::is_a<ASR::Var_t>(*x.m_target)) {
            target = LFortran::ASRUtils::EXPR2VAR(x.m_target)->m_name;
        } else if (ASR::is_a<ASR::ArrayRef_t>(*x.m_target)) {
            visit_ArrayRef(*ASR::down_cast<ASR::ArrayRef_t>(x.m_target));
            target = src;
        } else {
            LFORTRAN_ASSERT(false)
        }
        this->visit_expr(*x.m_value);
        std::string value = src;
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + target + " = " + value + ";\n";
    }

    void visit_ConstantInteger(const ASR::ConstantInteger_t &x) {
        src = std::to_string(x.m_n);
        last_expr_precedence = 2;
    }

    void visit_ConstantReal(const ASR::ConstantReal_t &x) {
        src = std::to_string(x.m_r);
        last_expr_precedence = 2;
    }

    void visit_ConstantString(const ASR::ConstantString_t &x) {
        src = "\"" + std::string(x.m_s) + "\"";
        last_expr_precedence = 2;
    }

    void visit_ConstantComplex(const ASR::ConstantComplex_t &x) {
        src = "{" + std::to_string(x.m_re) + ", " + std::to_string(x.m_im) + "}";
        last_expr_precedence = 2;
    }

    void visit_ConstantLogical(const ASR::ConstantLogical_t &x) {
        if (x.m_value == true) {
            src = "true";
        } else {
            src = "false";
        }
        last_expr_precedence = 2;
    }

    void visit_ConstantSet(const ASR::ConstantSet_t &x) {
        std::string out = "{";
        for (size_t i=0; i<x.n_elements; i++) {
            visit_expr(*x.m_elements[i]);
            out += src;
            if (i != x.n_elements - 1)
                out += ", ";
        }
        out += "}";
        src = out;
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
                visit_expr(*x.m_args[i].m_right);
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

    void visit_ConstantDictionary(const ASR::ConstantDictionary_t &x) {
        LFORTRAN_ASSERT(x.n_keys == x.n_values);
        std::string out = "{";
        for(size_t i=0; i<x.n_keys; i++) {
            out += "{";
            visit_expr(*x.m_keys[i]);
            out += src + ", ";
            visit_expr(*x.m_values[i]);
            out += src + "}";
            if (i!=x.n_keys-1) out += ", ";
        }
        out += "}";
        src = out;
        last_expr_precedence = 2;
    }

    void visit_Cast(const ASR::Cast_t &x) {
        visit_expr(*x.m_arg);
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
            case (ASR::cast_kindType::ComplexToReal) : {
                src = "std::real(" + src + ")";
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
        this->visit_expr(*x.m_left);
        std::string left = std::move(src);
        int left_precedence = last_expr_precedence;
        this->visit_expr(*x.m_right);
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

    void visit_UnaryOp(const ASR::UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        int expr_precedence = last_expr_precedence;
        if (x.m_type->type == ASR::ttypeType::Integer) {
            if (x.m_op == ASR::unaryopType::UAdd) {
                // src = src;
                // Skip unary plus, keep the previous precedence
            } else if (x.m_op == ASR::unaryopType::USub) {
                last_expr_precedence = 3;
                if (expr_precedence <= last_expr_precedence) {
                    src = "-" + src;
                } else {
                    src = "-(" + src + ")";
                }
            } else if (x.m_op == ASR::unaryopType::Invert) {
                last_expr_precedence = 3;
                if (expr_precedence <= last_expr_precedence) {
                    src = "~" + src;
                } else {
                    src = "~(" + src + ")";
                }

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
            } else if (x.m_op == ASR::unaryopType::USub) {
                last_expr_precedence = 3;
                if (expr_precedence <= last_expr_precedence) {
                    src = "-" + src;
                } else {
                    src = "-(" + src + ")";
                }
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
        } else if (x.m_type->type == ASR::ttypeType::Logical) {
            if (x.m_op == ASR::unaryopType::Not) {
                last_expr_precedence = 3;
                if (expr_precedence <= last_expr_precedence) {
                    src = "!" + src;
                } else {
                    src = "!(" + src + ")";
                }
                return;
            } else {
                throw CodeGenError("Unary type not implemented yet for Logical");
            }
        } else {
            throw CodeGenError("UnaryOp: type not supported yet");
        }
    }

    void visit_BinOp(const ASR::BinOp_t &x) {
        this->visit_expr(*x.m_left);
        std::string left = std::move(src);
        int left_precedence = last_expr_precedence;
        this->visit_expr(*x.m_right);
        std::string right = std::move(src);
        int right_precedence = last_expr_precedence;
        switch (x.m_op) {
            case (ASR::binopType::Add) : { last_expr_precedence = 6; break; }
            case (ASR::binopType::Sub) : { last_expr_precedence = 6; break; }
            case (ASR::binopType::Mul) : { last_expr_precedence = 5; break; }
            case (ASR::binopType::Div) : { last_expr_precedence = 5; break; }
            case (ASR::binopType::Pow) : {
                src = "std::pow(" + left + ", " + right + ")";
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

    void visit_StrOp(const ASR::StrOp_t &x) {
        this->visit_expr(*x.m_left);
        std::string left = std::move(src);
        int left_precedence = last_expr_precedence;
        this->visit_expr(*x.m_right);
        std::string right = std::move(src);
        int right_precedence = last_expr_precedence;
        last_expr_precedence = 6;
        if (left_precedence <= last_expr_precedence) {
            src += "std::string(" + left + ")";
        } else {
            src += left;
        }
        src += " + "; // handle only concatenation for now
        if (right_precedence <= last_expr_precedence) {
            src += "std::string(" + right + ")";
        } else {
            src += right;
        }
    }

    void visit_BoolOp(const ASR::BoolOp_t &x) {
        this->visit_expr(*x.m_left);
        std::string left = std::move(src);
        int left_precedence = last_expr_precedence;
        this->visit_expr(*x.m_right);
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

    void visit_ConstantArray(const ASR::ConstantArray_t &x) {
        std::string out = "from_std_vector<float>({";
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            out += src;
            if (i < x.n_args-1) out += ", ";
        }
        out += "})";
        src = out;
        last_expr_precedence = 2;
    }

    void visit_Print(const ASR::Print_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "std::cout ";
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
            out += "<< " + src + " ";
        }
        out += "<< std::endl;\n";
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
            visit_expr(*x.m_msg);
            out += src + ", ";
            visit_expr(*x.m_test);
            out += src + "));\n";
        } else {
            out += "assert (";
            visit_expr(*x.m_test);
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

    void visit_Write(const ASR::Write_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "std::cout ";
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
            out += "<< " + src + " ";
        }
        out += "<< std::endl;\n";
        src = out;
    }

    void visit_Read(const ASR::Read_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "// FIXME: READ: std::cout ";
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
            out += "<< " + src + " ";
        }
        out += "<< std::endl;\n";
        src = out;
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "while (";
        visit_expr(*x.m_test);
        out += src + ") {\n";
        indentation_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
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

    void visit_Stop(const ASR::Stop_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "exit(0);\n";
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
            if (c->type == ASR::exprType::ConstantInteger) {
                increment = ASR::down_cast<ASR::ConstantInteger_t>(c)->m_n;
            } else if (c->type == ASR::exprType::UnaryOp) {
                ASR::UnaryOp_t *u = ASR::down_cast<ASR::UnaryOp_t>(c);
                LFORTRAN_ASSERT(u->m_op == ASR::unaryopType::USub);
                LFORTRAN_ASSERT(u->m_operand->type == ASR::exprType::ConstantInteger);
                increment = - ASR::down_cast<ASR::ConstantInteger_t>(u->m_operand)->m_n;
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
        visit_expr(*a);
        out += src + "; " + lvname + cmp_op;
        visit_expr(*b);
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
            this->visit_stmt(*x.m_body[i]);
            out += src;
        }
        out += indent + "}\n";
        indentation_level -= 1;
        src = out;
    }

    void visit_DoConcurrentLoop(const ASR::DoConcurrentLoop_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "Kokkos::parallel_for(";
        out += "Kokkos::RangePolicy<Kokkos::DefaultExecutionSpace>(";
        visit_expr(*x.m_head.m_start);
        out += src + ", ";
        visit_expr(*x.m_head.m_end);
        out += src + "+1)";
        ASR::Variable_t *loop_var = LFortran::ASRUtils::EXPR2VAR(x.m_head.m_v);
        sym_info[get_hash((ASR::asr_t*) loop_var)].needs_declaration = false;
        out += ", KOKKOS_LAMBDA(const long " + std::string(loop_var->m_name)
                + ") {\n";
        indentation_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            out += src;
        }
        out += indent + "});\n";
        indentation_level -= 1;
        src = out;
    }

    void visit_ErrorStop(const ASR::ErrorStop_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "std::cerr << \"ERROR STOP\" << std::endl;\n";
        src += indent + "exit(1);\n";
    }

    void visit_If(const ASR::If_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "if (";
        visit_expr(*x.m_test);
        out += src + ") {\n";
        indentation_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            out += src;
        }
        out += indent + "}";
        if (x.n_orelse == 0) {
            out += "\n";
        } else {
            out += " else {\n";
            for (size_t i=0; i<x.n_orelse; i++) {
                this->visit_stmt(*x.m_orelse[i]);
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
        visit_expr(*x.m_test);
        out += src + ") ? (";
        visit_expr(*x.m_body);
        out += src + ") : (";
        visit_expr(*x.m_orelse);
        out += src + ")";
        src = out;
        last_expr_precedence = 16;
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        ASR::Subroutine_t *s = ASR::down_cast<ASR::Subroutine_t>(
            LFortran::ASRUtils::symbol_get_past_external(x.m_name));
        std::string out = indent + s->m_name + "(";
        for (size_t i=0; i<x.n_args; i++) {
            if (ASR::is_a<ASR::Var_t>(*x.m_args[i].m_value)) {
                ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(x.m_args[i].m_value);
                std::string arg_name = arg->m_name;
                out += arg_name;
            } else {
                this->visit_expr(*x.m_args[i].m_value);
                out += src;
            }
            if (i < x.n_args-1) out += ", ";
        }
        out += ");\n";
        src = out;
    }

};

Result<std::string> asr_to_cpp(Allocator &al, ASR::TranslationUnit_t &asr,
    diag::Diagnostics &diagnostics)
{
    pass_unused_functions(al, asr);
    ASRToCPPVisitor v(diagnostics);
    try {
        v.visit_asr((ASR::asr_t &)asr);
    } catch (const CodeGenError &e) {
        diagnostics.diagnostics.push_back(e.d);
        return Error();
    } catch (const Abort &) {
        return Error();
    }
    return v.src;
}

} // namespace LFortran
