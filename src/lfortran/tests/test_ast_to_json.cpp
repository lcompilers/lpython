#include <iostream>

#include <lfortran/ast_to_json.h>

int main(int argc, char *argv[])
{
    std::string s = LFortran::ast_to_json(nullptr);
    std::cout << "Result:" << std::endl;
    std::cout << s << std::endl;
    return 0;
}
