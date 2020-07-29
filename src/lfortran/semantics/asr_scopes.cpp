#include <iomanip>
#include <sstream>

#include <lfortran/semantics/asr_scopes.h>
#include <lfortran/pickle.h>

namespace LFortran  {

template< typename T >
std::string hexify(T i)
{
    std::stringbuf buf;
    std::ostream os(&buf);
    os << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
    return buf.str();
}

std::string SymbolTable::get_hash() {
    std::string str;
    for (auto &a : scope) {
        str += a.first + ": ";
        str += pickle(*a.second, false);
        str += ", ";
    }
    std::hash<std::string> hasher;
    size_t hash_int = hasher(str);
    return hexify(hash_int).substr(0, 7);
}


} // namespace LFortran
