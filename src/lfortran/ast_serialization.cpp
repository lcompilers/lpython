#include <string>

#include <libasr/config.h>
#include <lfortran/ast_serialization.h>
#include <lfortran/parser/parser.h>
#include <lfortran/parser/parser.tab.hh>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/bwriter.h>
#include <libasr/string_utils.h>

using LFortran::ASRUtils::symbol_parent_symtab;
using LFortran::ASRUtils::symbol_name;

namespace LFortran {

class ASTSerializationVisitor :
#ifdef WITH_LFORTRAN_BINARY_MODFILES
        public BinaryWriter,
#else
        public TextWriter,
#endif
        public AST::SerializationBaseVisitor<ASTSerializationVisitor>
{
public:
    void write_bool(bool b) {
        if (b) {
            write_int8(1);
        } else {
            write_int8(0);
        }
    }
};

std::string serialize(const AST::ast_t &ast) {
    ASTSerializationVisitor v;
    v.write_int8(ast.type);
    v.visit_ast(ast);
    return v.get_str();
}

std::string serialize(const AST::TranslationUnit_t &unit) {
    return serialize((AST::ast_t&)(unit));
}

class ASTDeserializationVisitor :
#ifdef WITH_LFORTRAN_BINARY_MODFILES
    public BinaryReader,
#else
    public TextReader,
#endif
    public AST::DeserializationBaseVisitor<ASTDeserializationVisitor>
{
public:
    ASTDeserializationVisitor(Allocator &al, const std::string &s) :
#ifdef WITH_LFORTRAN_BINARY_MODFILES
        BinaryReader(s),
#else
        TextReader(s),
#endif
        DeserializationBaseVisitor(al, true) {}

    bool read_bool() {
        uint8_t b = read_int8();
        return (b == 1);
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


} // namespace LFortran
