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

std::string format_type(const std::string &dims, const std::string &type,
        const std::string &name, bool use_ref, bool dummy, bool use_kokko=true,
        std::string kokko_ref="&", bool use_name=false, size_t size=0)
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
            if( use_kokko ) {
                fmt = "const Kokkos::View<" + c + type + dims + "> &" + name;
            } else {
                fmt = c + type + dims + " " + name;
            }
        } else {
            if( use_kokko ) {
                fmt = "Kokkos::View<" + type + dims + ">" + kokko_ref + " " + name;
                if( use_name ) {
                   fmt += "(\"" + name + "\"";
                   if( size > 0 ) {
                    fmt += ", " + std::to_string(size);
                   }
                   fmt += ")";
                }
            } else {
                fmt = type + dims + " " + name;
            }
        }
    }
    return fmt;
}

std::string remove_characters(std::string s, char c)
{
    s.erase(remove(s.begin(), s.end(), c), s.end());
    return s;
}

class ASRToCPPVisitor : public BaseCCPPVisitor<ASRToCPPVisitor>
{
public:

    std::string array_types_decls;
    std::map<std::string, std::map<std::string,
             std::map<size_t, std::string>>> eltypedims2arraytype;

    ASRToCPPVisitor(diag::Diagnostics &diag, Platform &platform,
                    int64_t default_lower_bound)
        : BaseCCPPVisitor(diag, platform, true, true, false,
                          default_lower_bound),
          array_types_decls(std::string("\nstruct dimension_descriptor\n"
                                        "{\n    int32_t lower_bound, length;\n};\n")) {}

    std::string convert_dims(size_t n_dims, ASR::dimension_t *m_dims, size_t& size)
    {
        std::string dims;
        size = 1;
        for (size_t i=0; i<n_dims; i++) {
            ASR::expr_t *length = m_dims[i].m_length;
            if (!length) {
                dims += "*";
            } else if (length) {
                ASR::expr_t* length_value = ASRUtils::expr_value(length);
                if( length_value ) {
                    int64_t length_int = -1;
                    ASRUtils::extract_value(length_value, length_int);
                    size *= (length_int);
                    dims += "[" + std::to_string(length_int) + "]";
                } else {
                    size = 0;
                    dims += "[ /* FIXME symbolic dimensions */ ]";
                }
            } else {
                throw CodeGenError("Dimension type not supported");
            }
        }
        return dims;
    }

    bool is_array_type_available(std::string& encoded_type_name, std::string& dims,
                                 size_t n_dims) {
        return eltypedims2arraytype.find(encoded_type_name) != eltypedims2arraytype.end() &&
               eltypedims2arraytype[encoded_type_name].find(dims) !=
               eltypedims2arraytype[encoded_type_name].end() &&
               eltypedims2arraytype[encoded_type_name][dims].find(n_dims) !=
               eltypedims2arraytype[encoded_type_name][dims].end();
    }

    std::string get_array_type(std::string type_name, std::string encoded_type_name,
                               std::string& dims,
                               size_t n_dims, bool make_ptr=true) {
        if( is_array_type_available(encoded_type_name, dims, n_dims) ) {
            if( make_ptr ) {
                return eltypedims2arraytype[encoded_type_name][dims][n_dims] + "*";
            } else {
                return eltypedims2arraytype[encoded_type_name][dims][n_dims];
            }
        }

        std::string struct_name;
        std::string new_array_type;
        std::string dims_copy = remove_characters(dims, '[');
        dims_copy = remove_characters(dims_copy, ']');
        dims_copy = remove_characters(dims_copy, '*');
        std::string name = encoded_type_name + "_" + dims_copy + "_" + std::to_string(n_dims);
        struct_name = "struct " + name;
        std::string array_data = format_type("*", type_name, "data", false, false, true, "*");
        new_array_type = struct_name + "\n{\n    " + array_data +
                            ";\n    dimension_descriptor dims[" +
                            std::to_string(n_dims) + "];\n    bool is_allocated;\n\n";
        std::string constructor = "    " + name + "(" + array_data + "_): data{data_} {};\n};\n";
        new_array_type += constructor;
        type_name = name + "*";
        eltypedims2arraytype[encoded_type_name][dims][n_dims] = name;
        array_types_decls += "\n" + new_array_type + "\n";
        return type_name;
    }

