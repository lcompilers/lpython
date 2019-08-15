%require "3.0"
%define api.pure full
%define api.value.type {struct LFortran::YYSTYPE}
%param {LFortran::Parser &p}

/*
// Uncomment this to enable parser tracing:
%define parse.trace
%printer { fprintf(yyo, "%s", $$.c_str()); } <string>
%printer { std::cerr << *$$; } <basic>
*/


%code requires // *.h
{
#include <lfortran/parser/parser.h>
}

%code // *.cpp
{

#include <lfortran/parser/parser.h>
#include <lfortran/parser/tokenizer.h>
#include <lfortran/parser/sem4b.h>

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



%token END_OF_FILE 0
%token <string> IDENTIFIER NUMERIC
%type <basic> st_expr expr

%left '-' '+'
%left '*' '/'
%right POW


%start st_expr

%%
st_expr
    : expr { $$ = $1; RESULT($$); }
    ;

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
