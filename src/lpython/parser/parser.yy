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
%token <string> TK_TYPE_COMMENT
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
%token TK_IS_NOT "is not"
%token TK_NOT_IN "not in"
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
%token KW_IN "in"
%token KW_IS "is"
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
%type <vec_ast> expr_list_opt
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
%type <ast> global_statement
%type <ast> if_statement_single
%type <ast> nonlocal_statement
%type <ast> assignment_statement
%type <vec_ast> target_list
%type <ast> ann_assignment_statement
%type <ast> delete_statement
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
%type <args> parameter_list_starargs
%type <vec_arg> defparameter_list
%type <vec_ast> decorators
%type <vec_ast> decorators_opt
%type <ast> class_def
%type <key_val> dict
%type <vec_key_val> dict_list
%type <ast> slice_item
%type <vec_ast> slice_item_list
%type <ast> with_statement
%type <ast> async_func_def
%type <ast> async_for_stmt
%type <ast> async_with_stmt
%type <keyword> keyword_item
%type <vec_keyword> keyword_items
%type <ast> function_call
%type <ast> primary
%type <ast> while_statement
%type <vec_ast> sep
%type <ast> sep_one
%type <ast> string
%type <ast> ternary_if_statement
/* %type <ast> list_comprehension */
%type <vec_ast> id_list
%type <ast> id_item
%type <ast> subscription
%type <comp> comp_for
%type <vec_comp> comp_for_items
%type <ast> lambda_expression
%type <vec_arg> lambda_id_list

// Precedence

%precedence ":="
%precedence LAMBDA
%left "or"
%left "and"
%precedence "not"
%left "==" "!=" ">=" ">" "<=" "<" "is not" "is" "not in" "in"
%left KW_IF KW_ELSE
%left "|"
%left "^"
%left "&"
%left ">>" "<<"
%left "-" "+"
%left "%" "//" "/" "@" "*"
%precedence UNARY
%right "**"
%precedence AWAIT
%precedence YIELD
%precedence "."

%start units

%%

// The order of rules does not matter in Bison (unlike in ANTLR). The
// precedence is specified not by the order but by %left and %right directives
// as well as with %dprec.

// ----------------------------------------------------------------------------
// Top level rules to be used for parsing.

// Higher %dprec means higher precedence

units
    : units script_unit   { RESULT($2); }
    | script_unit         { RESULT($1); }
    | sep
    ;

script_unit
    : statement
    ;

statements
    : TK_INDENT statements1 TK_DEDENT { $$ = $2; }
    ;

