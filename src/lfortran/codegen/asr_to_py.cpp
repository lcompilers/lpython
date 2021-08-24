#include <iostream>
#include <memory>

#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/string_utils.h>


/*
 *
 * This back-end generates wrapper code that allows Fortran to automatically be called from Python.
 * It also generates a C header file, so I suppose it indirectly generates C wrappers as well.
 * Currently, it outputs Cython, rather than the Python C API directly - much easier to implement.
 * The actual output files are:
 *  - a .h file, containing C-language function declarations *  - a .pxd file, basically containing the same information as the .h file, but in Cython's format.
 *  - a .pyx file, which is a Cython file that includes the actual python-callable wrapper functions.
 *
 *  Currently, this back-end only wraps functions that are marked "bind (c)" in the Fortran source.
 *  At some later point we will offer the functionality to generate bind (c) wrapper functions for
 *  normal Fortran subprograms, but for now, we don't offer this functionality.
 *
 *  --- H. Snyder, Aug 2021
 * 
 * */


/* 
 * The following technique is called X-macros, if you don't recognize it.
 * You should be able to look it up under that name for an explanation.
 */

#define CTYPELIST \
    _X(ASR::Integer_t, 1, "int8_t" ) \
    _X(ASR::Integer_t, 2, "int16_t" ) \
    _X(ASR::Integer_t, 4, "int32_t" ) \
    _X(ASR::Integer_t, 8, "int64_t" ) \
    \
    _X(ASR::Real_t,    4, "float" ) \
    _X(ASR::Real_t,    8, "double" ) \
    \
    _X(ASR::Complex_t, 4, "float _Complex" ) \
    _X(ASR::Complex_t, 8, "double _Complex" ) \
    \
    _X(ASR::Logical_t, 1,   "_Bool" ) \
    _X(ASR::Character_t, 1, "char" ) 


/*
 * We will use this list instead, once the ASR has the symbolic kind
 */ 

#define CTYPELIST_FUTURE \
    _X(ASR::Integer_t, c_int,           "int"   ) \
    _X(ASR::Integer_t, c_short,         "short" ) \
    _X(ASR::Integer_t, c_long,          "long"  ) \
    _X(ASR::Integer_t, c_long_long,     "long long" ) \
    _X(ASR::Integer_t, c_signed_char,   "signed char" ) \
    _X(ASR::Integer_t, c_size_t,        "size_t" ) \
    \
    _X(ASR::Integer_t, c_int8_t,        "int8_t" ) \
    _X(ASR::Integer_t, c_int16_t,       "int16_t" ) \
    _X(ASR::Integer_t, c_int32_t,       "int32_t" ) \
    _X(ASR::Integer_t, c_int64_t,       "int64_t" ) \
    \
    _X(ASR::Integer_t, c_int_least8_t,  "int_least8_t" ) \
    _X(ASR::Integer_t, c_int_least16_t, "int_least16_t" ) \
    _X(ASR::Integer_t, c_int_least32_t, "int_least32_t" ) \
    _X(ASR::Integer_t, c_int_least64_t, "int_least64_t" ) \
    \
    _X(ASR::Integer_t, c_int_fast8_t,   "int_fast8_t" ) \
    _X(ASR::Integer_t, c_int_fast16_t,  "int_fast16_t" ) \
    _X(ASR::Integer_t, c_int_fast32_t,  "int_fast32_t" ) \
    _X(ASR::Integer_t, c_int_fast64_t,  "int_fast64_t" ) \
    \
    _X(ASR::Integer_t, c_intmax_t,      "intmax_t" ) \
    _X(ASR::Integer_t, c_intptr_t,      "intptr_t" ) \
    _X(ASR::Integer_t, c_ptrdiff_t,     "ptrdiff_t" ) \
    \
    _X(ASR::Real_t,    c_float,         "float" ) \
    _X(ASR::Real_t,    c_double,        "double" ) \
    _X(ASR::Real_t,    c_long_double,   "long double" ) \
    \
    _X(ASR::Complex_t, c_float_complex, "float _Complex" ) \
    _X(ASR::Complex_t, c_double_complex, "double _Complex" ) \
    _X(ASR::Complex_t, c_long_double_complex, "long double _Complex" ) \
    \
    _X(ASR::Logical_t, c_bool,          "_Bool" ) \
    _X(ASR::Character_t, c_char,        "char" ) 


namespace LFortran {

using ASR::is_a;
using ASR::down_cast;
using ASR::down_cast2;

class ASRToPyVisitor : public ASR::BaseVisitor<ASRToPyVisitor>
{
public:
    // These store the strings that will become the contents of the generated .h, .pxd, .pyx files
    std::string chdr, pxd, pyx;

    // Stores the name of the current module being visited.
    // Value is meaningless after calling ASRToPyVisitor::visit_asr.
    std::string cur_module;

    // Are we assuming arrays to be in C order (row-major)? If not, assume Fortran order (column-major).
    bool c_order;

    // What's the file name of the C header file we're going to generate? (needed for the .pxd)
    std::string chdr_filename;

    ASRToPyVisitor(bool c_order_, std::string chdr_filename_) : c_order(c_order_), chdr_filename(chdr_filename_) {}

