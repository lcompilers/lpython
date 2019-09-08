#include <iostream>
#include <memory>

#include <llvm/IR/Module.h>

#include <lfortran/asr.h>
#include <lfortran/codegen/asr_to_llvm.h>


namespace LFortran {

class ASRToLLVMVisitor : public ASR::BaseVisitor<ASRToLLVMVisitor>
{
public:
    llvm::LLVMContext &context;
    std::unique_ptr<llvm::Module> module;
    ASRToLLVMVisitor(llvm::LLVMContext &context) : context{context} {}
    void visit_Function(const ASR::Function_t &x) {
        std::cout << "Function: " << x.m_name << std::endl;
        module = std::make_unique<llvm::Module>("LFortran", context);
    }
};

std::unique_ptr<llvm::Module> asr_to_llvm(ASR::asr_t &asr,
        llvm::LLVMContext &context)
{
    ASRToLLVMVisitor v(context);
    v.visit_asr(asr);
    return std::move(v.module);
}

} // namespace LFortran
