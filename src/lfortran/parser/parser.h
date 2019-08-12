#ifndef LFORTRAN_PARSER_PARSER_H
#define LFORTRAN_PARSER_PARSER_H

#include <fstream>
#include <algorithm>
#include <memory>

#include <lfortran/parser/tokenizer.h>

namespace LFortran
{

class Parser
{
    std::string inp;

public:
    Tokenizer m_tokenizer;
    //Base *res;

    void parse(const std::string &input);
    int parse();


private:
};

} // namespace LFortran

#endif
