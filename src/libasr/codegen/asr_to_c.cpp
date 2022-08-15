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

#include <map>
#include <utility>


namespace LFortran {

std::string format_type_c(const std::string &dims, const std::string &type,
        const std::string &name, bool use_ref, bool /*dummy*/)
{
    std::string fmt;
    std::string ref = "";
    if (use_ref) ref = "*";
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

    std::string array_types_decls;
    std::map<std::string, std::map<size_t, std::string>> eltypedims2arraytype;

    ASRToCVisitor(diag::Diagnostics &diag, Platform &platform,
                  int64_t default_lower_bound)
         : BaseCCPPVisitor(diag, platform, false, false, true, default_lower_bound),
           array_types_decls(std::string("\nstruct dimension_descriptor\n"
                                         "{\n    int32_t lower_bound, length;\n};\n"))  {
    }

    std::string convert_dims_c(size_t n_dims, ASR::dimension_t *m_dims,
                               bool convert_to_1d=false)
    {
        std::string dims;
        size_t size = 1;
        for (size_t i=0; i<n_dims; i++) {
            ASR::expr_t *length = m_dims[i].m_length;
            if (!length) {
                dims += "*";
            } else {
                ASR::expr_t* length_value = ASRUtils::expr_value(length);
                if( length_value ) {
                    int64_t length_int = -1;
                    ASRUtils::extract_value(length_value, length_int);
                    size *= length_int;
                    dims += "[" + std::to_string(length_int) + "]";
                } else {
                    size = 0;
                    dims += "[ /* FIXME symbolic dimensions */ ]";
                }
            }
        }
        if( convert_to_1d && size != 0 ) {
            dims = "[" + std::to_string(size) + "]";
        }
        return dims;
    }

    std::string get_array_type(std::string type_name, std::string encoded_type_name,
                               size_t n_dims, bool make_ptr=true) {
        if( eltypedims2arraytype.find(encoded_type_name) != eltypedims2arraytype.end() &&
            eltypedims2arraytype[encoded_type_name].find(n_dims) !=
            eltypedims2arraytype[encoded_type_name].end()) {
            if( make_ptr ) {
                return eltypedims2arraytype[encoded_type_name][n_dims] + "*";
            } else {
                return eltypedims2arraytype[encoded_type_name][n_dims];
            }
        }

        std::string struct_name;
        std::string new_array_type;
        struct_name = "struct " + encoded_type_name + "_" + std::to_string(n_dims);
        std::string array_data = format_type_c("*", type_name, "data", false, false);
        new_array_type = struct_name + "\n{\n    " + array_data +
                            ";\n    struct dimension_descriptor dims[" +
                            std::to_string(n_dims) + "];\n    bool is_allocated;\n};\n";
        type_name = struct_name + "*";
        eltypedims2arraytype[encoded_type_name][n_dims] = struct_name;
        array_types_decls += "\n" + new_array_type + "\n";
        return type_name;
    }

    void generate_array_decl(std::string& sub, std::string v_m_name,
                             std::string& type_name, std::string& dims,
                             std::string& encoded_type_name,
                             ASR::dimension_t* m_dims, int n_dims,
                             bool use_ref, bool dummy,
                             bool declare_value, bool is_pointer=false) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string type_name_copy = type_name;
        type_name = get_array_type(type_name, encoded_type_name, n_dims);
        std::string type_name_without_ptr = get_array_type(type_name, encoded_type_name, n_dims, false);
        if( declare_value ) {
            std::string variable_name = std::string(v_m_name) + "_value";
            sub = format_type_c("", type_name_without_ptr, variable_name, use_ref, dummy) + ";\n";
            sub += indent + format_type_c("", type_name, v_m_name, use_ref, dummy);
            sub += " = &" + variable_name + ";\n";
            if( !is_pointer ) {
                sub += indent + format_type_c(dims, type_name_copy, std::string(v_m_name) + "_data",
                                              use_ref, dummy) + ";\n";
                sub += indent + std::string(v_m_name) + "->data = " + std::string(v_m_name) + "_data;\n";
                for (int i = 0; i < n_dims; i++) {
                    if( m_dims[i].m_start ) {
                        this->visit_expr(*m_dims[i].m_start);
                        sub += indent + std::string(v_m_name) +
                            "->dims[" + std::to_string(i) + "].lower_bound = " + src + ";\n";
                    } else {
                        sub += indent + std::string(v_m_name) +
                            "->dims[" + std::to_string(i) + "].lower_bound = 0" + ";\n";
                    }
                    if( m_dims[i].m_length ) {
                        this->visit_expr(*m_dims[i].m_length);
                        sub += indent + std::string(v_m_name) +
                            "->dims[" + std::to_string(i) + "].length = " + src + ";\n";
                    } else {
                        sub += indent + std::string(v_m_name) +
                            "->dims[" + std::to_string(i) + "].length = 0" + ";\n";
                    }
                }
                sub.pop_back();
                sub.pop_back();
            }
        } else {
            sub = format_type_c("", type_name, v_m_name, use_ref, dummy);
        }
    }

