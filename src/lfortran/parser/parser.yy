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
%token <string> TK_NAME
%token <string> TK_DEF_OP
%token <n> TK_INTEGER
%token TK_REAL
%type <ast> expr
%type <ast> id
%type <ast> start_unit
%type <ast> program
%type <ast> subroutine
%type <ast> function
%type <ast> statement
%type <ast> assignment_statement
//%type <ast> exit_statement
%type <vec_ast> statements

%token TK_NEWLINE
%token TK_STRING

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

%token TK_NOT
%token TK_AND
%token TK_OR
%token TK_EQV
%token TK_NEQV

%token TK_TRUE
%token TK_FALSE

%token <string> KW_ABSTRACT
%token <string> KW_ALL
%token <string> KW_ALLOCATABLE
%token <string> KW_ALLOCATE
%token <string> KW_ASSIGNMENT
%token <string> KW_ASSOCIATE
%token <string> KW_ASYNCHRONOUS
%token <string> KW_BACKSPACE
%token <string> KW_BIND
%token <string> KW_BLOCK
%token <string> KW_CALL
%token <string> KW_CASE
%token <string> KW_CHARACTER
%token <string> KW_CLASS
%token <string> KW_CLOSE
%token <string> KW_CODIMENSION
%token <string> KW_COMMON
%token <string> KW_COMPLEX
%token <string> KW_CONCURRENT
%token <string> KW_CONTAINS
%token <string> KW_CONTIGUOUS
%token <string> KW_CONTINUE
%token <string> KW_CRITICAL
%token <string> KW_CYCLE
%token <string> KW_DATA
%token <string> KW_DEALLOCATE
%token <string> KW_DEFAULT
%token <string> KW_DEFERRED
%token <string> KW_DIMENSION
%token <string> KW_DO
%token <string> KW_DOWHILE
%token <string> KW_DOUBLE
%token <string> KW_ELEMENTAL
%token <string> KW_ELSE
%token <string> KW_END
%token <string> KW_ENTRY
%token <string> KW_ENUM
%token <string> KW_ENUMERATOR
%token <string> KW_EQUIVALENCE
%token <string> KW_ERRMSG
%token <string> KW_ERROR
%token <string> KW_EXIT
%token <string> KW_EXTENDS
%token <string> KW_EXTERNAL
%token <string> KW_FILE
%token <string> KW_FINAL
%token <string> KW_FLUSH
%token <string> KW_FORALL
%token <string> KW_FORMAT
%token <string> KW_FORMATTED
%token <string> KW_FUNCTION
%token <string> KW_GENERIC
%token <string> KW_GO
%token <string> KW_IF
%token <string> KW_IMPLICIT
%token <string> KW_IMPORT
%token <string> KW_IMPURE
%token <string> KW_IN
%token <string> KW_INCLUDE
%token <string> KW_INOUT
%token <string> KW_INQUIRE
%token <string> KW_INTEGER
%token <string> KW_INTENT
%token <string> KW_INTERFACE
%token <string> KW_INTRINSIC
%token <string> KW_IS
%token <string> KW_KIND
%token <string> KW_LEN
%token <string> KW_LOCAL
%token <string> KW_LOCAL_INIT
%token <string> KW_LOGICAL
%token <string> KW_MODULE
%token <string> KW_MOLD
%token <string> KW_NAME
%token <string> KW_NAMELIST
%token <string> KW_NOPASS
%token <string> KW_NON_INTRINSIC
%token <string> KW_NON_OVERRIDABLE
%token <string> KW_NON_RECURSIVE
%token <string> KW_NONE
%token <string> KW_NULLIFY
%token <string> KW_ONLY
%token <string> KW_OPEN
%token <string> KW_OPERATOR
%token <string> KW_OPTIONAL
%token <string> KW_OUT
%token <string> KW_PARAMETER
%token <string> KW_PASS
%token <string> KW_POINTER
%token <string> KW_PRECISION
%token <string> KW_PRINT
%token <string> KW_PRIVATE
%token <string> KW_PROCEDURE
%token <string> KW_PROGRAM
%token <string> KW_PROTECTED
%token <string> KW_PUBLIC
%token <string> KW_PURE
%token <string> KW_QUIET
%token <string> KW_RANK
%token <string> KW_READ
%token <string> KW_REAL
%token <string> KW_RECURSIVE
%token <string> KW_RESULT
%token <string> KW_RETURN
%token <string> KW_REWIND
%token <string> KW_SAVE
%token <string> KW_SELECT
%token <string> KW_SEQUENCE
%token <string> KW_SHARED
%token <string> KW_SOURCE
%token <string> KW_STAT
%token <string> KW_STOP
%token <string> KW_SUBMODULE
%token <string> KW_SUBROUTINE
%token <string> KW_TARGET
%token <string> KW_TEAM
%token <string> KW_TEAM_NUMBER
%token <string> KW_THEN
%token <string> KW_TO
%token <string> KW_TYPE
%token <string> KW_UNFORMATTED
%token <string> KW_USE
%token <string> KW_VALUE
%token <string> KW_VOLATILE
%token <string> KW_WHERE
%token <string> KW_WHILE
%token <string> KW_WRITE

