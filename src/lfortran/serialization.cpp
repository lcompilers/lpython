#include <string>

#include <lfortran/serialization.h>
#include <lfortran/parser/parser.h>
#include <lfortran/parser/parser.tab.hh>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>


namespace LFortran {

std::string uint64_to_string(uint64_t i) {
    char bytes[4];
    bytes[0] = (i >> 24) & 0xFF;
    bytes[1] = (i >> 16) & 0xFF;
    bytes[2] = (i >>  8) & 0xFF;
    bytes[3] =  i        & 0xFF;
    return std::string(bytes, 4);
}

uint64_t string_to_uint64(const char *p) {
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

uint64_t string_to_uint64(const std::string &s) {
    return string_to_uint64(&s[0]);
}

class ASTSerializationVisitor : public
                             AST::SerializationBaseVisitor<ASTSerializationVisitor>
{
private:
    std::string s;
public:
    std::string get_str() {
        return s;
    }

    void write_int8(uint8_t i) {
        char c=i;
        s.append(std::string(&c, 1));
    }

    void write_int64(uint64_t i) {
        s.append(uint64_to_string(i));
    }

    void write_bool(bool b) {
        if (b) {
            write_int8(1);
        } else {
            write_int8(0);
        }
    }

    void write_string(const std::string &t) {
        write_int64(t.size());
        s.append(t);
    }
};

std::string serialize(AST::ast_t &ast) {
    ASTSerializationVisitor v;
    v.write_int8(ast.type);
    v.visit_ast(ast);
    return v.get_str();
}

std::string serialize(AST::TranslationUnit_t &unit) {
    return serialize((AST::ast_t&)(unit));
}

class ASTDeserializationVisitor : public
                             AST::DeserializationBaseVisitor<ASTDeserializationVisitor>
{
private:
    std::string s;
    size_t pos;
public:
    ASTDeserializationVisitor(Allocator &al, const std::string &s) :
            DeserializationBaseVisitor(al), s{s}, pos{0} {}

    uint8_t read_int8() {
        if (pos+1 > s.size()) {
            throw LFortranException("String is too short for deserialization.");
        }
        uint8_t n = s[pos];
        pos += 1;
        return n;
    }

    uint64_t read_int64() {
        if (pos+4 > s.size()) {
            throw LFortranException("String is too short for deserialization.");
        }
        uint64_t n = string_to_uint64(&s[pos]);
        pos += 4;
        return n;
    }

    bool read_bool() {
        uint8_t b = read_int8();
        return (b == 1);
    }

    std::string read_string() {
        size_t n = read_int64();
        if (pos+n > s.size()) {
            throw LFortranException("String is too short for deserialization.");
        }
        std::string r = std::string(&s[pos], n);
        pos += n;
        return r;
    }

    char* read_cstring() {
        std::string s = read_string();
        LFortran::Str cs;
        cs.from_str_view(s);
        char* p = cs.c_str(al);
        return p;
    }
};

AST::ast_t* deserialize_ast(Allocator &al, const std::string &s) {
    ASTDeserializationVisitor v(al, s);
    return v.deserialize_node();
}

// ----------------------------------------------------------------

class ASRSerializationVisitor : public
                             ASR::SerializationBaseVisitor<ASRSerializationVisitor>
{
private:
    std::string s;
public:
    std::string get_str() {
        return s;
    }

    void write_int8(uint8_t i) {
        char c=i;
        s.append(std::string(&c, 1));
    }

    void write_int64(uint64_t i) {
        s.append(uint64_to_string(i));
    }

    void write_bool(bool b) {
        if (b) {
            write_int8(1);
        } else {
            write_int8(0);
        }
    }

    void write_string(const std::string &t) {
        write_int64(t.size());
        s.append(t);
    }

    void write_symbol(const ASR::symbol_t &x) {
        write_int64(symbol_parent_symtab(&x)->counter);
        write_int8(x.type);
        write_string(symbol_name(&x));
    }
};

std::string serialize(ASR::asr_t &asr) {
    ASRSerializationVisitor v;
    v.write_int8(asr.type);
    v.visit_asr(asr);
    return v.get_str();
}

std::string serialize(ASR::TranslationUnit_t &unit) {
    return serialize((ASR::asr_t&)(unit));
}

class ASRDeserializationVisitor : public
                             ASR::DeserializationBaseVisitor<ASRDeserializationVisitor>
{
private:
    std::string s;
    size_t pos;
public:
    ASRDeserializationVisitor(Allocator &al, const std::string &s) :
            DeserializationBaseVisitor(al), s{s}, pos{0} {}

    uint8_t read_int8() {
        if (pos+1 > s.size()) {
            throw LFortranException("String is too short for deserialization.");
        }
        uint8_t n = s[pos];
        pos += 1;
        return n;
    }

    uint64_t read_int64() {
        if (pos+4 > s.size()) {
            throw LFortranException("String is too short for deserialization.");
        }
        uint64_t n = string_to_uint64(&s[pos]);
        pos += 4;
        return n;
    }

    bool read_bool() {
        uint8_t b = read_int8();
        return (b == 1);
    }

    std::string read_string() {
        size_t n = read_int64();
        if (pos+n > s.size()) {
            throw LFortranException("String is too short for deserialization.");
        }
        std::string r = std::string(&s[pos], n);
        pos += n;
        return r;
    }

    char* read_cstring() {
        std::string s = read_string();
        LFortran::Str cs;
        cs.from_str_view(s);
        char* p = cs.c_str(al);
        return p;
    }

    ASR::symbol_t *read_symbol() {
        uint64_t symtab_id = read_int64();
        uint64_t symbol_type = read_int8();
        std::string symbol_name  = read_string();
        LFORTRAN_ASSERT(id_symtab_map.find(symtab_id) != id_symtab_map.end());
        SymbolTable *symtab = id_symtab_map[symtab_id];
        if (symtab->scope.find(symbol_name) == symtab->scope.end()) {
            // Symbol is not in the symbol table yet. We construct an empty
            // symbol of the correct type and put it in the symbol table.
            // Later when constructing the symbol table, we will check for this
            // and fill it in correctly.
            ASR::symbolType ty = static_cast<ASR::symbolType>(symbol_type);
            ASR::symbol_t *s;
            switch (ty) {
                case (ASR::symbolType::Function) : {
                    Location loc;
                    s = ASR::down_cast<ASR::symbol_t>(ASR::make_Function_t(al,
                        loc,
                        nullptr, nullptr,
                        nullptr, 0,
                        nullptr, 0,
                        nullptr, ASR::abiType::Source,
                        ASR::accessType::Public));
                    break;
                }
                default : throw LFortranException("Symbol type not supported");
            }
            symtab->scope[symbol_name] = s;
        }
        ASR::symbol_t *sym = symtab->scope[symbol_name];
        return sym;
    }

    void symtab_insert_symbol(SymbolTable &symtab, const std::string &name,
        ASR::symbol_t *sym) {
        if (symtab.scope.find(name) == symtab.scope.end()) {
            symtab.scope[name] = sym;
        } else {
            // We have to copy the contents of `sym` into `sym2` without
            // changing the `sym2` pointer already in the table
            ASR::symbol_t *sym2 = symtab.scope[name];
            LFORTRAN_ASSERT(sym2->type == sym->type);
            switch (sym2->type) {
                case (ASR::symbolType::Function) : {
                    ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(sym);
                    ASR::Function_t *f2 = ASR::down_cast<ASR::Function_t>(sym2);
                    f2->base.type = f->base.type;
                    f2->base.base.type = f->base.base.type;
                    f2->base.base.loc = f->base.base.loc;
                    f2->m_symtab = f->m_symtab;
                    f2->m_name = f->m_name;
                    f2->m_args = f->m_args;
                    f2->n_args = f->n_args;
                    f2->m_body = f->m_body;
                    f2->n_body = f->n_body;
                    f2->m_return_var = f->m_return_var;
                    f2->m_abi = f->m_abi;
                    f2->m_access = f->m_access;
                    break;
                }
                default : throw LFortranException("Symbol type not supported");
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
        for (auto &a : x.m_global_scope->scope) {
            this->visit_symbol(*a.second);
        }
    }

    void visit_Program(const Program_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        x.m_symtab->parent = parent_symtab;
        for (auto &a : x.m_symtab->scope) {
            this->visit_symbol(*a.second);
        }
        current_symtab = parent_symtab;
    }

    void visit_Subroutine(const Subroutine_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        x.m_symtab->parent = parent_symtab;
        for (auto &a : x.m_symtab->scope) {
            this->visit_symbol(*a.second);
        }
        current_symtab = parent_symtab;
    }

    void visit_Function(const Function_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        x.m_symtab->parent = parent_symtab;
        for (auto &a : x.m_symtab->scope) {
            this->visit_symbol(*a.second);
        }
        current_symtab = parent_symtab;
    }

};

} // namespace ASR

ASR::asr_t* deserialize_asr(Allocator &al, const std::string &s) {
    ASRDeserializationVisitor v(al, s);
    ASR::asr_t *node = v.deserialize_node();
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(node);

    // Connect the `parent` member of symbol tables
    ASR::FixParentSymtabVisitor p;
    p.visit_TranslationUnit(*tu);

    LFORTRAN_ASSERT(asr_verify(*tu));

    return node;
}

} // namespace LFortran
