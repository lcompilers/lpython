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
    class Module;
    class Function;
    namespace orc {
        class KaleidoscopeJIT;
    }
}

namespace LFortran {

class LLVMModule
{
public:
    std::unique_ptr<llvm::Module> m;
    LLVMModule(std::unique_ptr<llvm::Module> m);
    ~LLVMModule();
};

class LLVMEvaluator
{
private:
    std::unique_ptr<llvm::orc::KaleidoscopeJIT> jit;
    std::unique_ptr<llvm::LLVMContext> context;
    std::string target_triple;
public:
    LLVMEvaluator();
    ~LLVMEvaluator();
    std::unique_ptr<llvm::Module> parse_module(const std::string &source);
    void add_module(const std::string &source);
    void add_module(std::unique_ptr<llvm::Module> mod);
    void add_module(std::unique_ptr<LLVMModule> m);
    int64_t intfn(const std::string &name);
    void voidfn(const std::string &name);
    void save_object_file(llvm::Module &m, const std::string &filename);
    static std::string module_to_string(llvm::Module &m);
    static void print_version_message();
    llvm::LLVMContext &get_context();
};

} // namespace LFortran

#endif // LFORTRAN_EVALUATOR_H
