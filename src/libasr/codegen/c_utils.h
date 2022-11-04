#ifndef LFORTRAN_C_UTILS_H
#define LFORTRAN_C_UTILS_H

#include <libasr/asr.h>
#include <libasr/asr_utils.h>

namespace LFortran {
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

namespace CUtils {

    static inline std::string get_tuple_type_code(ASR::Tuple_t *tup) {
        std::string result = "tuple_";
        for (size_t i = 0; i < tup->n_type; i++) {
            result += ASRUtils::get_type_code(tup->m_type[i], true);
            if (i + 1 != tup->n_type) {
                result += "_";
            }
        }
        return result;
    }

    static inline std::string get_c_type_from_ttype_t(ASR::ttype_t* t,
            bool is_c=true) {
        int kind = ASRUtils::extract_kind_from_ttype_t(t);
        std::string type_src = "";
        switch( t->type ) {
            case ASR::ttypeType::Integer: {
                type_src = "int" + std::to_string(kind * 8) + "_t";
                break;
            }
            case ASR::ttypeType::Logical: {
                type_src = "bool";
                break;
            }
            case ASR::ttypeType::Real: {
                if( kind == 4 ) {
                    type_src = "float";
                } else if( kind == 8 ) {
                    type_src = "double";
                } else {
                    throw CodeGenError(std::to_string(kind * 8) + "-bit floating points not yet supported.");
                }
                break;
            }
            case ASR::ttypeType::Character: {
                type_src = "char*";
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
            case ASR::ttypeType::Struct: {
                ASR::Struct_t* der_type = ASR::down_cast<ASR::Struct_t>(t);
                type_src = std::string("struct ") + ASRUtils::symbol_name(der_type->m_derived_type);
                break;
            }
            case ASR::ttypeType::List: {
                ASR::List_t* list_type = ASR::down_cast<ASR::List_t>(t);
                std::string list_element_type = get_c_type_from_ttype_t(list_type->m_type);
                std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
                type_src = "struct list_" + list_type_code;
                break;
            }
            case ASR::ttypeType::Tuple: {
                ASR::Tuple_t* tup_type = ASR::down_cast<ASR::Tuple_t>(t);
                type_src = "struct " + get_tuple_type_code(tup_type);
                break;
            }
            case ASR::ttypeType::Complex: {
                if( kind == 4 ) {
                    if( is_c ) {
                        type_src = "float complex";
                    } else {
                        type_src = "std::complex<float>";
                    }
                } else if( kind == 8 ) {
                    if( is_c ) {
                        type_src = "double complex";
                    } else {
                        type_src = "std::complex<double>";
                    }
                } else {
                    throw CodeGenError(std::to_string(kind * 8) + "-bit floating points not yet supported.");
                }
                break;
            }
            default: {
                throw CodeGenError("Type " + ASRUtils::type_to_str_python(t) + " not supported yet.");
            }
        }
        return type_src;
    }

    static inline std::string deepcopy(std::string target, std::string value, ASR::ttype_t* m_type) {
        switch (m_type->type) {
            case ASR::ttypeType::Character: {
                return "strcpy(" + target + ", " + value + ");";
            }
            default: {
                return target + " = " + value + ";";
            }
        }
    }

} // namespace CUtils


namespace CPPUtils {
    static inline std::string deepcopy(std::string target, std::string value, ASR::ttype_t* /*m_type*/) {
            return target + " = " + value + ";";
    }
} // namespace CPPUtils

typedef std::string (*DeepCopyFunction)(std::string, std::string, ASR::ttype_t*);

class CCPPList {
    private:

        std::map<std::string, std::pair<std::string, std::string>> typecode2listtype;
        std::map<std::string, std::map<std::string, std::string>> typecode2listfuncs;
        std::map<std::string, std::string> compare_list_eles;

        int indentation_level, indentation_spaces;

        std::string generated_code;
        std::string list_func_decls;

        SymbolTable* global_scope;
        DeepCopyFunction deepcopy_function;

    public:

        CCPPList() {
            generated_code.clear();
            list_func_decls.clear();
        }

        void set_deepcopy_function(DeepCopyFunction func) {
            deepcopy_function = func;
        }

        void set_indentation(int indendation_level_, int indendation_space_) {
            indentation_level = indendation_level_;
            indentation_spaces = indendation_space_;
        }

        void set_global_scope(SymbolTable* global_scope_) {
            global_scope = global_scope_;
        }

        std::string get_list_type(ASR::List_t* list_type,
            std::string list_element_type) {
            if (ASR::is_a<ASR::List_t>(*list_type->m_type)) {
                ASR::List_t* list_type_in = ASR::down_cast<ASR::List_t>(list_type->m_type);
                std::string list_element_type_in = CUtils::get_c_type_from_ttype_t(list_type_in->m_type);
                // Make sure that nested list are initialized
                get_list_type(list_type_in, list_element_type_in);
            }
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            if( typecode2listtype.find(list_type_code) != typecode2listtype.end() ) {
                return typecode2listtype[list_type_code].first;
            }
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string list_struct_type = "struct list_" + list_type_code;
            typecode2listtype[list_type_code] = std::make_pair(list_struct_type, list_element_type);
            list_func_decls += indent + list_struct_type + " {\n";
            list_func_decls += indent + tab + "int32_t capacity;\n";
            list_func_decls += indent + tab + "int32_t current_end_point;\n";
            list_func_decls += indent + tab + list_element_type + "* data;\n";
            list_func_decls += indent + "};\n\n";
            generate_compare_list_element(list_type->m_type);
            list_init(list_struct_type, list_type_code, list_element_type);
            list_deepcopy(list_struct_type, list_type_code, list_element_type, list_type->m_type);
            resize_if_needed(list_struct_type, list_type_code, list_element_type);
            list_append(list_struct_type, list_type_code, list_element_type, list_type->m_type);
            list_insert(list_struct_type, list_type_code, list_element_type, list_type->m_type);
            list_find_item_position(list_struct_type, list_type_code, list_element_type, list_type->m_type);
            list_remove(list_struct_type, list_type_code, list_element_type, list_type->m_type);
            list_clear(list_struct_type, list_type_code, list_element_type);
            list_concat(list_struct_type, list_type_code, list_element_type, list_type->m_type);
            return list_struct_type;
        }

        std::string get_list_deepcopy_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecode2listfuncs[list_type_code]["list_deepcopy"];
        }

        std::string get_list_init_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecode2listfuncs[list_type_code]["list_init"];
        }

