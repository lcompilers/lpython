#include <iostream>
#include <memory>

#include <llvm/ADT/STLExtras.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Vectorize.h>
#include <llvm/ExecutionEngine/ObjectCache.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <lfortran/asr.h>
#include <lfortran/codegen/asr_to_llvm.h>
#include <lfortran/exception.h>


namespace LFortran {

class IRBuilder : public llvm::IRBuilder<>
{
};

class ASRToLLVMVisitor : public ASR::BaseVisitor<ASRToLLVMVisitor>
{
public:
    llvm::LLVMContext &context;
    std::unique_ptr<llvm::Module> module;

    IRBuilder *builder;
    llvm::Value *tmp;

    ASRToLLVMVisitor(llvm::LLVMContext &context) : context{context} {}

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        throw CodeGenError("Not implemented yet.");
    }

    void visit_Function(const ASR::Function_t &x) {
        module = std::make_unique<llvm::Module>("LFortran", context);
        module->setDataLayout("");
        std::vector<llvm::Type *> args;
        llvm::FunctionType *function_type = llvm::FunctionType::get(
                llvm::Type::getInt64Ty(context), args, false);
        llvm::Function *F = llvm::Function::Create(function_type,
                llvm::Function::ExternalLinkage, x.m_name, module.get());
        llvm::BasicBlock *BB = llvm::BasicBlock::Create(context,
                "EntryBlock", F);
        llvm::IRBuilder<> _builder = llvm::IRBuilder<>(BB);
        builder = reinterpret_cast<IRBuilder *>(&_builder);
        builder->SetInsertPoint(BB);

        for (size_t i=0; i<x.n_body; i++) {
            LFORTRAN_ASSERT(x.m_body[i]->base.type == ASR::asrType::stmt)
            this->visit_stmt(*x.m_body[i]);
        }

        llvm::verifyFunction(*F);
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        //this->visit_expr(*x.m_target);
        //LFORTRAN_ASSERT(x.m_target.m_id == "f");
        this->visit_expr(*x.m_value);
        llvm::Value *ret_val = tmp;
        builder->CreateRet(ret_val);
    }
    void visit_Num(const ASR::Num_t &x) {
        tmp = llvm::ConstantInt::get(context, llvm::APInt(64, x.m_n));
    }

};

std::unique_ptr<LLVMModule> asr_to_llvm(ASR::asr_t &asr,
        llvm::LLVMContext &context)
{
    ASRToLLVMVisitor v(context);
    v.visit_asr(asr);
    return std::make_unique<LLVMModule>(std::move(v.module));
}

} // namespace LFortran
