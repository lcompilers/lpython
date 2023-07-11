#include <iomanip>
#include <sstream>

#include <libasr/asr_scopes.h>
#include <libasr/asr_utils.h>

std::string lcompilers_unique_ID;

namespace LCompilers  {

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

void SymbolTable::mark_all_variables_external(Allocator &al) {
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
            case (ASR::symbolType::Module) : {
                ASR::Module_t *v = ASR::down_cast<ASR::Module_t>(a.second);
                v->m_symtab->mark_all_variables_external(al);
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

std::string SymbolTable::get_unique_name(const std::string &name, bool use_unique_id) {
    std::string unique_name = name;
    if( use_unique_id ) {
        unique_name += "_" + lcompilers_unique_ID;
    }
    int counter = 1;
    while (scope.find(unique_name) != scope.end()) {
        unique_name = name + std::to_string(counter);
        counter++;
    }
    return unique_name;
}

void SymbolTable::move_symbols_from_global_scope(Allocator &al,
        SymbolTable *module_scope, Vec<char *> &syms,
        SetChar &mod_dependencies) {
    // TODO: This isn't scalable. We have write a visitor in asdl_cpp.py
    syms.reserve(al, 4);
    mod_dependencies.reserve(al, 4);
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
                        mod_dependencies.push_back(al, es_name);
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
                mod_dependencies.push_back(al, es->m_module_name);
                es->m_parent_symtab = module_scope;
                LCOMPILERS_ASSERT(ASRUtils::symbol_get_past_external(a.second));
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
