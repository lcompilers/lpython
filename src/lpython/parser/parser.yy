%require "3.0"
%define api.pure
%define api.value.type {LFortran::YYSTYPE}
%param {LFortran::Parser &p}
%locations
%expect    0   // shift/reduce conflicts

// Uncomment this to get verbose error messages
//%define parse.error verbose

/*
// Uncomment this to enable parser tracing. Then in the main code, set
// extern int yydebug;
// yydebug=1;
%define parse.trace
%printer { fprintf(yyo, "%s", $$.str().c_str()); } <string>
%printer { fprintf(yyo, "%d", $$); } <n>
%printer { std::cerr << "AST TYPE: " << $$->type; } <ast>
*/


%code requires // *.h
{
#include <lpython/parser/parser.h>
}

%code // *.cpp
{

#include <lpython/parser/parser.h>
#include <lpython/parser/tokenizer.h>
#include <lpython/parser/semantics.h>

int yylex(LFortran::YYSTYPE *yylval, YYLTYPE *yyloc, LFortran::Parser &p)
{
    return p.m_tokenizer.lex(p.m_a, *yylval, *yyloc, p.diag);
} // ylex

void yyerror(YYLTYPE *yyloc, LFortran::Parser &p, const std::string &msg)
{
    p.handle_yyerror(*yyloc, msg);
}

#define YYLLOC_DEFAULT(Current, Rhs, N)                                 \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first   = YYRHSLOC (Rhs, 1).first;                  \
          (Current).last    = YYRHSLOC (Rhs, N).last;                   \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first   = (Current).last   =                        \
            YYRHSLOC (Rhs, 0).last;                                     \
        }                                                               \
    while (0)

} // code


// -----------------------------------------------------------------------------
// List of tokens
// All tokens that we use (including "+" and such) are declared here first
// using the %token line. Each token will end up a member of the "enum
// yytokentype" in parser.tab.hh. Tokens can have a string equivalent (such as
// "+" for TK_PLUS) that is used later in the file to simplify reading it, but
// it is equivalent to TK_PLUS. Bison also allows so called "character token
// type" which are specified using single quotes (and that bypass the %token
// definitions), and those are not used here, and we test that the whole file
// does not contain any single quotes to ensure that.
//
// If this list is updated, update also token2text() in parser.cpp.

// Terminal tokens
%token END_OF_FILE 0
%token TK_NEWLINE
%token TK_INDENT
%token TK_DEDENT
%token <string> TK_NAME
%token <n> TK_INTEGER
%token <f> TK_REAL
%token <f> TK_IMAG_NUM
%token TK_PLUS "+"
%token TK_MINUS "-"
%token TK_STAR "*"
%token TK_SLASH "/"
%token TK_COLON ":"
%token TK_SEMICOLON ";"
%token TK_COMMA ","
%token TK_EQUAL "="
%token TK_LPAREN "("
%token TK_RPAREN ")"
%token TK_LBRACKET "["
%token TK_RBRACKET "]"
%token TK_LBRACE "{"
%token TK_RBRACE "}"
%token TK_PERCENT "%"
%token TK_VBAR "|"
%token TK_AMPERSAND "&"
%token TK_DOT "."
%token TK_TILDE "~"
%token TK_CARET "^"
%token TK_AT "@"
%token <string> TK_STRING
%token <string> TK_COMMENT
%token <string> TK_EOLCOMMENT
%token TK_POW "**"
%token TK_FLOOR_DIV "//"
%token TK_RIGHTSHIFT ">>"
%token TK_LEFTSHIFT "<<"
%token TK_PLUS_EQUAL "+="
%token TK_MIN_EQUAL "-="
%token TK_STAR_EQUAL "*="
%token TK_SLASH_EQUAL "/="
%token TK_PERCENT_EQUAL "%="
%token TK_AMPER_EQUAL "&="
%token TK_VBAR_EQUAL "|="
%token TK_CARET_EQUAL "^="
%token TK_ATEQUAL "@="
%token TK_RARROW "->"
%token TK_COLONEQUAL ":="
%token TK_ELLIPSIS "..."
%token TK_LEFTSHIFT_EQUAL "<<="
%token TK_RIGHTSHIFT_EQUAL ">>="
%token TK_POW_EQUAL "**="
%token TK_DOUBLESLASH_EQUAL "//="
%token TK_EQ "=="
%token TK_NE "!="
%token TK_LT "<"
%token TK_LE "<="
%token TK_GT ">"
%token TK_GE ">="
%token TK_NOT "not"
%token TK_AND "and"
%token TK_OR "or"
%token TK_TRUE "True"
%token TK_FALSE "False"


