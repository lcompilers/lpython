%require "3.0"
%define api.pure full
%define api.value.type {struct LFortran::YYSTYPE}
%param {LFortran::Parser &p}

/*
// Uncomment this to enable parser tracing. Then in the main code, set
// extern int yydebug;
// yydebug=1;
%define parse.trace
%printer { fprintf(yyo, "%s", $$.c_str()); } <string>
%printer { std::cerr << "AST TYPE: " << $$->type; } <ast>
*/


%code requires // *.h
{
#include <lfortran/parser/parser.h>
}

%code // *.cpp
{

#include <lfortran/parser/parser.h>
#include <lfortran/parser/tokenizer.h>
#include <lfortran/parser/semantics.h>

int yylex(LFortran::YYSTYPE *yylval, LFortran::Parser &p)
{
    return p.m_tokenizer.lex(*yylval);
} // ylex

void yyerror(LFortran::Parser &p, const std::string &msg)
{
    throw LFortran::ParserError(msg);
}

// Force YYCOPY to not use memcopy, but rather copy the structs one by one,
// which will cause C++ to call the proper copy constructors.
# define YYCOPY(Dst, Src, Count)              \
    do                                        \
      {                                       \
        YYSIZE_T yyi;                         \
        for (yyi = 0; yyi < (Count); yyi++)   \
          (Dst)[yyi] = (Src)[yyi];            \
      }                                       \
    while (0)

} // code


// -----------------------------------------------------------------------------
// List of tokens
// All tokens must be mentioned here in one of: %token, %type, %left, %right.
// Otherwise Bison will issue an error. Each token mentioned here will end up a
// member of the "enum yytokentype" in parser.tab.h. The only exception are
// implicit one character literal tokens specified as strings, such as '+' or
// '/'. Their codes are their character ord number.

%token END_OF_FILE 0
%token <string> IDENTIFIER
%token <n> NUMERIC
%type <ast> expr
%type <ast> start_unit
%type <ast> subroutine
%type <ast> statement
%type <ast> assignment_statement
%type <ast> exit_statement
%type <vec_ast> statements

%token TK_NEWLINE

%token TK_DBL_DOT
%token TK_DBL_COLON
%token TK_POW
%token TK_CONCAT
%token TK_ARROW

%token TK_EQ
%token TK_NE
%token TK_LT
%token TK_LE
%token TK_GT
%token TK_GE

%token KW_ABSTRACT
%token KW_ALL
%token KW_ALLOCATABLE
%token KW_ALLOCATE
%token KW_ASSIGNMENT
%token KW_ASSOCIATE
%token KW_ASYNCHRONOUS
%token KW_BACKSPACE
%token KW_BIND
%token KW_BLOCK
%token KW_CALL
%token KW_CASE
%token KW_CHARACTER
%token KW_CLASS
%token KW_CLOSE
%token KW_CODIMENSION
%token KW_COMMON
%token KW_COMPLEX
%token KW_CONCURRENT
%token KW_CONTAINS
%token KW_CONTIGUOUS
%token KW_CONTINUE
%token KW_CRITICAL
%token KW_CYCLE
%token KW_DATA
%token KW_DEALLOCATE
%token KW_DEFAULT
%token KW_DEFERRED
%token KW_DIMENSION
%token KW_DO
%token KW_DOWHILE
%token KW_DOUBLE
%token KW_ELEMENTAL
%token KW_ELSE
%token KW_END
%token KW_ENTRY
%token KW_ENUM
%token KW_ENUMERATOR
%token KW_EQUIVALENCE
%token KW_ERRMSG
%token KW_ERROR
%token KW_EXIT
%token KW_EXTENDS
%token KW_EXTERNAL
%token KW_FILE
%token KW_FINAL
%token KW_FLUSH
%token KW_FORALL
%token KW_FORMAT
%token KW_FORMATTED
%token KW_FUNCTION
%token KW_GENERIC
%token KW_GO
%token KW_IF
%token KW_IMPLICIT
%token KW_IMPORT
%token KW_IMPURE
%token KW_IN
%token KW_INCLUDE
%token KW_INOUT
%token KW_INQUIRE
%token KW_INTEGER
%token KW_INTENT
%token KW_INTERFACE
%token KW_INTRINSIC
%token KW_IS
%token KW_KIND
%token KW_LEN
%token KW_LOCAL
%token KW_LOCAL_INIT
%token KW_LOGICAL
%token KW_MODULE
%token KW_MOLD
%token KW_NAME
%token KW_NAMELIST
%token KW_NOPASS
%token KW_NON_INTRINSIC
%token KW_NON_OVERRIDABLE
%token KW_NON_RECURSIVE
%token KW_NONE
%token KW_NULLIFY
%token KW_ONLY
%token KW_OPEN
%token KW_OPERATOR
%token KW_OPTIONAL
%token KW_OUT
%token KW_PARAMETER
%token KW_PASS
%token KW_POINTER
%token KW_PRECISION
%token KW_PRINT
%token KW_PRIVATE
%token KW_PROCEDURE
%token KW_PROGRAM
%token KW_PROTECTED
%token KW_PUBLIC
%token KW_PURE
%token KW_QUIET
%token KW_RANK
%token KW_READ
%token KW_REAL
%token KW_RECURSIVE
%token KW_RESULT
%token KW_RETURN
%token KW_REWIND
%token KW_SAVE
%token KW_SELECT
%token KW_SEQUENCE
%token KW_SHARED
%token KW_SOURCE
%token KW_STAT
%token KW_STOP
%token KW_SUBMODULE
%token KW_SUBROUTINE
%token KW_TARGET
%token KW_TEAM
%token KW_TEAM_NUMBER
%token KW_THEN
%token KW_TO
%token KW_TYPE
%token KW_UNFORMATTED
%token KW_USE
%token KW_VALUE
%token KW_VOLATILE
%token KW_WHERE
%token KW_WHILE
%token KW_WRITE

%left '-' '+'
%left '*' '/'
%right TK_POW

%start start_unit

%%

// ----------------------------------------------------------------------------
// Subroutine/functions/program definitions

start_unit
    : subroutine { $$ = $1; RESULT($$); }
    | statement { $$ = $1; RESULT($$); }
    | expr { $$ = $1; RESULT($$); }
    ;

subroutine
    : KW_SUBROUTINE statement_sep statements statement_sep KW_SUBROUTINE { $$ = SUBROUTINE($3); }
    ;

// -----------------------------------------------------------------------------
// Control flow

statements
    : statements statement_sep statement { $$ = $1;  $$.push_back($3); }
    | statement { $$.push_back($1); }
    ;

statement_sep
    : TK_NEWLINE
    | ';'
    ;

statement
    : assignment_statement
    | exit_statement
    ;

assignment_statement
    : expr '=' expr { $$ = ASSIGNMENT($1, $3); }
    ;

exit_statement
    : KW_EXIT { $$ = EXIT(); }
    ;

// -----------------------------------------------------------------------------
// Fortran expression

expr
    : expr '+' expr { $$ = ADD($1, $3); }
    | expr '-' expr { $$ = SUB($1, $3); }
    | expr '*' expr { $$ = MUL($1, $3); }
    | expr '/' expr { $$ = DIV($1, $3); }
    | expr TK_POW expr { $$ = POW($1, $3); }
    | '(' expr ')' { $$ = $2; }
    | IDENTIFIER { $$ = SYMBOL($1); }
    | NUMERIC { $$ = INTEGER($1); }
    ;
