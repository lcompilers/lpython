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
%token KW_EXIT
%token KW_NEWLINE
%token KW_SUBROUTINE

%left '-' '+'
%left '*' '/'
%right POW


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
    : KW_NEWLINE
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
    | expr POW expr { $$ = POW($1, $3); }
    | '(' expr ')' { $$ = $2; }
    | IDENTIFIER { $$ = SYMBOL($1); }
    | NUMERIC { $$ = INTEGER($1); }
    ;
