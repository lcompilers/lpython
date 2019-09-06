#ifndef LFORTRAN_EVALUATOR_H
#define LFORTRAN_EVALUATOR_H

#include <iostream>

#include <llvm/IR/LLVMContext.h>
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Scalar.h"
#include <llvm/Transforms/Scalar/GVN.h>
#include "llvm/Transforms/Vectorize.h"
#include "llvm/ExecutionEngine/ObjectCache.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/TargetRegistry.h"

#include <lfortran/exception.h>

namespace LFortran {

uint64_t APInt_getint(const llvm::APInt &i);

class LLVMEvaluator
{
public:
    std::unique_ptr<llvm::ExecutionEngine> ee;
    std::unique_ptr<llvm::LLVMContext> context;
    std::string target_triple;
    llvm::TargetMachine *TM;
public:
    LLVMEvaluator();
    ~LLVMEvaluator();
    std::unique_ptr<llvm::Module> parse_module(const std::string &source);
    void add_module(const std::string &source);
    void add_module(std::unique_ptr<llvm::Module> mod);
    uint64_t intfn(const std::string &name);
    void voidfn(const std::string &name);
    uint64_t intfn(llvm::Function *f);
    void save_object_file(llvm::Module &m, const std::string &filename);
    std::string module_to_string(llvm::Module &m);
};

} // namespace LFortran

#endif // LFORTRAN_EVALUATOR_H
