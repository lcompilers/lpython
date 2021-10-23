#include <iostream>
#include <string>
#include <sstream>

#include <lfortran/parser/parser.h>
#include <lfortran/parser/parser.tab.hh>
#include <lfortran/diagnostics.h>
#include <lfortran/parser/parser_exception.h>

namespace LFortran
{

Result<AST::TranslationUnit_t*> parse(Allocator &al, const std::string &s,
        diag::Diagnostics &diagnostics)
{
    Parser p(al, diagnostics);
    try {
        p.parse(s);
    } catch (const parser_local::TokenizerError &e) {
        Error error;
        diagnostics.diagnostics.push_back(e.d);
        return error;
    } catch (const parser_local::ParserError &e) {
        Error error;
        diagnostics.diagnostics.push_back(e.d);
        return error;
    }
    Location l;
    if (p.result.size() == 0) {
        l.first=0;
        l.last=0;
    } else {
        l.first=p.result[0]->loc.first;
        l.last=p.result[p.result.size()-1]->loc.last;
    }
    return (AST::TranslationUnit_t*)AST::make_TranslationUnit_t(al, l,
        p.result.p, p.result.size());
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
    throw parser_local::ParserError("Parsing unsuccessful (internal compiler error)");
}

Result<std::vector<int>> tokens(Allocator &al, const std::string &input,
        diag::Diagnostics &diagnostics,
        std::vector<YYSTYPE> *stypes,
        std::vector<Location> *locations)
{
    Tokenizer t;
    t.set_string(input);
    std::vector<int> tst;
    int token = yytokentype::END_OF_FILE + 1; // Something different from EOF
    while (token != yytokentype::END_OF_FILE) {
        YYSTYPE y;
        Location l;
        try {
            token = t.lex(al, y, l, diagnostics);
        } catch (const parser_local::TokenizerError &e) {
            diagnostics.diagnostics.push_back(e.d);
            return Error();
        }
        tst.push_back(token);
        if (stypes) stypes->push_back(y);
        if (locations) locations->push_back(l);
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

// Checks that newlines are computed correctly
bool check_newlines(const std::string &s, const std::vector<uint32_t> &newlines) {
    std::vector<uint32_t> newlines2;
    for (uint32_t pos=0; pos < s.size(); pos++) {
        if (s[pos] == '\n') newlines2.push_back(pos);
    }
    if (newlines2.size() != newlines.size()) return false;
    for (size_t i=0; i < newlines2.size(); i++) {
        if (newlines2[i] != newlines[i]) return false;
    }
    return true;
}

std::string fix_continuation(const std::string &s, LocationManager &lm,
        bool fixed_form)
{
    if (fixed_form) {
        // `pos` is the position in the original code `s`
        // `out` is the final code (outcome)
        lm.get_newlines(s, lm.in_newlines);
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
                    lm.out_start.push_back(out.size());
                    lm.in_start.push_back(pos);
                    break;
                }
                case LineType::Statement : {
                    // Copy from column 7
                    pos += 6;
                    lm.out_start.push_back(out.size());
                    lm.in_start.push_back(pos);
                    copy_rest_of_line(out, s, pos);
                    break;
                }
                case LineType::LabeledStatement : {
                    // Copy the label
                    copy_label(out, s, pos);
                    // Copy from column 7
                    lm.out_start.push_back(out.size());
                    lm.in_start.push_back(pos);
                    copy_rest_of_line(out, s, pos);
                    break;
                }
                case LineType::Continuation : {
                    // Append from column 7 to previous line
                    out = out.substr(0, out.size()-1); // Remove the last '\n'
                    pos += 6;
                    lm.out_start.push_back(out.size());
                    lm.in_start.push_back(pos);
                    copy_rest_of_line(out, s, pos);
                    break;
                }
                case LineType::EndOfFile : {
                    break;
                }
            };
            if (lt == LineType::EndOfFile) break;
        }
        lm.in_start.push_back(pos);
        lm.out_start.push_back(out.size());
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
                if (ws_or_comment) lm.in_newlines.push_back(pos2-1);
                if (ws_or_comment) {
                    while (ws_or_comment) {
                        cont1(s, pos2, ws_or_comment);
                        if (ws_or_comment) lm.in_newlines.push_back(pos2-1);
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
            } else {
                if (s[pos] == '\n') lm.in_newlines.push_back(pos);
            }
            out += s[pos];
            pos++;
        }
        // set the size of the last interval
    //    lm.in_size.push_back(pos-lm.in_start[lm.in_start.size()-1]);

        LFORTRAN_ASSERT(check_newlines(s, lm.in_newlines))

        // Add the position of EOF as the last \n, whether or not the original
        // file has it
        lm.in_start.push_back(pos);
        lm.out_start.push_back(out.size());
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
        T(KW_NEW_INDEX, "new_index")
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

void Parser::handle_yyerror(const Location &loc, const std::string &msg)
{
    std::string message;
    if (msg == "syntax is ambiguous") {
        message = "Internal Compiler Error: syntax is ambiguous in the parser";
    } else if (msg == "syntax error") {
        LFortran::YYSTYPE yylval_;
        YYLTYPE yyloc_;
        this->m_tokenizer.cur = this->m_tokenizer.tok;
        int token = this->m_tokenizer.lex(this->m_a, yylval_, yyloc_, diag);
        if (token == yytokentype::END_OF_FILE) {
            message =  "End of file is unexpected here";
        } else if (token == yytokentype::TK_NEWLINE) {
            message =  "Newline is unexpected here";
        } else {
            std::string token_str = this->m_tokenizer.token();
            std::string token_type = token2text(token);
            if (token_str == token_type) {
                message =  "Token '" + token_str + "' is unexpected here";
            } else {
                message =  "Token '" + token_str + "' (of type '" + token2text(token) + "') is unexpected here";
            }
        }
    } else {
        message = "Internal Compiler Error: parser returned unknown error";
    }
    throw parser_local::ParserError(message, loc);
}

void populate_span(diag::Span &s, const LocationManager &lm,
        const std::string &input) {
    lm.pos_to_linecol(lm.output_to_input_pos(s.loc.first, false),
        s.first_line, s.first_column);
    lm.pos_to_linecol(lm.output_to_input_pos(s.loc.last, true),
        s.last_line, s.last_column);
    s.filename = lm.in_filename;
    for (uint32_t i = s.first_line; i <= s.last_line; i++) {
        s.source_code.push_back(get_line(input, i));
    }
}

// Loop over all labels and their spans, populate all of them
void populate_spans(diag::Diagnostic &d, const LocationManager &lm,
        const std::string &input) {
    for (auto &l : d.labels) {
        for (auto &s : l.spans) {
            populate_span(s, lm, input);
        }
    }
}

}
