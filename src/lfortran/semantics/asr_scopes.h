#ifndef LFORTRAN_SEMANTICS_ASR_SCOPES_H
#define LFORTRAN_SEMANTICS_ASR_SCOPES_H

struct Symbol
{
    char* name;
    int type; // 1 = real, 2 = integer
    int intent; // 0 = local variable, >0 dummy: 1 = in, 2 = out, 3 = inout
};

struct SubroutineScope {
    std::map<std::string, Symbol> subroutine_scope;
};

#endif // LFORTRAN_SEMANTICS_ASR_SCOPES_H
