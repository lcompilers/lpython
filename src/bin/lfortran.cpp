#include <iostream>
#include <chrono>

#include <lfortran/parser/parser.h>
#include <lfortran/pickle.h>

int main(int argc, char *argv[])
{
    std::cout << "Interactive Fortran" << std::endl;
    std::cout << ">>> ";
    std::string input;
    std::getline(std::cin, input);
    std::cout << "Input:" << std::endl;
    //std::string t0 = "(a*z+3+2*x + 3*y - x/(z**2-4) - x**(y**z))";
    std::cout << input << std::endl;
    Allocator al(16*1024);
    LFortran::AST::ast_t* result = LFortran::parse2(al, input);
    std::string ast = LFortran::pickle(*result);
    std::cout << "AST" << std::endl;
    std::cout << ast << std::endl;
    return 0;
}
