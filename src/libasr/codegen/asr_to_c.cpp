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
#include <libasr/pass/class_constructor.h>


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
    // Sync: Not sure of the following code
    // std::string ref = "", ptr = "";
    // if (dims.size() > 0) ptr = "*";
    // if (use_ref) ref = "&";
    // fmt = type + " " + ptr + ref + name;
    std::string ref = "";
    if (use_ref) ref = "&";
    if( dims == "*" ) {
        fmt = type + " " + dims + ref + name;
    } else {
        fmt = type + " " + ref + name + dims;
    }
    return fmt;
}

class ASRToCVisitor : public BaseCCPPVisitor<ASRToCVisitor>
{
public:

    ASRToCVisitor(diag::Diagnostics &diag, Platform &platform)
         : BaseCCPPVisitor(diag, platform, false, false, true) {}

    std::string convert_dims_c(size_t n_dims, ASR::dimension_t *m_dims)
    {
        std::string dims;
        for (size_t i=0; i<n_dims; i++) {
            ASR::expr_t *start = m_dims[i].m_start;
            ASR::expr_t *end = m_dims[i].m_end;
            if (!start && !end) {
                dims += "*";
            } else if (start && end) {
                ASR::expr_t* start_value = ASRUtils::expr_value(start);
                ASR::expr_t* end_value = ASRUtils::expr_value(end);
                if( start_value && end_value ) {
                    int64_t start_int = -1, end_int = -1;
                    ASRUtils::extract_value(start_value, start_int);
                    ASRUtils::extract_value(end_value, end_int);
                    dims += "[" + std::to_string(end_int - start_int + 1) + "]";
                } else {
                    this->visit_expr(*start);
                    std::string start_expr = std::move(src);
                    this->visit_expr(*end);
                    std::string end_expr = std::move(src);
                    dims += "[" + end_expr + " - " + start_expr + " + 1]";
                }
            } else {
                throw CodeGenError("Dimension type not supported");
            }
        }
        return dims;
    }