    std::string convert_variable_decl(const ASR::Variable_t &v,
                                      bool pre_initialise_derived_type=true,
                                      bool use_static=true)
    {
        std::string sub;
        bool use_ref = (v.m_intent == LFortran::ASRUtils::intent_out ||
                        v.m_intent == LFortran::ASRUtils::intent_inout);
        bool is_array = ASRUtils::is_array(v.m_type);
        bool dummy = LFortran::ASRUtils::is_arg_dummy(v.m_intent);
        if (ASRUtils::is_pointer(v.m_type)) {
            ASR::ttype_t *t2 = ASR::down_cast<ASR::Pointer_t>(v.m_type)->m_type;
            if (ASRUtils::is_integer(*t2)) {
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(t2);
                std::string type_name = "int" + std::to_string(t->m_kind * 8) + "_t";
                if( !ASRUtils::is_array(v.m_type) ) {
                    type_name.append(" *");
                }
                if( is_array ) {
                    std::string dims = convert_dims_c(t->n_dims, t->m_dims, true);
                    std::string encoded_type_name = "i" + std::to_string(t->m_kind * 8);
                    generate_array_decl(sub, std::string(v.m_name), type_name, dims,
                                        encoded_type_name, t->m_dims, t->n_dims,
                                        use_ref, dummy,
                                        v.m_intent != ASRUtils::intent_in &&
                                        v.m_intent != ASRUtils::intent_inout, true);
                } else {
                    std::string dims = convert_dims_c(t->n_dims, t->m_dims);
                    sub = format_type_c(dims, type_name, v.m_name, use_ref, dummy);
                }
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
            use_ref = use_ref && !is_array;
            if (ASRUtils::is_integer(*v.m_type)) {
                headers.insert("inttypes");
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(v.m_type);
                std::string type_name = "int" + std::to_string(t->m_kind * 8) + "_t";
                if( is_array ) {
                    dims = convert_dims_c(t->n_dims, t->m_dims, true);
                    std::string encoded_type_name = "i" + std::to_string(t->m_kind * 8);
                    generate_array_decl(sub, std::string(v.m_name), type_name, dims,
                                        encoded_type_name, t->m_dims, t->n_dims,
                                        use_ref, dummy,
                                        v.m_intent != ASRUtils::intent_in &&
                                        v.m_intent != ASRUtils::intent_inout);
                } else {
                    dims = convert_dims_c(t->n_dims, t->m_dims);
                    sub = format_type_c(dims, type_name, v.m_name, use_ref, dummy);
                }
            } else if (ASRUtils::is_real(*v.m_type)) {
                ASR::Real_t *t = ASR::down_cast<ASR::Real_t>(v.m_type);
                std::string type_name = "float";
                if (t->m_kind == 8) type_name = "double";
                if( is_array ) {
                    dims = convert_dims_c(t->n_dims, t->m_dims, true);
                    std::string encoded_type_name = "f" + std::to_string(t->m_kind * 8);
                    generate_array_decl(sub, std::string(v.m_name), type_name, dims,
                                        encoded_type_name, t->m_dims, t->n_dims,
                                        use_ref, dummy,
                                        v.m_intent != ASRUtils::intent_in &&
                                        v.m_intent != ASRUtils::intent_inout);
                } else {
                    dims = convert_dims_c(t->n_dims, t->m_dims);
                    sub = format_type_c(dims, type_name, v.m_name, use_ref, dummy);
                }
            } else if (ASRUtils::is_complex(*v.m_type)) {
                headers.insert("complex");
                ASR::Complex_t *t = ASR::down_cast<ASR::Complex_t>(v.m_type);
                std::string type_name = "float complex";
                if (t->m_kind == 8) type_name = "double complex";
                if( is_array ) {
                    dims = convert_dims_c(t->n_dims, t->m_dims, true);
                    std::string encoded_type_name = "c" + std::to_string(t->m_kind * 8);
                    generate_array_decl(sub, std::string(v.m_name), type_name, dims,
                                        encoded_type_name, t->m_dims, t->n_dims,
                                        use_ref, dummy,
                                        v.m_intent != ASRUtils::intent_in &&
                                        v.m_intent != ASRUtils::intent_inout);
                } else {
                    dims = convert_dims_c(t->n_dims, t->m_dims);
                    sub = format_type_c(dims, type_name, v.m_name, use_ref, dummy);
                }
            } else if (ASRUtils::is_logical(*v.m_type)) {
                ASR::Logical_t *t = ASR::down_cast<ASR::Logical_t>(v.m_type);
                dims = convert_dims_c(t->n_dims, t->m_dims);
                sub = format_type_c(dims, "bool", v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_character(*v.m_type)) {
                ASR::Character_t *t = ASR::down_cast<ASR::Character_t>(v.m_type);
                std::string dims = convert_dims_c(t->n_dims, t->m_dims);
                sub = format_type_c(dims, "char *", v.m_name, use_ref, dummy);
            } else if (ASR::is_a<ASR::Derived_t>(*v.m_type)) {
                std::string indent(indentation_level*indentation_spaces, ' ');
                ASR::Derived_t *t = ASR::down_cast<ASR::Derived_t>(v.m_type);
                std::string der_type_name = ASRUtils::symbol_name(t->m_derived_type);
                 if( is_array ) {
                    dims = convert_dims_c(t->n_dims, t->m_dims, true);
                    std::string encoded_type_name = "x" + der_type_name;
                    std::string type_name = std::string("struct ") + der_type_name;
                    generate_array_decl(sub, std::string(v.m_name), type_name, dims,
                                        encoded_type_name, t->m_dims, t->n_dims,
                                        use_ref, dummy,
                                        v.m_intent != ASRUtils::intent_in &&
                                        v.m_intent != ASRUtils::intent_inout);
                } else if( v.m_intent == ASRUtils::intent_local && pre_initialise_derived_type) {
                    dims = convert_dims_c(t->n_dims, t->m_dims);
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
                    if( t->n_dims != 0 ) {
                        sub += " = " + value_var_name;
                    } else {
                        sub += " = &" + value_var_name;
                    }
                    return sub;
                } else {
                    dims = convert_dims_c(t->n_dims, t->m_dims);
                    if( v.m_intent == ASRUtils::intent_in ||
                        v.m_intent == ASRUtils::intent_inout ) {
                        use_ref = false;
                        dims = "";
                    }
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
#include <string.h>
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

        for (auto &item : x.m_global_scope->get_scope()) {
            if (ASR::is_a<ASR::DerivedType_t>(*item.second)) {
                array_types_decls += "struct " + item.first + ";\n\n";
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
                array_types_decls += src;
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
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
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
        src = to_include + head + array_types_decls + unit_src;
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
        array_types_decls += open_struct + body + end_struct;
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
            default : throw LCompilersException("Not implemented");
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
            if( ASRUtils::is_array(ASRUtils::expr_type(x.m_values[i])) ) {
                src += "->data";
            }
            ASR::ttype_t* value_type = ASRUtils::expr_type(x.m_values[i]);
            out += get_print_type(value_type, ASR::is_a<ASR::ArrayItem_t>(*x.m_values[i]));
            v.push_back(src);
            if (i+1!=x.n_values) {
                out += "\%s";
                v.push_back(separator);
            }
        }
        if (x.m_end) {
            this->visit_expr(*x.m_end);
            out += "\%s\"";
            v.push_back(src);
        } else {
            out += "\\n\"";
        }
        if (!v.empty()) {
            for (auto s: v) {
                out += ", " + s;
            }
        }
        out += ");\n";
        src = out;
    }

    void visit_ArrayItem(const ASR::ArrayItem_t &x) {
        this->visit_expr(*x.m_v);
        std::string array = src;
        std::string out = array;
        ASR::dimension_t* m_dims;
        ASRUtils::extract_dimensions_from_ttype(ASRUtils::expr_type(x.m_v), m_dims);
        out += "->data[";
        std::string index = "";
        for (size_t i=0; i<x.n_args; i++) {
            std::string current_index = "";
            if (x.m_args[i].m_right) {
                this->visit_expr(*x.m_args[i].m_right);
            } else {
                src = "/* FIXME right index */";
            }

            current_index += "(" + src + " - " + array + "->dims["
                                + std::to_string(i) + "].lower_bound)";
            for( size_t j = i + 1; j < x.n_args; j++ ) {
                std::string length = array + "->dims[" + std::to_string(j) + "].length";
                current_index += " * " + length;
            }
            index += current_index;
            if (i < x.n_args - 1) {
                index += " + ";
            }
        }
        out += index + "]";
        last_expr_precedence = 2;
        src = out;
    }

    void visit_StringItem(const ASR::StringItem_t& x) {
        this->visit_expr(*x.m_idx);
        std::string idx = std::move(src);
        this->visit_expr(*x.m_arg);
        std::string str = std::move(src);
        src = "(char *)memcpy((char *)calloc(2U, sizeof(char)), "
                + str + " + " + idx + " - 1, 1U)";
    }

    void visit_StringLen(const ASR::StringLen_t &x) {
        this->visit_expr(*x.m_arg);
        src = "strlen(" + src + ")";
    }

};

Result<std::string> asr_to_c(Allocator &al, ASR::TranslationUnit_t &asr,
    diag::Diagnostics &diagnostics, Platform &platform,
    int64_t default_lower_bound)
{

    LCompilers::PassOptions pass_options;
    pass_options.always_run = true;
    pass_unused_functions(al, asr, pass_options);
    pass_replace_class_constructor(al, asr, pass_options);
    ASRToCVisitor v(diagnostics, platform, default_lower_bound);
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
