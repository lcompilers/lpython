#include <iostream>
#include <memory>

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/codegen/asr_to_c.h>
#include <libasr/codegen/asr_to_c_cpp.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/string_utils.h>
#include <libasr/pass/unused_functions.h>


namespace LFortran {

std::string convert_dims_c(size_t n_dims, ASR::dimension_t *m_dims)
{
    std::string dims;
    for (size_t i=0; i<n_dims; i++) {
        ASR::expr_t *start = m_dims[i].m_start;
        ASR::expr_t *end = m_dims[i].m_end;
        if (!start && !end) {
            dims += "*";
        } else if (start && end) {
            if (ASR::is_a<ASR::IntegerConstant_t>(*start) && ASR::is_a<ASR::IntegerConstant_t>(*end)) {
                ASR::IntegerConstant_t *s = ASR::down_cast<ASR::IntegerConstant_t>(start);
                ASR::IntegerConstant_t *e = ASR::down_cast<ASR::IntegerConstant_t>(end);
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

std::string format_type_c(const std::string &dims, const std::string &type,
        const std::string &name, bool use_ref, bool /*dummy*/)
{
    std::string fmt;
    if (dims.size() == 0) {
        std::string ref;
        if (use_ref) ref = "&";
        fmt = type + " " + ref + name;
    } else {
        throw CodeGenError("Dimensions is not supported yet.");
    }
    return fmt;
}

class ASRToCVisitor : public BaseCCPPVisitor<ASRToCVisitor>
{
public:

    ASRToCVisitor(diag::Diagnostics &diag) : BaseCCPPVisitor(diag) {}

    std::string convert_variable_decl(const ASR::Variable_t &v)
    {
        std::string sub;
        bool use_ref = (v.m_intent == LFortran::ASRUtils::intent_out || v.m_intent == LFortran::ASRUtils::intent_inout);
        bool dummy = LFortran::ASRUtils::is_arg_dummy(v.m_intent);
        if (ASRUtils::is_pointer(v.m_type)) {
            ASR::ttype_t *t2 = ASR::down_cast<ASR::Pointer_t>(v.m_type)->m_type;
            if (ASRUtils::is_integer(*t2)) {
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(t2);
                std::string dims = convert_dims_c(t->n_dims, t->m_dims);
                sub = format_type_c(dims, "int *", v.m_name, use_ref, dummy);
            } else {
                diag.codegen_error_label("Type number '"
                    + std::to_string(v.m_type->type)
                    + "' not supported", {v.base.base.loc}, "");
                throw Abort();
            }
        } else {
            if (ASRUtils::is_integer(*v.m_type)) {
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(v.m_type);
                std::string dims = convert_dims_c(t->n_dims, t->m_dims);
                std::string type_name = "int";
                if (t->m_kind == 8) type_name = "long long";
                sub = format_type_c(dims, type_name, v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_real(*v.m_type)) {
                ASR::Real_t *t = ASR::down_cast<ASR::Real_t>(v.m_type);
                std::string dims = convert_dims_c(t->n_dims, t->m_dims);
                std::string type_name = "float";
                if (t->m_kind == 8) type_name = "double";
                sub = format_type_c(dims, type_name, v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_complex(*v.m_type)) {
                ASR::Complex_t *t = ASR::down_cast<ASR::Complex_t>(v.m_type);
                std::string dims = convert_dims_c(t->n_dims, t->m_dims);
                std::string type_name = "float complex";
                if (t->m_kind == 8) type_name = "double complex";
                sub = format_type_c(dims, type_name, v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_logical(*v.m_type)) {
                ASR::Logical_t *t = ASR::down_cast<ASR::Logical_t>(v.m_type);
                std::string dims = convert_dims_c(t->n_dims, t->m_dims);
                sub = format_type_c(dims, "bool", v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_character(*v.m_type)) {
                // TODO
            } else if (ASR::is_a<ASR::Derived_t>(*v.m_type)) {
                ASR::Derived_t *t = ASR::down_cast<ASR::Derived_t>(v.m_type);
                std::string dims = convert_dims_c(t->n_dims, t->m_dims);
                sub = format_type_c(dims, "struct", v.m_name, use_ref, dummy);
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
                = LFortran::ASRUtils::determine_module_dependencies(x);
            for (auto &item : build_order) {
                LFORTRAN_ASSERT(x.m_global_scope->get_scope().find(item)
                    != x.m_global_scope->get_scope().end());
                if (startswith(item, "lfortran_intrinsic")) {
                    ASR::symbol_t *mod = x.m_global_scope->get_symbol(item);
                    visit_symbol(*mod);
                    unit_src += src;
                }
            }
        }

        // Process procedures first:
        for (auto &item : x.m_global_scope->get_scope()) {
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
            LFORTRAN_ASSERT(x.m_global_scope->get_scope().find(item)
                != x.m_global_scope->get_scope().end());
            if (!startswith(item, "lfortran_intrinsic")) {
                ASR::symbol_t *mod = x.m_global_scope->get_symbol(item);
                visit_symbol(*mod);
                unit_src += src;
            }
        }

        // Then the main program:
        for (auto &item : x.m_global_scope->get_scope()) {
            if (ASR::is_a<ASR::Program_t>(*item.second)) {
                visit_symbol(*item.second);
                unit_src += src;
            }
        }

        src = unit_src;
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
                decl += convert_variable_decl(*v) + ";\n";
            }
        }

        std::string body;
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            body += src;
        }

        src = contains
                + "int main(int argc, char* argv[])\n{\n"
                + decl + body
                + indent1 + "return 0;\n}\n";
        indentation_level -= 2;
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
            bool is_float = ASR::down_cast<ASR::Real_t>(return_var->m_type)->m_kind == 4;
            if (is_float) {
                sub = "float ";
            } else {
                sub = "double ";
            }
        } else if (ASRUtils::is_logical(*return_var->m_type)) {
            sub = "bool ";
        } else if (ASRUtils::is_character(*return_var->m_type)) {
            sub = "char * ";
        } else if (ASRUtils::is_complex(*return_var->m_type)) {
            bool is_float = ASR::down_cast<ASR::Complex_t>(return_var->m_type)->m_kind == 4;
            if (is_float) {
                sub = "float complex ";
            } else {
                sub = "double complex ";
            }
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
        sub += ")";
        if (x.m_abi == ASR::abiType::BindC) {
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
            indentation_level -= 1;
        }
        sub += "\n";
        src = sub;
    }

    void visit_LogicalConstant(const ASR::LogicalConstant_t &x) {
        if (x.m_value == true) {
            src = "true";
        } else {
            src = "false";
        }
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

    std::string get_print_type(ASR::ttype_t *t) {
        switch (t->type) {
            case ASR::ttypeType::Integer: {
                ASR::Integer_t *i = (ASR::Integer_t*)t;
                switch (i->m_kind) {
                    case 1: { return "%d"; }
                    case 2: { return "%d"; }
                    case 4: { return "%d"; }
                    case 8: { return "%lli"; }
                    default: { throw LFortranException("Integer kind not supported"); }
                }
            }
            case ASR::ttypeType::Real: {
                ASR::Real_t *r = (ASR::Real_t*)t;
                switch (r->m_kind) {
                    case 4: { return "%f"; }
                    case 8: { return "%lf"; }
                    default: { throw LFortranException("Float kind not supported"); }
                }
            }
            case ASR::ttypeType::Logical: {
                return "%d";
            }
            case ASR::ttypeType::Character: {
                return "%s";
            }
            default : throw LFortranException("Not implemented");
        }
    }

    void visit_Print(const ASR::Print_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "printf(\"";
        std::vector<std::string> v;
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
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

    void visit_ErrorStop(const ASR::ErrorStop_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "fprintf(stderr, \"ERROR STOP\");\n";
        src += indent + "exit(1);\n";
    }

};

Result<std::string> asr_to_c(Allocator &al, ASR::TranslationUnit_t &asr,
    diag::Diagnostics &diagnostics)
{
    pass_unused_functions(al, asr);
    ASRToCVisitor v(diagnostics);
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