    std::string convert_variable_decl(const ASR::Variable_t &v,
                                      bool pre_initialise_derived_type=true, bool use_static=true)
    {
        std::string sub;
        bool use_ref = (v.m_intent == LFortran::ASRUtils::intent_out || v.m_intent == LFortran::ASRUtils::intent_inout);
        bool dummy = LFortran::ASRUtils::is_arg_dummy(v.m_intent);
        if (ASRUtils::is_pointer(v.m_type)) {
            ASR::ttype_t *t2 = ASR::down_cast<ASR::Pointer_t>(v.m_type)->m_type;
            if (ASRUtils::is_integer(*t2)) {
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(t2);
                std::string dims = convert_dims_c(t->n_dims, t->m_dims);
                std::string type_name = "int" + std::to_string(t->m_kind * 8) + "_t";
                if( !ASRUtils::is_array(v.m_type) ) {
                    type_name.append(" *");
                }
                sub = format_type_c(dims, type_name, v.m_name, use_ref, dummy);
            } else if(ASR::is_a<ASR::Derived_t>(*t2)) {
                ASR::Derived_t *t = ASR::down_cast<ASR::Derived_t>(t2);
                std::string der_type_name = ASRUtils::symbol_name(t->m_derived_type);
                std::string dims = convert_dims_c(t->n_dims, t->m_dims);
                sub = format_type_c(dims, "struct " + der_type_name + "*",
                                    v.m_name, use_ref, dummy);
            } else {
                diag.codegen_error_label("Type number '"
                    + std::to_string(v.m_type->type)
                    + "' not supported", {v.base.base.loc}, "");
                throw Abort();
            }
        } else {
            std::string dims;
            if (ASRUtils::is_integer(*v.m_type)) {
                headers.insert("inttypes");
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(v.m_type);
                dims = convert_dims_c(t->n_dims, t->m_dims);
                std::string type_name;
                if (t->m_kind == 1) {
                    type_name = "int8_t";
                } else if (t->m_kind == 2) {
                    type_name = "int16_t";
                } else if (t->m_kind == 4) {
                    type_name = "int32_t";
                } else if (t->m_kind == 8) {
                    type_name = "int64_t";
                }
                sub = format_type_c(dims, type_name, v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_real(*v.m_type)) {
                ASR::Real_t *t = ASR::down_cast<ASR::Real_t>(v.m_type);
                dims = convert_dims_c(t->n_dims, t->m_dims);
                std::string type_name = "float";
                if (t->m_kind == 8) type_name = "double";
                sub = format_type_c(dims, type_name, v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_complex(*v.m_type)) {
                headers.insert("complex");
                ASR::Complex_t *t = ASR::down_cast<ASR::Complex_t>(v.m_type);
                dims = convert_dims_c(t->n_dims, t->m_dims);
                std::string type_name = "float complex";
                if (t->m_kind == 8) type_name = "double complex";
                sub = format_type_c(dims, type_name, v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_logical(*v.m_type)) {
                ASR::Logical_t *t = ASR::down_cast<ASR::Logical_t>(v.m_type);
                dims = convert_dims_c(t->n_dims, t->m_dims);
                sub = format_type_c(dims, "bool", v.m_name, use_ref, dummy);
            }else if (ASRUtils::is_character(*v.m_type)) {
                ASR::Character_t *t = ASR::down_cast<ASR::Character_t>(v.m_type);
                std::string dims = convert_dims_c(t->n_dims, t->m_dims);
                sub = format_type_c(dims, "char *", v.m_name, use_ref, dummy);
            } else if (ASR::is_a<ASR::Derived_t>(*v.m_type)) {
                std::string indent(indentation_level*indentation_spaces, ' ');
                ASR::Derived_t *t = ASR::down_cast<ASR::Derived_t>(v.m_type);
                std::string der_type_name = ASRUtils::symbol_name(t->m_derived_type);
                std::string dims = convert_dims_c(t->n_dims, t->m_dims);
                if( v.m_intent == ASRUtils::intent_local && pre_initialise_derived_type ) {
                    std::string value_var_name = v.m_parent_symtab->get_unique_name(std::string(v.m_name) + "_value");
                    sub = format_type_c(dims, "struct " + der_type_name,
                                        value_var_name, use_ref, dummy);
                    if (v.m_symbolic_value) {
                        this->visit_expr(*v.m_symbolic_value);
                        std::string init = src;
                        sub += "=" + init;
                    }
                    sub += ";\n";
                    sub += indent + format_type_c("", "struct " + der_type_name + "*", v.m_name, use_ref, dummy);
                    sub += "= &" + value_var_name;
                    return sub;
                } else {
                    sub = format_type_c(dims, "struct " + der_type_name + "*",
                                        v.m_name, use_ref, dummy);
                }
            } else if (ASR::is_a<ASR::CPtr_t>(*v.m_type)) {
                sub = format_type_c("", "void*", v.m_name, false, false);
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

        std::string head =
R"(
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <lfortran_intrinsics.h>

#define ASSERT(cond)                                                           \
    {                                                                          \
        if (!(cond)) {                                                         \
            printf("%s%s", "ASSERT failed: ", __FILE__);                       \
            printf("%s%s", "\nfunction ", __func__);                           \
            printf("%s%d%s", "(), line number ", __LINE__, " at \n");          \
            printf("%s%s", #cond, "\n");                                       \
            exit(1);                                                           \
        }                                                                      \
    }
#define ASSERT_MSG(cond, msg)                                                  \
    {                                                                          \
        if (!(cond)) {                                                         \
            printf("%s%s", "ASSERT failed: ", __FILE__);                       \
            printf("%s%s", "\nfunction ", __func__);                           \
            printf("%s%d%s", "(), line number ", __LINE__, " at \n");          \
            printf("%s%s", #cond, "\n");                                       \
            printf("%s", "ERROR MESSAGE:\n");                                  \
            printf("%s%s", msg, "\n");                                         \
            exit(1);                                                           \
        }                                                                      \
    }

)";
        unit_src += head;

        for (auto &item : x.m_global_scope->get_scope()) {
            if (ASR::is_a<ASR::DerivedType_t>(*item.second)) {
                unit_src += "struct " + item.first + ";\n\n";
            }
        }

        for (auto &item : x.m_global_scope->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
                unit_src += convert_variable_decl(*v) + ";\n";
            }
        }

        for (auto &item : x.m_global_scope->get_scope()) {
            if (ASR::is_a<ASR::DerivedType_t>(*item.second)) {
                visit_symbol(*item.second);
                unit_src += src;
            }
        }

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
                unit_src += declare_all_functions(*p->m_symtab);
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
                    if( ASRUtils::get_body_size(mod) != 0 ) {
                        visit_symbol(*mod);
                        unit_src += src;
                    }
                }
            }
        }

        // Process procedures first:
        for (auto &item : x.m_global_scope->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)
                || ASR::is_a<ASR::Subroutine_t>(*item.second)) {
                if( ASRUtils::get_body_size(item.second) != 0 ) {
                    visit_symbol(*item.second);
                    unit_src += src;
                }
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
        std::string to_include = "";
        for (auto s: headers) {
            to_include += "#include <" + s + ".h>\n";
        }
        src = to_include + unit_src;
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
        src = contains
                + "int main(int argc, char* argv[])\n{\n"
                + decl + body
                + indent1 + "return 0;\n}\n";
        indentation_level -= 2;
    }

    void visit_DerivedType(const ASR::DerivedType_t& x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        indentation_level += 1;
        std::string open_struct = indent + "struct " + std::string(x.m_name) + " {\n";
        std::string body = "";
        indent.push_back(' ');
        for( size_t i = 0; i < x.n_members; i++ ) {
            ASR::symbol_t* member = x.m_symtab->get_symbol(x.m_members[i]);
            LFORTRAN_ASSERT(ASR::is_a<ASR::Variable_t>(*member));
            body += indent + convert_variable_decl(*ASR::down_cast<ASR::Variable_t>(member), false) + ";\n";
        }
        indentation_level -= 1;
        std::string end_struct = "};\n\n";
        src = open_struct + body + end_struct;
    }

    void visit_LogicalConstant(const ASR::LogicalConstant_t &x) {
        if (x.m_value == true) {
            src = "true";
        } else {
            src = "false";
        }
        last_expr_precedence = 2;
    }

    void visit_Assert(const ASR::Assert_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent;
        if (x.m_msg) {
            out += "ASSERT_MSG(";
            visit_expr(*x.m_test);
            out += src + ", ";
            visit_expr(*x.m_msg);
            out += src + ");\n";
        } else {
            out += "ASSERT(";
            visit_expr(*x.m_test);
            out += src + ");\n";
        }
        src = out;
    }

    std::string get_print_type(ASR::ttype_t *t, bool deref_ptr) {
        switch (t->type) {
            case ASR::ttypeType::Integer: {
                ASR::Integer_t *i = (ASR::Integer_t*)t;
                switch (i->m_kind) {
                    case 1: { return "%d"; }
                    case 2: { return "%d"; }
                    case 4: { return "%d"; }
                    case 8: {
                        if (platform == Platform::Linux) {
                            return "%li";
                        } else {
                            return "%lli";
                        }
                    }
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
            case ASR::ttypeType::CPtr: {
                return "%p";
            }
            case ASR::ttypeType::Pointer: {
                if( !deref_ptr ) {
                    return "%p";
                } else {
                    ASR::Pointer_t* type_ptr = ASR::down_cast<ASR::Pointer_t>(t);
                    return get_print_type(type_ptr->m_type, false);
                }
            }
            default : throw LFortranException("Not implemented");
        }
    }

    void visit_Print(const ASR::Print_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "printf(\"";
        std::vector<std::string> v;
        std::string separator;
        if (x.m_separator) {
            this->visit_expr(*x.m_separator);
            separator = src;
        } else {
            separator = "\" \"";
        }
        for (size_t i=0; i<x.n_values; i++) {
            this->visit_expr(*x.m_values[i]);
            ASR::ttype_t* value_type = ASRUtils::expr_type(x.m_values[i]);
            out += get_print_type(value_type, ASR::is_a<ASR::ArrayRef_t>(*x.m_values[i]));
            v.push_back(src);
            if (i+1!=x.n_values) {
                out += "\%s";
                v.push_back(separator);
            }
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

};

Result<std::string> asr_to_c(Allocator &al, ASR::TranslationUnit_t &asr,
    diag::Diagnostics &diagnostics, Platform &platform)
{
    pass_unused_functions(al, asr, true);
    pass_replace_class_constructor(al, asr);
    ASRToCVisitor v(diagnostics, platform);
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
