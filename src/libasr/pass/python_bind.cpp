#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/replace_arr_slice.h>

namespace LCompilers {
    class PythonABI_To_CABI: public ASR::BaseWalkVisitor<PythonABI_To_CABI> {
    };

    void pass_python_bind(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options) {
        bool bindpython_used = false;
        for (auto &item : unit.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(item.second);
                if (ASRUtils::get_FunctionType(f)->m_abi == ASR::abiType::BindPython) {
                    bindpython_used = true;
                }
            }
            else if (ASR::is_a<ASR::Module_t>(*item.second)) {
                ASR::Module_t *module = ASR::down_cast<ASR::Module_t>(item.second);
                for (auto &module_item: module->m_symtab->get_scope() ) {
                    if (ASR::is_a<ASR::Function_t>(*module_item.second)) {
                        ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(module_item.second);
                        if (ASRUtils::get_FunctionType(f)->m_abi == ASR::abiType::BindPython) {
                            bindpython_used = false;
                        }
                    }
                }
            }

            if (bindpython_used) { 
                break;
            }
        }

        if (bindpython_used) {
            LCompilers::LPython::DynamicLibrary cpython_lib;
            LCompilers::LPython::open_cpython_library(cpython_lib);
        }
    }
}