    std::string convert_variable_decl(ASR::Variable_t *arg)
    {
        std::string ret = "";
        bool has_dims = false;

        // This is much more "C-style" than C++ style, but
        // I'm using the type list table I generated above to generate a sequence of if-blocks
        #define _X(T,NUM,STR) \
        if ( is_a<T>(*arg->m_type) && (down_cast<T>(arg->m_type)->m_kind == NUM) ) { \
            ret = STR; \
            if(down_cast<T>(arg->m_type)->n_dims > 0) has_dims=true; \
        } else

        CTYPELIST {};
        #undef _X

        // If none of the generated if-blocks got triggered, then the ret string will be empty
        if(ret.empty())
            throw CodeGenError("Type not supported");

        // The argument is a pointer, unless it is not an array AND has the value type
        // If the argument is not a pointer, but is intent(in), it should be a ptr-to-const.
        if (has_dims || !arg->m_value_attr) {
            ret += " *";
            if (ASR::intentType::In == arg->m_intent) ret = "const " + ret;
        }


        ret += " ";
        ret += arg->m_name;

        return ret;
    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LFORTRAN_ASSERT(x.n_items == 0);

        std::string chdr_tmp ;
        std::string pxd_tmp  ;
        std::string pyx_tmp  ;

        chdr_tmp =  "// This file was automatically generated by the LFortran compiler.\n";
        chdr_tmp += "// Editing by hand is discouraged.\n\n";
        chdr_tmp += "#include <stdint.h>\n\n";

        pxd_tmp =   "# This file was automatically generated by the LFortran compiler.\n";
        pxd_tmp +=  "# Editing by hand is discouraged.\n\n";
        pxd_tmp +=  "from libc.stdint cimport int8_t, int16_t, int32_t, int64_t\n";
        pxd_tmp +=  "cdef extern from \"" + chdr_filename + "\":\n";

        // Process loose procedures first
        for (auto &item : x.m_global_scope->scope) {
            if (is_a<ASR::Function_t>(*item.second)
                    || is_a<ASR::Subroutine_t>(*item.second)) {
                visit_symbol(*item.second);

                chdr_tmp += chdr;
                pxd_tmp  += pxd;
                pyx_tmp  += pyx;
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

                chdr_tmp += chdr;
                pxd_tmp  += pxd;
                pyx_tmp  += pyx;
            }
        }

        // There's no need to process the `program` statement, which 
        // is the only other thing that can appear at the top level.

        chdr = chdr_tmp;
        pyx  = pyx_tmp;
        pxd  = pxd_tmp;
    }

    void visit_Module(const ASR::Module_t &x) {
        cur_module = x.m_name;

        // Generate code for nested subroutines and functions first:
        std::string chdr_tmp ;
        std::string pxd_tmp  ;
        std::string pyx_tmp  ;

        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Subroutine_t>(*item.second)) {
                ASR::Subroutine_t *s = ASR::down_cast<ASR::Subroutine_t>(item.second);
                visit_Subroutine(*s);

                chdr_tmp += chdr;
                pxd_tmp  += pxd;
                pyx_tmp  += pyx;
            }
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);

                chdr_tmp += chdr;
                pxd_tmp  += pxd;
                pyx_tmp  += pyx;
            }
        }


        chdr = chdr_tmp;
        pyx  = pyx_tmp;
        pxd  = pxd_tmp;

        cur_module.clear();
    }

    void visit_Subroutine(const ASR::Subroutine_t &x) {

        // Only process bind(c) subprograms for now
        if (x.m_abi != ASR::abiType::BindC) return;

        // Return type and function name
        bool bindc_name_not_given = x.m_bindc_name == NULL || !strcmp("",x.m_bindc_name);
        std::string effective_name = bindc_name_not_given ? x.m_name : x.m_bindc_name;

        chdr = "void " + effective_name + " (";

        // Arguments
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(x.m_args[i]);
            LFORTRAN_ASSERT(LFortran::ASRUtils::is_arg_dummy(arg->m_intent));

            chdr += convert_variable_decl(arg);
            if (i < x.n_args-1) chdr += ", ";
        }
        chdr += ")";
        pxd = "    " + chdr + "\n";
        chdr += ";\n" ;
    }


    void visit_Function(const ASR::Function_t &x) {

        // Only process bind(c) subprograms for now
        if (x.m_abi != ASR::abiType::BindC) return;

        // Return type and function name
        bool bindc_name_not_given = x.m_bindc_name == NULL || !strcmp("",x.m_bindc_name);
        std::string effective_name = bindc_name_not_given ? x.m_name : x.m_bindc_name;

        ASR::Variable_t *return_var = LFortran::ASRUtils::EXPR2VAR(x.m_return_var);
        #define _X(T,NUM,STR) \
        if ( is_a<T>(*return_var->m_type) && (down_cast<T>(return_var->m_type)->m_kind == NUM) ) { \
            chdr = STR; \
        } else

        CTYPELIST {
            throw CodeGenError("Return type not supported");
        };
        #undef _X

        
        chdr += " " + effective_name + " (";

        // Arguments
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(x.m_args[i]);
            LFORTRAN_ASSERT(LFortran::ASRUtils::is_arg_dummy(arg->m_intent));

            chdr += convert_variable_decl(arg);
            if (i < x.n_args-1) chdr += ", ";
        }
        chdr += ")";
        pxd = "    " + chdr + "\n";
        chdr += ";\n" ;
    }

};

std::tuple<std::string, std::string, std::string> asr_to_py(ASR::TranslationUnit_t &asr, bool c_order, std::string chdr_filename)
{
    ASRToPyVisitor v (c_order, chdr_filename);
    v.visit_asr((ASR::asr_t &)asr);

    return std::make_tuple(v.chdr, v.pxd, v.pyx);
}

} // namespace LFortran
