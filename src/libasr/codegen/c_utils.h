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

class CCPPDSUtils {
    private:

        std::map<std::string, std::string> typecodeToDStype;
        std::map<std::string, std::map<std::string, std::string>> typecodeToDSfuncs;
        std::map<std::string, std::string> compareTwoDS;

        int indentation_level, indentation_spaces;

        std::string generated_code;
        std::string func_decls;

        SymbolTable* global_scope;
        DeepCopyFunction deepcopy_function;

    public:

        CCPPDSUtils() {
            generated_code.clear();
            func_decls.clear();
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

        std::string get_deepcopy(ASR::ttype_t *t, std::string value, std::string target) {
            if (ASR::is_a<ASR::List_t>(*t)) {
                ASR::List_t* list_type = ASR::down_cast<ASR::List_t>(t);
                std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
                std::string func = typecodeToDSfuncs[list_type_code]["list_deepcopy"];
                return func + "(&" + value + ", &" + target + ");";
            } else if (ASR::is_a<ASR::Tuple_t>(*t)) {
                ASR::Tuple_t* tup_type = ASR::down_cast<ASR::Tuple_t>(t);
                std::string tup_type_code = CUtils::get_tuple_type_code(tup_type);
                std::string func = typecodeToDSfuncs[tup_type_code]["tuple_deepcopy"];
                return func + "(" + value + ", &" + target + ");";
            } else if (ASR::is_a<ASR::Character_t>(*t)) {
                return "strcpy(" + target + ", " + value + ");";
            } else {
                return target + " = " + value  + ";";
            }
        }

        bool is_non_primitive_DT(ASR::ttype_t *t) {
            return ASR::is_a<ASR::List_t>(*t) || ASR::is_a<ASR::Tuple_t>(*t);
        }

        std::string get_type(ASR::ttype_t *t) {
            LFORTRAN_ASSERT(is_non_primitive_DT(t));
            if (ASR::is_a<ASR::List_t>(*t)) {
                ASR::List_t* list_type = ASR::down_cast<ASR::List_t>(t);
                return get_list_type(list_type);
            } else if (ASR::is_a<ASR::Tuple_t>(*t)) {
                ASR::Tuple_t* tup_type = ASR::down_cast<ASR::Tuple_t>(t);
                return get_tuple_type(tup_type);
            }
            LFORTRAN_ASSERT(false);
        }

        std::string get_list_type(ASR::List_t* list_type) {
            std::string list_element_type = CUtils::get_c_type_from_ttype_t(list_type->m_type);
            if (is_non_primitive_DT(list_type->m_type)) {
                // Make sure the nested types work
                get_type(list_type->m_type);
            }
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            if( typecodeToDStype.find(list_type_code) != typecodeToDStype.end() ) {
                return typecodeToDStype[list_type_code];
            }
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string list_struct_type = "struct list_" + list_type_code;
            typecodeToDStype[list_type_code] = list_struct_type;
            func_decls += indent + list_struct_type + " {\n";
            func_decls += indent + tab + "int32_t capacity;\n";
            func_decls += indent + tab + "int32_t current_end_point;\n";
            func_decls += indent + tab + list_element_type + "* data;\n";
            func_decls += indent + "};\n\n";
            generate_compare_funcs((ASR::ttype_t *)list_type);
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
            return typecodeToDSfuncs[list_type_code]["list_deepcopy"];
        }

        std::string get_list_init_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecodeToDSfuncs[list_type_code]["list_init"];
        }

