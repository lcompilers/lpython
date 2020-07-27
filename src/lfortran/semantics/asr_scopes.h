#ifndef LFORTRAN_SEMANTICS_ASR_SCOPES_H
#define LFORTRAN_SEMANTICS_ASR_SCOPES_H

#include <map>

namespace LFortran  {

struct TranslationUnitSymbol
{
    char* name;
    // Currently only Subroutines
    // TODO: one would need to check the type of the node present here,
    // the name should be in it, so we can probably remove the "char* name"
    // above.
    // ASR::asr_t *node;
};

struct TranslationUnitScope {
    std::map<std::string, TranslationUnitSymbol> scope;
};

struct SubroutineSymbol
{
    char* name;
    int type; // 1 = real, 2 = integer
    int intent; // 0 = local variable, >0 dummy: 1 = in, 2 = out, 3 = inout
};

struct SubroutineScope {
    std::map<std::string, SubroutineSymbol> scope;
};

} // namespace LFortran

#endif // LFORTRAN_SEMANTICS_ASR_SCOPES_H
