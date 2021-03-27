#include <string>

#include <lfortran/serialization.h>
#include <lfortran/parser/parser.h>
#include <lfortran/parser/parser.tab.hh>
#include <lfortran/asr_utils.h>


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

class SerializationVisitor : public
                             AST::SerializationBaseVisitor<SerializationVisitor>
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
    SerializationVisitor v;
    v.write_int8(ast.type);
    v.visit_ast(ast);
    return v.get_str();
}

std::string serialize(AST::TranslationUnit_t &unit) {
    return serialize((AST::ast_t&)(unit));
}

class DeserializationVisitor : public
                             AST::DeserializationBaseVisitor<DeserializationVisitor>
{
private:
    std::string s;
    size_t pos;
public:
    DeserializationVisitor(Allocator &al, const std::string &s) :
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

AST::ast_t* deserialize(Allocator &al, const std::string &s) {
    DeserializationVisitor v(al, s);
    return v.deserialize_node();
}

}