// Terminal tokens: Keywords
%token KW_AS
%token KW_ASSERT
%token KW_ASYNC
%token KW_AWAIT
%token KW_BREAK
%token KW_CLASS
%token KW_CONTINUE
%token KW_DEF
%token KW_DEL
%token KW_ELIF
%token KW_ELSE
%token KW_EXCEPT
%token KW_FINALLY
%token KW_FOR
%token KW_FROM
%token KW_GLOBAL
%token KW_IF
%token KW_IMPORT
%token KW_IN
%token KW_IS
%token KW_LAMBDA
%token KW_NONE
%token KW_NONLOCAL
%token KW_PASS
%token KW_RAISE
%token KW_RETURN
%token KW_TRY
%token KW_WHILE
%token KW_WITH
%token KW_YIELD

// Nonterminal tokens

%type <ast> script_unit
%type <ast> id
%type <ast> expr
%type <vec_ast> expr_list
//%type <vec_ast> expr_list_opt
%type <ast> tuple_list
%type <ast> statement
%type <vec_ast> statements
%type <vec_ast> statements1
%type <ast> single_line_statement
%type <ast> multi_line_statement
%type <ast> augassign_statement
%type <operator_type> augassign_op
%type <ast> pass_statement
%type <ast> continue_statement
%type <ast> break_statement
%type <ast> raise_statement
%type <ast> assert_statement
%type <ast> import_statement
//%type <ast> global_statement
%type <ast> if_statement_single
//%type <ast> nonlocal_statement
%type <ast> assignment_statement
%type <vec_ast> target_list
%type <vec_ast> target_item_list
%type <ast> target_item
%type <ast> target
%type <ast> ann_assignment_statement
%type <ast> delete_statement
%type <ast> del_target
%type <vec_ast> del_target_list
%type <ast> return_statement
%type <ast> expression_statment
%type <vec_ast> module
%type <alias> module_as_id
%type <vec_alias> module_item_list
%type <ast> if_statement
%type <ast> elif_statement
%type <ast> for_statement
%type <ast> except_statement
%type <vec_ast> except_list
%type <ast> try_statement
%type <ast> function_def
%type <args> parameter_list_opt
%type <arg> parameter
%type <arg> defparameter
%type <args> parameter_list_starargs
%type <vec_arg> defparameter_list
%type <vec_ast> decorators
%type <vec_ast> decorators_opt
%type <ast> class_def
%type <key_val> dict
%type <vec_key_val> dict_list
%type <ast> slice_item
%type <vec_ast> slice_item_list
//%type <ast> with_statement
//%type <withitem> with_item
//%type <vec_withitem> with_item_list
%type <ast> async_func_def
%type <ast> async_for_stmt
//%type <ast> async_with_stmt
//%type <keyword> keyword_item
//%type <vec_keyword> keyword_items
//%type <ast> function_call
//%type <ast> primary
%type <vec_ast> sep
%type <ast> sep_one

// Precedence

%left ","
%precedence ":="
%left "or"
%left "and"
%precedence "not"
%left "==" "!=" ">=" ">" "<=" "<" //"is not" "is" "not in" "in"
%left "|"
%left "^"
%left "&"
%left ">>" "<<"
%left "-" "+"
%left "%" "//" "/" "@" "*"
%precedence UNARY
%right "**"
%precedence AWAIT
%precedence "."

