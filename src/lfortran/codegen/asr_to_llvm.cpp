#include <iostream>
#include <memory>

#include <llvm/IR/Module.h>

#include <lfortran/asr.h>
#include <lfortran/codegen/asr_to_llvm.h>


namespace LFortran {

class ASRToLLVMVisitor : public ASR::BaseVisitor<ASRToLLVMVisitor>
{
public:
    std::unique_ptr<llvm::Module> module;
    void visit_Function(const ASR::Function_t &x) {
        std::cout << "Function: " << x.m_name << std::endl;
    }
};

std::unique_ptr<llvm::Module> asr_to_llvm(ASR::asr_t &asr)
{
    ASRToLLVMVisitor v;
    v.visit_asr(asr);
    return std::move(v.module);
}

} // namespace LFortran