    void generate_array_decl(std::string& sub, std::string v_m_name,
                             std::string& type_name, std::string& dims,
                             std::string& encoded_type_name,
                             ASR::dimension_t* m_dims, int n_dims, size_t size,
                             bool use_ref, bool dummy,
                             bool declare_value, bool is_pointer=false) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string type_name_copy = type_name;
        type_name = get_array_type(type_name, encoded_type_name, dims, n_dims);
        std::string type_name_without_ptr = get_array_type(type_name, encoded_type_name, dims,
                                                           n_dims, false);
        if( declare_value ) {
            std::string variable_name = std::string(v_m_name) + "_value";
            if( !is_pointer ) {
                sub = format_type("*", type_name_copy, std::string(v_m_name) + "_data",
                                  use_ref, dummy, true, "", true, size) + ";\n" + indent;
            } else {
                sub = indent;
            }
            sub += format_type("", type_name_without_ptr, variable_name, use_ref, dummy, false);
            if( !is_pointer ) {
                sub += "(&" + std::string(v_m_name) + "_data);\n";
            } else {
                sub += ";\n";
            }
            sub += indent + format_type("", type_name, v_m_name, use_ref, dummy, false);
            sub += " = &" + variable_name + ";\n";
            if( !is_pointer ) {
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
            sub = format_type("", type_name, v_m_name, use_ref, dummy, false);
        }
    }

    std::string generate_templates_for_arrays(std::string v_name) {
        std::string typename_T = "T" + std::to_string(template_number);
        template_for_Kokkos += "typename " + typename_T + ", ";
        template_number += 1;
        return typename_T + "* " + v_name;
    }

