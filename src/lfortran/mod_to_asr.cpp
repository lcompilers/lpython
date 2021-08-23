#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <map>
#include <memory>

#include <zlib.h>

#include <lfortran/asr.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/mod_to_asr.h>
#include <lfortran/string_utils.h>
#include <lfortran/containers.h>


namespace LFortran {

using ASR::down_cast;
using ASR::down_cast2;

int uncompress_gzip(uint8_t *out, uint64_t *out_size, uint8_t *in,
        uint64_t in_size)
{
    // The code below is roughly equivalent to:
    //     return uncompress(out, out_size, in, in_size);
    // except that it enables gzip support in inflateInit2().
    int zlib_status;
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = in;
    strm.avail_in = in_size;
    zlib_status = inflateInit2(&strm, 15 | 32);
    if (zlib_status < 0) return zlib_status;
    strm.next_out = out;
    strm.avail_out = *out_size;
    zlib_status = inflate(&strm, Z_NO_FLUSH);
    inflateEnd(&strm);
    *out_size = *out_size - strm.avail_out;
    return zlib_status;
}

std::string extract_gzip(std::vector<uint8_t> &buffer)
{
    std::vector<uint8_t> data(1024*1024);
    uint64_t data_size = data.size();
    int res = uncompress_gzip(&data[0], &data_size, &buffer[0], buffer.size());
    switch(res) {
        case Z_OK:
        case Z_STREAM_END:
            break;
        case Z_MEM_ERROR:
            throw LFortranException("ZLIB: out of memory");
        case Z_BUF_ERROR:
            throw LFortranException("ZLIB: output buffer was not large enough");
        case Z_DATA_ERROR:
            throw LFortranException("ZLIB: the input data was corrupted or incomplete");
        default:
            throw LFortranException("ZLIB: unknown error (" + std::to_string(res) + ")");
    }
    return std::string((char*) &data[0], data_size);
}

// 'abc' -> abc
std::string str_(const std::string &s)
{
    return s.substr(1, s.size()-2);
}


std::vector<std::string> tokenize(const std::string &s) {
    return split(replace(replace(s, "\\(", " ( "), "\\)", " ) "));
}

struct Item {
    enum {
        list, str, integer, token
    } kind;
    std::string s;
    uint64_t i;
    std::vector<Item> l;
};

#define EXPECT(arg_i, arg_kind)                                  \
    if ((arg_i).kind != (arg_kind)) {                            \
        throw LFortranException("Unexpected item kind"); \
    }

uint64_t item_integer(const Item &i)
{
    EXPECT(i, Item::integer);
    return i.i;
}

const std::string& item_string(const Item &i)
{
    EXPECT(i, Item::str);
    return i.s;
}

const std::string& item_token(const Item &i)
{
    EXPECT(i, Item::token);
    return i.s;
}

const std::vector<Item>& item_list(const Item &i)
{
    EXPECT(i, Item::list);
    return i.l;
}

Item read_from_tokens(const std::vector<std::string> &tokens, size_t &pos)
{
    if (pos == tokens.size()) {
        throw LFortranException("Unexpected EOF while reading");
    }
    std::string token = tokens[pos];
    LFORTRAN_ASSERT(token.size() > 0);
    pos++;
    if (token == "(") {
        Item r;
        r.kind = Item::list;
        while (tokens[pos] != ")") {
            r.l.push_back(read_from_tokens(tokens, pos));
        }
        pos++; // pop off ')'
        return r;
    } else if (token == ")") {
        throw LFortranException("Unexpected )");
    } else {
        Item r;
        if (token[0] == '\'') {
            r.kind = Item::str;
            r.s = str_(token);
        } else if (token[0] >= '0' && token[0] <= '9') {
            r.kind = Item::integer;
            r.i = std::stoi(token);
        } else {
            r.kind = Item::token;
            r.s = token;
        }
        return r;
    }
}



Item parse(const std::string &source)
{
    size_t pos = 0;
    return read_from_tokens(tokenize(source), pos);
}

std::string format_item(const Item &i)
{
    if (i.kind == Item::str) {
        return "\"" + i.s + "\"";
    } else if (i.kind == Item::integer) {
        return std::to_string(i.i);
    } else if (i.kind == Item::token) {
        return i.s;
    } else {
        LFORTRAN_ASSERT(i.kind == Item::list);
        std::string s;
        s += "(";
        for (size_t j=0; j<i.l.size(); j++) {
            s += format_item(i.l[j]);
            if (j < i.l.size()-1) s+= " ";
        }
        s += ")";
        return s;
    }
}

struct GSymbol {
    uint64_t id;
    std::string name;
    std::string module_name;
    bool is_public;
    //std::vector<Item> info;
    Item info;
    enum {
        variable, procedure, module
    } kind;
    struct {
        int intent;
        bool dummy;
        ASR::ttype_t *type;
        ASR::symbol_t *var;
    } v;
    struct {
        // type
        std::vector<uint64_t> arg_ids;
        uint64_t return_sym_id;
        ASR::symbol_t *proc;
    } p;
};

ASR::ttype_t* parse_type(Allocator &al, const std::vector<Item> &l)
{
    LFORTRAN_ASSERT(l.size() == 7);
    std::string name = item_token(l[0]);
    if (name == "INTEGER") {
        Location loc;
        ASR::asr_t *t = ASR::make_Integer_t(al, loc, 4, nullptr, 0);
        return down_cast<ASR::ttype_t>(t);
    } else {
        throw LFortranException("Type not supported yet");
    }
}

ASR::TranslationUnit_t* parse_gfortran_mod_file(Allocator &al, const std::string &s)
{
    std::vector<std::string> s2 = split(s);
    int version = std::atoi(&str_(s2[3])[0]);
    if (version != 14) {
        throw LFortranException("Only GFortran module version 14 is implemented so far");
    }
    std::vector<std::string> s3 = slice(s2, 7);
    std::string s4 = "(" + join(" ", s3) + ")";

    std::map<uint64_t, GSymbol> gsymtab;
    SymbolTable *parent_scope = al.make_new<SymbolTable>(nullptr);

    Item mod = parse(s4);
    EXPECT(mod, Item::list);
    if (mod.l.size() == 8) {
        std::vector<Item> symtab = item_list(mod.l[6]);
        if (symtab.size() % 6 != 0) {
            throw LFortranException("Symtab not multiple of 6");
        }
        for (size_t i=0; i < symtab.size(); i+=6) {
            GSymbol s;
            s.id = item_integer(symtab[i]);
            s.name = item_string(symtab[i+1]);
            s.module_name = item_string(symtab[i+2]);
            s.info = symtab[i+5];
            s.is_public = false;
            std::vector<Item> info = item_list(symtab[i+5]);
            LFORTRAN_ASSERT(info.size() == 12);
            std::vector<Item> info_sym_info = item_list(info[0]);
            std::string kind = item_token(info_sym_info[0]);
            if (kind == "VARIABLE") {
                s.kind = GSymbol::variable;
                std::vector<Item> sym_type = item_list(info[2]);
                s.v.type = parse_type(al, sym_type);
                Str a;
                a.from_str_view(s.name);
                char *name = a.c_str(al);
                Location loc;
                ASR::asr_t *asr = ASR::make_Variable_t(al, loc, nullptr,
                    name, ASR::intentType::In, nullptr, nullptr,
                    ASR::storage_typeType::Default, s.v.type,
                    ASR::abiType::GFortranModule,
                    ASR::Public, ASR::presenceType::Required, false);
                s.v.var = down_cast<ASR::symbol_t>(asr);
            } else if (kind == "PROCEDURE") {
                s.kind = GSymbol::procedure;
                Location loc;
                SymbolTable *proc_symtab = al.make_new<SymbolTable>(parent_scope);
                Str a;
                a.from_str_view(s.name);
                char *name = a.c_str(al);
                ASR::asr_t *asr = ASR::make_Subroutine_t(al, loc,
                    proc_symtab, name, nullptr, 0,
                    nullptr, 0, ASR::abiType::GFortranModule, ASR::Public, 
                    ASR::Interface, nullptr);
                s.p.proc = down_cast<ASR::symbol_t>(asr);
                std::string sym_name = s.name;
                if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
                    throw SemanticError("Procedure already defined", asr->loc);
                }
                parent_scope->scope[sym_name] = ASR::down_cast<ASR::symbol_t>(asr);

                std::vector<Item> args = item_list(info[5]);
                for (auto &arg : args) {
                    s.p.arg_ids.push_back(item_integer(arg));
                }
            } else if (kind == "MODULE") {
                s.kind = GSymbol::module;
            } else {
                throw LFortranException("Symbol kind not supported");
            }

            if (gsymtab.find(s.id) != gsymtab.end()) {
                throw LFortranException("Symbol redeclared");
            }
            gsymtab[s.id] = s;
        }

        std::vector<Item> public_symbols = item_list(mod.l[7]);
        if (public_symbols.size() % 3 != 0) {
            throw LFortranException("Public symbols list not multiple of 3");
        }
        for (size_t i=0; i < public_symbols.size(); i+=3) {
            std::string symbol_name = item_string(public_symbols[i]);
            uint64_t ambiguous_flag = item_integer(public_symbols[i+1]);
            uint64_t symbol_id = item_integer(public_symbols[i+2]);
            if (ambiguous_flag != 0) {
                throw LFortranException("Ambiguous symbols not supported yet");
            }
            if (gsymtab.find(symbol_id) == gsymtab.end()) {
                throw LFortranException("Public symbol not defined in the symbol table");
            }
            gsymtab[symbol_id].is_public = true;
            if (gsymtab[symbol_id].name != symbol_name) {
                throw LFortranException("Public symbol name mismatch");
            }
        }
    } else {
        throw LFortranException("Unexpected number of items");
    }

