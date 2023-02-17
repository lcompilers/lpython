#include <iomanip>
#include <sstream>

#include <libasr/asr_scopes.h>
#include <libasr/asr_utils.h>

namespace LCompilers  {

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

unsigned int symbol_table_counter = 0;

SymbolTable::SymbolTable(SymbolTable *parent) : parent{parent} {
    symbol_table_counter++;
    counter = symbol_table_counter;
}

void SymbolTable::reset_global_counter() {
    symbol_table_counter = 0;
}

void SymbolTable::mark_all_variables_external(Allocator &/*al*/) {
    for (auto &a : scope) {
        switch (a.second->type) {
            case (ASR::symbolType::Variable) : {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(a.second);
                v->m_abi = ASR::abiType::Interactive;
                break;
            }
            case (ASR::symbolType::Function) : {
                ASR::Function_t *v = ASR::down_cast<ASR::Function_t>(a.second);
                ASR::FunctionType_t* v_func_type = ASR::down_cast<ASR::FunctionType_t>(v->m_function_signature);
                v_func_type->m_abi = ASR::abiType::Interactive;
                v->m_body = nullptr;
                v->n_body = 0;
                break;
            }
            default : {};
        }
    }
}

ASR::symbol_t *SymbolTable::find_scoped_symbol(const std::string &name,
        size_t n_scope_names, char **m_scope_names) {
    const SymbolTable *s = this;
    for(size_t i=0; i < n_scope_names; i++) {
        std::string scope_name = m_scope_names[i];
        if (s->scope.find(scope_name) != scope.end()) {
            ASR::symbol_t *sym = s->scope.at(scope_name);
            s = ASRUtils::symbol_symtab(sym);
            if (s == nullptr) {
                // The m_scope_names[i] found in the appropriate symbol table,
                // but points to a symbol that itself does not have a symbol
                // table
                return nullptr;
            }
        } else {
            // The m_scope_names[i] not found in the appropriate symbol table
            return nullptr;
        }
    }
    if (s->scope.find(name) != scope.end()) {
        ASR::symbol_t *sym = s->scope.at(name);
        LCOMPILERS_ASSERT(sym)
        return sym;
    } else {
        // The `name` not found in the appropriate symbol table
        return nullptr;
    }
}

std::string SymbolTable::get_unique_name(const std::string &name) {
    std::string unique_name = name;
    int counter = 1;
    while (scope.find(unique_name) != scope.end()) {
        unique_name = name + std::to_string(counter);
        counter++;
    }
    return unique_name;
}

} // namespace LCompilers
