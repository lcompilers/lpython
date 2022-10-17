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

    std::string get_c_type_from_ttype_t(ASR::ttype_t* t, bool is_c=true) {
        int kind = ASRUtils::extract_kind_from_ttype_t(t);
        std::string type_src = "";
        switch( t->type ) {
            case ASR::ttypeType::Integer: {
                type_src = "int" + std::to_string(kind * 8) + "_t";
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
                std::string list_type_c = list_api->get_list_type(list_type, list_element_type);
                type_src = list_type_c;
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
}

#endif // LFORTRAN_C_UTILS_H