    /*
    std::cout << "Symbol table" << std::endl;
    for (auto &item : gsymtab) {
        std::cout << item.second.id << " " << item.second.name << " " << item.second.module_name << " " << item.second.is_public << " ";
        std::cout << format_item(item.second.info) << std::endl;
        //std::cout << item.second.kind << std::endl;
        if (item.second.kind == GSymbol::procedure) {
            for (auto &arg : item.second.p.arg_ids) {
                std::cout << arg << " " << gsymtab[arg].name << " ";
            }
            std::cout << std::endl;
        }
    }
    */


    ASR::asr_t *asr;
    Location loc;
    asr = ASR::make_TranslationUnit_t(al, loc,
        parent_scope, nullptr, 0);
    ASR::TranslationUnit_t *tu = down_cast2<ASR::TranslationUnit_t>(asr);
    LFORTRAN_ASSERT(asr_verify(*tu));
    return tu;

    //std::cout << format_item(mod);

    //std::cout << s;
}

ASR::TranslationUnit_t *mod_to_asr(Allocator &al, std::string filename)
{
    std::ifstream in(filename, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(in), {});
    std::string s;
    if (buffer.size() > 2 && buffer[0] == 0x1f && buffer[1] == 0x8b) {
        // GZip format
        s = extract_gzip(buffer);
    } else {
        // Assuming plain text
        s = std::string((char*) &buffer[0], buffer.size());
    }

    if (startswith(s, "GFORTRAN module")) {
        return parse_gfortran_mod_file(al, s);
    } else {
        throw LFortranException("Unknown module file format");
    }
}

} // namespace LFortran
