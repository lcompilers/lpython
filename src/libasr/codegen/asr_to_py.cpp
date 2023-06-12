#include <iostream>
#include <memory>
#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/string_utils.h>


/*
 *
 * This back-end generates wrapper code that allows Fortran to automatically be called from Python.
 * It also generates a C header file, so I suppose it indirectly generates C wrappers as well.
 * Currently, it outputs Cython, rather than the Python C API directly - much easier to implement.
 * The actual output files are:
 *  - a .h file, containing C-language function declarations *
 *  - a .pxd file, basically containing the same information as the .h file, but in Cython's format.
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
 * We will use this list instead, once the ASR has symbolic kind information.

#define CTYPELIST_FUTURE \
    _X(ASR::Integer_t, "c_int",           "int"   ) \
    _X(ASR::Integer_t, "c_short",         "short" ) \
    _X(ASR::Integer_t, "c_long",          "long"  ) \
    _X(ASR::Integer_t, "c_long_long",     "long long" ) \
    _X(ASR::Integer_t, "c_signed_char",   "signed char" ) \
    _X(ASR::Integer_t, "c_size_t",        "size_t" ) \
    \
    _X(ASR::Integer_t, "c_int8_t",        "int8_t" ) \
    _X(ASR::Integer_t, "c_int16_t",       "int16_t" ) \
    _X(ASR::Integer_t, "c_int32_t",       "int32_t" ) \
    _X(ASR::Integer_t, "c_int64_t",       "int64_t" ) \
    \
    _X(ASR::Integer_t, "c_int_least8_t",  "int_least8_t" ) \
    _X(ASR::Integer_t, "c_int_least16_t", "int_least16_t" ) \
    _X(ASR::Integer_t, "c_int_least32_t", "int_least32_t" ) \
    _X(ASR::Integer_t, "c_int_least64_t", "int_least64_t" ) \
    \
    _X(ASR::Integer_t, "c_int_fast8_t",   "int_fast8_t" ) \
    _X(ASR::Integer_t, "c_int_fast16_t",  "int_fast16_t" ) \
    _X(ASR::Integer_t, "c_int_fast32_t",  "int_fast32_t" ) \
    _X(ASR::Integer_t, "c_int_fast64_t",  "int_fast64_t" ) \
    \
    _X(ASR::Integer_t, "c_intmax_t",      "intmax_t" ) \
    _X(ASR::Integer_t, "c_intptr_t",      "intptr_t" ) \
    _X(ASR::Integer_t, "c_ptrdiff_t",     "ptrdiff_t" ) \
    \
    _X(ASR::Real_t,    "c_float",         "float" ) \
    _X(ASR::Real_t,    "c_double",        "double" ) \
    _X(ASR::Real_t,    "c_long_double",   "long double" ) \
    \
    _X(ASR::Complex_t, "c_float_complex", "float _Complex" ) \
    _X(ASR::Complex_t, "c_double_complex", "double _Complex" ) \
    _X(ASR::Complex_t, "c_long_double_complex", "long double _Complex" ) \
    \
    _X(ASR::Logical_t, "c_bool",          "_Bool" ) \
    _X(ASR::Character_t, "c_char",        "char" )
 */

