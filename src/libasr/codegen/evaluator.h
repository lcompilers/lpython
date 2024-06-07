#ifndef LFORTRAN_EVALUATOR_H
#define LFORTRAN_EVALUATOR_H

#include <complex>
#include <iostream>
#include <memory>

#include <libasr/alloc.h>
#include <libasr/asr_scopes.h>
#include <libasr/asr.h>
#include <libasr/utils.h>

// Forward declare all needed LLVM classes without importing any LLVM header
// files. Those are only imported in evaluator.cpp and nowhere else, to speed
// up compilation.
namespace llvm {
    class ExecutionEngine;
    class LLVMContext;
    class Module;
    class Function;
    class TargetMachine;
    namespace orc {
        class KaleidoscopeJIT;
    }
}

namespace LCompilers {

class LLVMModule
{
public:
    std::unique_ptr<llvm::Module> m_m;
    LLVMModule(std::unique_ptr<llvm::Module> m);
    ~LLVMModule();
    std::string str();
    // Return a function return type as a string (real / integer)
    std::string get_return_type(const std::string &fn_name);
};

class LLVMEvaluator
{
private:
    std::unique_ptr<llvm::orc::KaleidoscopeJIT> jit;
    std::unique_ptr<llvm::LLVMContext> context;
    std::string target_triple;
    llvm::TargetMachine *TM;
public:
    LLVMEvaluator(const std::string &t = "");
    ~LLVMEvaluator();
    std::unique_ptr<llvm::Module> parse_module(const std::string &source);
    void add_module(const std::string &source);
    void add_module(std::unique_ptr<llvm::Module> mod);
    void add_module(std::unique_ptr<LLVMModule> m);
    intptr_t get_symbol_address(const std::string &name);
    std::string get_asm(llvm::Module &m);
    void save_asm_file(llvm::Module &m, const std::string &filename);
    void save_object_file(llvm::Module &m, const std::string &filename);
    void create_empty_object_file(const std::string &filename);
    void opt(llvm::Module &m);
    static std::string module_to_string(llvm::Module &m);
    static void print_version_message();
    llvm::LLVMContext &get_context();
    static void print_targets();
    static std::string get_default_target_triple();

    template<class T>
    T execfn(const std::string &name) {
        intptr_t addr = get_symbol_address(name);
        T (*f)() = (T (*)())addr;
        return f();
    }
};


} // namespace LCompilers

#endif // LFORTRAN_EVALUATOR_H
