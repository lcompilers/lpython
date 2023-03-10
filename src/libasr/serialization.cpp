#include <string>

#include <libasr/config.h>
#include <libasr/serialization.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/bwriter.h>
#include <libasr/string_utils.h>
#include <libasr/exception.h>

using LCompilers::ASRUtils::symbol_parent_symtab;
using LCompilers::ASRUtils::symbol_name;

namespace LCompilers {


class ASRSerializationVisitor :
#ifdef WITH_LFORTRAN_BINARY_MODFILES
        public BinaryWriter,
#else
        public TextWriter,
#endif
        public ASR::SerializationBaseVisitor<ASRSerializationVisitor>
{
public:
    void write_bool(bool b) {
        if (b) {
            write_int8(1);
        } else {
            write_int8(0);
        }
    }

    void write_symbol(const ASR::symbol_t &x) {
        write_int64(symbol_parent_symtab(&x)->counter);
        write_int8(x.type);
        write_string(symbol_name(&x));
    }
};

std::string serialize(const ASR::asr_t &asr) {
    ASRSerializationVisitor v;
    v.write_int8(asr.type);
    v.visit_asr(asr);
    return v.get_str();
}

std::string serialize(const ASR::TranslationUnit_t &unit) {
    return serialize((ASR::asr_t&)(unit));
}

class ASRDeserializationVisitor :
#ifdef WITH_LFORTRAN_BINARY_MODFILES
        public BinaryReader,
#else
        public TextReader,
#endif
        public ASR::DeserializationBaseVisitor<ASRDeserializationVisitor>
{
public:
    ASRDeserializationVisitor(Allocator &al, const std::string &s,
        bool load_symtab_id) :
#ifdef WITH_LFORTRAN_BINARY_MODFILES
            BinaryReader(s),
#else
            TextReader(s),
#endif
            DeserializationBaseVisitor(al, load_symtab_id) {}

    bool read_bool() {
        uint8_t b = read_int8();
        return (b == 1);
    }

    char* read_cstring() {
        std::string s = read_string();
        LCompilers::Str cs;
        cs.from_str_view(s);
        char* p = cs.c_str(al);
        return p;
    }

#define READ_SYMBOL_CASE(x)                                \
    case (ASR::symbolType::x) : {                          \
        s = (ASR::symbol_t*)al.make_new<ASR::x##_t>();     \
        s->type = ASR::symbolType::x;                      \
        s->base.type = ASR::asrType::symbol;               \
        s->base.loc.first = 0;                             \
        s->base.loc.last = 0;                              \
        break;                                             \
    }

#define INSERT_SYMBOL_CASE(x)                              \
    case (ASR::symbolType::x) : {                          \
        memcpy(sym2, sym, sizeof(ASR::x##_t));             \
        break;                                             \
    }

    ASR::symbol_t *read_symbol() {
        uint64_t symtab_id = read_int64();
        // TODO: read the symbol's location information here, after saving
        // it in write_symbol() above
        uint64_t symbol_type = read_int8();
        std::string symbol_name  = read_string();
        LCOMPILERS_ASSERT(id_symtab_map.find(symtab_id) != id_symtab_map.end());
        SymbolTable *symtab = id_symtab_map[symtab_id];
        if (symtab->get_symbol(symbol_name) == nullptr) {
            // Symbol is not in the symbol table yet. We construct an empty
            // symbol of the correct type and put it in the symbol table.
            // Later when constructing the symbol table, we will check for this
            // and fill it in correctly.
            ASR::symbolType ty = static_cast<ASR::symbolType>(symbol_type);
            ASR::symbol_t *s;
            switch (ty) {
                READ_SYMBOL_CASE(Program)
                READ_SYMBOL_CASE(Module)
                READ_SYMBOL_CASE(Function)
                READ_SYMBOL_CASE(GenericProcedure)
                READ_SYMBOL_CASE(ExternalSymbol)
                READ_SYMBOL_CASE(StructType)
                READ_SYMBOL_CASE(Variable)
                READ_SYMBOL_CASE(ClassProcedure)
                default : throw LCompilersException("Symbol type not supported");
            }
            symtab->add_symbol(symbol_name, s);
        }
        ASR::symbol_t *sym = symtab->get_symbol(symbol_name);
        return sym;
    }

    void symtab_insert_symbol(SymbolTable &symtab, const std::string &name,
        ASR::symbol_t *sym) {
        if (symtab.get_symbol(name) == nullptr) {
            symtab.add_symbol(name, sym);
        } else {
            // We have to copy the contents of `sym` into `sym2` without
            // changing the `sym2` pointer already in the table
            ASR::symbol_t *sym2 = symtab.get_symbol(name);
            switch (sym->type) {
                INSERT_SYMBOL_CASE(Program)
                INSERT_SYMBOL_CASE(Module)
                INSERT_SYMBOL_CASE(Function)
                INSERT_SYMBOL_CASE(GenericProcedure)
                INSERT_SYMBOL_CASE(ExternalSymbol)
                INSERT_SYMBOL_CASE(StructType)
                INSERT_SYMBOL_CASE(Variable)
                INSERT_SYMBOL_CASE(ClassProcedure)
                default : throw LCompilersException("Symbol type not supported");
            }
        }
    }
};

namespace ASR {

class FixParentSymtabVisitor : public BaseWalkVisitor<FixParentSymtabVisitor>
{
private:
    SymbolTable *current_symtab;
public:
    void visit_TranslationUnit(const TranslationUnit_t &x) {
        current_symtab = x.m_global_scope;
        x.m_global_scope->asr_owner = (asr_t*)&x;
        for (auto &a : x.m_global_scope->get_scope()) {
            this->visit_symbol(*a.second);
        }
    }

    void visit_Program(const Program_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        x.m_symtab->parent = parent_symtab;
        x.m_symtab->asr_owner = (asr_t*)&x;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        current_symtab = parent_symtab;
    }

    void visit_Module(const Module_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        x.m_symtab->parent = parent_symtab;
        x.m_symtab->asr_owner = (asr_t*)&x;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        current_symtab = parent_symtab;
    }

    void visit_AssociateBlock(const AssociateBlock_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        x.m_symtab->parent = parent_symtab;
        x.m_symtab->asr_owner = (asr_t*)&x;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        current_symtab = parent_symtab;
    }

    void visit_Block(const Block_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        x.m_symtab->parent = parent_symtab;
        x.m_symtab->asr_owner = (asr_t*)&x;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        current_symtab = parent_symtab;
    }

    void visit_Function(const Function_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        x.m_symtab->parent = parent_symtab;
        x.m_symtab->asr_owner = (asr_t*)&x;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        current_symtab = parent_symtab;
    }

    void visit_StructType(const StructType_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        x.m_symtab->parent = parent_symtab;
        x.m_symtab->asr_owner = (asr_t*)&x;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        current_symtab = parent_symtab;
    }

    void visit_EnumType(const EnumType_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        x.m_symtab->parent = parent_symtab;
        x.m_symtab->asr_owner = (asr_t*)&x;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        current_symtab = parent_symtab;
    }

};

class FixExternalSymbolsVisitor : public BaseWalkVisitor<FixExternalSymbolsVisitor>
{
private:
    SymbolTable *global_symtab;
    SymbolTable *current_scope;
    SymbolTable *external_symtab;
public:
    int attempt;
    bool fixed_external_syms;


    FixExternalSymbolsVisitor(SymbolTable &symtab) : external_symtab{&symtab},
    attempt{0}, fixed_external_syms{true} {}

    void visit_TranslationUnit(const TranslationUnit_t &x) {
        global_symtab = x.m_global_scope;
        for (auto &a : x.m_global_scope->get_scope()) {
            this->visit_symbol(*a.second);
        }
    }

    void visit_Module(const Module_t& x) {
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        BaseWalkVisitor<FixExternalSymbolsVisitor>::visit_Module(x);
        current_scope = current_scope_copy;
    }

    void visit_Function(const Function_t& x) {
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        BaseWalkVisitor<FixExternalSymbolsVisitor>::visit_Function(x);
        current_scope = current_scope_copy;
    }

    void visit_AssociateBlock(const AssociateBlock_t& x) {
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        BaseWalkVisitor<FixExternalSymbolsVisitor>::visit_AssociateBlock(x);
        current_scope = current_scope_copy;
    }

    void visit_ExternalSymbol(const ExternalSymbol_t &x) {
        if (x.m_external != nullptr) {
            // Nothing to do, the external symbol is already resolved
            return;
        }
        LCOMPILERS_ASSERT(x.m_external == nullptr);
        if (x.m_module_name == nullptr) {
            throw LCompilersException("The ExternalSymbol was referenced in some ASR node, but it was not loaded as part of the SymbolTable");
        }
        std::string module_name = x.m_module_name;
        std::string original_name = x.m_original_name;
        if (startswith(module_name, "lfortran_intrinsic_iso")) {
            module_name = module_name.substr(19);
        }

        if (global_symtab->get_symbol(module_name) != nullptr) {
            Module_t *m = down_cast<Module_t>(global_symtab->get_symbol(module_name));
            symbol_t *sym = m->m_symtab->find_scoped_symbol(original_name, x.n_scope_names, x.m_scope_names);
            if (sym) {
                // FIXME: this is a hack, we need to pass in a non-const `x`.
                ExternalSymbol_t &xx = const_cast<ExternalSymbol_t&>(x);
                xx.m_external = sym;
            } else {
                throw LCompilersException("ExternalSymbol cannot be resolved, the symbol '"
                    + original_name + "' was not found in the module '"
                    + module_name + "' (but the module was found)");
            }
        } else if (external_symtab->get_symbol(module_name) != nullptr) {
            Module_t *m = down_cast<Module_t>(external_symtab->get_symbol(module_name));
            symbol_t *sym = m->m_symtab->find_scoped_symbol(original_name, x.n_scope_names, x.m_scope_names);
            if (sym) {
                // FIXME: this is a hack, we need to pass in a non-const `x`.
                ExternalSymbol_t &xx = const_cast<ExternalSymbol_t&>(x);
                xx.m_external = sym;
            } else {
                throw LCompilersException("ExternalSymbol cannot be resolved, the symbol '"
                    + original_name + "' was not found in the module '"
                    + module_name + "' (but the module was found)");
            }
        } else if (current_scope->resolve_symbol(module_name) != nullptr) {
            ASR::symbol_t* m_sym = ASRUtils::symbol_get_past_external(
                                    current_scope->resolve_symbol(module_name));
            if( !m_sym ) {
                fixed_external_syms = false;
                return ;
            }

            symbol_t *sym = nullptr;
            if( ASR::is_a<ASR::StructType_t>(*m_sym) ) {
                StructType_t *m = down_cast<StructType_t>(m_sym);
                sym = m->m_symtab->find_scoped_symbol(original_name,
                        x.n_scope_names, x.m_scope_names);
            } else if( ASR::is_a<ASR::EnumType_t>(*m_sym) ) {
                EnumType_t *m = down_cast<EnumType_t>(m_sym);
                sym = m->m_symtab->find_scoped_symbol(original_name,
                        x.n_scope_names, x.m_scope_names);
            }
            if (sym) {
                // FIXME: this is a hack, we need to pass in a non-const `x`.
                ExternalSymbol_t &xx = const_cast<ExternalSymbol_t&>(x);
                xx.m_external = sym;
            } else {
                throw LCompilersException("ExternalSymbol cannot be resolved, the symbol '"
                    + original_name + "' was not found in the module '"
                    + module_name + "' (but the module was found)");
            }
        } else {
            if( attempt <= 1 ) {
                fixed_external_syms = false;
                return ;
            }
            throw LCompilersException("ExternalSymbol cannot be resolved, the module '"
                + module_name + "' was not found, so the symbol '"
                + original_name + "' could not be resolved");
        }
    }


};

} // namespace ASR

// Fix ExternalSymbol's symbol to point to symbols from `external_symtab`
// or from `unit`.
void fix_external_symbols(ASR::TranslationUnit_t &unit,
        SymbolTable &external_symtab) {
    ASR::FixExternalSymbolsVisitor e(external_symtab);
    e.fixed_external_syms = true;
    e.attempt = 1;
    e.visit_TranslationUnit(unit);
    if( !e.fixed_external_syms ) {
        e.attempt = 2;
        e.visit_TranslationUnit(unit);
    }
}

ASR::asr_t* deserialize_asr(Allocator &al, const std::string &s,
        bool load_symtab_id, SymbolTable & /*external_symtab*/) {
    return deserialize_asr(al, s, load_symtab_id);
}

ASR::asr_t* deserialize_asr(Allocator &al, const std::string &s,
        bool load_symtab_id) {
    ASRDeserializationVisitor v(al, s, load_symtab_id);
    ASR::asr_t *node = v.deserialize_node();
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(node);

    // Connect the `parent` member of symbol tables
    // Also set the `asr_owner` member correctly for all symbol tables
    ASR::FixParentSymtabVisitor p;
    p.visit_TranslationUnit(*tu);

#if defined(WITH_LFORTRAN_ASSERT)
        diag::Diagnostics diagnostics;
        if (!asr_verify(*tu, false, diagnostics)) {
            std::cerr << diagnostics.render2();
            throw LCompilersException("Verify failed");
        };
#endif

    return node;
}

} // namespace LCompilers
