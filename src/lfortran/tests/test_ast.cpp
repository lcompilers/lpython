#include <iostream>

#include <lfortran/parser/alloc.h>
#include <lfortran/casts.h>
#include <lfortran/ast.h>
#include <lfortran/asr.h>

int main(int argc, char *argv[])
{
    std::cout << "OK: " << LFortran::AST::operatorType::Pow  << std::endl;
    std::cout << "OK: " << LFortran::ASR::operatorType::Pow  << std::endl;
    return 0;
}