%start expr

%%

// The order of rules does not matter in Bison (unlike in ANTLR). The
// precedence is specified not by the order but by %left and %right directives
// as well as with %dprec.

// ----------------------------------------------------------------------------
// Top level rules to be used for parsing.

// Higher %dprec means higher precedence

expr
    : id { $$ = $1; }
    | TK_INTEGER { $$ = INTEGER($1, @$); }
    | TK_STRING { $$ = STRING($1, @$); }
    | TK_REAL { $$ = FLOAT($1, @$); }
    | TK_IMAG_NUM { $$ = COMPLEX($1, @$); }
    | TK_TRUE { $$ = BOOL(true, @$); }
    | TK_FALSE { $$ = BOOL(false, @$); }
    | "(" expr ")" { $$ = $2; }
    | expr "," expr { $$ = LIST($1, @$); }
    | expr "." id { $$ = ATTRIBUTE_REF($1, $3, @$); }
    | "{" "}" { $$ = DICT_01(@$); }
    | KW_AWAIT expr %prec AWAIT { $$ = AWAIT($2, @$); }
    | id ":=" expr { $$ = NAMEDEXPR($1, $3, @$); }
    | "*" expr { $$ = STARRED_ARG($2, @$); }

    | expr "+" expr { $$ = BINOP($1, Add, $3, @$); }
    | expr "-" expr { $$ = BINOP($1, Sub, $3, @$); }
    | expr "*" expr { $$ = BINOP($1, Mult, $3, @$); }
    | expr "/" expr { $$ = BINOP($1, Div, $3, @$); }
    | expr "%" expr { $$ = BINOP($1, Mod, $3, @$); }
    | "-" expr %prec UNARY { $$ = UNARY($2, USub, @$); }
    | "+" expr %prec UNARY { $$ = UNARY($2, UAdd, @$); }
    | "~" expr %prec UNARY { $$ = UNARY($2, Invert, @$); }
    | expr "**" expr { $$ = BINOP($1, Pow, $3, @$); }
    | expr "//" expr { $$ = BINOP($1, FloorDiv, $3, @$); }
    | expr "@" expr { $$ = BINOP($1, MatMult, $3, @$); }

    | expr "&" expr { $$ = BINOP($1, BitAnd, $3, @$); }
    | expr "|" expr { $$ = BINOP($1, BitOr, $3, @$); }
    | expr "^" expr { $$ = BINOP($1, BitXor, $3, @$); }
    | expr "<<" expr { $$ = BINOP($1, LShift, $3, @$); }
    | expr ">>" expr { $$ = BINOP($1, RShift, $3, @$); }

    | expr "==" expr { $$ = COMPARE($1, Eq, $3, @$); }
    | expr "!=" expr { $$ = COMPARE($1, NotEq, $3, @$); }
    | expr "<" expr { $$ = COMPARE($1, Lt, $3, @$); }
    | expr "<=" expr { $$ = COMPARE($1, LtE, $3, @$); }
    | expr ">" expr { $$ = COMPARE($1, Gt, $3, @$); }
    | expr ">=" expr { $$ = COMPARE($1, GtE, $3, @$); }

    | expr "and" expr { $$ = BOOLOP($1, And, $3, @$); }
    | expr "or" expr { $$ = BOOLOP($1, Or, $3, @$); }
    | "not" expr { $$ = UNARY($2, Not, @$); }

    ;

id
    : TK_NAME { $$ = SYMBOL($1, @$); }
    ;

sep
    : sep sep_one { $$ = $1; LIST_ADD($$, $2); }
    | sep_one { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

sep_one
    : TK_NEWLINE {}
    | TK_COMMENT {}
    | TK_EOLCOMMENT {}
    | ";" {}
    ;
