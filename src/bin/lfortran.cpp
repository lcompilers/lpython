#include <iostream>
#include <chrono>

#include <lfortran/parser/parser.h>
#include <lfortran/pickle.h>

int main(int argc, char *argv[])
{
    std::cout << "Interactive Fortran" << std::endl;
    std::cout << "  * Use Ctrl-D to exit" << std::endl;
    std::cout << "  * Use Enter to submit" << std::endl;
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
            std::cout << "irrecoverable stream error" << std::endl;
            return 1;
        }
        if (std::cin.rdstate() & std::ios_base::failbit) {
            std::cout << "input/output operation failed (formatting or extraction error)" << std::endl;
            return 2;
        }
        if (std::cin.rdstate() != std::ios_base::goodbit) {
            std::cout << "Unknown error" << std::endl;
            return 3;
        }
        std::cout << "Input:" << std::endl;
        std::cout << input << std::endl;
        Allocator al(16*1024);
        LFortran::AST::ast_t* result = LFortran::parse2(al, input);
        std::string ast = LFortran::pickle(*result);
        std::cout << "AST" << std::endl;
        std::cout << ast << std::endl;
    }
    return 0;
}