        std::string get_list_append_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecode2listfuncs[list_type_code]["list_append"];
        }

        std::string get_list_insert_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecode2listfuncs[list_type_code]["list_insert"];
        }

        std::string get_list_resize_func(std::string list_type_code) {
            return typecode2listfuncs[list_type_code]["list_resize"];
        }

        std::string get_list_remove_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecode2listfuncs[list_type_code]["list_remove"];
        }

        std::string get_list_concat_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecode2listfuncs[list_type_code]["list_concat"];
        }

        std::string get_list_find_item_position_function(std::string list_type_code) {
            return typecode2listfuncs[list_type_code]["list_find_item"];
        }

        std::string get_list_clear_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecode2listfuncs[list_type_code]["list_clear"];
        }

        std::string get_generated_code() {
            return generated_code;
        }

        std::string get_list_func_decls() {
            return list_func_decls;
        }

        void generate_compare_list_element(ASR::ttype_t *t) {
            std::string type_code = ASRUtils::get_type_code(t, true);
            std::string list_element_type = typecode2listtype[type_code].second;
            if (compare_list_eles.find(type_code) != compare_list_eles.end()) {
                return;
            }
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string cmp_func = global_scope->get_unique_name("compare_" + type_code);
            compare_list_eles[type_code] = cmp_func;
            std::string tmp_gen = "";
            if (ASR::is_a<ASR::List_t>(*t)) {
                std::string signature = "bool " + cmp_func + "(" + list_element_type + " a, " + list_element_type+ " b)";
                list_func_decls += indent + "inline " + signature + ";\n";
                signature = indent + signature;
                tmp_gen += indent + signature + " {\n";
                ASR::ttype_t *tt = ASR::down_cast<ASR::List_t>(t)->m_type;
                generate_compare_list_element(tt);
                std::string ele_func = compare_list_eles[ASRUtils::get_type_code(tt, true)];
                tmp_gen += indent + tab + "if (a.current_end_point != b.current_end_point)\n";
                tmp_gen += indent + tab + tab + "return false;\n";
                tmp_gen += indent + tab + "for (int i=0; i<a.current_end_point; i++) {\n";
                tmp_gen += indent + tab + tab + "if (!" + ele_func + "(a.data[i], b.data[i]))\n";
                tmp_gen += indent + tab + tab + tab + "return false;\n";
                tmp_gen += indent + tab + "}\n";
                tmp_gen += indent + tab + "return true;\n";

            } else {
                std::string signature = "bool " + cmp_func + "(" + list_element_type + " a, " + list_element_type + " b)";
                list_func_decls += indent + "inline " + signature + ";\n";
                signature = indent + signature;
                tmp_gen += indent + signature + " {\n";
                tmp_gen += indent + tab + "return a == b;\n";
            }
            tmp_gen += indent + "}\n\n";
            generated_code += tmp_gen;
        }

        void list_init(std::string list_struct_type,
            std::string list_type_code,
            std::string list_element_type) {
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string list_init_func = global_scope->get_unique_name("list_init_" + list_type_code);
            typecode2listfuncs[list_type_code]["list_init"] = list_init_func;
            std::string signature = "void " + list_init_func + "(" + list_struct_type + "* x, int32_t capacity)";
            list_func_decls += indent + "inline " + signature + ";\n";
            signature = indent + signature;
            generated_code += indent + signature + " {\n";
            generated_code += indent + tab + "x->capacity = capacity;\n";
            generated_code += indent + tab + "x->current_end_point = 0;\n";
            generated_code += indent + tab + "x->data = (" + list_element_type + "*) " +
                              "malloc(capacity * sizeof(" + list_element_type + "));\n";
            generated_code += indent + "}\n\n";
        }

        void list_clear(std::string list_struct_type,
            std::string list_type_code,
            std::string list_element_type) {
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string list_init_func = global_scope->get_unique_name("list_clear_" + list_type_code);
            typecode2listfuncs[list_type_code]["list_clear"] = list_init_func;
            std::string signature = "void " + list_init_func + "(" + list_struct_type + "* x)";
            list_func_decls += indent + "inline " + signature + ";\n";
            signature = indent + signature;
            generated_code += indent + signature + " {\n";
            generated_code += indent + tab + "free(x->data);\n";
            generated_code += indent + tab + "x->capacity = 4;\n";
            generated_code += indent + tab + "x->current_end_point = 0;\n";
            generated_code += indent + tab + "x->data = (" + list_element_type + "*) " +
                              "malloc(x->capacity * sizeof(" + list_element_type + "));\n";
            generated_code += indent + "}\n\n";
        }

        void list_deepcopy(std::string list_struct_type,
            std::string list_type_code,
            std::string list_element_type, ASR::ttype_t *m_type) {
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string list_dc_func = global_scope->get_unique_name("list_deepcopy_" + list_type_code);
            typecode2listfuncs[list_type_code]["list_deepcopy"] = list_dc_func;
            std::string signature = "void " + list_dc_func + "("
                                + list_struct_type + "* src, "
                                + list_struct_type + "* dest)";
            list_func_decls += "inline " + signature + ";\n";
            generated_code += indent + signature + " {\n";
            generated_code += indent + tab + "dest->capacity = src->capacity;\n";
            generated_code += indent + tab + "dest->current_end_point = src->current_end_point;\n";
            generated_code += indent + tab + "dest->data = (" + list_element_type + "*) " +
                              "malloc(src->capacity * sizeof(" + list_element_type + "));\n";
            generated_code += indent + tab + "memcpy(dest->data, src->data, " +
                                "src->capacity * sizeof(" + list_element_type + "));\n";
            if (ASR::is_a<ASR::List_t>(*m_type)) {
                ASR::ttype_t *tt = ASR::down_cast<ASR::List_t>(m_type)->m_type;
                std::string deep_copy_func = typecode2listfuncs[ASRUtils::get_type_code(tt, true)]["list_deepcopy"];
                LFORTRAN_ASSERT(deep_copy_func.size() > 0);
                generated_code += indent + tab + "for(int i=0; i<src->current_end_point; i++)\n";
                generated_code += indent + tab + tab + deep_copy_func + "(&src->data[i], &dest->data[i]);\n";
            }
            generated_code += indent + "}\n\n";
        }

        void list_concat(std::string list_struct_type,
            std::string list_type_code,
            std::string list_element_type, ASR::ttype_t *m_type) {
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string list_con_func = global_scope->get_unique_name("list_concat_" + list_type_code);
            typecode2listfuncs[list_type_code]["list_concat"] = list_con_func;
            std::string init_func = typecode2listfuncs[list_type_code]["list_init"];
            std::string signature = list_struct_type + "* " + list_con_func + "("
                                + list_struct_type + "* left, "
                                + list_struct_type + "* right)";
            list_func_decls += "inline " + signature + ";\n";
            generated_code += indent + signature + " {\n";
            generated_code += indent + tab + list_struct_type + " *result = (" + list_struct_type + "*)malloc(sizeof(" +
                                list_struct_type + "));\n";
            generated_code += indent + tab + init_func + "(result, left->current_end_point + right->current_end_point);\n";
            if (ASR::is_a<ASR::List_t>(*m_type)) {
                ASR::ttype_t *tt = ASR::down_cast<ASR::List_t>(m_type)->m_type;
                std::string deep_copy_func = typecode2listfuncs[ASRUtils::get_type_code(tt, true)]["list_deepcopy"];
                LFORTRAN_ASSERT(deep_copy_func.size() > 0);
                generated_code += indent + tab + "for(int i=0; i<left->current_end_point; i++)\n";
                generated_code += indent + tab + tab + deep_copy_func + "(&left->data[i], &result->data[i]);\n";
                generated_code += indent + tab + "for(int i=0; i<right->current_end_point; i++)\n";
                generated_code += indent + tab + tab + deep_copy_func + "(&right->data[i], &result->data[i+left->current_end_point]);\n";
            } else {
                generated_code += indent + tab + "memcpy(result->data, left->data, " +
                                    "left->current_end_point * sizeof(" + list_element_type + "));\n";
                generated_code += indent + tab + "memcpy(result->data + left->current_end_point, right->data, " +
                                    "right->current_end_point * sizeof(" + list_element_type + "));\n";
            }
            generated_code += indent + tab + "result->current_end_point = left->current_end_point + right->current_end_point;\n";
            generated_code += indent + tab + "return result;\n";
            generated_code += indent + "}\n\n";
        }

        void resize_if_needed(std::string list_struct_type,
            std::string list_type_code,
            std::string list_element_type) {
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string list_resize_func = global_scope->get_unique_name("resize_if_needed_" + list_type_code);
            typecode2listfuncs[list_type_code]["list_resize"] = list_resize_func;
            std::string signature = "void " + list_resize_func + "(" + list_struct_type + "* x)";
            list_func_decls += indent + "inline " + signature + ";\n";
            signature = indent + signature;
            generated_code += indent + signature + " {\n";
            generated_code += indent + tab + "if (x->capacity == x->current_end_point) {\n";
            generated_code += indent + tab + tab + "x->capacity = 2 * x->capacity + 1;\n";
            generated_code += indent + tab + tab + "x->data = (" + list_element_type + "*) " +
                              "realloc(x->data, x->capacity * sizeof(" + list_element_type + "));\n";
            generated_code += indent + tab + "}\n";
            generated_code += indent + "}\n\n";
        }

        void list_append(std::string list_struct_type,
            std::string list_type_code,
            std::string list_element_type, ASR::ttype_t* m_type) {
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string list_append_func = global_scope->get_unique_name("list_append_" + list_type_code);
            typecode2listfuncs[list_type_code]["list_append"] = list_append_func;
            std::string signature = "void " + list_append_func + "("
                                + list_struct_type + "* x, "
                                + list_element_type + " element)";
            list_func_decls += "inline " + signature + ";\n";
            generated_code += indent + signature + " {\n";
            std::string list_resize_func = get_list_resize_func(list_type_code);
            generated_code += indent + tab + list_resize_func + "(x);\n";
            if( ASR::is_a<ASR::Character_t>(*m_type) ) {
                generated_code += indent + tab + "x->data[x->current_end_point] = (char*) malloc(40 * sizeof(char));\n";
            }
            if (ASR::is_a<ASR::List_t>(*m_type)) {
                ASR::ttype_t *tt = ASR::down_cast<ASR::List_t>(m_type)->m_type;
                std::string deep_copy_func = typecode2listfuncs[ASRUtils::get_type_code(tt, true)]["list_deepcopy"];
                LFORTRAN_ASSERT(deep_copy_func.size() > 0);
                generated_code += indent + tab + deep_copy_func + "(&element, &x->data[x->current_end_point]);\n";
            } else {
                generated_code += indent + tab + deepcopy_function("x->data[x->current_end_point]", "element", m_type) + "\n";
            }
            generated_code += indent + tab + "x->current_end_point += 1;\n";
            generated_code += indent + "}\n\n";
        }

        void list_insert(std::string list_struct_type,
            std::string list_type_code, std::string list_element_type,
            ASR::ttype_t* m_type) {
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string list_insert_func = global_scope->get_unique_name("list_insert_" + list_type_code);
            typecode2listfuncs[list_type_code]["list_insert"] = list_insert_func;
            std::string signature = "void " + list_insert_func + "("
                                + list_struct_type + "* x, "
                                + "int pos, "
                                + list_element_type + " element)";
            list_func_decls += "inline " + signature + ";\n";
            generated_code += indent + signature + " {\n";
            std::string list_resize_func = get_list_resize_func(list_type_code);
            generated_code += indent + tab + list_resize_func + "(x);\n";
            generated_code += indent + tab + "int pos_ptr = pos;\n";
            generated_code += indent + tab + list_element_type + " tmp_ptr = x->data[pos];\n";
            generated_code += indent + tab + list_element_type + " tmp;\n";

            generated_code += indent + tab + "while (x->current_end_point > pos_ptr) {\n";
            generated_code += indent + tab + tab + "tmp = x->data[pos_ptr + 1];\n";
            generated_code += indent + tab + tab + "x->data[pos_ptr + 1] = tmp_ptr;\n";
            generated_code += indent + tab + tab + "tmp_ptr = tmp;\n";
            generated_code += indent + tab + tab + "pos_ptr++;\n";
            generated_code += indent + tab + "}\n\n";

            if( ASR::is_a<ASR::Character_t>(*m_type) ) {
                generated_code += indent + tab + "x->data[pos] = (char*) malloc(40 * sizeof(char));\n";
            }
            generated_code += indent + tab + deepcopy_function("x->data[pos]", "element", m_type) + "\n";
            generated_code += indent + tab + "x->current_end_point += 1;\n";
            generated_code += indent + "}\n\n";
        }

        void list_find_item_position(std::string list_struct_type,
            std::string list_type_code, std::string list_element_type,
            ASR::ttype_t* /*m_type*/) {
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string list_find_item_pos_func = global_scope->get_unique_name("list_find_item_" + list_type_code);
            typecode2listfuncs[list_type_code]["list_find_item"] = list_find_item_pos_func;
            std::string signature = "int " + list_find_item_pos_func + "("
                                + list_struct_type + "* x, "
                                + list_element_type + " element)";
            std::string cmp_func = compare_list_eles[list_type_code];
            list_func_decls += "inline " + signature + ";\n";
            generated_code += indent + signature + " {\n";
            generated_code += indent + tab + "int el_pos = 0;\n";
            generated_code += indent + tab + "while (x->current_end_point > el_pos) {\n";
            generated_code += indent + tab + tab + "if (" + cmp_func + "(x->data[el_pos], element)) return el_pos;\n";
            generated_code += indent + tab + tab + "el_pos++;\n";
            generated_code += indent + tab + "}\n";
            generated_code += indent + tab + "return -1;\n";
            generated_code += indent + "}\n\n";
        }

        void list_remove(std::string list_struct_type,
            std::string list_type_code, std::string list_element_type,
            ASR::ttype_t* /*m_type*/) {
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string list_remove_func = global_scope->get_unique_name("list_remove_" + list_type_code);
            typecode2listfuncs[list_type_code]["list_remove"] = list_remove_func;
            std::string signature = "void " + list_remove_func + "("
                                + list_struct_type + "* x, "
                                + list_element_type + " element)";
            list_func_decls += "inline " + signature + ";\n";
            generated_code += indent + signature + " {\n";
            std::string find_item_pos_func = get_list_find_item_position_function(list_type_code);
            generated_code += indent + tab + "int el_pos = " + find_item_pos_func + "(x, element);\n";
            generated_code += indent + tab + "while (x->current_end_point > el_pos) {\n";
            generated_code += indent + tab + tab + "int tmp = el_pos + 1;\n";
            generated_code += indent + tab + tab + "x->data[el_pos] = x->data[tmp];\n";
            generated_code += indent + tab + tab + "el_pos = tmp;\n";
            generated_code += indent + tab + "}\n";

            generated_code += indent + tab + "x->current_end_point -= 1;\n";
            generated_code += indent + "}\n\n";
        }

        ~CCPPList() {
            typecode2listtype.clear();
            generated_code.clear();
            compare_list_eles.clear();
        }
};