statements1
    : statements1 statement { $$ = $1; LIST_ADD($$, $2); }
    | statement { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

statement
    : single_line_statement sep { $$ = $1; }
    | multi_line_statement
    | multi_line_statement sep { $$ = $1; }
    ;

single_line_statement
    : expression_statment
    | assert_statement
    | assignment_statement
    | augassign_statement
    | ann_assignment_statement
    | pass_statement
    | delete_statement
    | return_statement
    | raise_statement
    | break_statement
    | continue_statement
    | import_statement
    | global_statement
    | if_statement_single
    | nonlocal_statement
    ;

multi_line_statement
    : if_statement
    | for_statement
    | try_statement
    | with_statement
    | function_def
    | class_def
    | async_func_def
    | async_for_stmt
    | async_with_stmt
    | while_statement
    ;

expression_statment
    : expr { $$ = EXPR_01($1, @$); }
    ;

pass_statement
    : KW_PASS { $$ = PASS(@$); }
    ;

break_statement
    : KW_BREAK { $$ = BREAK(@$); }
    ;

continue_statement
    : KW_CONTINUE { $$ = CONTINUE(@$); }
    ;

raise_statement
    : KW_RAISE { $$ = RAISE_01(@$); }
    | KW_RAISE expr { $$ = RAISE_02($2, @$); }
    | KW_RAISE expr KW_FROM expr { $$ = RAISE_03($2, $4, @$); }
    ;

assert_statement
    : KW_ASSERT expr { $$ = ASSERT_01($2, @$); }
    | KW_ASSERT expr "," expr { $$ = ASSERT_02($2, $4, @$); }
    ;

target_list
    : target_list expr "=" { $$ = $1; LIST_ADD($$, $2); }
    | expr "=" { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

assignment_statement
    : target_list expr { $$ = ASSIGNMENT($1, $2, @$); }
    | target_list expr TK_TYPE_COMMENT { $$ = ASSIGNMENT2($1, $2, $3, @$); }
    ;

augassign_statement
    : expr augassign_op expr { $$ = AUGASSIGN_01($1, $2, $3, @$); }
    ;

augassign_op
    : "+=" { $$ = OPERATOR(Add, @$); }
    | "-=" { $$ = OPERATOR(Sub, @$); }
    | "*=" { $$ = OPERATOR(Mult, @$); }
    | "/=" { $$ = OPERATOR(Div, @$); }
    | "%=" { $$ = OPERATOR(Mod, @$); }
    | "&=" { $$ = OPERATOR(BitAnd, @$); }
    | "|=" { $$ = OPERATOR(BitOr, @$); }
    | "^=" { $$ = OPERATOR(BitXor, @$); }
    | "<<=" { $$ = OPERATOR(LShift, @$); }
    | ">>=" { $$ = OPERATOR(RShift, @$); }
    | "**=" { $$ = OPERATOR(Pow, @$); }
    | "//=" { $$ = OPERATOR(FloorDiv, @$); }
    ;

ann_assignment_statement
    : expr ":" expr { $$ = ANNASSIGN_01($1, $3, @$); }
    | expr ":" expr "=" expr { $$ = ANNASSIGN_02($1, $3, $5, @$); }
    ;

delete_statement
    : KW_DEL expr_list { $$ = DELETE_01($2, @$); }
    | KW_DEL expr_list "," { $$ = DELETE_01($2, @$); }
    ;

return_statement
    : KW_RETURN { $$ = RETURN_01(@$); }
    | KW_RETURN expr { $$ = RETURN_02($2, @$); }
    ;

module
    : module "." id { $$ = $1; LIST_ADD($$, $3); }
    | id { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

module_as_id
    : module { $$ = MOD_ID_01($1, @$); }
    | module KW_AS id { $$ = MOD_ID_02($1, $3, @$); }
    | "*" { $$ = MOD_ID_03("*", @$); }
    ;

module_item_list
    : module_item_list "," module_as_id { $$ = $1; PLIST_ADD($$, $3); }
    | module_as_id { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

dot_list
    : dot_list "." { DOT_COUNT_01(); }
    | "." { DOT_COUNT_01(); }
    | dot_list "..." { DOT_COUNT_02(); }
    | "..." { DOT_COUNT_02(); }
    ;

import_statement
    : KW_IMPORT module_item_list { $$ = IMPORT_01($2, @$); }
    | KW_FROM module KW_IMPORT module_item_list {
        $$ = IMPORT_02($2, $4, @$); }
    | KW_FROM module KW_IMPORT "(" module_item_list ")" {
        $$ = IMPORT_02($2, $5, @$); }
    | KW_FROM dot_list KW_IMPORT module_item_list {
        $$ = IMPORT_03($4, @$); }
    | KW_FROM dot_list module KW_IMPORT module_item_list {
        $$ = IMPORT_04($3, $5, @$); }
    | KW_FROM dot_list KW_IMPORT "(" module_item_list ")" {
        $$ = IMPORT_03($5, @$); }
    | KW_FROM dot_list module KW_IMPORT "(" module_item_list ")" {
        $$ = IMPORT_04($3, $6, @$); }
    ;

global_statement
    : KW_GLOBAL expr_list { $$ = GLOBAL($2, @$); }
    ;

if_statement_single
    : KW_IF expr TK_COLON single_line_statement { $$ = IF_01($2, $4, @$); }
    ;

ternary_if_statement
    : expr KW_IF expr KW_ELSE expr { $$ = TERNARY($3, $1, $5, @$); }
    ;

nonlocal_statement
    : KW_NONLOCAL expr_list { $$ = NON_LOCAL($2, @$); }
    ;

elif_statement
    : KW_ELIF expr ":" sep statements { $$ = IF_STMT_01($2, $5, @$); }
    | KW_ELIF expr ":" sep statements KW_ELSE ":" sep statements {
        $$ = IF_STMT_02($2, $5, $9, @$); }
    | KW_ELIF expr ":" sep statements elif_statement {
        $$ = IF_STMT_03($2, $5, $6, @$); }
    ;

if_statement
    : KW_IF expr ":" sep statements { $$ = IF_STMT_01($2, $5, @$); }
    | KW_IF expr ":" sep statements KW_ELSE ":" sep statements {
        $$ = IF_STMT_02($2, $5, $9, @$); }
    | KW_IF expr ":" sep statements elif_statement {
        $$ = IF_STMT_03($2, $5, $6, @$); }
    ;

for_statement
    : KW_FOR expr KW_IN expr ":" sep statements {
        $$ = FOR_01($2, $4, $7, @$); }
    | KW_FOR expr KW_IN expr ":" sep statements KW_ELSE ":"
        sep statements { $$ = FOR_02($2, $4, $7, $11, @$); }
    | KW_FOR expr KW_IN expr ":"
        TK_TYPE_COMMENT TK_NEWLINE statements {
        $$ = FOR_03($2, $4, $6, $8, @$); }
    | KW_FOR expr KW_IN expr ":"
        TK_TYPE_COMMENT TK_NEWLINE statements
        KW_ELSE ":" sep statements { $$ = FOR_04($2, $4, $8, $12, $6, @$); }
    ;

except_statement
    : KW_EXCEPT ":" sep statements { $$ = EXCEPT_01($4, @$); }
    | KW_EXCEPT expr ":" sep statements { $$ = EXCEPT_02($2, $5, @$); }
    | KW_EXCEPT expr KW_AS id ":" sep statements { $$ = EXCEPT_03($2, $4, $7, @$); }
    ;
except_list
    : except_list except_statement { $$ = $1; LIST_ADD($$, $2); }
    | except_statement { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

try_statement
    : KW_TRY ":" sep statements except_list { $$ = TRY_01($4, $5, @$); }
    | KW_TRY ":" sep statements except_list KW_ELSE ":" sep statements {
        $$ = TRY_02($4, $5, $9, @$); }
    | KW_TRY ":" sep statements except_list KW_FINALLY ":" sep statements {
        $$ = TRY_03($4, $5, $9, @$); }
    | KW_TRY ":" sep statements except_list KW_ELSE ":" sep statements KW_FINALLY
        ":" sep statements { $$ = TRY_04($4, $5, $9, $13, @$); }
    | KW_TRY ":" sep statements KW_FINALLY ":" sep statements { $$ = TRY_05($4, $8, @$); }
    ;

with_statement
    : KW_WITH expr_list ":" sep statements { $$ = WITH($2, $5, @$); }
    | KW_WITH "(" expr_list "," ")" ":" sep statements {
        $$ = WITH($3, $8, @$); }
    | KW_WITH expr_list ":" TK_TYPE_COMMENT TK_NEWLINE
        statements { $$ = WITH_01($2, $6, $4, @$); }
    | KW_WITH "(" expr_list "," ")" ":" TK_TYPE_COMMENT
        TK_NEWLINE statements { $$ = WITH_01($3, $9, $7, @$); }
    ;

decorators_opt
    : decorators { $$ = $1; }
    | %empty { LIST_NEW($$); }
    ;

decorators
    : decorators "@" expr sep { $$ = $1; LIST_ADD($$, $3); }
    | "@" expr sep { LIST_NEW($$); LIST_ADD($$, $2); }
    ;

parameter
    : id { $$ = ARGS_01($1, @$); }
    | id ":" expr { $$ = ARGS_02($1, $3, @$); }
    | id "=" expr { $$ = ARGS_03($1, $3, @$); }
    | id ":" expr "=" expr { $$ = ARGS_04($1, $3, $5, @$); }
    ;

defparameter_list
    : defparameter_list "," parameter { $$ = $1; LIST_ADD($$, $3); }
    | parameter { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

parameter_list_starargs
    : "*" parameter { $$ = STAR_ARGS_01($2, @$); }
    | "*" parameter "," defparameter_list { $$ = STAR_ARGS_02($2, $4, @$); }
    | "*" parameter "," defparameter_list "," "**" parameter {
        $$ = STAR_ARGS_03($2, $4, $7, @$); }
    | "*" parameter "," "**" parameter { $$ = STAR_ARGS_04($2, $5, @$); }
    | "**" parameter { $$ = STAR_ARGS_05($2, @$); }

    | defparameter_list "," "*" parameter { $$ = STAR_ARGS_06($1, $4, @$); }
    | defparameter_list "," "*" parameter "," defparameter_list {
        $$ = STAR_ARGS_07($1, $4, $6, @$); }
    | defparameter_list "," "*" parameter "," defparameter_list "," "**" parameter {
        $$ = STAR_ARGS_08($1, $4, $6, $9, @$); }
    | defparameter_list "," "*" parameter "," "**" parameter {
        $$ = STAR_ARGS_09($1, $4, $7, @$); }
    | defparameter_list "," "**" parameter { $$ = STAR_ARGS_10($1, $4, @$); }

    | defparameter_list "," "/" { $$ = STAR_ARGS_11($1, @$); }
    | defparameter_list "," "/" "," "*" parameter {
        $$ = STAR_ARGS_12($1, $6, @$); }
    | defparameter_list "," "/" "," "*" parameter "," defparameter_list {
        $$ = STAR_ARGS_13($1, $6, $8, @$); }
    | defparameter_list "," "/" "," "*" parameter "," defparameter_list
        "," "**" parameter { $$ = STAR_ARGS_14($1, $6, $8, $11, @$); }
    | defparameter_list "," "/" "," "**" parameter {
        $$ = STAR_ARGS_15($1, $6, @$); }
    | defparameter_list "," "/" "," "*" parameter "," "**" parameter {
        $$ = STAR_ARGS_16($1, $6, $9, @$); }
    | defparameter_list "," "/" "," defparameter_list "," "*" parameter {
        $$ = STAR_ARGS_17($1, $5, $8, @$); }
    | defparameter_list "," "/" "," defparameter_list "," "**" parameter {
        $$ = STAR_ARGS_18($1, $5, $8, @$); }
    | defparameter_list "," "/" "," defparameter_list "," "*" parameter
        "," "**" parameter { $$ = STAR_ARGS_19($1, $5, $8, $11, @$); }
    | defparameter_list "," "/" "," defparameter_list "," "*" parameter
            "," defparameter_list "," "**" parameter {
        $$ = STAR_ARGS_20($1, $5, $8, $10, $13, @$); }
    ;

parameter_list_opt
    : defparameter_list comma_opt { $$ = FUNC_ARG_LIST_01($1, @$); }
    | parameter_list_starargs { $$ = $1; }
    | %empty { $$ = FUNC_ARG_LIST_02(@$); }
    | defparameter_list "," "/" "," defparameter_list {
        $$ = FUNC_ARG_LIST_03($1, $5, @$); }
    ;

comma_opt
    : ","
    | %empty
    ;

function_def
    : decorators_opt KW_DEF id "(" parameter_list_opt ")" ":"
        sep statements { $$ = FUNCTION_01($1, $3, $5, $9, @$); }
    | decorators_opt KW_DEF id "(" parameter_list_opt ")" "->" expr ":"
        sep statements { $$ = FUNCTION_02($1, $3, $5, $8, $11, @$); }
    | decorators_opt KW_DEF id "(" parameter_list_opt ")" ":"
        TK_TYPE_COMMENT TK_NEWLINE statements {
        $$ = FUNCTION_03($1, $3, $5, $10, $8, @$); }
    | decorators_opt KW_DEF id "(" parameter_list_opt ")" "->" expr ":"
        TK_TYPE_COMMENT TK_NEWLINE statements {
            $$ = FUNCTION_04($1, $3, $5, $8, $12, $10, @$); }
    ;

class_def
    : decorators_opt KW_CLASS id ":" sep statements {
        $$ = CLASS_01($1, $3, $6, @$); }
    | decorators_opt KW_CLASS id "(" expr_list_opt ")" ":" sep statements {
        $$ = CLASS_02($1, $3, $5, $9, @$); }
    | decorators_opt KW_CLASS id "(" expr_list "," keyword_items ")"
        ":" sep statements { $$ = CLASS_03($1, $3, $5, $7, $11, @$); }
    | decorators_opt KW_CLASS id "(" keyword_items "," expr_list ")"
        ":" sep statements { $$ = CLASS_03($1, $3, $7, $5, $11, @$); }
    | decorators_opt KW_CLASS id "(" keyword_items ")" ":" sep statements {
        $$ = CLASS_04($1, $3, $5, $9, @$); }
    ;

async_func_def
    : decorators KW_ASYNC KW_DEF id "(" parameter_list_opt ")" ":" sep
        statements { $$ = ASYNC_FUNCTION_01($1, $4, $6, $10, @$); }
    | decorators KW_ASYNC KW_DEF id "(" parameter_list_opt ")" "->" expr ":"
        sep statements { $$ = ASYNC_FUNCTION_02($1, $4, $6, $9, $12, @$); }
    | KW_ASYNC KW_DEF id "(" parameter_list_opt ")" ":" sep
        statements { $$ = ASYNC_FUNCTION_03($3, $5, $9, @$); }
    | KW_ASYNC KW_DEF id "(" parameter_list_opt ")" "->" expr ":"
        sep statements { $$ = ASYNC_FUNCTION_04($3, $5, $8, $11, @$); }
    | decorators KW_ASYNC KW_DEF id "(" parameter_list_opt ")" ":"
        TK_TYPE_COMMENT TK_NEWLINE statements {
        $$ = ASYNC_FUNCTION_05($1, $4, $6, $11, $9, @$); }
    | decorators KW_ASYNC KW_DEF id "(" parameter_list_opt ")" "->" expr ":"
        TK_TYPE_COMMENT TK_NEWLINE statements {
        $$ = ASYNC_FUNCTION_06($1, $4, $6, $9, $13, $11, @$); }
    | KW_ASYNC KW_DEF id "(" parameter_list_opt ")" ":"
        TK_TYPE_COMMENT TK_NEWLINE statements {
        $$ = ASYNC_FUNCTION_07($3, $5, $10, $8, @$); }
    | KW_ASYNC KW_DEF id "(" parameter_list_opt ")" "->" expr ":"
        TK_TYPE_COMMENT TK_NEWLINE statements {
        $$ = ASYNC_FUNCTION_08($3, $5, $8, $12, $10, @$); }
    ;

async_for_stmt
    : KW_ASYNC KW_FOR expr KW_IN expr ":" sep statements {
        $$ = ASYNC_FOR_01($3, $5, $8, @$); }
    | KW_ASYNC KW_FOR expr KW_IN expr ":" sep
        statements KW_ELSE ":" sep statements {
        $$ = ASYNC_FOR_02($3, $5, $8, $12, @$); }
    | KW_ASYNC KW_FOR expr KW_IN expr ":"
        TK_TYPE_COMMENT TK_NEWLINE statements {
        $$ = ASYNC_FOR_03($3, $5, $9, $7, @$); }
    | KW_ASYNC KW_FOR expr KW_IN expr ":"
        TK_TYPE_COMMENT TK_NEWLINE statements KW_ELSE ":" sep statements {
        $$ = ASYNC_FOR_04($3, $5, $9, $13, $7, @$); }
    ;

async_with_stmt
    : KW_ASYNC KW_WITH expr_list ":" sep statements {
        $$ = ASYNC_WITH($3, $6, @$); }
    | KW_ASYNC KW_WITH "(" expr_list "," ")" ":" sep statements {
        $$ = ASYNC_WITH($4, $9, @$); }
    | KW_ASYNC KW_WITH expr_list ":" TK_TYPE_COMMENT
        TK_NEWLINE statements { $$ = ASYNC_WITH_01($3, $7, $5, @$); }
    | KW_ASYNC KW_WITH "(" expr_list "," ")" ":" TK_TYPE_COMMENT
        TK_NEWLINE statements { $$ = ASYNC_WITH_01($4, $10, $8, @$); }
    ;

while_statement
    : KW_WHILE expr ":" sep statements { $$ = WHILE_01($2, $5, @$); }
    | KW_WHILE expr ":" sep statements KW_ELSE ":" sep statements {
        $$ = WHILE_02($2, $5, $9, @$); }
    ;

slice_item_list
    : slice_item_list "," slice_item { $$ = $1; LIST_ADD($$, $3); }
    | slice_item { LIST_NEW($$); LIST_ADD($$, $1); }

slice_item
    : ":"                    { $$ = SLICE_01(nullptr, nullptr, nullptr, @$); }
    | expr ":"               { $$ = SLICE_01(     $1, nullptr, nullptr, @$); }
    | ":" expr               { $$ = SLICE_01(nullptr,      $2, nullptr, @$); }
    | expr ":" expr          { $$ = SLICE_01(     $1,      $3, nullptr, @$); }
    | ":" ":"                { $$ = SLICE_01(nullptr, nullptr, nullptr, @$); }
    | ":" ":" expr           { $$ = SLICE_01(nullptr, nullptr,      $3, @$); }
    | expr ":" ":"           { $$ = SLICE_01(     $1, nullptr, nullptr, @$); }
    | ":" expr ":"           { $$ = SLICE_01(nullptr,      $2, nullptr, @$); }
    | expr ":" ":" expr      { $$ = SLICE_01(     $1, nullptr,      $4, @$); }
    | ":" expr ":" expr      { $$ = SLICE_01(nullptr,      $2,      $4, @$); }
    | expr ":" expr ":"      { $$ = SLICE_01(     $1,      $3, nullptr, @$); }
    | expr ":" expr ":" expr { $$ = SLICE_01(     $1,      $3,      $5, @$); }
    | expr                   { $$ = $1; }
    ;

expr_list_opt
    : expr_list { $$ = $1; }
    | %empty { LIST_NEW($$); }
    ;

expr_list
    : expr_list "," expr %prec "not" { $$ = $1; LIST_ADD($$, $3); }
    | expr %prec "not" { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

dict
    : expr ":" expr { $$ = DICT_EXPR($1, $3, @$); }
    ;

dict_list
    : dict_list "," dict { $$ = $1; LIST_ADD($$, $3); }
    | dict { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

tuple_list
    : slice_item_list { $$ = TUPLE($1, @$); }
    ;

id_list
    : id_list "," id { $$ = $1; LIST_ADD($$, $3); }
    | id { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

id_item
    : id_list { $$ = ID_TUPLE_01($1, @$); }
    | id_list "," { $$ = ID_TUPLE_03($1, @$); }
    | "(" id ")" { $$ = $2; }
    | "(" id_list "," ")" { $$ = ID_TUPLE_03($2, @$); }
    | "(" id_list ","  id ")" { $$ = ID_TUPLE_01(TUPLE_($2, $4), @$); }
    ;

keyword_item
    : id "=" expr { $$ = CALL_KEYWORD_01($1, $3, @$); }
    | "**" expr { $$ = CALL_KEYWORD_02($2, @$); }
    ;

keyword_items
    : keyword_items "," keyword_item { $$ = $1; PLIST_ADD($$, $3); }
    | keyword_item { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

primary
    : id { $$ = $1; }
    | string { $$ = $1; }
    | expr "." id { $$ = ATTRIBUTE_REF($1, $3, @$); }
    ;

comp_for
    : KW_FOR id_item KW_IN expr { $$ = COMP_FOR_01($2, $4, @$); }
    | KW_FOR id_item KW_IN expr KW_IF expr {
        $$ = COMP_FOR_02($2, $4, $6, @$); }
    ;

comp_for_items
    : comp_for_items comp_for { $$ = $1; PLIST_ADD($$, $2); }
    | comp_for { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

function_call
    : primary "(" expr_list_opt ")" { $$ = CALL_01($1, $3, @$); }
    | primary "(" expr_list "," ")" { $$ = CALL_01($1, $3, @$); }
    | primary "(" expr_list "," keyword_items comma_opt ")" {
        $$ = CALL_02($1, $3, $5, @$); }
    | primary "(" keyword_items "," expr_list comma_opt ")" {
        $$ = CALL_02($1, $5, $3, @$); }
    | primary "(" keyword_items comma_opt ")" { $$ = CALL_03($1, $3, @$); }
    | primary "(" expr comp_for_items ")" {
        $$ = CALL_04($1, GENERATOR_EXPR($3, $4, @$), @$); }
    | function_call "(" expr_list_opt ")" { $$ = CALL_01($1, $3, @$); }
    | function_call "(" expr_list "," ")" { $$ = CALL_01($1, $3, @$); }
    | function_call "(" expr_list "," keyword_items comma_opt ")" {
        $$ = CALL_02($1, $3, $5, @$); }
    | function_call "(" keyword_items "," expr_list comma_opt ")" {
        $$ = CALL_02($1, $5, $3, @$); }
    | function_call "(" keyword_items comma_opt ")" { $$ = CALL_03($1, $3, @$); }
    | function_call "(" expr comp_for_items ")" {
        $$ = CALL_04($1, GENERATOR_EXPR($3, $4, @$), @$); }
    ;

subscription
    : primary "[" tuple_list "]" { $$ = SUBSCRIPT_01($1, $3, @$); }
    | function_call "[" tuple_list "]" { $$ = SUBSCRIPT_01($1, $3, @$); }
    | "[" expr_list_opt "]" "[" tuple_list "]" {
        $$ = SUBSCRIPT_01(LIST($2, @$), $5, @$); }
    | "{" expr_list "}" "[" tuple_list "]" {
        $$ = SUBSCRIPT_01(SET($2, @$), $5, @$); }
    | "(" expr ")" "[" tuple_list "]" { $$ = SUBSCRIPT_01($2, $5, @$); }
    | "{" dict_list "}" "[" tuple_list "]" {
        $$ = SUBSCRIPT_01(DICT_02($2, @$), $5, @$); }
    | subscription "[" tuple_list "]" { $$ = SUBSCRIPT_01($1, $3, @$); }
    ;

string
    : string TK_STRING { $$ = STRING2($1, $2, @$); } // TODO
    | TK_STRING { $$ = STRING1($1, @$); }
    | id TK_STRING { $$ = STRING3($1, $2, @$); }
    ;

lambda_id_list
    : lambda_id_list "," id { $$ = $1; LIST_ADD($$, ARGS_01($3, @$)); }
    | id { LIST_NEW($$); LIST_ADD($$, ARGS_01($1, @$)); }
    ;

lambda_expression
    : KW_LAMBDA ":" expr %prec LAMBDA {
        $$ = LAMBDA_01(FUNC_ARG_LIST_02(@$), $3, @$); }
    | KW_LAMBDA lambda_id_list ":" expr %prec LAMBDA {
        $$ = LAMBDA_01(FUNC_ARG_LIST_01($2, @$), $4, @$); }
    ;

expr
    : id { $$ = $1; }
    | TK_INTEGER { $$ = INTEGER($1, @$); }
    | string { $$ = $1; }
    | TK_REAL { $$ = FLOAT($1, @$); }
    | TK_IMAG_NUM { $$ = COMPLEX($1, @$); }
    | TK_TRUE { $$ = BOOL(true, @$); }
    | TK_FALSE { $$ = BOOL(false, @$); }
    | KW_NONE { $$ = NONE(@$); }
    | TK_ELLIPSIS { $$ = ELLIPSIS(@$); }
    | "(" expr ")" { $$ = $2; }
    | "(" ")" { $$ = TUPLE_EMPTY(@$); }
    | "(" expr_list ","  expr ")" { $$ = TUPLE_01(TUPLE_($2, $4), @$); }
    | function_call { $$ = $1; }
    | subscription { $$ = $1; }
    | "[" expr_list_opt "]" { $$ = LIST($2, @$); }
    | "[" expr_list "," "]" { $$ = LIST($2, @$); }
    | "{" expr_list "}" { $$ = SET($2, @$); }
    | "{" expr_list "," "}" { $$ = SET($2, @$); }
    | expr "." id { $$ = ATTRIBUTE_REF($1, $3, @$); }

    | "{" "}" { $$ = DICT_01(@$); }
    | "{" dict_list "}" { $$ = DICT_02($2, @$); }
    | KW_AWAIT expr %prec AWAIT { $$ = AWAIT($2, @$); }
    | KW_YIELD %prec YIELD { $$ = YIELD_01(@$); }
    | KW_YIELD expr %prec YIELD { $$ = YIELD_02($2, @$); }
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
    | expr "is" expr { $$ = COMPARE($1, Is, $3, @$); }
    | expr "is not" expr { $$ = COMPARE($1, IsNot, $3, @$); }
    | expr "in" expr { $$ = COMPARE($1, In, $3, @$); }
    | expr "not in" expr { $$ = COMPARE($1, NotIn, $3, @$); }

    | expr "and" expr { $$ = BOOLOP($1, And, $3, @$); }
    | expr "or" expr { $$ = BOOLOP($1, Or, $3, @$); }
    | "not" expr { $$ = UNARY($2, Not, @$); }

    // Comprehension
    | "[" expr comp_for_items "]" { $$ = LIST_COMP_1($2, $3, @$); }
    | "{" expr comp_for_items "}" { $$ = SET_COMP_1($2, $3, @$); }
    | "{" expr ":" expr comp_for_items "}" { $$ = DICT_COMP_1($2, $4, $5, @$); }

    | ternary_if_statement { $$ = $1; }
    | lambda_expression { $$ = $1; }
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