namespace LCompilers {

namespace {

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
    };

}

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
    // What's the name of the pxd file (minus the .pxd extension)
    std::string pxdf;

    ASRToPyVisitor(bool c_order_, std::string chdr_filename_) :
            c_order(c_order_),
            chdr_filename(chdr_filename_),
            pxdf(chdr_filename_)
    {
        // we need to get get the pxd filename (minus extension), so we can import it in the pyx file
        // knock off ".h" from the c header filename
        pxdf.erase(--pxdf.end());
        pxdf.erase(--pxdf.end());
        // this is an unfortunate hack, but we have to add something so that the pxd and pyx filenames
        // are different (beyond just their extensions). If we don't, the cython emits a warning.
        // TODO we definitely need to change this somehow because right now this "append _pxd" trick
        // exists in two places (bin/lfortran.cpp, and here), which could easily cause breakage.
        pxdf += "_pxd";
    }

    std::tuple<std::string, std::string, std::string, std::string, std::string>
    helper_visit_arguments(size_t n_args, ASR::expr_t ** args)
    {

        struct arg_info {
            ASR::Variable_t*  asr_obj;
            std::string       ctype;
            int               ndims;

            std::vector<std::string> ubound_varnames;
            std::vector<std::pair<std::string,int> > i_am_ubound_of;
        };

        std::vector<arg_info> arg_infos;


        /* get_arg_infos */ for (size_t i=0; i<n_args; i++) {

            ASR::Variable_t *arg = ASRUtils::EXPR2VAR(args[i]);
            LCOMPILERS_ASSERT(ASRUtils::is_arg_dummy(arg->m_intent));

            // TODO add support for (or emit error on) assumed-shape arrays
            // TODO add support for interoperable derived types

            arg_info this_arg_info;

            const char * errmsg1 = "pywrap does not yet support array dummy arguments with lower bounds other than 1.";
            const char * errmsg2 = "pywrap can only generate wrappers for array dummy arguments "
                                   "if the upper bound is a constant integer, or another (scalar) dummy argument.";

            // Generate a sequence of if-blocks to determine the type, using the type list defined above
            #define _X(ASR_TYPE, KIND, CTYPE_STR) \
            if ( is_a<ASR_TYPE>(*ASRUtils::type_get_past_array(arg->m_type)) && \
                (down_cast<ASR_TYPE>(arg->m_type)->m_kind == KIND) ) { \
                ASR::dimension_t* m_dims = nullptr; \
                size_t n_dims = ASRUtils::extract_dimensions_from_ttype(arg->m_type, m_dims); \
                this_arg_info.asr_obj = arg;                                                       \
                this_arg_info.ctype   = CTYPE_STR;                                                 \
                this_arg_info.ndims   = n_dims;                                                    \
                for (int j = 0; j < this_arg_info.ndims; j++) {                                    \
                    auto lbound_ptr = m_dims[j].m_start;                                           \
                    if (!is_a<ASR::IntegerConstant_t>(*lbound_ptr)) {                              \
                        throw CodeGenError(errmsg1);                                               \
                    }                                                                              \
                    if (down_cast<ASR::IntegerConstant_t>(lbound_ptr)->m_n != 1) {                 \
                        throw CodeGenError(errmsg1);                                               \
                    }                                                                              \
                    if (is_a<ASR::Var_t>(*m_dims[j].m_length)) {                                   \
                        ASR::Variable_t *dimvar = ASRUtils::EXPR2VAR(m_dims[j].m_length);          \
                        this_arg_info.ubound_varnames.push_back(dimvar->m_name);                   \
                    } else if (!is_a<ASR::IntegerConstant_t>(*lbound_ptr)) {                       \
                        throw CodeGenError(errmsg2);                                               \
                    }                                                                              \
                }                                                                                  \
            } else

            CTYPELIST {
                // We end up in this block if none of the above if-blocks were triggered
                throw CodeGenError("Type not supported");
            };
            #undef _X

            arg_infos.push_back(this_arg_info);

        } /* get_arg_infos */


        /* mark_array_bound_vars */ for(auto arg_iter = arg_infos.begin(); arg_iter != arg_infos.end(); arg_iter++) {

            /* some dummy args might just be the sizes of other dummy args, e.g.:

            subroutine foo(n,x)
                integer :: n, x(n)
            end subroutine

            We don't actually want `n` in the python wrapper's arguments - the Python programmer
            shouldn't need to explicitly pass sizes. From the get_arg_infos block, we already have
            the mapping from `x` to `n`, but we also need the opposite - we need be able to look at
            `n` and know that it's related to `x`. So let's do a pass over the arg_infos list and
            assemble that information.

            */

            for (auto bound_iter =  arg_iter->ubound_varnames.begin();
                      bound_iter != arg_iter->ubound_varnames.end();
                      bound_iter++ ) {
                for (unsigned int j = 0; j < arg_infos.size(); j++) {
                    if (0 == std::string(arg_infos[j].asr_obj->m_name).compare(*bound_iter)) {
                        arg_infos[j].i_am_ubound_of.push_back(std::make_pair(arg_iter->asr_obj->m_name, j));
                    }
                }
            }

        } /* mark_array_bound_vars */


        /* apply_c_order */ if(c_order) {

            for(auto arg_iter = arg_infos.begin(); arg_iter != arg_infos.end(); arg_iter++) {

                for (auto bound =  arg_iter->i_am_ubound_of.begin();
                          bound != arg_iter->i_am_ubound_of.end();
                          bound++) {
                    auto x = std::make_pair(bound->first, - bound->second -1);
                    bound->swap(x);
                }

            }

        } /* apply_c_order */

        std::string c, cyargs, fargs, pyxbody, return_statement;

        /* build_return_strings */ for(auto it = arg_infos.begin(); it != arg_infos.end(); it++) {

            std::string c_wip, cyargs_wip, fargs_wip, rtn_wip;

            c_wip = it->ctype;

            // Get type for cython wrapper argument, from the C type name
            if (it->ndims > 0) {
                std::string mode     = c_order   ? ", mode=\"c\"" : ", mode=\"fortran\"";
                std::string strndims = it->ndims > 1 ? ", ndim="+std::to_string(it->ndims) : "";
                cyargs_wip += "ndarray[" + it->ctype + strndims + mode + "]";
            } else {
                cyargs_wip += it->ctype;
            }

            // Fortran defaults to pass-by-reference, so the C argument is a pointer, unless
            // it is not an array AND it has the value type.
            if (it->ndims > 0 || !it->asr_obj->m_value_attr) {
                c_wip += " *";
                fargs_wip = "&";
                // If the argument is intent(in) and a pointer, it should be a ptr-to-const.
                if (ASR::intentType::In == it->asr_obj->m_intent) c_wip = "const " + c_wip;
            }

            c_wip += " ";
            c_wip += it->asr_obj->m_name;

            cyargs_wip += " ";
            cyargs_wip += it->asr_obj->m_name;

            fargs_wip += it->asr_obj->m_name;
            if(it->ndims > 0) {
                fargs_wip += "[0";
                for(int h = 1; h < it->ndims; h++)
                    fargs_wip += ",0";
                fargs_wip += "]";
            }

            if (ASR::intentType::Out == it->asr_obj->m_intent ||
                ASR::intentType::InOut == it->asr_obj->m_intent) {
                rtn_wip = it->asr_obj->m_name;
            }


            if(!it->i_am_ubound_of.empty()) {
                cyargs_wip.clear();
                auto& i_am_ubound_of = it->i_am_ubound_of[0];
                pyxbody += "    cdef " + it->ctype + " ";
                pyxbody += it->asr_obj->m_name;
                pyxbody += " = ";
                pyxbody += i_am_ubound_of.first + ".shape[" + std::to_string(i_am_ubound_of.second) + "]\n";
                for(unsigned int k = 1; k < it->i_am_ubound_of.size(); k++) {
                    auto& i_am_ubound_of_k = it->i_am_ubound_of[k];
                    pyxbody += "    assert(" + i_am_ubound_of_k.first + ".shape[" + std::to_string(i_am_ubound_of_k.second) + "] == "
                                             + i_am_ubound_of.first   + ".shape[" + std::to_string(i_am_ubound_of.second)   + "])\n";
                }
            }

            if(!c.empty()       && !c_wip.empty())      c += ", ";
            if(!fargs.empty()   && !fargs_wip.empty())  fargs += ", ";
            if(!cyargs.empty()  && !cyargs_wip.empty()) cyargs += ", ";
            if(!return_statement.empty() && !rtn_wip.empty()) return_statement += ", ";

            c      += c_wip;
            fargs  += fargs_wip;
            cyargs += cyargs_wip;
            return_statement += rtn_wip;


        } /* build_return_strings */

        return std::make_tuple(c, cyargs, fargs, pyxbody, return_statement);
    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LCOMPILERS_ASSERT(x.n_items == 0);

        std::string chdr_tmp ;
        std::string pxd_tmp  ;
        std::string pyx_tmp  ;

        chdr_tmp =  "// This file was automatically generated by the LCompilers compiler.\n";
        chdr_tmp += "// Editing by hand is discouraged.\n\n";
        chdr_tmp += "#include <stdint.h>\n\n";

        pxd_tmp =   "# This file was automatically generated by the LCompilers compiler.\n";
        pxd_tmp +=  "# Editing by hand is discouraged.\n\n";
        pxd_tmp +=  "from libc.stdint cimport int8_t, int16_t, int32_t, int64_t\n";
        pxd_tmp +=  "cdef extern from \"" + chdr_filename + "\":\n";


        pyx_tmp =  "# This file was automatically generated by the LCompilers compiler.\n";
        pyx_tmp += "# Editing by hand is discouraged.\n\n";
        pyx_tmp += "from numpy cimport import_array, ndarray, int8_t, int16_t, int32_t, int64_t\n";
        pyx_tmp += "from numpy import empty, int8, int16, int32, int64\n";
        pyx_tmp += "cimport " + pxdf + " \n\n";

        // Process loose procedures first
        for (auto &item : x.m_global_scope->get_scope()) {
            if (is_a<ASR::Function_t>(*item.second)) {
                visit_symbol(*item.second);

                chdr_tmp += chdr;
                pxd_tmp  += pxd;
                pyx_tmp  += pyx;
            }
        }

        // Then do all the modules in the right order
        std::vector<std::string> build_order
            = ASRUtils::determine_module_dependencies(x);
        for (auto &item : build_order) {
            LCOMPILERS_ASSERT(x.m_global_scope->get_scope().find(item)
                    != x.m_global_scope->get_scope().end());
            if (!startswith(item, "lfortran_intrinsic")) {
                ASR::symbol_t *mod = x.m_global_scope->get_symbol(item);
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

        for (auto &item : x.m_symtab->get_scope()) {
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

    void visit_Function(const ASR::Function_t &x) {

        // Only process bind(c) subprograms for now
        if (ASRUtils::get_FunctionType(x)->m_abi != ASR::abiType::BindC) return;

        // Return type and function name
        bool bindc_name_not_given = ASRUtils::get_FunctionType(x)->m_bindc_name == NULL ||
                                    !strcmp("", ASRUtils::get_FunctionType(x)->m_bindc_name);
        std::string effective_name = bindc_name_not_given ? x.m_name : ASRUtils::get_FunctionType(x)->m_bindc_name;

        ASR::Variable_t *rtnvar = ASRUtils::EXPR2VAR(x.m_return_var);
        std::string rtnvar_type;
        #define _X(ASR_TYPE, KIND, CTYPE_STR) \
        if ( is_a<ASR_TYPE>(*rtnvar->m_type) && (down_cast<ASR_TYPE>(rtnvar->m_type)->m_kind == KIND) ) { \
            rtnvar_type = CTYPE_STR;                                                                     \
        } else

        CTYPELIST {
            throw CodeGenError("Unrecognized or non-interoperable return type/kind");
        }
        #undef _X
        std::string rtnvar_name = effective_name + "_rtnval__";

        chdr = rtnvar_type + " " + effective_name + " (";

        std::string c_args, cy_args, call_args, pyx_body, rtn_statement;
        std::tie(c_args,cy_args,call_args,pyx_body,rtn_statement) = helper_visit_arguments(x.n_args, x.m_args);

        std::string rtnarg_str =  rtnvar_name;
        if(!rtn_statement.empty()) rtnarg_str += ", ";
        rtn_statement = "    return " + rtnarg_str  + rtn_statement;

        chdr += c_args + ")";
        pxd = "    " + chdr + "\n";
        chdr += ";\n" ;

        pyx = "def " + effective_name + " (" + cy_args + "):\n";
        pyx += pyx_body;
        pyx += "    cdef " + rtnvar_type + " " + rtnvar_name  + " = " + pxdf +"."+ effective_name + " (" + call_args + ")\n";
        pyx += rtn_statement + "\n\n";

    }

};

std::tuple<std::string, std::string, std::string> asr_to_py(ASR::TranslationUnit_t &asr, bool c_order, std::string chdr_filename)
{
    ASRToPyVisitor v (c_order, chdr_filename);
    v.visit_asr((ASR::asr_t &)asr);

    return std::make_tuple(v.chdr, v.pxd, v.pyx);
}

} // namespace LCompilers
