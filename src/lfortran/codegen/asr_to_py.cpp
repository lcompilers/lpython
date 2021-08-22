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
 *  - a .h file, containing C-language function declarations
 *  - a .pxd file, basically containing the same information as the .h file, but in Cython's format.
 *  - a .pyx file, which is a Cython file that includes the actual python-callable wrapper functions.
 *
 *  IMPORTANT NOTE:
 *  Currently, this back-end only wraps functions that are marked "bind (c)" in the Fortran source.
 *  At some later point we will offer the functionality to generate bind (c) wrapper functions for
 *  normal Fortran subprograms, but for now, we don't offer this functionality.
 *
 *
 *  --- H. Snyder, Aug 2021
 * 
 * */

namespace LFortran {

using ASR::is_a;
using ASR::down_cast;
using ASR::down_cast2;

std::string convert_dims(size_t n_dims, ASR::dimension_t *m_dims);

std::string format_type(const std::string &dims, const std::string &type,
        const std::string &name, bool use_ref, bool dummy);


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

    ASRToPyVisitor(bool c_order_) : c_order(c_order_) {}

    std::string convert_variable_decl(const ASR::Variable_t &v)
    {
        std::string sub;
        bool use_ref = (v.m_intent == LFortran::ASRUtils::intent_out || v.m_intent == LFortran::ASRUtils::intent_inout);
        bool dummy = LFortran::ASRUtils::is_arg_dummy(v.m_intent);
        if (is_a<ASR::Integer_t>(*v.m_type)) {
            ASR::Integer_t *t = down_cast<ASR::Integer_t>(v.m_type);
            std::string dims = convert_dims(t->n_dims, t->m_dims);
            sub = format_type(dims, "int", v.m_name, use_ref, dummy);
        } else if (is_a<ASR::Real_t>(*v.m_type)) {
            ASR::Real_t *t = down_cast<ASR::Real_t>(v.m_type);
            std::string dims = convert_dims(t->n_dims, t->m_dims);
            sub = format_type(dims, "float", v.m_name, use_ref, dummy);
        } else if (is_a<ASR::Logical_t>(*v.m_type)) {
            ASR::Logical_t *t = down_cast<ASR::Logical_t>(v.m_type);
            std::string dims = convert_dims(t->n_dims, t->m_dims);
            sub = format_type(dims, "bool", v.m_name, use_ref, dummy);
        } else if (is_a<ASR::Character_t>(*v.m_type)) {
            ASR::Character_t *t = down_cast<ASR::Character_t>(v.m_type);
            std::string dims = convert_dims(t->n_dims, t->m_dims);
            sub = format_type(dims, "std::string", v.m_name, use_ref, dummy);
            if (v.m_symbolic_value) {
                this->visit_expr(*v.m_symbolic_value);
                std::string init = chdr;
                sub += "=" + init;
            }
        } else {
            throw CodeGenError("Type not supported");
        }
        return sub;
    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LFORTRAN_ASSERT(x.n_items == 0);

	std::string chdr_tmp ;
	std::string pxd_tmp  ;
	std::string pyx_tmp  ;
        
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
    }

    void visit_Subroutine(const ASR::Subroutine_t &x) {
	
	// Only process bind(c) subprograms for now
	// if (something) return;

        std::string sub = "void " + std::string(x.m_name) + "(";
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(x.m_args[i]);
            LFORTRAN_ASSERT(LFortran::ASRUtils::is_arg_dummy(arg->m_intent));
            sub += convert_variable_decl(*arg);
            if (i < x.n_args-1) sub += ", ";
        }
        sub += ")\n";

        chdr = sub;
    }


    void visit_Function(const ASR::Function_t &x) {
	// Only process bind(c) subprograms for now
	// if (something) return;

        std::string sub = "void " + std::string(x.m_name) + "(";
        for (size_t i=0; i<x.n_args; i++) {
            ASR::Variable_t *arg = LFortran::ASRUtils::EXPR2VAR(x.m_args[i]);
            LFORTRAN_ASSERT(LFortran::ASRUtils::is_arg_dummy(arg->m_intent));
            sub += convert_variable_decl(*arg);
            if (i < x.n_args-1) sub += ", ";
        }
        sub += ")\n";

        chdr = sub;
    }

};

std::tuple<std::string, std::string, std::string> asr_to_py(ASR::TranslationUnit_t &asr, bool c_order)
{
    ASRToPyVisitor v (c_order);
    v.visit_asr((ASR::asr_t &)asr);
    return std::make_tuple(v.chdr, v.pxd, v.pyx);
}

} // namespace LFortran
