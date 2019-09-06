#ifndef LFORTRAN_EVALUATOR_H
#define LFORTRAN_EVALUATOR_H

#include <iostream>
#include <memory>

// Forward declare all needed LLVM classes without importing any LLVM header
// files. Those are only imported in evaluator.cpp and nowhere else, to speed
// up compilation.
namespace llvm {
    class ExecutionEngine;
    class LLVMContext;
    class TargetMachine;
    class Module;
    class Function;
}

namespace LFortran {

class LLVMEvaluator
{
private:
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
    int64_t intfn(const std::string &name);
    void voidfn(const std::string &name);
    void save_object_file(llvm::Module &m, const std::string &filename);
    std::string module_to_string(llvm::Module &m);
    static void print_version_message();
};

} // namespace LFortran

#endif // LFORTRAN_EVALUATOR_H
