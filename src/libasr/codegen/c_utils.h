#ifndef LFORTRAN_C_UTILS_H
#define LFORTRAN_C_UTILS_H

#include <libasr/asr.h>
#include <libasr/asr_utils.h>

namespace LCompilers {

    static inline std::string format_type_c(const std::string &dims, const std::string &type,
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

    static inline bool is_non_primitive_DT(ASR::ttype_t *t) {
        return ASR::is_a<ASR::List_t>(*t) || ASR::is_a<ASR::Tuple_t>(*t) || ASR::is_a<ASR::Struct_t>(*t);
    }

    class CUtilFunctions {

        private:

            SymbolTable* global_scope;
            std::map<std::string, std::string> util2func;

            int indentation_level, indentation_spaces;

        public:

            std::string util_func_decls;
            std::string util_funcs;

            CUtilFunctions() {
                util2func.clear();
                util_func_decls.clear();
                util_funcs.clear();
            }

            void set_indentation(int indendation_level_, int indendation_space_) {
                indentation_level = indendation_level_;
                indentation_spaces = indendation_space_;
            }

            void set_global_scope(SymbolTable* global_scope_) {
                global_scope = global_scope_;
            }

            std::string get_generated_code() {
                return util_funcs;
            }

            std::string get_util_func_decls() {
                return util_func_decls;
            }

            void array_size() {
                std::string indent(indentation_level * indentation_spaces, ' ');
                std::string tab(indentation_spaces, ' ');
                std::string array_size_func;
                if( util2func.find("array_size") == util2func.end() ) {
                    array_size_func = global_scope->get_unique_name("array_size");
                    util2func["array_size"] = array_size_func;
                } else {
                    return ;
                }
                array_size_func = util2func["array_size"];
                std::string signature = "static inline int32_t " + array_size_func + "(struct dimension_descriptor dims[], size_t n)";
                util_func_decls += indent + signature + ";\n";
                std::string body = indent + signature + " {\n";
                body += indent + tab + "int32_t size = 1;\n";
                body += indent + tab + "for (size_t i = 0; i < n; i++) {\n";
                body += indent + tab + tab + "size *= dims[i].length;\n";
                body += indent + tab + "}\n";
                body += indent + tab + "return size;\n";
                body += indent + "}\n\n";
                util_funcs += body;
            }

            void array_deepcopy(ASR::ttype_t* array_type_asr, std::string array_type_name,
                                std::string array_encoded_type_name, std::string array_type_str) {
                LCOMPILERS_ASSERT(!is_non_primitive_DT(array_type_asr));
                std::string indent(indentation_level * indentation_spaces, ' ');
                std::string tab(indentation_spaces, ' ');
                std::string array_dc_func;
                if( util2func.find("array_deepcopy_" + array_encoded_type_name) == util2func.end() ) {
                    array_dc_func = global_scope->get_unique_name("array_deepcopy_" + array_encoded_type_name);
                    util2func["array_deepcopy_" + array_encoded_type_name] = array_dc_func;
                } else {
                    return ;
                }
                array_dc_func = util2func["array_deepcopy_" + array_encoded_type_name];
                std::string array_types_decls = "";
                std::string signature = "void " + array_dc_func + "("
                                    + array_type_str + " src, "
                                    + array_type_str + " dest)";
                util_func_decls += "inline " + signature + ";\n";
                std::string body = indent + signature + " {\n";
                body += indent + tab + "int32_t src_size = " + get_array_size() + "(src->dims, src->n_dims);\n";
                body += indent + tab + "memcpy(dest->data, src->data, src_size * sizeof(" + array_type_name +"));\n";
                body += indent + tab + "memcpy(dest->dims, src->dims, 32 * sizeof(struct dimension_descriptor));\n";
                body += indent + tab + "dest->n_dims = src->n_dims;\n";
                body += indent + tab + "dest->is_allocated = src->is_allocated;\n";
                body += indent + "}\n\n";
                util_funcs += body;
            }

            void array_reshape(std::string array_type, std::string shape_type,
                std::string return_type, std::string element_type,
                std::string array_type_code) {
                std::string indent(indentation_level * indentation_spaces, ' ');
                std::string tab(indentation_spaces, ' ');
                std::string array_reshape_func;
                if( util2func.find("array_reshape_" + array_type_code) == util2func.end() ) {
                    array_reshape_func = global_scope->get_unique_name("array_reshape_" + array_type_code);
                    util2func["array_reshape_" + array_type_code] = array_reshape_func;
                } else {
                    return ;
                }
                array_reshape_func = util2func["array_reshape_" + array_type_code];
                std::string signature = "static inline " + return_type + "* " + array_reshape_func + "(" +
                                        array_type + " array" + ", " + shape_type + " shape)";
                util_func_decls += indent + signature + ";\n";
                std::string body = indent + signature + " {\n";
                body += indent + tab + "int32_t n = shape->dims[0].length;\n";
                body += indent + tab + return_type + "* reshaped = (" + return_type + "*) malloc(sizeof(" + return_type + "));\n";
                body += indent + tab + "int32_t array_size_ = " + get_array_size() + "(array->dims, array->n_dims);\n";
                body += indent + tab + "int32_t shape_size_ = " + get_array_size() + "(shape->dims, shape->n_dims);\n";
                body += indent + tab + "int32_t reshaped_size = 1;\n";
                body += indent + tab + "for (int32_t i = 0; i < shape_size_; i++) {\n";
                body += indent + tab + tab + "reshaped_size *= shape->data[i];\n";
                body += indent + tab + "}\n";
                body += indent + tab + "ASSERT(array_size_ == reshaped_size);\n";
                body += indent + tab + "reshaped->data = (" + element_type + "*) malloc(sizeof(" + element_type + ")*array_size_);\n";
                body += indent + tab + "reshaped->data = (" + element_type + "*) memcpy(reshaped->data, array->data, sizeof(" + element_type + ")*array_size_);\n";
                body += indent + tab + "reshaped->n_dims = shape_size_;\n";
                body += indent + tab + "for (int32_t i = 0; i < shape_size_; i++) {\n";
                body += indent + tab + tab + "reshaped->dims[i].lower_bound = 0;\n";
                body += indent + tab + tab + "reshaped->dims[i].length = shape->data[i];\n";
                body += indent + tab + "}\n";
                body += indent + tab + "return reshaped;\n";
                body += indent + "}\n\n";
                util_funcs += body;
            }

            void array_constant(std::string return_type, std::string element_type,
                std::string array_type_code) {
                std::string indent(indentation_level * indentation_spaces, ' ');
                std::string tab(indentation_spaces, ' ');
                std::string array_const_func;
                if( util2func.find("array_constant_" + array_type_code) == util2func.end() ) {
                    array_const_func = global_scope->get_unique_name("array_constant_" + array_type_code);
                    util2func["array_constant_" + array_type_code] = array_const_func;
                } else {
                    return ;
                }
                array_const_func = util2func["array_constant_" + array_type_code];
                std::string signature = "static inline " + return_type + "* " + array_const_func + "(int32_t n, ...)";
                util_func_decls += indent + signature + ";\n";
                std::string body = indent + signature + " {\n";
                body += indent + tab + return_type + "* const_array  = (" + return_type + "*) malloc(sizeof(" + return_type + "));\n";
                body += indent + tab + "va_list ap;\n";
                body += indent + tab + "va_start(ap, n);\n";
                body += indent + tab + "const_array->data = (" + element_type + "*) malloc(sizeof(" + element_type + ")*n);\n";
                body += indent + tab + "const_array->n_dims = 1;\n";
                body += indent + tab + "const_array->dims[0].lower_bound = 0;\n";
                body += indent + tab + "const_array->dims[0].length = n;\n";
                body += indent + tab + "for (int32_t i = 0; i < n; i++) {\n";
                body += indent + tab + tab + "const_array->data[i] = va_arg(ap, " + element_type +");\n";
                body += indent + tab + "}\n";
                body += indent + tab + "va_end(ap);\n";
                body += indent + tab + "return const_array;\n";
                body += indent + "}\n\n";
                util_funcs += body;
            }

            std::string get_array_size() {
                array_size();
                return util2func["array_size"];
            }

            std::string get_array_reshape(
                std::string array_type, std::string shape_type,
                std::string return_type, std::string element_type,
                std::string array_type_code) {
                array_reshape(array_type, shape_type,
                            return_type, element_type,
                            array_type_code);
                return util2func["array_reshape_" + array_type_code];
            }

            std::string get_array_constant(std::string return_type,
                std::string element_type, std::string encoded_type) {
                array_constant(return_type, element_type, encoded_type);
                return util2func["array_constant_" + encoded_type];
            }

            std::string get_array_deepcopy(ASR::ttype_t* array_type_asr,
            std::string array_type_name, std::string array_encoded_type_name,
            std::string array_type_str) {
                array_deepcopy(array_type_asr, array_type_name,
                               array_encoded_type_name, array_type_str);
                return util2func["array_deepcopy_" + array_encoded_type_name];
            }
    };

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

    static inline std::string get_struct_type_code(ASR::Struct_t* struct_t) {
        return ASRUtils::symbol_name(struct_t->m_derived_type);
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

} // namespace CUtils


class CCPPDSUtils {
    private:

        std::map<std::string, std::string> typecodeToDStype;
        std::map<std::string, std::map<std::string, std::string>> typecodeToDSfuncs;
        std::map<std::string, std::string> compareTwoDS;
        std::map<std::string, std::string> printFuncs;
        std::map<std::string, std::string> eltypedims2arraytype;
        CUtils::CUtilFunctions* c_utils_functions;

        int indentation_level, indentation_spaces;

        std::string generated_code;
        std::string func_decls;

        SymbolTable* global_scope;
        bool is_c;
        Platform platform;

    public:

        CCPPDSUtils(bool is_c, Platform &platform): is_c{is_c}, platform{platform} {
            generated_code.clear();
            func_decls.clear();
        }

        void set_c_utils_functions(CUtils::CUtilFunctions* c_utils_functions_) {
            c_utils_functions = c_utils_functions_;
        }

        void set_indentation(int indendation_level_, int indendation_space_) {
            indentation_level = indendation_level_;
            indentation_spaces = indendation_space_;
        }

        void set_global_scope(SymbolTable* global_scope_) {
            global_scope = global_scope_;
        }

        std::string get_compare_func(ASR::ttype_t *t) {
            std::string type_code = ASRUtils::get_type_code(t, true);
            return compareTwoDS[type_code];
        }

        std::string get_print_func(ASR::ttype_t *t) {
            std::string type_code = ASRUtils::get_type_code(t, true);
            return printFuncs[type_code];
        }

        std::string get_deepcopy(ASR::ttype_t *t, std::string value, std::string target) {
            std::string result;
            switch (t->type) {
                case ASR::ttypeType::List : {
                    ASR::List_t* list_type = ASR::down_cast<ASR::List_t>(t);
                    std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
                    std::string func = typecodeToDSfuncs[list_type_code]["list_deepcopy"];
                    result = func + "(&" + value + ", &" + target + ");";
                    break;
                }
                case ASR::ttypeType::Tuple : {
                    ASR::Tuple_t* tup_type = ASR::down_cast<ASR::Tuple_t>(t);
                    std::string tup_type_code = CUtils::get_tuple_type_code(tup_type);
                    std::string func = typecodeToDSfuncs[tup_type_code]["tuple_deepcopy"];
                    result = func + "(" + value + ", &" + target + ");";
                    break;
                }
                case ASR::ttypeType::Character : {
                    if (is_c) {
                        result = "_lfortran_strcpy(&" + target + ", " + value + ");";
                    } else {
                        result = target + " = " + value  + ";";
                    }
                    break;
                }
                case ASR::ttypeType::Struct: {
                    std::string func = get_struct_deepcopy_func(t);
                    result = func + "(" + value + ", " + target + ");";
                    break;
                }
                case ASR::ttypeType::Integer:
                case ASR::ttypeType::Real:
                case ASR::ttypeType::Complex:
                case ASR::ttypeType::Logical: {
                    if( !ASRUtils::is_array(t) ) {
                        result = target + " = " + value  + ";";
                    } else {
                        if( is_c ) {
                            std::string func = get_array_deepcopy_func(t);
                            result = func + "(" + value + ", " + target + ");";
                        } else {
                            result = target + " = " + value + ";";
                        }
                    }
                    break;
                }
                default: {
                    result = target + " = " + value  + ";";
                }
            }
            return result;
        }

        std::string get_type(ASR::ttype_t *t) {
            LCOMPILERS_ASSERT(CUtils::is_non_primitive_DT(t));
            if (ASR::is_a<ASR::List_t>(*t)) {
                ASR::List_t* list_type = ASR::down_cast<ASR::List_t>(t);
                return get_list_type(list_type);
            } else if (ASR::is_a<ASR::Tuple_t>(*t)) {
                ASR::Tuple_t* tup_type = ASR::down_cast<ASR::Tuple_t>(t);
                return get_tuple_type(tup_type);
            }
            LCOMPILERS_ASSERT(false);
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
                case ASR::ttypeType::Complex: {
                    return "(%f, %f)";
                }
                case ASR::ttypeType::Pointer: {
                    if( !deref_ptr ) {
                        return "%p";
                    } else {
                        ASR::Pointer_t* type_ptr = ASR::down_cast<ASR::Pointer_t>(t);
                        return get_print_type(type_ptr->m_type, false);
                    }
                }
                case ASR::ttypeType::Enum: {
                    ASR::ttype_t* enum_underlying_type = ASRUtils::get_contained_type(t);
                    return get_print_type(enum_underlying_type, deref_ptr);
                }
                case ASR::ttypeType::Const: {
                    ASR::ttype_t* const_underlying_type = ASRUtils::get_contained_type(t);
                    return get_print_type(const_underlying_type, deref_ptr);
                }
                default : throw LCompilersException("Not implemented");
            }
        }

        std::string get_array_type(std::string type_name, std::string encoded_type_name,
                               std::string& array_types_decls, bool make_ptr=true,
                               bool create_if_not_present=true) {
            if( eltypedims2arraytype.find(encoded_type_name) != eltypedims2arraytype.end() ) {
                if( make_ptr ) {
                    return eltypedims2arraytype[encoded_type_name] + "*";
                } else {
                    return eltypedims2arraytype[encoded_type_name];
                }
            }

            LCOMPILERS_ASSERT(create_if_not_present);

            std::string struct_name;
            std::string new_array_type;
            struct_name = "struct " + encoded_type_name;
            std::string array_data = format_type_c("*", type_name, "data", false, false);
            new_array_type = struct_name + "\n{\n    " + array_data +
                                ";\n    struct dimension_descriptor dims[32];\n" +
                                "    int32_t n_dims;\n"
                                "    bool is_allocated;\n};\n";
            if( make_ptr ) {
                type_name = struct_name + "*";
            }
            eltypedims2arraytype[encoded_type_name] = struct_name;
            array_types_decls += "\n" + new_array_type + "\n";
            return type_name;
        }

        std::string get_list_type(ASR::List_t* list_type) {
            std::string list_element_type = CUtils::get_c_type_from_ttype_t(list_type->m_type);
            if (CUtils::is_non_primitive_DT(list_type->m_type)) {
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
            generate_print_funcs((ASR::ttype_t *)list_type);
            list_init(list_struct_type, list_type_code, list_element_type);
            list_deepcopy(list_struct_type, list_type_code, list_element_type, list_type->m_type);
            resize_if_needed(list_struct_type, list_type_code, list_element_type);
            list_append(list_struct_type, list_type_code, list_element_type, list_type->m_type);
            list_insert(list_struct_type, list_type_code, list_element_type, list_type->m_type);
            list_find_item_position(list_struct_type, list_type_code, list_element_type, list_type->m_type);
            list_remove(list_struct_type, list_type_code, list_element_type, list_type->m_type);
            list_clear(list_struct_type, list_type_code, list_element_type);
            list_concat(list_struct_type, list_type_code, list_element_type, list_type->m_type);
            list_section(list_struct_type, list_type_code);
            return list_struct_type;
        }

        std::string get_list_deepcopy_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecodeToDSfuncs[list_type_code]["list_deepcopy"];
        }

        std::string get_struct_deepcopy_func(ASR::ttype_t* struct_type_asr) {
            ASR::Struct_t* struct_type = ASR::down_cast<ASR::Struct_t>(struct_type_asr);
            std::string struct_type_code = CUtils::get_struct_type_code(struct_type);
            if( typecodeToDSfuncs.find(struct_type_code) == typecodeToDSfuncs.end() ) {
                struct_deepcopy(struct_type_asr);
            }
            return typecodeToDSfuncs[struct_type_code]["struct_deepcopy"];
        }

        std::string get_array_deepcopy_func(ASR::ttype_t* array_type_asr) {
            LCOMPILERS_ASSERT(is_c);
            std::string array_type_name = CUtils::get_c_type_from_ttype_t(array_type_asr);
            std::string array_encoded_type_name = ASRUtils::get_type_code(array_type_asr, true, false, false);
            std::string array_types_decls = "";
            std::string array_type_str = get_array_type(array_type_name, array_encoded_type_name,
                                                            array_types_decls, true, false);
            return c_utils_functions->get_array_deepcopy(array_type_asr, array_type_name,
                array_encoded_type_name, array_type_str);
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

        std::string get_list_section_func(ASR::List_t* list_type) {
            std::string list_type_code = ASRUtils::get_type_code(list_type->m_type, true);
            return typecodeToDSfuncs[list_type_code]["list_section"];
        }

        std::string get_generated_code() {
            return generated_code;
        }

        std::string get_func_decls() {
            return func_decls;
        }

        void generate_print_funcs(ASR::ttype_t *t) {
            std::string type_code = ASRUtils::get_type_code(t, true);
            if (printFuncs.find(type_code) != printFuncs.end()) {
                return;
            }
            std::string element_type = CUtils::get_c_type_from_ttype_t(t);
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string p_func = global_scope->get_unique_name("print_" + type_code);
            printFuncs[type_code] = p_func;
            std::string tmp_gen = "";
            std::string signature = "void " + p_func + "(" + element_type + " a)";
            func_decls += indent + "inline " + signature + ";\n";
            signature = indent + signature;
            if (ASR::is_a<ASR::List_t>(*t)) {
                ASR::ttype_t *tt = ASR::down_cast<ASR::List_t>(t)->m_type;
                generate_print_funcs(tt);
                std::string ele_func = printFuncs[ASRUtils::get_type_code(tt, true)];
                tmp_gen += indent + signature + " {\n";
                tmp_gen += indent + tab + "printf(\"[\");\n";
                tmp_gen += indent + tab + "for (int i=0; i<a.current_end_point; i++) {\n";
                tmp_gen += indent + tab + tab + ele_func + "(a.data[i]);\n";
                tmp_gen += indent + tab + tab + "if (i+1!=a.current_end_point)\n";
                tmp_gen += indent + tab + tab + tab + "printf(\", \");\n";
                tmp_gen += indent + tab + "}\n";
                tmp_gen += indent + tab + "printf(\"]\");\n";
            } else if (ASR::is_a<ASR::Tuple_t>(*t)) {
                ASR::Tuple_t *tt = ASR::down_cast<ASR::Tuple_t>(t);
                tmp_gen += indent + signature + " {\n";
                tmp_gen += indent + tab + "printf(\"(\");\n";
                for (size_t i=0; i<tt->n_type; i++) {
                    generate_print_funcs(tt->m_type[i]);
                    std::string ele_func = printFuncs[ASRUtils::get_type_code(tt->m_type[i], true)];
                    std::string num = std::to_string(i);
                    tmp_gen += indent + tab + ele_func + "(a.element_" + num + ");\n";
                    if (i+1 != tt->n_type)
                        tmp_gen += indent + tab + "printf(\", \");\n";
                }
                tmp_gen += indent + tab + "printf(\")\");\n";
            } else if (ASR::is_a<ASR::Complex_t>(*t)) {
                tmp_gen += indent + signature + " {\n";
                std::string print_type = get_print_type(t, false);
                tmp_gen += indent + tab + "printf(\"" + print_type + "\", creal(a), cimag(a));\n";
            } else if (ASR::is_a<ASR::Character_t>(*t)) {
                tmp_gen += indent + signature + " {\n";
                std::string print_type = get_print_type(t, false);
                tmp_gen += indent + tab + "printf(\"'" + print_type + "'\", a);\n";
            } else {
                tmp_gen += indent + signature + " {\n";
                std::string print_type = get_print_type(t, false);
                tmp_gen += indent + tab + "printf(\"" + print_type + "\", a);\n";
            }
            tmp_gen += indent + "}\n\n";
            generated_code += tmp_gen;
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

        void struct_deepcopy(ASR::ttype_t* struct_type_asr) {
            ASR::Struct_t* struct_type = ASR::down_cast<ASR::Struct_t>(struct_type_asr);
            ASR::StructType_t* struct_type_t = ASR::down_cast<ASR::StructType_t>(
                ASRUtils::symbol_get_past_external(struct_type->m_derived_type));
            std::string struct_type_code = CUtils::get_struct_type_code(struct_type);
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string struct_dc_func = global_scope->get_unique_name("struct_deepcopy_" + struct_type_code);
            typecodeToDSfuncs[struct_type_code]["struct_deepcopy"] = struct_dc_func;
            std::string struct_type_str = CUtils::get_c_type_from_ttype_t(struct_type_asr);
            std::string signature = "void " + struct_dc_func + "("
                                + struct_type_str + "* src, "
                                + struct_type_str + "* dest)";
            func_decls += "inline " + signature + ";\n";
            generated_code += indent + signature + " {\n";
            for( auto item: struct_type_t->m_symtab->get_scope() ) {
                ASR::ttype_t* member_type_asr = ASRUtils::symbol_type(item.second);
                if( CUtils::is_non_primitive_DT(member_type_asr) ||
                    ASR::is_a<ASR::Character_t>(*member_type_asr) ) {
                    generated_code += indent + tab + get_deepcopy(member_type_asr, "&(src->" + item.first + ")",
                                 "&(dest->" + item.first + ")") + ";\n";
                } else if( ASRUtils::is_array(member_type_asr) ) {
                    ASR::dimension_t* m_dims = nullptr;
                    size_t n_dims = ASRUtils::extract_dimensions_from_ttype(member_type_asr, m_dims);
                    if( ASRUtils::is_fixed_size_array(m_dims, n_dims) ) {
                        std::string array_size = std::to_string(ASRUtils::get_fixed_size_of_array(m_dims, n_dims));
                        array_size += "*sizeof(" + CUtils::get_c_type_from_ttype_t(member_type_asr) + ")";
                        generated_code += indent + tab + "memcpy(dest->" + item.first + ", src->" + item.first +
                                            ", " + array_size + ");\n";
                    } else {
                        generated_code += indent + tab + get_deepcopy(member_type_asr, "src->" + item.first,
                                            "dest->" + item.first) + ";\n";
                    }
                } else {
                    generated_code += indent + tab + "dest->" + item.first + " = " + " src->" + item.first + ";\n";
                }
            }
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
                LCOMPILERS_ASSERT(deep_copy_func.size() > 0);
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
                LCOMPILERS_ASSERT(deep_copy_func.size() > 0);
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
                generated_code += indent + tab + "x->data[x->current_end_point] = NULL;\n";
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
                generated_code += indent + tab + "x->data[pos] = NULL;\n";
            }
            generated_code += indent + tab + get_deepcopy(m_type, "element", "x->data[pos]") + "\n";
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

        void list_section(std::string list_struct_type, std::string list_type_code) {
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string tab(indentation_spaces, ' ');
            std::string list_section_func = global_scope->get_unique_name("list_section_" + list_type_code);
            typecodeToDSfuncs[list_type_code]["list_section"] = list_section_func;
            std::string signature = list_struct_type + "* " + list_section_func + "("
                                + list_struct_type + "* x, "
                                + "int32_t idx1, int32_t idx2, int32_t step, bool i1_present, bool i2_present)";
            func_decls += "inline " + signature + ";\n";
            std::string tmp_gen = "";
            tmp_gen += indent + signature + " {\n";
            tmp_gen += indent + tab + "int s_len = x->current_end_point;\n";
            tmp_gen += indent + tab + "if (step == 0) {\n";
            tmp_gen += indent + tab + tab + "printf(\"slice step cannot be zero\");\n";
            tmp_gen += indent + tab + tab + "exit(1);\n" + tab + "}\n";
            tmp_gen += indent + tab + "idx1 = idx1 < 0 ? idx1 + s_len : idx1;\n";
            tmp_gen += indent + tab + "idx2 = idx2 < 0 ? idx2 + s_len : idx2;\n";
            tmp_gen += indent + tab + "idx1 = i1_present ? idx1 : (step > 0 ? 0 : s_len-1);\n";
            tmp_gen += indent + tab + "idx2 = i2_present ? idx2 : (step > 0 ? s_len : -1);\n";
            tmp_gen += indent + tab + "idx2 = step > 0 ? (idx2 > s_len ? s_len : idx2) : idx2;\n";
            tmp_gen += indent + tab + "idx1 = step < 0 ? (idx1 >= s_len ? s_len-1 : idx1) : idx1;\n";
            tmp_gen += indent + tab + list_struct_type + " *__tmp = (" +
                    list_struct_type + "*) malloc(sizeof(" + list_struct_type + "));\n";
            std::string list_init_func = typecodeToDSfuncs[list_type_code]["list_init"];
            tmp_gen += indent + tab + list_init_func + "(__tmp, 4);\n";
            tmp_gen += indent + tab + "int s_i = idx1;\n";
            tmp_gen += indent + tab + "while((step > 0 && s_i >= idx1 && s_i < idx2) ||\n";
            tmp_gen += indent + tab + "    (step < 0 && s_i <= idx1 && s_i > idx2)) {\n";
            std::string list_append_func  = typecodeToDSfuncs[list_type_code]["list_append"];
            tmp_gen += indent + tab + list_append_func + "(__tmp, x->data[s_i]);\n";
            tmp_gen += indent + tab + "s_i+=step;\n" + indent + tab + "}\n";
            tmp_gen += indent + tab + "return __tmp;\n}\n\n";
            generated_code += tmp_gen;
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
                if (CUtils::is_non_primitive_DT(tuple_type->m_type[i])) {
                    // Make sure the nested types work
                    get_type(tuple_type->m_type[i]);
                }
                tmp_gen += indent + tab + \
                    CUtils::get_c_type_from_ttype_t(tuple_type->m_type[i]) + " element_" + std::to_string(i) + ";\n";
            }
            tmp_gen += indent + "};\n\n";
            func_decls += tmp_gen;
            generate_compare_funcs((ASR::ttype_t *)tuple_type);
            generate_print_funcs((ASR::ttype_t *)tuple_type);
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
                                "NULL;\n";
                }
                tmp_gen += indent + tab + get_deepcopy(t->m_type[i], "src.element_" + n,
                                "dest->element_" + n) + "\n";
            }
            tmp_gen += indent + tab + "dest->length = src.length;\n";
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

} // namespace LCompilers

#endif // LFORTRAN_C_UTILS_H