    std::string convert_variable_decl(const ASR::Variable_t &v, bool use_static=true,
                                      bool use_templates_for_arrays=false)
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
                size_t size;
                std::string dims = convert_dims(t->n_dims, t->m_dims, size);
                std::string type_name = "int" + std::to_string(t->m_kind * 8) + "_t";
                if( is_array ) {
                    if( use_templates_for_arrays ) {
                        sub += generate_templates_for_arrays(std::string(v.m_name));
                    } else {
                        std::string encoded_type_name = "i" + std::to_string(t->m_kind * 8);
                        generate_array_decl(sub, std::string(v.m_name), type_name, dims,
                                            encoded_type_name, t->m_dims, t->n_dims, size,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out, true);
                    }
                } else {
                    sub = format_type(dims, type_name, v.m_name, use_ref, dummy);
                }
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
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(v.m_type);
                size_t size;
                dims = convert_dims(t->n_dims, t->m_dims, size);
                std::string type_name = "int" + std::to_string(t->m_kind * 8) + "_t";
                if( is_array ) {
                    if( use_templates_for_arrays ) {
                        sub += generate_templates_for_arrays(std::string(v.m_name));
                    } else {
                        std::string encoded_type_name = "i" + std::to_string(t->m_kind * 8);
                        generate_array_decl(sub, std::string(v.m_name), type_name, dims,
                                            encoded_type_name, t->m_dims, t->n_dims, size,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out);
                    }
                } else {
                    sub = format_type(dims, type_name, v.m_name, use_ref, dummy);
                }
            } else if (ASRUtils::is_real(*v.m_type)) {
                ASR::Real_t *t = ASR::down_cast<ASR::Real_t>(v.m_type);
                size_t size;
                dims = convert_dims(t->n_dims, t->m_dims, size);
                std::string type_name = "float";
                if (t->m_kind == 8) type_name = "double";
                if( is_array ) {
                    if( use_templates_for_arrays ) {
                        sub += generate_templates_for_arrays(std::string(v.m_name));
                    } else {
                        std::string encoded_type_name = "f" + std::to_string(t->m_kind * 8);
                        generate_array_decl(sub, std::string(v.m_name), type_name, dims,
                                            encoded_type_name, t->m_dims, t->n_dims, size,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out);
                    }
                } else {
                    sub = format_type(dims, type_name, v.m_name, use_ref, dummy);
                }
            } else if (ASRUtils::is_complex(*v.m_type)) {
                ASR::Complex_t *t = ASR::down_cast<ASR::Complex_t>(v.m_type);
                size_t size;
                dims = convert_dims(t->n_dims, t->m_dims, size);
                std::string type_name = "std::complex<float>";
                if (t->m_kind == 8) type_name = "std::complex<double>";
                if( is_array ) {
                    if( use_templates_for_arrays ) {
                        sub += generate_templates_for_arrays(std::string(v.m_name));
                    } else {
                        std::string encoded_type_name = "c" + std::to_string(t->m_kind * 8);
                        generate_array_decl(sub, std::string(v.m_name), type_name, dims,
                                            encoded_type_name, t->m_dims, t->n_dims, size,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out);
                    }
                } else {
                    sub = format_type(dims, type_name, v.m_name, use_ref, dummy);
                }
            } else if (ASRUtils::is_logical(*v.m_type)) {
                ASR::Logical_t *t = ASR::down_cast<ASR::Logical_t>(v.m_type);
                size_t size;
                dims = convert_dims(t->n_dims, t->m_dims, size);
                sub = format_type(dims, "bool", v.m_name, use_ref, dummy);
            } else if (ASRUtils::is_character(*v.m_type)) {
                ASR::Character_t *t = ASR::down_cast<ASR::Character_t>(v.m_type);
                size_t size;
                dims = convert_dims(t->n_dims, t->m_dims, size);
                sub = format_type(dims, "std::string", v.m_name, use_ref, dummy);
            } else if (ASR::is_a<ASR::Derived_t>(*v.m_type)) {
                ASR::Derived_t *t = ASR::down_cast<ASR::Derived_t>(v.m_type);
                std::string der_type_name = ASRUtils::symbol_name(t->m_derived_type);
                size_t size;
                dims = convert_dims(t->n_dims, t->m_dims, size);
                if( is_array ) {
                    if( use_templates_for_arrays ) {
                        sub += generate_templates_for_arrays(std::string(v.m_name));
                    } else {
                        std::string encoded_type_name = "x" + der_type_name;
                        std::string type_name = std::string("struct ") + der_type_name;
                        generate_array_decl(sub, std::string(v.m_name), type_name, dims,
                                            encoded_type_name, t->m_dims, t->n_dims, size,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out);
                    }
                } else {
                    sub = format_type(dims, "struct", v.m_name, use_ref, dummy);
                }
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
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
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

        src = headers + array_types_decls + unit_src;
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

    void visit_ArrayConstant(const ASR::ArrayConstant_t &x) {
        std::string indent(indentation_level * indentation_spaces, ' ');
        from_std_vector_helper = indent + "Kokkos::View<float*> r;\n";
        std::string out = "from_std_vector<float>({";
        for (size_t i=0; i<x.n_args; i++) {
            this->visit_expr(*x.m_args[i]);
            out += src;
            if (i < x.n_args-1) out += ", ";
        }
        out += "})";
        from_std_vector_helper += indent + "r = " + out + ";\n";
        src = "&r";
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

    void visit_StringItem(const ASR::StringItem_t& x) {
        this->visit_expr(*x.m_idx);
        std::string idx = std::move(src);
        this->visit_expr(*x.m_arg);
        std::string str = std::move(src);
        src = str + "[" + idx + " - 1]";
    }

    void visit_StringLen(const ASR::StringLen_t &x) {
        this->visit_expr(*x.m_arg);
        src = src + ".length()";
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
        if (x.m_end) {
            this->visit_expr(*x.m_end);
            out += "<< " + src + ";\n";
        } else {
            out += "<< std::endl;\n";
        }
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

    void visit_ArrayItem(const ASR::ArrayItem_t &x) {
        this->visit_expr(*x.m_v);
        std::string array = src;
        std::string out = array;
        ASR::dimension_t* m_dims;
        ASRUtils::extract_dimensions_from_ttype(ASRUtils::expr_type(x.m_v), m_dims);
        out += "->data->operator[](";
        std::string index = "";
        for (size_t i=0; i<x.n_args; i++) {
            std::string current_index = "";
            if (x.m_args[i].m_right) {
                this->visit_expr(*x.m_args[i].m_right);
            } else {
                src = "/* FIXME right index */";
            }
            out += src;
            out += " - " + array + "->dims[" + std::to_string(i) + "].lower_bound";
            if (i < x.n_args - 1) {
                out += ", ";
            }
        }
        out += ")";
        last_expr_precedence = 2;
        src = out;
    }

};

Result<std::string> asr_to_cpp(Allocator &al, ASR::TranslationUnit_t &asr,
    diag::Diagnostics &diagnostics, Platform &platform,
    int64_t default_lower_bound)
{
    LCompilers::PassOptions pass_options;
    pass_options.always_run = true;
    pass_unused_functions(al, asr, pass_options);
    ASRToCPPVisitor v(diagnostics, platform, default_lower_bound);
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
