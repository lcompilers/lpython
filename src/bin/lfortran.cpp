#include <iostream>
#include <chrono>

#include <lfortran/parser/parser.h>
#include <lfortran/pickle.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/codegen/asr_to_llvm.h>
#include <lfortran/codegen/evaluator.h>

int main(int argc, char *argv[])
{
    std::cout << "Interactive Fortran" << std::endl;
    std::cout << "  * Use Ctrl-D to exit" << std::endl;
    std::cout << "  * Use Enter to submit" << std::endl;
    std::cout << "Try: function f(); f = 42; end function" << std::endl;
    while (true) {
        std::cout << ">>> ";
        std::string input;
        std::getline(std::cin, input);
        if (std::cin.rdstate() & std::ios_base::eofbit) {
            std::cout << std::endl;
            std::cout << "Exiting." << std::endl;
            return 0;
        }
        if (std::cin.rdstate() & std::ios_base::badbit) {
            std::cout << "Irrecoverable stream error." << std::endl;
            std::cout << "Exiting." << std::endl;
            return 1;
        }
        if (std::cin.rdstate() & std::ios_base::failbit) {
            std::cout << "Input/output operation failed (formatting or extraction error)." << std::endl;
            std::cout << "Exiting." << std::endl;
            return 2;
        }
        if (std::cin.rdstate() != std::ios_base::goodbit) {
            std::cout << "Unknown error." << std::endl;
            std::cout << "Exiting." << std::endl;
            return 3;
        }
        std::cout << "Input:" << std::endl;
        std::cout << input << std::endl;

        // Src -> AST
        Allocator al(16*1024);
        LFortran::AST::ast_t* ast = LFortran::parse2(al, input);
        std::cout << "AST" << std::endl;
        std::cout << LFortran::pickle(*ast) << std::endl;


        // AST -> ASR
        LFortran::ASR::asr_t* asr = LFortran::ast_to_asr(al, *ast);
        std::cout << "ASR" << std::endl;
        std::cout << LFortran::pickle(*asr) << std::endl;

        // ASR -> LLVM
        LFortran::LLVMEvaluator e = LFortran::LLVMEvaluator();
        std::unique_ptr<LFortran::LLVMModule> m = LFortran::asr_to_llvm(*asr,
                e.get_context());
        std::cout << "LLVM IR:" << std::endl;
        std::cout << m->str() << std::endl;

        // LLVM -> Machine code -> Execution
        e.add_module(std::move(m));
        int r = e.intfn("f");
        std::cout << "Result:" << std::endl;
        std::cout << r << std::endl;
    }
    return 0;
}
