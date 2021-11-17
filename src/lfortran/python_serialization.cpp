#include <string>

#include <lfortran/config.h>
#include <lfortran/python_serialization.h>
#include <lfortran/bwriter.h>
#include <lfortran/string_utils.h>


namespace LFortran::Python {

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
