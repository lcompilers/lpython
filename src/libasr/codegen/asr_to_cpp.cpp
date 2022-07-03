#include <iostream>
#include <memory>

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/codegen/asr_to_cpp.h>
#include <libasr/codegen/asr_to_c_cpp.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/string_utils.h>
#include <libasr/pass/unused_functions.h>


namespace LFortran {

std::string convert_dims(size_t n_dims, ASR::dimension_t *m_dims)
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

class ASRToCPPVisitor : public BaseCCPPVisitor<ASRToCPPVisitor>
{
public:
    ASRToCPPVisitor(diag::Diagnostics &diag, Platform &platform)
        : BaseCCPPVisitor(diag, platform, true, true, false) {}

    std::string convert_variable_decl(const ASR::Variable_t &v, bool use_static=true)
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
            std::string dims;
            if (ASRUtils::is_integer(*v.m_type)) {
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(v.m_type);
                dims = convert_dims(t->n_dims, t->m_dims);
                std::string type_name = "int";
                if (t->m_kind == 8) type_name = "long long";
                sub = format_type(dims, type_name, v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_real(*v.m_type)) {
                ASR::Real_t *t = ASR::down_cast<ASR::Real_t>(v.m_type);
                dims = convert_dims(t->n_dims, t->m_dims);
                std::string type_name = "float";
                if (t->m_kind == 8) type_name = "double";
                sub = format_type(dims, type_name, v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_complex(*v.m_type)) {
                ASR::Complex_t *t = ASR::down_cast<ASR::Complex_t>(v.m_type);
                dims = convert_dims(t->n_dims, t->m_dims);
                std::string type_name = "std::complex<float>";
                if (t->m_kind == 8) type_name = "std::complex<double>";
                sub = format_type(dims, type_name, v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_logical(*v.m_type)) {
                ASR::Logical_t *t = ASR::down_cast<ASR::Logical_t>(v.m_type);
                dims = convert_dims(t->n_dims, t->m_dims);
                sub = format_type(dims, "bool", v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_character(*v.m_type)) {
                ASR::Character_t *t = ASR::down_cast<ASR::Character_t>(v.m_type);
                dims = convert_dims(t->n_dims, t->m_dims);
                sub = format_type(dims, "std::string", v.m_name, use_ref, dummy);
            } else if (ASR::is_a<ASR::Derived_t>(*v.m_type)) {
                ASR::Derived_t *t = ASR::down_cast<ASR::Derived_t>(v.m_type);
                dims = convert_dims(t->n_dims, t->m_dims);
                sub = format_type(dims, "struct", v.m_name, use_ref, dummy);
            } else {
                diag.codegen_error_label("Type number '"
                    + std::to_string(v.m_type->type)
                    + "' not supported", {v.base.base.loc}, "");
                throw Abort();
            }
            if (dims.size() == 0 && v.m_storage == ASR::storage_typeType::Save && use_static) {
                sub = "static " + sub;
            }
            if (dims.size() == 0 && v.m_symbolic_value) {
                this->visit_expr(*v.m_symbolic_value);
                std::string init = src;
                sub += "=" + init;
            }
        }
        return sub;
    }


    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        global_scope = x.m_global_scope;
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
#include <complex>
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


        // Pre-declare all functions first, then generate code
        // Otherwise some function might not be found.
        unit_src += "// Forward declarations\n";
        unit_src += declare_all_functions(*x.m_global_scope);
        // Now pre-declare all functions from modules and programs
        for (auto &item : x.m_global_scope->get_scope()) {
            if (ASR::is_a<ASR::Module_t>(*item.second)) {
                ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(item.second);
                unit_src += declare_all_functions(*m->m_symtab);
            } else if (ASR::is_a<ASR::Program_t>(*item.second)) {
                ASR::Program_t *p = ASR::down_cast<ASR::Program_t>(item.second);
                unit_src += "namespace {\n"
                            + declare_all_functions(*p->m_symtab)
                            + "}\n";
            }
        }
        unit_src += "\n";
        unit_src += "// Implementations\n";

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
        std::string decl;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
                decl += indent1;
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

    void visit_ComplexConstructor(const ASR::ComplexConstructor_t &x) {
        this->visit_expr(*x.m_re);
        std::string re = src;
        this->visit_expr(*x.m_im);
        std::string im = src;
        src = "std::complex<float>(" + re + ", " + im + ")";
        if (ASRUtils::extract_kind_from_ttype_t(x.m_type) == 8) {
            src = "std::complex<double>(" + re + ", " + im + ")";
        }
        last_expr_precedence = 2;
    }

    void visit_ComplexConstant(const ASR::ComplexConstant_t &x) {
        std::string re = std::to_string(x.m_re);
        std::string im = std::to_string(x.m_im);
        src = "std::complex<float>(" + re + ", " + im + ")";
        if (ASRUtils::extract_kind_from_ttype_t(x.m_type) == 8) {
            src = "std::complex<double>(" + re + ", " + im + ")";
        }
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

    void visit_SetConstant(const ASR::SetConstant_t &x) {
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

    void visit_DictConstant(const ASR::DictConstant_t &x) {
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

    void visit_StringConcat(const ASR::StringConcat_t &x) {
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

    void visit_ArrayConstant(const ASR::ArrayConstant_t &x) {
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
        std::string out = indent + "std::cout ", sep;
        if (x.m_separator) {
            this->visit_expr(*x.m_separator);
            sep = src;
        } else {
            sep = "\" \"";
        }
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
            out += "<< " + src + " ";
            if (i+1 != x.n_values) {
                out += "<< " + sep + " ";
            }
        }
        out += "<< std::endl;\n";
        src = out;
    }

    void visit_FileWrite(const ASR::FileWrite_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "std::cout ";
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
            out += "<< " + src + " ";
        }
        out += "<< std::endl;\n";
        src = out;
    }

    void visit_FileRead(const ASR::FileRead_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "// FIXME: READ: std::cout ";
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
            out += "<< " + src + " ";
        }
        out += "<< std::endl;\n";
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

};

Result<std::string> asr_to_cpp(Allocator &al, ASR::TranslationUnit_t &asr,
    diag::Diagnostics &diagnostics, Platform &platform)
{
    pass_unused_functions(al, asr, true);
    ASRToCPPVisitor v(diagnostics, platform);
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