class CCPPTuple {
    private:

        std::map<std::string, std::string> typecode2tupletype;
        std::map<std::string, std::map<std::string, std::string>> typecode2tuplefuncs;

        int indentation_level, indentation_spaces;

        std::string generated_code;
        std::string tuple_func_decls;

        SymbolTable* global_scope;
        DeepCopyFunction deepcopy_function;

    public:

        CCPPTuple() {
            generated_code.clear();
            tuple_func_decls.clear();
        }

        void set_deepcopy_function(DeepCopyFunction func) {
            deepcopy_function = func;
        }

        void set_indentation(int indendation_level_, int indendation_space_) {
            indentation_level = indendation_level_;
            indentation_spaces = indendation_space_;
        }

        void set_global_scope(SymbolTable* global_scope_) {
            global_scope = global_scope_;
        }

        std::string get_tuple_type(ASR::Tuple_t* tuple_type) {
            std::string tuple_type_code = CUtils::get_tuple_type_code(tuple_type);
            if (typecode2tupletype.find(tuple_type_code) != typecode2tupletype.end()) {
                return typecode2tupletype[tuple_type_code];
            }
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string tuple_struct_type = "struct " + tuple_type_code;
            typecode2tupletype[tuple_type_code] = tuple_struct_type;
            tuple_func_decls += indent + tuple_struct_type + " {\n";
            tuple_func_decls += indent + tab + "int32_t length;\n";
            for (size_t i = 0; i < tuple_type->n_type; i++) {
                tuple_func_decls += indent + tab + \
                    CUtils::get_c_type_from_ttype_t(tuple_type->m_type[i]) + " element_" + std::to_string(i) + ";\n";
            }
            tuple_func_decls += indent + "};\n\n";
            return tuple_struct_type;
        }


        std::string get_generated_code() {
            return generated_code;
        }

        std::string get_tuple_func_decls() {
            return tuple_func_decls;
        }

        ~CCPPTuple() {
            typecode2tupletype.clear();
            generated_code.clear();
        }
};

} // namespace LFortran

#endif // LFORTRAN_C_UTILS_H
