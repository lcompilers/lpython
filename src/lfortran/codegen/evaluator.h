#ifndef LFORTRAN_EVALUATOR_H
#define LFORTRAN_EVALUATOR_H

#include <iostream>
#include <memory>

#include <lfortran/parser/alloc.h>
#include <lfortran/semantics/asr_scopes.h>

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
public:
    LLVMEvaluator();
    ~LLVMEvaluator();
    std::unique_ptr<llvm::Module> parse_module(const std::string &source);
    void add_module(const std::string &source);
    void add_module(std::unique_ptr<llvm::Module> mod);
    void add_module(std::unique_ptr<LLVMModule> m);
    int64_t intfn(const std::string &name);
    float floatfn(const std::string &name);
    void voidfn(const std::string &name);
    void save_asm_file(llvm::Module &m, const std::string &filename);
    void save_object_file(llvm::Module &m, const std::string &filename);
    static std::string module_to_string(llvm::Module &m);
    static void print_version_message();
    llvm::LLVMContext &get_context();
};

class FortranEvaluator
{
private:
    Allocator al;
    LLVMEvaluator e;
    SymbolTable *symbol_table;
public:
    enum ResultType {
        integer, real, none
    };
    struct Result {
        ResultType type;
        union {
            int64_t i;
            float f;
        };
    };
    FortranEvaluator();
    ~FortranEvaluator();
    Result evaluate(const std::string &code);
};

} // namespace LFortran

#endif // LFORTRAN_EVALUATOR_H
