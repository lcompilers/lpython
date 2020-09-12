#include <iomanip>
#include <sstream>

#include <lfortran/semantics/asr_scopes.h>
#include <lfortran/pickle.h>
#include <lfortran/asr_utils.h>

namespace LFortran  {

// This function is taken from:
// https://github.com/aappleby/smhasher/blob/61a0530f28277f2e850bfc39600ce61d02b518de/src/MurmurHash2.cpp#L37
uint32_t murmur_hash(const void * key, int len, uint32_t seed)
{
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.
    const uint32_t m = 0x5bd1e995;
    const int r = 24;
    // Initialize the hash to a 'random' value
    uint32_t h = seed ^ len;
    // Mix 4 bytes at a time into the hash
    const unsigned char * data = (const unsigned char *)key;
    while(len >= 4)
    {
        uint32_t k = *(uint32_t*)data;
        k *= m;
        k ^= k >> r;
        k *= m;
        h *= m;
        h ^= k;
        data += 4;
        len -= 4;
    }
    // Handle the last few bytes of the input array
    switch(len)
    {
        case 3: h ^= data[2] << 16; // fall through
        case 2: h ^= data[1] << 8;  // fall through
        case 1: h ^= data[0];
            h *= m;
    };
    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}

uint32_t murmur_hash_str(const std::string &s, uint32_t seed)
{
    return murmur_hash(&s[0], s.length(), seed);
}

uint32_t murmur_hash_int(uint64_t i, uint32_t seed)
{
    return murmur_hash(&i, 8, seed);
}

template< typename T >
std::string hexify(T i)
{
    std::stringbuf buf;
    std::ostream os(&buf);
    os << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
    return buf.str();
}

// TODO: Calculate the hash more robustly and using all the fields and for
// any ASR node:
// https://gitlab.com/lfortran/lfortran/-/issues/189
// For now we hash a few fields to make the hash unique enough.
uint32_t SymbolTable::get_hash_uint32() {
    std::string str;
    uint32_t hash = 3;
    for (auto &a : scope) {
        hash = murmur_hash(&a.first[0], a.first.length(), hash);
        hash = murmur_hash_int(a.second->type, hash);
        switch (a.second->type) {
            case (ASR::asrType::var) : {
                ASR::Variable_t *v = ASR::down_cast4<ASR::Variable_t>(a.second);
                hash = murmur_hash_str(v->m_name, hash);
                hash = murmur_hash_int(v->m_intent, hash);
                hash = murmur_hash_int(v->m_type->type, hash);
                break;
            }
            case (ASR::asrType::mod) : {
                break;
            }
            case (ASR::asrType::prog) : {
                ASR::Program_t *v = ASR::down_cast4<ASR::Program_t>(a.second);
                hash = murmur_hash_str(v->m_name, hash);
                hash = murmur_hash_int(v->m_symtab->get_hash_uint32(), hash);
                break;
            }
            case (ASR::asrType::sub) : {
                ASR::Subroutine_t *v = ASR::down_cast4<ASR::Subroutine_t>(a.second);
                hash = murmur_hash_str(v->m_name, hash);
                hash = murmur_hash_int(v->m_symtab->get_hash_uint32(), hash);
                break;
            }
            case (ASR::asrType::fn) : {
                ASR::Function_t *v = ASR::down_cast4<ASR::Function_t>(a.second);
                hash = murmur_hash_str(v->m_name, hash);
                hash = murmur_hash_int(v->m_symtab->get_hash_uint32(), hash);
                break;
            }
            default : throw LFortranException("Type not supported in SymbolTable");
        }
    }
    return hash;
}

std::string SymbolTable::get_hash() {
    uint32_t hash = get_hash_uint32();
    return hexify(hash);
}

void SymbolTable::mark_all_variables_external() {
    for (auto &a : scope) {
        switch (a.second->type) {
            case (ASR::asrType::var) : {
                ASR::Variable_t *v = ASR::down_cast4<ASR::Variable_t>(a.second);
                v->m_intent = intent_external;
                break;
            }
            default : {};
        }
    }
}

} // namespace LFortran