        std::string get_list_append_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecodeToDSfuncs[list_type_code]["list_append"];
        }

        std::string get_list_insert_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecodeToDSfuncs[list_type_code]["list_insert"];
        }

        std::string get_list_resize_func(std::string list_type_code) {
            return typecodeToDSfuncs[list_type_code]["list_resize"];
        }

        std::string get_list_remove_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecodeToDSfuncs[list_type_code]["list_remove"];
        }

        std::string get_list_concat_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecodeToDSfuncs[list_type_code]["list_concat"];
        }

        std::string get_list_find_item_position_function(std::string list_type_code) {
            return typecodeToDSfuncs[list_type_code]["list_find_item"];
        }

        std::string get_list_clear_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecodeToDSfuncs[list_type_code]["list_clear"];
        }

        std::string get_generated_code() {
            return generated_code;
        }

        std::string get_func_decls() {
            return func_decls;
        }

        void generate_compare_funcs(ASR::ttype_t *t) {
            std::string type_code = ASRUtils::get_type_code(t, true);
            if (compareTwoDS.find(type_code) != compareTwoDS.end()) {
                return;
            }
            std::string element_type = CUtils::get_c_type_from_ttype_t(t);
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string cmp_func = global_scope->get_unique_name("compare_" + type_code);
            compareTwoDS[type_code] = cmp_func;
            std::string tmp_gen = "";
            if (ASR::is_a<ASR::List_t>(*t)) {
                std::string signature = "bool " + cmp_func + "(" + element_type + " a, " + element_type + " b)";
                func_decls += indent + "inline " + signature + ";\n";
                signature = indent + signature;
                tmp_gen += indent + signature + " {\n";
                ASR::ttype_t *tt = ASR::down_cast<ASR::List_t>(t)->m_type;
                generate_compare_funcs(tt);
                std::string ele_func = compareTwoDS[ASRUtils::get_type_code(tt, true)];
                tmp_gen += indent + tab + "if (a.current_end_point != b.current_end_point)\n";
                tmp_gen += indent + tab + tab + "return false;\n";
                tmp_gen += indent + tab + "for (int i=0; i<a.current_end_point; i++) {\n";
                tmp_gen += indent + tab + tab + "if (!" + ele_func + "(a.data[i], b.data[i]))\n";
                tmp_gen += indent + tab + tab + tab + "return false;\n";
                tmp_gen += indent + tab + "}\n";
                tmp_gen += indent + tab + "return true;\n";

            } else if (ASR::is_a<ASR::Tuple_t>(*t)) {
                ASR::Tuple_t *tt = ASR::down_cast<ASR::Tuple_t>(t);
                std::string signature = "bool " + cmp_func + "(" + element_type + " a, " + element_type+ " b)";
                func_decls += indent + "inline " + signature + ";\n";
                signature = indent + signature;
                tmp_gen += indent + signature + " {\n";
                tmp_gen += indent + tab + "if (a.length != b.length)\n";
                tmp_gen += indent + tab + tab + "return false;\n";
                tmp_gen += indent + tab + "bool ans = true;\n";
                for (size_t i=0; i<tt->n_type; i++) {
                    generate_compare_funcs(tt->m_type[i]);
                    std::string ele_func = compareTwoDS[ASRUtils::get_type_code(tt->m_type[i], true)];
                    std::string num = std::to_string(i);
                    tmp_gen += indent + tab + "ans &= " + ele_func + "(a.element_" +
                                num + ", " + "b.element_" + num + ");\n";
                }
                tmp_gen += indent + tab + "return ans;\n";
            } else if (ASR::is_a<ASR::Character_t>(*t)) {
                std::string signature = "bool " + cmp_func + "(" + element_type + " a, " + element_type + " b)";
                func_decls += indent + "inline " + signature + ";\n";
                signature = indent + signature;
                tmp_gen += indent + signature + " {\n";
                tmp_gen += indent + tab + "return strcmp(a, b) == 0;\n";
            } else {
                std::string signature = "bool " + cmp_func + "(" + element_type + " a, " + element_type + " b)";
                func_decls += indent + "inline " + signature + ";\n";
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
            typecodeToDSfuncs[list_type_code]["list_init"] = list_init_func;
            std::string signature = "void " + list_init_func + "(" + list_struct_type + "* x, int32_t capacity)";
            func_decls += indent + "inline " + signature + ";\n";
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
            typecodeToDSfuncs[list_type_code]["list_clear"] = list_init_func;
            std::string signature = "void " + list_init_func + "(" + list_struct_type + "* x)";
            func_decls += indent + "inline " + signature + ";\n";
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
            typecodeToDSfuncs[list_type_code]["list_deepcopy"] = list_dc_func;
            std::string signature = "void " + list_dc_func + "("
                                + list_struct_type + "* src, "
                                + list_struct_type + "* dest)";
            func_decls += "inline " + signature + ";\n";
            generated_code += indent + signature + " {\n";
            generated_code += indent + tab + "dest->capacity = src->capacity;\n";
            generated_code += indent + tab + "dest->current_end_point = src->current_end_point;\n";
            generated_code += indent + tab + "dest->data = (" + list_element_type + "*) " +
                              "malloc(src->capacity * sizeof(" + list_element_type + "));\n";
            generated_code += indent + tab + "memcpy(dest->data, src->data, " +
                                "src->capacity * sizeof(" + list_element_type + "));\n";
            if (ASR::is_a<ASR::List_t>(*m_type)) {
                ASR::ttype_t *tt = ASR::down_cast<ASR::List_t>(m_type)->m_type;
                std::string deep_copy_func = typecodeToDSfuncs[ASRUtils::get_type_code(tt, true)]["list_deepcopy"];
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
            typecodeToDSfuncs[list_type_code]["list_concat"] = list_con_func;
            std::string init_func = typecodeToDSfuncs[list_type_code]["list_init"];
            std::string signature = list_struct_type + "* " + list_con_func + "("
                                + list_struct_type + "* left, "
                                + list_struct_type + "* right)";
            func_decls += "inline " + signature + ";\n";
            generated_code += indent + signature + " {\n";
            generated_code += indent + tab + list_struct_type + " *result = (" + list_struct_type + "*)malloc(sizeof(" +
                                list_struct_type + "));\n";
            generated_code += indent + tab + init_func + "(result, left->current_end_point + right->current_end_point);\n";
            if (ASR::is_a<ASR::List_t>(*m_type)) {
                ASR::ttype_t *tt = ASR::down_cast<ASR::List_t>(m_type)->m_type;
                std::string deep_copy_func = typecodeToDSfuncs[ASRUtils::get_type_code(tt, true)]["list_deepcopy"];
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
            typecodeToDSfuncs[list_type_code]["list_resize"] = list_resize_func;
            std::string signature = "void " + list_resize_func + "(" + list_struct_type + "* x)";
            func_decls += indent + "inline " + signature + ";\n";
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
            typecodeToDSfuncs[list_type_code]["list_append"] = list_append_func;
            std::string signature = "void " + list_append_func + "("
                                + list_struct_type + "* x, "
                                + list_element_type + " element)";
            func_decls += "inline " + signature + ";\n";
            generated_code += indent + signature + " {\n";
            std::string list_resize_func = get_list_resize_func(list_type_code);
            generated_code += indent + tab + list_resize_func + "(x);\n";
            if( ASR::is_a<ASR::Character_t>(*m_type) ) {
                generated_code += indent + tab + "x->data[x->current_end_point] = (char*) malloc(40 * sizeof(char));\n";
            }
            generated_code += indent + tab + \
                        get_deepcopy(m_type, "element", "x->data[x->current_end_point]") + "\n";
            generated_code += indent + tab + "x->current_end_point += 1;\n";
            generated_code += indent + "}\n\n";
        }

        void list_insert(std::string list_struct_type,
            std::string list_type_code, std::string list_element_type,
            ASR::ttype_t* m_type) {
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string list_insert_func = global_scope->get_unique_name("list_insert_" + list_type_code);
            typecodeToDSfuncs[list_type_code]["list_insert"] = list_insert_func;
            std::string signature = "void " + list_insert_func + "("
                                + list_struct_type + "* x, "
                                + "int pos, "
                                + list_element_type + " element)";
            func_decls += "inline " + signature + ";\n";
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
            typecodeToDSfuncs[list_type_code]["list_find_item"] = list_find_item_pos_func;
            std::string signature = "int " + list_find_item_pos_func + "("
                                + list_struct_type + "* x, "
                                + list_element_type + " element)";
            std::string cmp_func = compareTwoDS[list_type_code];
            func_decls += "inline " + signature + ";\n";
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
            typecodeToDSfuncs[list_type_code]["list_remove"] = list_remove_func;
            std::string signature = "void " + list_remove_func + "("
                                + list_struct_type + "* x, "
                                + list_element_type + " element)";
            func_decls += "inline " + signature + ";\n";
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

        std::string get_tuple_deepcopy_func(ASR::Tuple_t* tup_type) {
            std::string tuple_type_code = CUtils::get_tuple_type_code(tup_type);
            return typecodeToDSfuncs[tuple_type_code]["tuple_deepcopy"];
        }


        std::string get_tuple_type(ASR::Tuple_t* tuple_type) {
            std::string tuple_type_code = CUtils::get_tuple_type_code(tuple_type);
            if (typecodeToDStype.find(tuple_type_code) != typecodeToDStype.end()) {
                return typecodeToDStype[tuple_type_code];
            }
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string tuple_struct_type = "struct " + tuple_type_code;
            typecodeToDStype[tuple_type_code] = tuple_struct_type;
            std::string tmp_gen = "";
            tmp_gen += indent + tuple_struct_type + " {\n";
            tmp_gen += indent + tab + "int32_t length;\n";
            for (size_t i = 0; i < tuple_type->n_type; i++) {
                if (is_non_primitive_DT(tuple_type->m_type[i])) {
                    // Make sure the nested types work
                    get_type(tuple_type->m_type[i]);
                }
                tmp_gen += indent + tab + \
                    CUtils::get_c_type_from_ttype_t(tuple_type->m_type[i]) + " element_" + std::to_string(i) + ";\n";
            }
            tmp_gen += indent + "};\n\n";
            func_decls += tmp_gen;
            generate_compare_funcs((ASR::ttype_t *)tuple_type);
            tuple_deepcopy(tuple_type, tuple_type_code);
            return tuple_struct_type;
        }

        void tuple_deepcopy(ASR::Tuple_t *t, std::string tuple_type_code) {
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string tup_dc_func = global_scope->get_unique_name("tuple_deepcopy_" + tuple_type_code);
            typecodeToDSfuncs[tuple_type_code]["tuple_deepcopy"] = tup_dc_func;
            std::string tuple_struct_type = typecodeToDStype[tuple_type_code];
            std::string signature = "void " + tup_dc_func + "("
                                + tuple_struct_type + " src, "
                                + tuple_struct_type + "* dest)";
            std::string tmp_def = "", tmp_gen = "";
            tmp_def += "inline " + signature + ";\n";
            tmp_gen += indent + signature + " {\n";
            for (size_t i=0; i<t->n_type; i++) {
                std::string n = std::to_string(i);
                if (ASR::is_a<ASR::Character_t>(*t->m_type[i])) {
                    tmp_gen += indent + tab + "dest->element_" + n + " = " + \
                                "(char *) malloc(40*sizeof(char));\n";
                }
                tmp_gen += indent + tab + get_deepcopy(t->m_type[i], "src.element_" + n,
                                "dest->element_" + n) + "\n";
            }
            tmp_gen += indent + "}\n\n";
            func_decls += tmp_def;
            generated_code += tmp_gen;
        }

        ~CCPPDSUtils() {
            typecodeToDStype.clear();
            generated_code.clear();
            compareTwoDS.clear();
        }
};

} // namespace LFortran

#endif // LFORTRAN_C_UTILS_H
