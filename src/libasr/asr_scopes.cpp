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

void SymbolTable::move_symbols_from_global_scope(Allocator &al,
        SymbolTable *module_scope, Vec<char *> &syms,
        Vec<char *> &mod_dependencies, Vec<char *> &func_dependencies,
        Vec<ASR::stmt_t*> &var_init) {
    // TODO: This isn't scalable. We have write a visitor in asdl_cpp.py
    syms.reserve(al, 4);
    mod_dependencies.reserve(al, 4);
    func_dependencies.reserve(al, 4);
    var_init.reserve(al, 4);
    for (auto &a : scope) {
        switch (a.second->type) {
            case (ASR::symbolType::Module): {
                // Pass
                break;
            } case (ASR::symbolType::Function) : {
                ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(a.second);
                for (size_t i = 0; i < fn->n_dependencies; i++ ) {
                    ASR::symbol_t *s = fn->m_symtab->get_symbol(
                        fn->m_dependencies[i]);
                    if (s == nullptr) {
                        std::string block_name = "block";
                        ASR::symbol_t *block_s = fn->m_symtab->get_symbol(block_name);
                        int32_t j = 1;
                        while(block_s != nullptr) {
                            while(block_s != nullptr) {
                                ASR::Block_t *b = ASR::down_cast<ASR::Block_t>(block_s);
                                s = b->m_symtab->get_symbol(fn->m_dependencies[i]);
                                if (s == nullptr) {
                                    block_s = b->m_symtab->get_symbol("block");
                                } else {
                                    break;
                                }
                            }
                            if (s == nullptr) {
                                block_s = fn->m_symtab->get_symbol(block_name +
                                    std::to_string(j));
                                j++;
                            } else {
                                break;
                            }
                        }
                    }
                    if (s == nullptr) {
                        s = fn->m_symtab->parent->get_symbol(fn->m_dependencies[i]);
                    }
                    if (s != nullptr && ASR::is_a<ASR::ExternalSymbol_t>(*s)) {
                        char *es_name = ASR::down_cast<
                            ASR::ExternalSymbol_t>(s)->m_module_name;
                        if (!present(mod_dependencies, es_name)) {
                            mod_dependencies.push_back(al, es_name);
                        }
                    }
                }
                fn->m_symtab->parent = module_scope;
                module_scope->add_symbol(a.first, (ASR::symbol_t *) fn);
                syms.push_back(al, s2c(al, a.first));
                break;
            } case (ASR::symbolType::GenericProcedure) : {
                ASR::GenericProcedure_t *es = ASR::down_cast<ASR::GenericProcedure_t>(a.second);
                es->m_parent_symtab = module_scope;
                module_scope->add_symbol(a.first, (ASR::symbol_t *) es);
                syms.push_back(al, s2c(al, a.first));
                break;
            } case (ASR::symbolType::ExternalSymbol) : {
                ASR::ExternalSymbol_t *es = ASR::down_cast<ASR::ExternalSymbol_t>(a.second);
                if (!present(mod_dependencies, es->m_module_name)) {
                    mod_dependencies.push_back(al, es->m_module_name);
                }
                es->m_parent_symtab = module_scope;
                ASR::symbol_t *s = ASRUtils::symbol_get_past_external(a.second);
                LCOMPILERS_ASSERT(s);
                if (ASR::is_a<ASR::Variable_t>(*s)) {
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(s);
                    if (v->m_symbolic_value && !ASR::is_a<ASR::Const_t>(*v->m_type)
                            && ASR::is_a<ASR::List_t>(*v->m_type)) {
                        ASR::expr_t* target = ASRUtils::EXPR(ASR::make_Var_t(
                            al, v->base.base.loc, (ASR::symbol_t *) es));
                        ASR::expr_t *value = v->m_symbolic_value;
                        v->m_symbolic_value = nullptr;
                        v->m_value = nullptr;
                        if (ASR::is_a<ASR::FunctionCall_t>(*value)) {
                            ASR::FunctionCall_t *call =
                                ASR::down_cast<ASR::FunctionCall_t>(value);
                            ASR::Module_t *m = ASRUtils::get_sym_module(s);
                            ASR::symbol_t *func = m->m_symtab->get_symbol(
                                ASRUtils::symbol_name(call->m_name));
                            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(func);
                            std::string func_name = std::string(m->m_name) +
                                "@" + f->m_name;
                            ASR::symbol_t *es_func;
                            if (!module_scope->get_symbol(func_name)) {
                                es_func = ASR::down_cast<ASR::symbol_t>(
                                    ASR::make_ExternalSymbol_t(al, f->base.base.loc,
                                    module_scope, s2c(al, func_name), func, m->m_name,
                                    nullptr, 0, s2c(al, f->m_name), ASR::accessType::Public));
                                module_scope->add_symbol(func_name, es_func);
                                if (!present(func_dependencies, s2c(al, func_name))) {
                                    func_dependencies.push_back(al, s2c(al,func_name));
                                }
                            } else {
                                es_func = module_scope->get_symbol(func_name);
                            }
                            value = ASRUtils::EXPR(ASR::make_FunctionCall_t(al,
                                call->base.base.loc, es_func, call->m_original_name,
                                call->m_args, call->n_args, call->m_type,
                                call->m_value, call->m_dt));
                        }
                        ASR::asr_t* assign = ASR::make_Assignment_t(al,
                            v->base.base.loc, target, value, nullptr);
                        var_init.push_back(al, ASRUtils::STMT(assign));
                    }
                }
                module_scope->add_symbol(a.first, (ASR::symbol_t *) es);
                syms.push_back(al, s2c(al, a.first));
                break;
            } case (ASR::symbolType::StructType) : {
                ASR::StructType_t *st = ASR::down_cast<ASR::StructType_t>(a.second);
                st->m_symtab->parent = module_scope;
                module_scope->add_symbol(a.first, (ASR::symbol_t *) st);
                syms.push_back(al, s2c(al, a.first));
                break;
            } case (ASR::symbolType::EnumType) : {
                ASR::EnumType_t *et = ASR::down_cast<ASR::EnumType_t>(a.second);
                et->m_symtab->parent = module_scope;
                module_scope->add_symbol(a.first, (ASR::symbol_t *) et);
                syms.push_back(al, s2c(al, a.first));
                break;
            } case (ASR::symbolType::UnionType) : {
                ASR::UnionType_t *ut = ASR::down_cast<ASR::UnionType_t>(a.second);
                ut->m_symtab->parent = module_scope;
                module_scope->add_symbol(a.first, (ASR::symbol_t *) ut);
                syms.push_back(al, s2c(al, a.first));
                break;
            } case (ASR::symbolType::Variable) : {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(a.second);
                v->m_parent_symtab = module_scope;
                // Make the Assignment statement only for the data-types (List,
                // Dict, ...), that cannot be handled in the LLVM global scope
                if (v->m_symbolic_value && !ASR::is_a<ASR::Const_t>(*v->m_type)
                        && ASR::is_a<ASR::List_t>(*v->m_type)) {
                    ASR::expr_t* v_expr = ASRUtils::EXPR(ASR::make_Var_t(
                        al, v->base.base.loc, (ASR::symbol_t *) v));
                    ASR::asr_t* assign = ASR::make_Assignment_t(al,
                        v->base.base.loc, v_expr, v->m_symbolic_value, nullptr);
                    var_init.push_back(al, ASRUtils::STMT(assign));
                    v->m_symbolic_value = nullptr;
                    v->m_value = nullptr;
                    Vec<char*> v_dependencies;
                    v_dependencies.reserve(al, 1);
                    ASRUtils::collect_variable_dependencies(al,
                        v_dependencies, v->m_type);
                    v->m_dependencies = v_dependencies.p;
                    v->n_dependencies = v_dependencies.size();
                }
                module_scope->add_symbol(a.first, (ASR::symbol_t *) v);
                syms.push_back(al, s2c(al, a.first));
                break;
            } default : {
                throw LCompilersException("Moving the symbol:`" + a.first +
                    "` from global scope is not implemented yet");
            }
        }
    }
}

} // namespace LCompilers
