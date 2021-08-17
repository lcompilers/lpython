#include <iostream>
#include <string>
#include <sstream>

#include <lfortran/parser/parser.h>
#include <lfortran/parser/parser.tab.hh>

namespace LFortran
{

AST::TranslationUnit_t* parse(Allocator &al, const std::string &s)
{
    Parser p(al);
    p.parse(s);
    Location l;
    if (p.result.size() == 0) {
        l.first_line=0;
        l.first_column=0;
        l.last_line=0;
        l.last_column=0;
    } else {
        l.first_line=p.result[0]->loc.first_line;
        l.first_column=p.result[0]->loc.first_column;
        l.last_line=p.result[p.result.size()-1]->loc.last_line;
        l.last_column=p.result[p.result.size()-1]->loc.last_column;
    }
    return (AST::TranslationUnit_t*)AST::make_TranslationUnit_t(al, l,
        p.result.p, p.result.size());
}

AST::TranslationUnit_t* parse2(Allocator &al, const std::string &code_original,
        bool use_colors, bool fixed_form)
{
    LFortran::LocationManager lm;
    std::string code_prescanned = LFortran::fix_continuation(code_original, lm,
            fixed_form);
    AST::TranslationUnit_t* result;
    try {
        result = parse(al, code_prescanned);
    } catch (const LFortran::ParserError &e) {
        int token;
        if (e.msg() == "syntax is ambiguous") {
            token = -2;
        } else {
            token = e.token;
        }
        std::cerr << format_syntax_error("input", code_prescanned, e.loc, token, nullptr, use_colors);
        throw;
    } catch (const LFortran::TokenizerError &e) {
        std::cerr << format_syntax_error("input", code_prescanned, e.loc, -1, &e.token, use_colors);
        throw;
    }
    return result;
}

void Parser::parse(const std::string &input)
{
    inp = input;
    if (inp.size() > 0) {
        if (inp[inp.size()-1] != '\n') inp.append("\n");
    } else {
        inp.append("\n");
    }
    m_tokenizer.set_string(inp);
    if (yyparse(*this) == 0) {
        return;
    }
    Location loc;
    throw ParserError("Parsing Unsuccessful", loc, 0);
}

std::vector<int> tokens(Allocator &al, const std::string &input,
        std::vector<LFortran::YYSTYPE> *stypes)
{
    LFortran::Tokenizer t;
    t.set_string(input);
    std::vector<int> tst;
    int token = yytokentype::END_OF_FILE + 1; // Something different from EOF
    while (token != yytokentype::END_OF_FILE) {
        LFortran::YYSTYPE y;
        LFortran::Location l;
        token = t.lex(al, y, l);
        tst.push_back(token);
        if (stypes) stypes->push_back(y);
    }
    return tst;
}

void cont1(const std::string &s, size_t &pos, bool &ws_or_comment)
{
    ws_or_comment = true;
    bool in_comment = false;
    while (s[pos] != '\n') {
        if (s[pos] == '!') in_comment = true;
        if (!in_comment) {
            if (s[pos] != ' ' && s[pos] != '\t') {
                ws_or_comment = false;
                return;
            }
        }
        pos++;
    }
    pos++;
}

enum LineType {
    Comment, Statement, LabeledStatement, Continuation, EndOfFile
};

// Determines the type of line
// `pos` points to the first character (column) of the line
// The line ends with either `\n` or `\0`.
LineType determine_line_type(const unsigned char *pos)
{
    int col=1;
    if (*pos == '\n') {
        // Empty line => classified as comment
        return LineType::Comment;
    } else if (*pos == '*' || *pos == 'c' || *pos == 'C' || *pos == '!') {
        // Comment
        return LineType::Comment;
    } else if (*pos == '\0') {
        return LineType::EndOfFile;
    } else {
        while (*pos == ' ') {
            pos++;
            col+=1;
        }
        if (*pos == '\n' || *pos == '\0') return LineType::Comment;
        if (*pos == '!' && col != 6) return LineType::Comment;
        if (col == 6) {
            if (*pos == ' ' || *pos == '0') {
                return LineType::Statement;
            } else {
                return LineType::Continuation;
            }
        }
        if (col <= 6) {
            return LineType::LabeledStatement;
        } else {
            return LineType::Statement;
        }
    }
}

void skip_rest_of_line(const std::string &s, size_t &pos)
{
    while (pos < s.size() && s[pos] != '\n') {
        pos++;
    }
    pos++; // Skip the last '\n'
}

// Parses string, including possible continuation lines
void parse_string(std::string &out, const std::string &s, size_t &pos)
{
    char quote = s[pos];
    LFORTRAN_ASSERT(quote == '"' || quote == '\'');
    out += s[pos];
    pos++;
    while (pos < s.size() && ! (s[pos] == quote && s[pos+1] != quote)) {
        if (s[pos] == '\n') {
            pos++;
            pos += 6;
            continue;
        }
        if (s[pos] == quote && s[pos+1] == quote) {
            out += s[pos];
            pos++;
        }
        out += s[pos];
        pos++;
    }
    out += s[pos]; // Copy the last quote
    pos++;
}

bool is_num(char c)
{
    return '0' <= c && c <= '9';
}

void copy_label(std::string &out, const std::string &s, size_t &pos)
{
    size_t col = 1;
    while (pos < s.size() && s[pos] != '\n' && col <= 6) {
        out += s[pos];
        pos++;
        col++;
    }
}

void copy_rest_of_line(std::string &out, const std::string &s, size_t &pos)
{
    while (pos < s.size() && s[pos] != '\n') {
        if (s[pos] == '"' || s[pos] == '\'') {
            parse_string(out, s, pos);
        } else if (s[pos] == '!') {
            skip_rest_of_line(s, pos);
            out += '\n';
            return;
        } else {
            out += s[pos];
            pos++;
        }
    }
    out += s[pos]; // Copy the last `\n'
    pos++;
}

std::string fix_continuation(const std::string &s, LocationManager &lm,
        bool fixed_form)
{
    if (fixed_form) {
        // `pos` is the position in the original code `s`
        // `out` is the final code (outcome)
        lm.out_start.push_back(0);
        lm.in_start.push_back(0);
        std::string out;
        size_t pos = 0;
        /* Note:
         * This should be a valid fixed form prescanner, except the following
         * features which are currently not implemented:
         *
         *   * Continuation lines after comment(s) or empty lines (they will be
         *     appended to the previous comment, and thus skipped)
         *   * Characters after column 72 are included, but should be ignored
         *   * White space is preserved (but should be removed)
         *
         * The parser together with this fixed form prescanner works as a fixed
         * form parser with some limitations. Due to the last point above,
         * white space is not ignored because it is needed for the parser, so
         * the following are not supported:
         *
         *   * Extra space: `.  and.`, `3.5 55 d0`, ...
         *   * Missing space: `doi=1,5`, `callsome_subroutine(x)`
         *
         * It turns out most fixed form codes use white space as one would
         * expect, so it is not such a big problem and the fixes needed to do
         * in the fixed form Fortran code are relatively minor in practice.
         */
        while (true) {
            const char *p = &s[pos];
            LineType lt = determine_line_type((const unsigned char*)p);
            switch (lt) {
                case LineType::Comment : {
                    // Skip
                    skip_rest_of_line(s, pos);
                    break;
                }
                case LineType::Statement : {
                    // Copy from column 7
                    pos += 6;
                    copy_rest_of_line(out, s, pos);
                    break;
                }
                case LineType::LabeledStatement : {
                    // Copy the label
                    copy_label(out, s, pos);
                    // Copy from column 7
                    copy_rest_of_line(out, s, pos);
                    break;
                }
                case LineType::Continuation : {
                    // Append from column 7 to previous line
                    out = out.substr(0, out.size()-1); // Remove the last '\n'
                    pos += 6;
                    copy_rest_of_line(out, s, pos);
                    break;
                }
                case LineType::EndOfFile : {
                    break;
                }
            };
            if (lt == LineType::EndOfFile) break;
        }
        return out;
    } else {
        // `pos` is the position in the original code `s`
        // `out` is the final code (outcome)
        lm.out_start.push_back(0);
        lm.in_start.push_back(0);
        std::string out;
        size_t pos = 0;
        bool in_comment = false;
        while (pos < s.size()) {
            if (s[pos] == '!') in_comment = true;
            if (in_comment && s[pos] == '\n') in_comment = false;
            if (!in_comment && s[pos] == '&') {
                size_t pos2=pos+1;
                bool ws_or_comment;
                cont1(s, pos2, ws_or_comment);
                if (ws_or_comment) {
                    while (ws_or_comment) {
                        cont1(s, pos2, ws_or_comment);
                    }
                    // `pos` will move by more than 1, close the old interval
    //                lm.in_size.push_back(pos-lm.in_start[lm.in_start.size()-1]);
                    // Move `pos`
                    pos = pos2;
                    if (s[pos] == '&') pos++;
                    // Start a new interval (just the starts, the size will be
                    // filled in later)
                    lm.out_start.push_back(out.size());
                    lm.in_start.push_back(pos);
                }
            }
            out += s[pos];
            pos++;
        }
        // set the size of the last interval
    //    lm.in_size.push_back(pos-lm.in_start[lm.in_start.size()-1]);

        return out;
    }
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
        T(TK_NEWLINE, "newline")
        T(TK_NAME, "identifier")
        T(TK_DEF_OP, "defined operator")
        T(TK_INTEGER, "integer")
        T(TK_REAL, "real")
        T(TK_BOZ_CONSTANT, "BOZ constant")

        T(TK_PLUS, "+")
        T(TK_MINUS, "-")
        T(TK_STAR, "*")
        T(TK_SLASH, "/")
        T(TK_COLON, ":")
        T(TK_SEMICOLON, ";")
        T(TK_COMMA, ",")
        T(TK_EQUAL, "=")
        T(TK_LPAREN, "(")
        T(TK_RPAREN, ")")
        T(TK_LBRACKET, "[")
        T(TK_RBRACKET, "]")
        T(TK_RBRACKET_OLD, "/)")
        T(TK_PERCENT, "%")
        T(TK_VBAR, "|")

        T(TK_STRING, "string")
        T(TK_COMMENT, "comment")
        T(TK_LABEL, "label")

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

        T(TK_FORMAT, "format")

        T(KW_ABSTRACT, "abstract")
        T(KW_ALL, "all")
        T(KW_ALLOCATABLE, "allocatable")
        T(KW_ALLOCATE, "allocate")
        T(KW_ASSIGN, "assign")
        T(KW_ASSIGNMENT, "assignment")
        T(KW_ASSOCIATE, "associate")
        T(KW_ASYNCHRONOUS, "asynchronous")
        T(KW_BACKSPACE, "backspace")
        T(KW_BIND, "bind")
        T(KW_BLOCK, "block")
        T(KW_CALL, "call")
        T(KW_CASE, "case")
        T(KW_CHANGE, "change")
        T(KW_CHANGE_TEAM, "changeteam")
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
        T(KW_DOUBLE_PRECISION, "doubleprecision")
        T(KW_ELEMENTAL, "elemental")
        T(KW_ELSE, "else")
        T(KW_ELSEIF, "elseif")
        T(KW_ELSEWHERE, "elsewhere")

        T(KW_END, "end")
        T(KW_END_DO, "end do")
        T(KW_ENDDO, "enddo")
        T(KW_END_IF, "end if")
        T(KW_ENDIF, "endif")
        T(KW_END_INTERFACE, "end interface")
        T(KW_ENDINTERFACE, "endinterface")
        T(KW_END_TYPE, "end type")
        T(KW_ENDTYPE, "endtype")
        T(KW_END_PROGRAM, "end program")
        T(KW_ENDPROGRAM, "endprogram")
        T(KW_END_MODULE, "end module")
        T(KW_ENDMODULE, "endmodule")
        T(KW_END_SUBMODULE, "end submodule")
        T(KW_ENDSUBMODULE, "endsubmodule")
        T(KW_END_BLOCK, "end block")
        T(KW_ENDBLOCK, "endblock")
        T(KW_END_BLOCK_DATA, "end block data")
        T(KW_ENDBLOCKDATA, "endblockdata")
        T(KW_END_SUBROUTINE, "end subroutine")
        T(KW_ENDSUBROUTINE, "endsubroutine")
        T(KW_END_FUNCTION, "end function")
        T(KW_ENDFUNCTION, "endfunction")
        T(KW_END_PROCEDURE, "end procedure")
        T(KW_ENDPROCEDURE, "endprocedure")
        T(KW_END_ENUM, "end enum")
        T(KW_ENDENUM, "endenum")
        T(KW_END_SELECT, "end select")
        T(KW_ENDSELECT, "endselect")
        T(KW_END_ASSOCIATE, "end associate")
        T(KW_ENDASSOCIATE, "endassociate")
        T(KW_END_FORALL, "end forall")
        T(KW_ENDFORALL, "endforall")
        T(KW_END_WHERE, "end where")
        T(KW_ENDWHERE, "endwhere")
        T(KW_END_CRITICAL, "end critical")
        T(KW_ENDCRITICAL, "endcritical")
        T(KW_END_FILE, "end file")
        T(KW_ENDFILE, "endfile")
        T(KW_END_TEAM, "end team")
        T(KW_ENDTEAM, "endteam")

        T(KW_ENTRY, "entry")
        T(KW_ENUM, "enum")
        T(KW_ENUMERATOR, "enumerator")
        T(KW_EQUIVALENCE, "equivalence")
        T(KW_ERRMSG, "errmsg")
        T(KW_ERROR, "error")
        T(KW_EVENT, "event")
        T(KW_EXIT, "exit")
        T(KW_EXTENDS, "extends")
        T(KW_EXTERNAL, "external")
        T(KW_FILE, "file")
        T(KW_FINAL, "final")
        T(KW_FLUSH, "flush")
        T(KW_FORALL, "forall")
        T(KW_FORMATTED, "formatted")
        T(KW_FORM, "form")
        T(KW_FORM_TEAM, "formteam")
        T(KW_FUNCTION, "function")
        T(KW_GENERIC, "generic")
        T(KW_GO, "go")
        T(KW_GOTO, "goto")
        T(KW_IF, "if")
        T(KW_IMAGES, "images")
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
        T(KW_MEMORY, "memory")
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
        T(KW_POST, "post")
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
        T(KW_REDUCE, "reduce")
        T(KW_RESULT, "result")
        T(KW_RETURN, "return")
        T(KW_REWIND, "rewind")
        T(KW_SAVE, "save")
        T(KW_SELECT, "select")
        T(KW_SELECT_CASE, "selectcase")
        T(KW_SELECT_RANK, "selectrank")
        T(KW_SELECT_TYPE, "selecttype")
        T(KW_SEQUENCE, "sequence")
        T(KW_SHARED, "shared")
        T(KW_SOURCE, "source")
        T(KW_STAT, "stat")
        T(KW_STOP, "stop")
        T(KW_SUBMODULE, "submodule")
        T(KW_SUBROUTINE, "subroutine")
        T(KW_SYNC, "sync")
        T(KW_SYNC_ALL, "syncall")
        T(KW_SYNC_IMAGES, "synimages")
        T(KW_SYNC_MEMORY, "syncmemory")
        T(KW_SYNC_TEAM, "syncteam")
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
        T(KW_WAIT, "wait")
        T(KW_WHERE, "where")
        T(KW_WHILE, "while")
        T(KW_WRITE, "write")
        default : {
            std::cout << "TOKEN: " << token << std::endl;
            throw LFortranException("Token conversion not implemented yet.");
        }
    }
}

