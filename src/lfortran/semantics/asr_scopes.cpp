#include <iomanip>

#include <lfortran/semantics/asr_scopes.h>
#include <lfortran/pickle.h>

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
    uint32_t hash = murmur_hash(&str[0], str.size(), 3);
    return hexify(hash);
}


} // namespace LFortran
