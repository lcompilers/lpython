#ifndef LFORTRAN_SEMANTICS_ASR_SCOPES_H
#define LFORTRAN_SEMANTICS_ASR_SCOPES_H

#include <map>
#include <hash_map>
#include <iomanip>
#include <sstream>

namespace LFortran  {

namespace ASR {
    struct asr_t;
}
std::string pickle(ASR::asr_t &asr, bool colors);

template< typename T >
std::string hexify(T i)
{
    std::stringbuf buf;
    std::ostream os(&buf);
    os << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
    return buf.str();
}

struct SymbolTable {
    std::map<std::string, ASR::asr_t*> scope;

    // TODO: move this to a cpp file, together with the associated headers
    //
    // Determines a stable hash based on the content of the symbol table
    std::string get_hash() {
        std::string str;
        for (auto &a : scope) {
            str += a.first + ": ";
            str += pickle(*a.second, false);
            str += ", ";
        }
        std::hash<std::string> hasher;
        size_t hash_int = hasher(str);
        return hexify(hash_int);
    }
};

const int intent_local=0; // local variable (not a dummy argument)
const int intent_in   =1; // dummy argument, intent(in)
const int intent_out  =2; // dummy argument, intent(out)
const int intent_inout=3; // dummy argument, intent(inout)
const int intent_return_var=4; // return variable of a function

} // namespace LFortran

#endif // LFORTRAN_SEMANTICS_ASR_SCOPES_H