const static std::string redon  = "\033[0;31m";
const static std::string redoff = "\033[0;00m";

std::string highlight_line(const std::string &line,
        const size_t first_column,
        const size_t last_column,
        bool use_colors)
{
    std::stringstream out;
    if (line.size() > 0) {
        out << line.substr(0, first_column-1);
        if (last_column <= line.size()) {
            if(use_colors) out << redon;
            out << line.substr(first_column-1,
                    last_column-first_column+1);
            if(use_colors) out << redoff;
            out << line.substr(last_column);
        }
    }
    out << std::endl;
    if (first_column > 0) {
        for (size_t i=0; i < first_column-1; i++) {
            out << " ";
        }
    }
    if(use_colors) out << redon << "^";
    else out << "^";
    for (size_t i=first_column; i < last_column; i++) {
        out << "~";
    }
    if(use_colors) out << redoff;
    out << std::endl;
    return out.str();
}

std::string format_syntax_error(const std::string &filename,
        const std::string &input, const Location &loc, const int token,
        const std::string *tstr, bool use_colors)
{
    std::stringstream out;
    out << filename << ":" << loc.first_line << ":" << loc.first_column;
    if (loc.first_line != loc.last_line) {
        out << " - " << loc.last_line << ":" << loc.last_column;
    }
    if(use_colors) out << " " << redon << "syntax error:" << redoff << " ";
    else out << " " << "syntax error:" <<  " ";
    if (token == -1) {
        LFORTRAN_ASSERT(tstr != nullptr);
        out << "token '";
        out << *tstr;
        out << "' is not recognized" << std::endl;
    } else if (token == -2) {
        out << "syntax is ambiguous" << std::endl;
    } else {
        if (token == yytokentype::END_OF_FILE) {
            out << "end of file is unexpected here" << std::endl;
        } else {
            out << "token type '";
            out << token2text(token);
            out << "' is unexpected here" << std::endl;
        }
    }
    if (loc.first_line == loc.last_line) {
        std::string line = get_line(input, loc.first_line);
        out << highlight_line(line, loc.first_column, loc.last_column, use_colors);
    } else {
        out << "first (" << loc.first_line << ":" << loc.first_column;
        out << ")" << std::endl;
        std::string line = get_line(input, loc.first_line);
        out << highlight_line(line, loc.first_column, line.size(), use_colors);
        out << "last (" << loc.last_line << ":" << loc.last_column;
        out << ")" << std::endl;
        line = get_line(input, loc.last_line);
        out << highlight_line(line, 1, loc.last_column, use_colors);
    }
    return out.str();
}

std::string format_semantic_error(const std::string &filename,
        const std::string &input, const Location &loc,
        const std::string msg, bool use_colors)
{
    std::stringstream out;
    out << filename << ":" << loc.first_line << ":" << loc.first_column;
    if (loc.first_line != loc.last_line) {
        out << " - " << loc.last_line << ":" << loc.last_column;
    }
    if(use_colors) out << " " << redon << "semantic error:" << redoff << " ";
    else out << " " << "syntax error:" <<  " ";
    out << msg << std::endl;
    if (loc.first_line == loc.last_line) {
        std::string line = get_line(input, loc.first_line);
        out << highlight_line(line, loc.first_column, loc.last_column, use_colors);
    } else {
        out << "first (" << loc.first_line << ":" << loc.first_column;
        out << ")" << std::endl;
        std::string line = get_line(input, loc.first_line);
        out << highlight_line(line, loc.first_column, line.size(), use_colors);
        out << "last (" << loc.last_line << ":" << loc.last_column;
        out << ")" << std::endl;
        line = get_line(input, loc.last_line);
        out << highlight_line(line, 1, loc.last_column, use_colors);
    }
    return out.str();
}

}