%left '-' '+'
%left '*' '/'
%right TK_POW

%start start_unit

%%

// ----------------------------------------------------------------------------
// Subroutine/functions/program definitions

start_unit
    : program { $$ = $1; RESULT($$); }
    | subroutine { $$ = $1; RESULT($$); }
    | function { $$ = $1; RESULT($$); }
    | statement { $$ = $1; RESULT($$); }
    | expr { $$ = $1; RESULT($$); }
    ;

program
    : KW_PROGRAM id sep statements sep KW_END KW_PROGRAM {
            $$ = PROGRAM($2, $4); }
    ;

subroutine
    : KW_SUBROUTINE id sep statements sep KW_END KW_SUBROUTINE {
            $$ = SUBROUTINE($2, $4); }
    ;

function
    : KW_FUNCTION id sep statements sep KW_END KW_FUNCTION {
            $$ = FUNCTION($2, $4); }
    ;

// -----------------------------------------------------------------------------
// Control flow

statements
    : statements sep statement { $$ = $1;  $$.push_back($3); }
    | statement { $$.push_back($1); }
    ;

sep
    : sep sep_one
    | sep_one
    ;

sep_one
    : TK_NEWLINE
    | ';'
    ;

statement
    : assignment_statement
//    | exit_statement
    ;

assignment_statement
    : expr '=' expr { $$ = ASSIGNMENT($1, $3); }
    ;

/*
exit_statement
    : KW_EXIT { $$ = EXIT(); }
    ;
*/

// -----------------------------------------------------------------------------
// Fortran expression

expr
    : expr '+' expr { $$ = ADD($1, $3); }
    | expr '-' expr { $$ = SUB($1, $3); }
    | expr '*' expr { $$ = MUL($1, $3); }
    | expr '/' expr { $$ = DIV($1, $3); }
    | expr TK_POW expr { $$ = POW($1, $3); }
    | '(' expr ')' { $$ = $2; }
    | id { $$ = $1; }
    | TK_INTEGER { $$ = INTEGER($1); }
    ;

