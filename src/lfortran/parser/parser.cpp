#include <string>
#include <sstream>

#include <lfortran/parser/parser.h>
#include <lfortran/parser/parser.tab.hh>

namespace LFortran
{

LFortran::AST::ast_t *parse(Allocator &al, const std::string &s)
{
    Parser p(al);
    p.parse(s);
    LFORTRAN_ASSERT(p.result.size() >= 1);
    return p.result[0];
}

LFortran::AST::ast_t *parse2(Allocator &al, const std::string &s)
{
    LFortran::AST::ast_t *result;
    try {
        result = parse(al, s);
    } catch (const LFortran::ParserError &e) {
        int token;
        if (e.msg() == "syntax is ambiguous") {
            token = -2;
        } else {
            token = e.token;
        }
        show_syntax_error("input", s, e.loc, token);
        throw;
    } catch (const LFortran::TokenizerError &e) {
        show_syntax_error("input", s, e.loc, -1, &e.token);
        throw;
    }
    return result;
}

std::vector<LFortran::AST::ast_t*> parsen(Allocator &al, const std::string &s)
{
    Parser p(al);
    p.parse(s);
    return p.result;
}

void Parser::parse(const std::string &input)
{
    inp = input;
    if (inp[inp.size()-1] != '\n') inp.append("\n");
    m_tokenizer.set_string(inp);
    if (yyparse(*this) == 0) {
        LFORTRAN_ASSERT(result.size() >= 1);
        return;
    }
    Location loc;
    throw ParserError("Parsing Unsuccessful", loc, 0);
}

std::string get_line(std::string str, int n)
{
    std::string line;
    std::stringstream s(str);
    for (int i=0; i < n; i++) {
        std::getline(s, line);
    }
    return line;
}

#define T(tk, name) case (yytokentype::tk) : return name;

std::string token2text(const int token)
{
    if (0 < token && token < 256) {
        char t = token;
        return std::string(&t, 1);
    }
    switch (token) {
        T(END_OF_FILE, "end of file")
        T(TK_NAME, "identifier")
        T(TK_DEF_OP, "defined operator")
        T(TK_INTEGER, "integer")
        T(TK_REAL, "real")
        T(TK_NEWLINE, "newline")
        T(TK_STRING, "string")
        T(TK_DBL_DOT, "..")
        T(TK_DBL_COLON, "::")
        T(TK_POW, "**")
        T(TK_CONCAT, "//")
        T(TK_ARROW, "=>")
        T(TK_EQ, "==")
        T(TK_NE, "!=")
        T(TK_LT, "<")
        T(TK_LE, "<=")
        T(TK_GT, ">")
        T(TK_GE, ">=")
        T(TK_NOT, ".not.")
        T(TK_AND, ".and.")
        T(TK_OR, ".or.")
        T(TK_EQV, ".eqv.")
        T(TK_NEQV, ".neqv.")
        T(TK_TRUE, ".true.")
        T(TK_FALSE, ".false.")
        T(KW_ABSTRACT, "abstract")
        T(KW_ALL, "all")
        T(KW_ALLOCATABLE, "allocatable")
        T(KW_ALLOCATE, "allocate")
        T(KW_ASSIGNMENT, "assignment")
        T(KW_ASSOCIATE, "associate")
        T(KW_ASYNCHRONOUS, "asynchronous")
        T(KW_BACKSPACE, "backspace")
        T(KW_BIND, "bind")
        T(KW_BLOCK, "block")
        T(KW_CALL, "call")
        T(KW_CASE, "case")
        T(KW_CHARACTER, "character")
        T(KW_CLASS, "class")
        T(KW_CLOSE, "close")
        T(KW_CODIMENSION, "codimension")
        T(KW_COMMON, "common")
        T(KW_COMPLEX, "complex")
        T(KW_CONCURRENT, "concurrent")
        T(KW_CONTAINS, "contains")
        T(KW_CONTIGUOUS, "contiguous")
        T(KW_CONTINUE, "continue")
        T(KW_CRITICAL, "critical")
        T(KW_CYCLE, "cycle")
        T(KW_DATA, "data")
        T(KW_DEALLOCATE, "deallocate")
        T(KW_DEFAULT, "default")
        T(KW_DEFERRED, "deferred")
        T(KW_DIMENSION, "dimension")
        T(KW_DO, "do")
        T(KW_DOWHILE, "dowhile")
        T(KW_DOUBLE, "double")
        T(KW_ELEMENTAL, "elemental")
        T(KW_ELSE, "else")
        T(KW_END, "end")
        T(KW_END_DO, "end do")
        T(KW_ENDDO, "enddo")
        T(KW_END_IF, "end if")
        T(KW_ENDIF, "endif")
        T(KW_ENTRY, "entry")
        T(KW_ENUM, "enum")
        T(KW_ENUMERATOR, "enumerator")
        T(KW_EQUIVALENCE, "equivalence")
        T(KW_ERRMSG, "errmsg")
        T(KW_ERROR, "error")
        T(KW_EXIT, "exit")
        T(KW_EXTENDS, "extends")
        T(KW_EXTERNAL, "external")
        T(KW_FILE, "file")
        T(KW_FINAL, "final")
        T(KW_FLUSH, "flush")
        T(KW_FORALL, "forall")
        T(KW_FORMAT, "format")
        T(KW_FORMATTED, "formatted")
        T(KW_FUNCTION, "function")
        T(KW_GENERIC, "generic")
        T(KW_GO, "go")
        T(KW_IF, "if")
        T(KW_IMPLICIT, "implicit")
        T(KW_IMPORT, "import")
        T(KW_IMPURE, "impure")
        T(KW_IN, "in")
        T(KW_INCLUDE, "include")
        T(KW_INOUT, "inout")
        T(KW_INQUIRE, "inquire")
        T(KW_INTEGER, "integer")
        T(KW_INTENT, "intent")
        T(KW_INTERFACE, "interface")
        T(KW_INTRINSIC, "intrinsic")
        T(KW_IS, "is")
        T(KW_KIND, "kind")
        T(KW_LEN, "len")
        T(KW_LOCAL, "local")
        T(KW_LOCAL_INIT, "local_init")
        T(KW_LOGICAL, "logical")
        T(KW_MODULE, "module")
        T(KW_MOLD, "mold")
        T(KW_NAME, "name")
        T(KW_NAMELIST, "namelist")
        T(KW_NOPASS, "nopass")
        T(KW_NON_INTRINSIC, "non_intrinsic")
        T(KW_NON_OVERRIDABLE, "non_overridable")
        T(KW_NON_RECURSIVE, "non_recursive")
        T(KW_NONE, "none")
        T(KW_NULLIFY, "nullify")
        T(KW_ONLY, "only")
        T(KW_OPEN, "open")
        T(KW_OPERATOR, "operator")
        T(KW_OPTIONAL, "optional")
        T(KW_OUT, "out")
        T(KW_PARAMETER, "parameter")
        T(KW_PASS, "pass")
        T(KW_POINTER, "pointer")
        T(KW_PRECISION, "precision")
        T(KW_PRINT, "print")
        T(KW_PRIVATE, "private")
        T(KW_PROCEDURE, "procedure")
        T(KW_PROGRAM, "program")
        T(KW_PROTECTED, "protected")
        T(KW_PUBLIC, "public")
        T(KW_PURE, "pure")
        T(KW_QUIET, "quiet")
        T(KW_RANK, "rank")
        T(KW_READ, "read")
        T(KW_REAL, "real")
        T(KW_RECURSIVE, "recursive")
        T(KW_RESULT, "result")
        T(KW_RETURN, "return")
        T(KW_REWIND, "rewind")
        T(KW_SAVE, "save")
        T(KW_SELECT, "select")
        T(KW_SEQUENCE, "sequence")
        T(KW_SHARED, "shared")
        T(KW_SOURCE, "source")
        T(KW_STAT, "stat")
        T(KW_STOP, "stop")
        T(KW_SUBMODULE, "submodule")
        T(KW_SUBROUTINE, "subroutine")
        T(KW_TARGET, "target")
        T(KW_TEAM, "team")
        T(KW_TEAM_NUMBER, "team_number")
        T(KW_THEN, "then")
        T(KW_TO, "to")
        T(KW_TYPE, "type")
        T(KW_UNFORMATTED, "unformatted")
        T(KW_USE, "use")
        T(KW_VALUE, "value")
        T(KW_VOLATILE, "volatile")
        T(KW_WHERE, "where")
        T(KW_WHILE, "while")
        T(KW_WRITE, "write")
        default : {
            std::cout << "TOKEN: " << token << std::endl;
            throw std::runtime_error("Token conversion not implemented yet.");
        }
    }
}

const static std::string redon  = "\033[0;31m";
const static std::string redoff = "\033[0;00m";

void highlight_line(const std::string &line,
        const size_t first_column,
        const size_t last_column
        )
{
    std::cout << line.substr(0, first_column-1);
    if (last_column <= line.size()) {
        std::cout << redon;
        std::cout << line.substr(first_column-1,
                last_column-first_column+1);
        std::cout << redoff;
        std::cout << line.substr(last_column);
    }
    std::cout << std::endl;;
    for (size_t i=0; i < first_column-1; i++) {
        std::cout << " ";
    }
    std::cout << redon << "^";
    for (size_t i=first_column; i < last_column; i++) {
        std::cout << "~";
    }
    std::cout << redoff << std::endl;
}

void show_syntax_error(const std::string &filename, const std::string &input,
        const Location &loc, const int token, const std::string *tstr)
{
    std::cout << filename << ":" << loc.first_line << ":" << loc.first_column;
    if (loc.first_line != loc.last_line) {
        std::cout << " - " << loc.last_line << ":" << loc.last_column;
    }
    std::cout << " " << redon << "syntax error:" << redoff << " ";
    if (token == -1) {
        LFORTRAN_ASSERT(tstr != nullptr);
        std::cout << "token '";
        std::cout << *tstr;
        std::cout << "' is not recognized" << std::endl;
    } else if (token == -2) {
        std::cout << "syntax is ambiguous" << std::endl;
    } else {
        if (token == yytokentype::END_OF_FILE) {
            std::cout << "end of file is unexpected here" << std::endl;
        } else {
            std::cout << "token type '";
            std::cout << token2text(token);
            std::cout << "' is unexpected here" << std::endl;
        }
    }
    if (loc.first_line == loc.last_line) {
        std::string line = get_line(input, loc.first_line);
        highlight_line(line, loc.first_column, loc.last_column);
    } else {
        std::cout << "first (" << loc.first_line << ":" << loc.first_column;
        std::cout << ")" << std::endl;
        std::string line = get_line(input, loc.first_line);
        highlight_line(line, loc.first_column, line.size());
        std::cout << "last (" << loc.last_line << ":" << loc.last_column;
        std::cout << ")" << std::endl;
        line = get_line(input, loc.last_line);
        highlight_line(line, 1, loc.last_column);
    }
}

}
