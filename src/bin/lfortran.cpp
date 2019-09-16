#include <iostream>
#include <chrono>

#include <lfortran/parser/parser.h>
#include <lfortran/pickle.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/codegen/asr_to_llvm.h>
#include <lfortran/codegen/evaluator.h>

void section(const std::string &s)
{
    std::cout << color(LFortran::style::bold) << color(LFortran::fg::blue) << s << color(LFortran::style::reset) << color(LFortran::fg::reset) << std::endl;
}

int main(int argc, char *argv[])
{
    std::cout << "Interactive Fortran. Experimental prototype, not ready for end users." << std::endl;
    std::cout << "  * Use Ctrl-D to exit" << std::endl;
    std::cout << "  * Use Enter to submit" << std::endl;
    std::cout << "Try: function f(); f = 42; end function" << std::endl;
    while (true) {
        std::cout << color(LFortran::style::bold) << color(LFortran::fg::green) << ">>> "
            << color(LFortran::style::reset) << color(LFortran::fg::reset);
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
        section("Input:");
        std::cout << input << std::endl;

        // Src -> AST
        Allocator al(16*1024);
        LFortran::AST::ast_t* ast;
        try {
            ast = LFortran::parse2(al, input);
        } catch (const LFortran::TokenizerError &e) {
            std::cout << "Tokenizing error: " << e.msg() << std::endl;
            continue;
        } catch (const LFortran::ParserError &e) {
            std::cout << "Parsing error: " << e.msg() << std::endl;
            continue;
        } catch (const LFortran::LFortranException &e) {
            std::cout << "Other LFortran exception: " << e.msg() << std::endl;
            continue;
        }
        section("AST:");
        std::cout << LFortran::pickle(*ast, true) << std::endl;


        // AST -> ASR
        LFortran::ASR::asr_t* asr;
        try {
            asr = LFortran::ast_to_asr(al, *ast);
        } catch (const LFortran::LFortranException &e) {
            std::cout << "LFortran exception: " << e.msg() << std::endl;
            continue;
        }
        section("ASR:");
        std::cout << LFortran::pickle(*asr, true) << std::endl;

        // ASR -> LLVM
        LFortran::LLVMEvaluator e = LFortran::LLVMEvaluator();
        std::unique_ptr<LFortran::LLVMModule> m;
        try {
            m = LFortran::asr_to_llvm(*asr, e.get_context());
        } catch (const LFortran::CodeGenError &e) {
            std::cout << "Code generation error: " << e.msg() << std::endl;
            continue;
        }
        section("LLVM IR:");
        std::cout << m->str() << std::endl;

        // LLVM -> Machine code -> Execution
        e.add_module(std::move(m));
        int r = e.intfn("f");
        section("Result:");
        std::cout << r << std::endl;
    }
    return 0;
}