id
    : TK_NAME { $$ = SYMBOL($1); }
    | KW_ABSTRACT { $$ = SYMBOL($1); }
    | KW_ALL { $$ = SYMBOL($1); }
    | KW_ALLOCATABLE { $$ = SYMBOL($1); }
    | KW_ALLOCATE { $$ = SYMBOL($1); }
    | KW_ASSIGNMENT { $$ = SYMBOL($1); }
    | KW_ASSOCIATE { $$ = SYMBOL($1); }
    | KW_ASYNCHRONOUS { $$ = SYMBOL($1); }
    | KW_BACKSPACE { $$ = SYMBOL($1); }
    | KW_BIND { $$ = SYMBOL($1); }
    | KW_BLOCK { $$ = SYMBOL($1); }
    | KW_CALL { $$ = SYMBOL($1); }
    | KW_CASE { $$ = SYMBOL($1); }
    | KW_CHARACTER { $$ = SYMBOL($1); }
    | KW_CLASS { $$ = SYMBOL($1); }
    | KW_CLOSE { $$ = SYMBOL($1); }
    | KW_CODIMENSION { $$ = SYMBOL($1); }
    | KW_COMMON { $$ = SYMBOL($1); }
    | KW_COMPLEX { $$ = SYMBOL($1); }
    | KW_CONCURRENT { $$ = SYMBOL($1); }
    | KW_CONTAINS { $$ = SYMBOL($1); }
    | KW_CONTIGUOUS { $$ = SYMBOL($1); }
    | KW_CONTINUE { $$ = SYMBOL($1); }
    | KW_CRITICAL { $$ = SYMBOL($1); }
    | KW_CYCLE { $$ = SYMBOL($1); }
    | KW_DATA { $$ = SYMBOL($1); }
    | KW_DEALLOCATE { $$ = SYMBOL($1); }
    | KW_DEFAULT { $$ = SYMBOL($1); }
    | KW_DEFERRED { $$ = SYMBOL($1); }
    | KW_DIMENSION { $$ = SYMBOL($1); }
    | KW_DO { $$ = SYMBOL($1); }
    | KW_DOWHILE { $$ = SYMBOL($1); }
    | KW_DOUBLE { $$ = SYMBOL($1); }
    | KW_ELEMENTAL { $$ = SYMBOL($1); }
    | KW_ELSE { $$ = SYMBOL($1); }
    | KW_END { $$ = SYMBOL($1); }
    | KW_ENTRY { $$ = SYMBOL($1); }
    | KW_ENUM { $$ = SYMBOL($1); }
    | KW_ENUMERATOR { $$ = SYMBOL($1); }
    | KW_EQUIVALENCE { $$ = SYMBOL($1); }
    | KW_ERRMSG { $$ = SYMBOL($1); }
    | KW_ERROR { $$ = SYMBOL($1); }
    | KW_EXIT { $$ = SYMBOL($1); }
    | KW_EXTENDS { $$ = SYMBOL($1); }
    | KW_EXTERNAL { $$ = SYMBOL($1); }
    | KW_FILE { $$ = SYMBOL($1); }
    | KW_FINAL { $$ = SYMBOL($1); }
    | KW_FLUSH { $$ = SYMBOL($1); }
    | KW_FORALL { $$ = SYMBOL($1); }
    | KW_FORMAT { $$ = SYMBOL($1); }
    | KW_FORMATTED { $$ = SYMBOL($1); }
    | KW_FUNCTION { $$ = SYMBOL($1); }
    | KW_GENERIC { $$ = SYMBOL($1); }
    | KW_GO { $$ = SYMBOL($1); }
    | KW_IF { $$ = SYMBOL($1); }
    | KW_IMPLICIT { $$ = SYMBOL($1); }
    | KW_IMPORT { $$ = SYMBOL($1); }
    | KW_IMPURE { $$ = SYMBOL($1); }
    | KW_IN { $$ = SYMBOL($1); }
    | KW_INCLUDE { $$ = SYMBOL($1); }
    | KW_INOUT { $$ = SYMBOL($1); }
    | KW_INQUIRE { $$ = SYMBOL($1); }
    | KW_INTEGER { $$ = SYMBOL($1); }
    | KW_INTENT { $$ = SYMBOL($1); }
    | KW_INTERFACE { $$ = SYMBOL($1); }
    | KW_INTRINSIC { $$ = SYMBOL($1); }
    | KW_IS { $$ = SYMBOL($1); }
    | KW_KIND { $$ = SYMBOL($1); }
    | KW_LEN { $$ = SYMBOL($1); }
    | KW_LOCAL { $$ = SYMBOL($1); }
    | KW_LOCAL_INIT { $$ = SYMBOL($1); }
    | KW_LOGICAL { $$ = SYMBOL($1); }
    | KW_MODULE { $$ = SYMBOL($1); }
    | KW_MOLD { $$ = SYMBOL($1); }
    | KW_NAME { $$ = SYMBOL($1); }
    | KW_NAMELIST { $$ = SYMBOL($1); }
    | KW_NOPASS { $$ = SYMBOL($1); }
    | KW_NON_INTRINSIC { $$ = SYMBOL($1); }
    | KW_NON_OVERRIDABLE { $$ = SYMBOL($1); }
    | KW_NON_RECURSIVE { $$ = SYMBOL($1); }
    | KW_NONE { $$ = SYMBOL($1); }
    | KW_NULLIFY { $$ = SYMBOL($1); }
    | KW_ONLY { $$ = SYMBOL($1); }
    | KW_OPEN { $$ = SYMBOL($1); }
    | KW_OPERATOR { $$ = SYMBOL($1); }
    | KW_OPTIONAL { $$ = SYMBOL($1); }
    | KW_OUT { $$ = SYMBOL($1); }
    | KW_PARAMETER { $$ = SYMBOL($1); }
    | KW_PASS { $$ = SYMBOL($1); }
    | KW_POINTER { $$ = SYMBOL($1); }
    | KW_PRECISION { $$ = SYMBOL($1); }
    | KW_PRINT { $$ = SYMBOL($1); }
    | KW_PRIVATE { $$ = SYMBOL($1); }
    | KW_PROCEDURE { $$ = SYMBOL($1); }
    | KW_PROGRAM { $$ = SYMBOL($1); }
    | KW_PROTECTED { $$ = SYMBOL($1); }
    | KW_PUBLIC { $$ = SYMBOL($1); }
    | KW_PURE { $$ = SYMBOL($1); }
    | KW_QUIET { $$ = SYMBOL($1); }
    | KW_RANK { $$ = SYMBOL($1); }
    | KW_READ { $$ = SYMBOL($1); }
    | KW_REAL { $$ = SYMBOL($1); }
    | KW_RECURSIVE { $$ = SYMBOL($1); }
    | KW_RESULT { $$ = SYMBOL($1); }
    | KW_RETURN { $$ = SYMBOL($1); }
    | KW_REWIND { $$ = SYMBOL($1); }
    | KW_SAVE { $$ = SYMBOL($1); }
    | KW_SELECT { $$ = SYMBOL($1); }
    | KW_SEQUENCE { $$ = SYMBOL($1); }
    | KW_SHARED { $$ = SYMBOL($1); }
    | KW_SOURCE { $$ = SYMBOL($1); }
    | KW_STAT { $$ = SYMBOL($1); }
    | KW_STOP { $$ = SYMBOL($1); }
    | KW_SUBMODULE { $$ = SYMBOL($1); }
    | KW_SUBROUTINE { $$ = SYMBOL($1); }
    | KW_TARGET { $$ = SYMBOL($1); }
    | KW_TEAM { $$ = SYMBOL($1); }
    | KW_TEAM_NUMBER { $$ = SYMBOL($1); }
    | KW_THEN { $$ = SYMBOL($1); }
    | KW_TO { $$ = SYMBOL($1); }
    | KW_TYPE { $$ = SYMBOL($1); }
    | KW_UNFORMATTED { $$ = SYMBOL($1); }
    | KW_USE { $$ = SYMBOL($1); }
    | KW_VALUE { $$ = SYMBOL($1); }
    | KW_VOLATILE { $$ = SYMBOL($1); }
    | KW_WHERE { $$ = SYMBOL($1); }
    | KW_WHILE { $$ = SYMBOL($1); }
    | KW_WRITE { $$ = SYMBOL($1); }
    ;
