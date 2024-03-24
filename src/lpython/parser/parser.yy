%require "3.0"
%define api.pure
%define api.value.type {LCompilers::LPython::YYSTYPE}
%param {LCompilers::LPython::Parser &p}
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

int yylex(LCompilers::LPython::YYSTYPE *yylval, YYLTYPE *yyloc,
    LCompilers::LPython::Parser &p)
{
    return p.m_tokenizer.lex(p.m_a, *yylval, *yyloc, p.diag);
} // ylex

void yyerror(YYLTYPE *yyloc, LCompilers::LPython::Parser &p, const std::string &msg)
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
%token <string> TK_FSTRING_START
%token <string> TK_FSTRING_MIDDLE
%token <string> TK_FSTRING_END
%token <string> TK_COMMENT
%token <string> TK_EOLCOMMENT
%token <string> TK_TYPE_COMMENT
%token <string> TK_TYPE_IGNORE
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
%token KW_YIELD_FROM

// Soft Keywords
%token KW_MATCH
%token KW_CASE

%token KW_STR_PREFIX
%type <string> KW_STR_PREFIX

// Nonterminal tokens

%type <ast> script_unit
%type <ast> id
%type <ast> expr
%type <vec_ast> expr_list
%type <vec_ast> expr_list_opt
%type <ast> tuple_list
%type <ast> statement
%type <vec_ast> statements
%type <vec_ast> sep_statements
%type <vec_ast> body_stmts
%type <vec_ast> single_line_statements
%type <vec_ast> statements1
%type <vec_ast> single_line_multi_statements
%type <vec_ast> single_line_multi_statements_opt
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
%type <ast> for_target_list
%type <ast> except_statement
%type <vec_ast> except_list
%type <ast> try_statement
%type <ast> function_def
%type <args> parameter_list_opt
%type <arg> parameter
%type <fn_arg> parameter_list
%type <args_> parameter_list_no_posonly
%type <var_kw> parameter_list_starargs
%type <vec_arg> defparameter_list
%type <vec_ast> decorators
%type <vec_ast> decorators_opt
%type <ast> class_def
%type <key_val> dict
%type <vec_key_val> dict_list
%type <ast> slice_item
%type <ast> slice_items
%type <vec_ast> slice_item_list
%type <ast> with_statement
%type <vec_withitem> with_as_items
%type <withitem> with_item
%type <vec_withitem> with_as_items_list_1
%type <vec_withitem> with_as_items_list_2
%type <ast> async_func_def
%type <ast> async_for_stmt
%type <ast> async_with_stmt
%type <vec_kw_or_star> positional_items
%type <kw_or_star> starred_and_keyword
%type <vec_keyword> keywords_arguments
%type <ast> function_call
%type <call_arg> call_arguement_list
%type <ast> primary
%type <ast> while_statement
%type <vec_ast> sep
%type <ast> sep_one
%type <string> type_comment
%type <ast> string
%type <ast> fstring
%type <ast> fstring_middle
%type <ast> ternary_if_statement
%type <ast> comprehension
%type <vec_ast> id_list
%type <vec_ast> id_list_opt
%type <ast> id_item
%type <ast> subscript
%type <comp> comp_for
%type <vec_ast> comp_if_items
%type <vec_comp> comp_for_items
%type <ast> lambda_expression
%type <args> lambda_parameter_list_opt
%type <arg> lambda_parameter
%type <fn_arg> lambda_parameter_list
%type <args_> lambda_parameter_list_no_posonly
%type <var_kw> lambda_parameter_list_starargs
%type <vec_arg> lambda_defparameter_list
%type <ast> yield_expr
%type <ast> match_statement
%type <ast> numbers
%type <ast> literal_pattern
%type <ast> singleton_pattern
%type <ast> attr
%type <vec_match_case> case_blocks
%type <pattern> pattern_1
%type <pattern> pattern_2
%type <pattern> closed_pattern
%type <vec_pattern> or_pattern
%type <pattern> sequence_pattern
%type <pattern> star_pattern
%type <vec_pattern> open_sequence_pattern
%type <pattern> mapping_pattern
%type <kw_val_pattern> key_value_pattern
%type <vec_kw_val_pattern> items_pattern
%type <vec_pattern> positional_patterns
%type <ast> class_name
%type <pattern> class_pattern
%type <vec_kw_val_pattern> keyword_patterns
%type <match_case> case_block

// Precedence

%precedence ":="
%precedence LAMBDA
%left KW_IF KW_ELSE
%left "or"
%left "and"
%precedence "not"
%left "==" "!=" ">=" ">" "<=" "<" "is not" "is" "not in" "in"
%precedence FOR
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
    : TK_INDENT statements1 TK_DEDENT { FLOC(@$, @2); LLOC(@$,@2); $$ = $2; }
    ;

sep_statements
    : sep statements { FLOC(@$, @2); $$ = $2; }
    | type_ignore_sep statements { FLOC(@$, @2); $$ = $2; }
    ;

body_stmts
    : single_line_statements { $$ = $1; }
    | sep_statements { $$ = $1; }
    ;

statements1
    : statements1 statement { $$ = $1; LIST_ADD($$, $2); }
    | statement { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

single_line_statements
    : single_line_multi_statements TK_NEWLINE { LLOC(@$,@1); $$ = $1; }
    | single_line_multi_statements TK_EOLCOMMENT { $$ = $1; }
    | single_line_statement TK_NEWLINE { LLOC(@$,@1); $$ = A2LIST(p.m_a, $1); }
    | single_line_statement TK_SEMICOLON TK_NEWLINE { $$ = A2LIST(p.m_a, $1); }
    | single_line_statement TK_SEMICOLON TK_EOLCOMMENT { $$ = A2LIST(p.m_a, $1); }
    | single_line_statement TK_EOLCOMMENT { $$ = A2LIST(p.m_a, $1); }
    ;

single_line_multi_statements
    : single_line_multi_statements_opt single_line_statement { $$ = $1; LIST_ADD($$, $2); }
    | single_line_multi_statements_opt single_line_statement ";" { $$ = $1; LIST_ADD($$, $2); }
    ;

single_line_multi_statements_opt
    : single_line_multi_statements_opt single_line_statement ";" { $$ = $1; LIST_ADD($$, $2); }
    | single_line_statement ";" { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

type_ignore_sep
    : TK_TYPE_IGNORE sep { extract_type_comment(p, @$, $1); }
    | sep TK_TYPE_IGNORE sep { extract_type_comment(p, @$, $2); }
    ;

statement
    : single_line_statement sep { LLOC(@$,@1); $$ = $1; }
    | single_line_statement type_ignore_sep { $$ = $1; }
    | multi_line_statement
    | multi_line_statement sep { LLOC(@$,@1); $$ = $1; }
    | multi_line_statement type_ignore_sep { $$ = $1; }
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
    | nonlocal_statement
    ;

multi_line_statement
    : if_statement
    | for_statement
    | try_statement
    | with_statement
    | match_statement
    | function_def
    | class_def
    | async_func_def
    | async_for_stmt
    | async_with_stmt
    | while_statement
    ;

yield_expr
    : KW_YIELD { $$ = YIELD_01(@$); }
    | KW_YIELD tuple_list { $$ = YIELD_02($2, @$); }
    | KW_YIELD_FROM tuple_list { $$ = YIELD_03($2, @$); }
    ;

expression_statment
    : tuple_list { $$ = EXPR_01($1, @$); }
    | yield_expr { $$ = EXPR_01($1, @$); }
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
    : target_list tuple_list "=" { $$ = $1; LIST_ADD($$, $2); }
    | tuple_list "=" { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

assignment_statement
    : target_list tuple_list { $$ = ASSIGNMENT($1, $2, @$); }
    | target_list yield_expr { $$ = ASSIGNMENT($1, $2, @$); }
    | target_list tuple_list TK_TYPE_COMMENT {
        $$ = ASSIGNMENT2($1, $2, $3, @$); }
    ;

augassign_statement
    : expr augassign_op tuple_list { $$ = AUGASSIGN_01($1, $2, $3, @$); }
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
    | "@=" { $$ = OPERATOR(MatMult, @$); }
    ;

ann_assignment_statement
    : expr ":" expr { $$ = ANNASSIGN_01($1, $3, @$); }
    | expr ":" expr "=" tuple_list { $$ = ANNASSIGN_02($1, $3, $5, @$); }
    ;

delete_statement
    : KW_DEL expr_list { $$ = DELETE_01($2, @$); }
    | KW_DEL expr_list "," { $$ = DELETE_01($2, @$); }
    ;

return_statement
    : KW_RETURN { $$ = RETURN_01(@$); }
    | KW_RETURN tuple_list { $$ = RETURN_02($2, @$); }
    | KW_RETURN yield_expr { $$ = RETURN_02($2, @$); }
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
    | module_item_list "," TK_TYPE_IGNORE module_as_id { $$ = $1;
        PLIST_ADD($$, $4); extract_type_comment(p, @$, $3); }
    | module_as_id { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

dot_list
    : dot_list "." { DOT_COUNT_01(); }
    | "." { DOT_COUNT_01(); }
    | dot_list "..." { DOT_COUNT_02(); }
    | "..." { DOT_COUNT_02(); }
    ;

type_ignore_opt
    : TK_TYPE_IGNORE { extract_type_comment(p, @$, $1); }
    | "," TK_TYPE_IGNORE { extract_type_comment(p, @$, $2); }
    | comma_opt
    ;

import_statement
    : KW_IMPORT module_item_list { $$ = IMPORT_01($2, @$); }
    | KW_FROM module KW_IMPORT module_item_list {
        $$ = IMPORT_02($2, $4, @$); }
    | KW_FROM module KW_IMPORT "(" module_item_list type_ignore_opt ")" {
        $$ = IMPORT_02($2, $5, @$); }
    | KW_FROM dot_list KW_IMPORT module_item_list {
        $$ = IMPORT_03($4, @$); }
    | KW_FROM dot_list module KW_IMPORT module_item_list {
        $$ = IMPORT_04($3, $5, @$); }
    | KW_FROM dot_list KW_IMPORT "(" module_item_list type_ignore_opt ")" {
        $$ = IMPORT_03($5, @$); }
    | KW_FROM dot_list module KW_IMPORT "(" module_item_list type_ignore_opt ")"
        { $$ = IMPORT_04($3, $6, @$); }
    ;

global_statement
    : KW_GLOBAL expr_list { $$ = GLOBAL($2, @$); }
    ;

ternary_if_statement
    : expr KW_IF expr KW_ELSE expr { $$ = TERNARY($3, $1, $5, @$); }
    ;

nonlocal_statement
    : KW_NONLOCAL expr_list { $$ = NON_LOCAL($2, @$); }
    ;

elif_statement
    : KW_ELIF expr ":" body_stmts { $$ = IF_STMT_01($2, $4, @$); }
    | KW_ELIF expr ":" body_stmts KW_ELSE ":" body_stmts {
        $$ = IF_STMT_02($2, $4, $7, @$); }
    | KW_ELIF expr ":" body_stmts elif_statement {
        $$ = IF_STMT_03($2, $4, $5, @$); }
    ;

if_statement
    : KW_IF expr ":" body_stmts { $$ = IF_STMT_01($2, $4, @$); }
    | KW_IF expr ":" body_stmts KW_ELSE ":" body_stmts {
        $$ = IF_STMT_02($2, $4, $7, @$); }
    | KW_IF expr ":" body_stmts elif_statement {
        $$ = IF_STMT_03($2, $4, $5, @$); }
    ;

for_target_list
    : expr %prec FOR { $$ = $1; }
    | expr_list "," expr %prec FOR { $$ = TUPLE_01(TUPLE_($1, $3), @$); }
    | expr_list "," %prec FOR { $$ = TUPLE_03($1, @$); }
    ;

tuple_list
    : expr { $$ = $1; }
    | expr_list "," expr { $$ = TUPLE_01(TUPLE_($1, $3), @$); }
    | expr_list "," { $$ = TUPLE_03($1, @$); }
    ;

for_statement
    : KW_FOR for_target_list KW_IN tuple_list ":" body_stmts {
        $$ = FOR_01($2, $4, $6, @$); }
    | KW_FOR for_target_list KW_IN tuple_list ":" body_stmts KW_ELSE ":"
        body_stmts { $$ = FOR_02($2, $4, $6, $9, @$); }
    | KW_FOR for_target_list KW_IN tuple_list ":" TK_TYPE_COMMENT sep
        statements { $$ = FOR_03($2, $4, $6, $8, @$); }
    | KW_FOR for_target_list KW_IN tuple_list ":" TK_TYPE_COMMENT sep
        statements KW_ELSE ":" body_stmts {
            $$ = FOR_04($2, $4, $8, $11, $6, @$); }
    ;

except_statement
    : KW_EXCEPT ":" body_stmts { $$ = EXCEPT_01($3, @$); }
    | KW_EXCEPT expr ":" body_stmts { $$ = EXCEPT_02($2, $4, @$); }
    | KW_EXCEPT expr KW_AS id ":" body_stmts { $$ = EXCEPT_03($2, $4, $6, @$); }
    ;

except_list
    : except_list except_statement { $$ = $1; LIST_ADD($$, $2); }
    | except_statement { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

try_statement
    : KW_TRY ":" body_stmts except_list { $$ = TRY_01($3, $4, @$); }
    | KW_TRY ":" body_stmts except_list KW_ELSE ":" body_stmts {
        $$ = TRY_02($3, $4, $7, @$); }
    | KW_TRY ":" body_stmts except_list KW_FINALLY ":" body_stmts {
        $$ = TRY_03($3, $4, $7, @$); }
    | KW_TRY ":" body_stmts except_list KW_ELSE ":" body_stmts KW_FINALLY
        ":" body_stmts { $$ = TRY_04($3, $4, $7, $10, @$); }
    | KW_TRY ":" body_stmts KW_FINALLY ":" body_stmts { $$ = TRY_05($3, $6, @$); }
    ;

with_item
    : expr KW_AS expr { $$ = WITH_ITEM_01($1, $3, @$); }
    | expr { $$ = WITH_ITEM_02($1, @$); }
    ;

with_as_items_list_1
    : with_as_items_list_1 "," with_item { $$ = $1; PLIST_ADD($$, $3); }
    | with_item { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

with_as_items_list_2
    : with_as_items_list_2 "," expr KW_AS expr {
        $$ = $1; PLIST_ADD($$, WITH_ITEM_01($3, $5, @$)); }
    | expr KW_AS expr { LIST_NEW($$); PLIST_ADD($$, WITH_ITEM_01($1, $3, @$)); }
    ;

with_as_items
    : with_as_items_list_1 { $$ = $1; }
    | "(" with_as_items_list_2 ")" { $$ = $2; }
    | "(" with_as_items_list_2 "," ")" { $$ = $2; }
    ;

with_statement
    : KW_WITH with_as_items ":" body_stmts { $$ = WITH_01($2, $4, @$); }
    | KW_WITH with_as_items ":" TK_TYPE_COMMENT sep statements {
        $$ = WITH_02($2, $6, $4, @$); }
    ;

class_name
    : id { $$ = $1; }
    | attr { $$ = $1; }
    ;

class_pattern
    : class_name "(" ")" { $$ = MATCH_CLASS_01($1, @$); }
    | class_name "(" positional_patterns ")" {
        $$ = MATCH_CLASS_02($1, $3, @$); }
    | class_name "(" positional_patterns "," keyword_patterns ")" {
        $$ = MATCH_CLASS_03($1, $3, $5, @$); }
    | class_name "(" keyword_patterns ")" { $$ = MATCH_CLASS_04($1, $3, @$); }
    ;

positional_patterns
    : positional_patterns "," pattern_2 { $$ = $1; LIST_ADD($$, $3); }
    | pattern_2 { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

keyword_patterns
    : keyword_patterns "," id "=" pattern_2 { $$ = $1;
        PLIST_ADD($$, KEY_VAL_PATTERN(p.m_a, EXPR($3), $5)); }
    | id "=" pattern_2 { LIST_NEW($$);
        PLIST_ADD($$, KEY_VAL_PATTERN(p.m_a, EXPR($1), $3)); }
    ;

mapping_pattern
    :  "{" "}" { $$ = MATCH_MAPPING_01(@$); }
    |  "{" items_pattern "}" { $$ = MATCH_MAPPING_02($2, @$); }
    |  "{" items_pattern "," "**" id "}" { $$ = MATCH_MAPPING_03($2, $5, @$); }
    |  "{" "**" id "}" { $$ = MATCH_MAPPING_04($3, @$); }

items_pattern
    : items_pattern "," key_value_pattern { $$ = $1; PLIST_ADD($$, $3); }
    | key_value_pattern { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

key_value_pattern
    : literal_pattern ":" pattern_2 {
        $$ = KEY_VAL_PATTERN(p.m_a, EXPR($1), $3); }
    ;

star_pattern
    : "*" id { $$ = MATCH_STAR($2, @$); }
    | pattern_2 { $$ = $1; }
    ;

open_sequence_pattern
    : open_sequence_pattern "," star_pattern { $$ = $1; LIST_ADD($$, $3); }
    | star_pattern { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

sequence_pattern
    : "[" "]" { $$ = MATCH_SEQUENCE_01(@$); }
    | "(" ")" { $$ = MATCH_SEQUENCE_01(@$); }
    | "(" open_sequence_pattern "," ")" { $$ = MATCH_SEQUENCE_02($2, @$); }
    | "(" open_sequence_pattern "," star_pattern ")" { LIST_ADD($2, $4);
        $$ = MATCH_SEQUENCE_02($2, @$); }
    | "[" open_sequence_pattern "]" { $$ = MATCH_SEQUENCE_02($2, @$); }
    | "[" open_sequence_pattern "," "]" { $$ = MATCH_SEQUENCE_02($2, @$); }
    ;

attr
    : attr "." id { $$ = ATTRIBUTE_REF($1, $3, @$); }
    | id "." id { $$ = ATTRIBUTE_REF($1, $3, @$); }
    ;

numbers
    : numbers "+" numbers { $$ = BINOP($1, Add, $3, @$); }
    | numbers "-" numbers { $$ = BINOP($1, Sub, $3, @$); }
    | "-" numbers { $$ = UNARY($2, USub, @$); }
    | TK_INTEGER { $$ = INTEGER($1, @$); }
    | TK_REAL { $$ = FLOAT($1, @$); }
    | TK_IMAG_NUM { $$ = COMPLEX($1, @$); }
    ;

literal_pattern
    : numbers { $$ = $1; }
    | string { $$ = $1; }
    | attr { $$ = $1; }
    ;

singleton_pattern
    : KW_NONE  { $$ = NONE(@$); }
    | TK_TRUE { $$ = BOOL(true, @$); }
    | TK_FALSE { $$ = BOOL(false, @$); }
    ;

closed_pattern
    : literal_pattern { $$ = MATCH_VALUE($1, @$); }
    | singleton_pattern { $$ = MATCH_SINGLETON($1, @$); }
    | id { $$ = MATCH_AS_02($1, @$); }
    | "(" pattern_2 ")" { $$ = $2; }
    | sequence_pattern { $$ = $1; }
    | mapping_pattern { $$ = $1; }
    | class_pattern { $$ = $1; }
    ;

or_pattern
    : or_pattern "|" closed_pattern { $$ = $1; LIST_ADD($$, $3); }
    | closed_pattern "|" closed_pattern { LIST_NEW($$);
        LIST_ADD($$, $1); LIST_ADD($$, $3);}
    ;

pattern_2
    : closed_pattern { $$ = $1; }
    | or_pattern { $$ = MATCH_OR($1, @$); }
    | closed_pattern KW_AS id { $$ = MATCH_AS_01($1, $3, @$); }
    | or_pattern KW_AS id { $$ = MATCH_AS_01(MATCH_OR($1, @$), $3, @$); }
    ;

pattern_1
    : open_sequence_pattern "," star_pattern { LIST_ADD($1, $3);
        $$ = MATCH_SEQUENCE_02($1, @$); }
    | open_sequence_pattern "," { $$ = MATCH_SEQUENCE_02($1, @$); }
    ;

case_block
    : KW_CASE pattern_1 ":" body_stmts { $$ = MATCH_CASE_01($2, $4, @$); }
    | KW_CASE pattern_2 ":" body_stmts { $$ = MATCH_CASE_01($2, $4, @$); }
    | KW_CASE pattern_1 KW_IF expr ":" body_stmts {
        $$ = MATCH_CASE_02($2, $4, $6, @$); }
    | KW_CASE pattern_2 KW_IF expr ":" body_stmts {
        $$ = MATCH_CASE_02($2, $4, $6, @$); }
    ;

case_blocks
    : case_blocks case_block { $$ = $1; PLIST_ADD($$, $2); }
    | case_block { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

match_statement
    : KW_MATCH tuple_list ":" sep TK_INDENT case_blocks TK_DEDENT {
        $$ = MATCH_01($2, $6, @$); }
    ;

decorators_opt
    : decorators { $$ = $1; }
    | %empty { LIST_NEW($$); }
    ;

decorators
    : decorators "@" expr sep { $$ = $1; LIST_ADD($$, $3); }
    | "@" expr sep { LIST_NEW($$); LIST_ADD($$, $2); }
    | decorators "@" expr TK_TYPE_IGNORE sep { $$ = $1;
        LIST_ADD($$, $3); extract_type_comment(p, @$, $4); }
    | "@" expr TK_TYPE_IGNORE sep { LIST_NEW($$); LIST_ADD($$, $2);
        extract_type_comment(p, @$, $3); }
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
    | defparameter_list "," type_comment parameter { $$ = $1;
        LIST_ADD($$, $4); ADD_TYPE_COMMENT($1, $3, @$); }
    | defparameter_list "," type_comment {
        $$ = $1; ADD_TYPE_COMMENT($1, $3, @$); }
    ;

parameter_list
    : defparameter_list "," "/" comma_opt {
        $$ = PARAMETER_LIST_01($1, nullptr); }
    | defparameter_list "," "/" "," parameter_list_no_posonly {
        $$ = PARAMETER_LIST_01($1, $5); }
    | parameter_list_no_posonly { $$ = PARAMETER_LIST_02($1); }
    ;

parameter_list_no_posonly
    : defparameter_list comma_opt { $$ = PARAMETER_LIST_03($1, nullptr); }
    | defparameter_list "," parameter_list_starargs {
        $$ = PARAMETER_LIST_03($1, $3); }
    | parameter_list_starargs { $$ = PARAMETER_LIST_04($1); }
    ;

parameter_list_starargs
    : "*" "," defparameter_list comma_opt { $$ = PARAMETER_LIST_05($3); }
    | "*" "," "**" parameter comma_opt { $$ = PARAMETER_LIST_06($4); }
    | "*" "," defparameter_list "," "**" parameter comma_opt {
        $$ = PARAMETER_LIST_07($3, $6); }
    | "*" parameter comma_opt { $$ = PARAMETER_LIST_08($2); }
    | "*" parameter "," defparameter_list comma_opt {
        $$ = PARAMETER_LIST_09($2, $4); }
    | "*" parameter "," "**" parameter comma_opt {
        $$ = PARAMETER_LIST_10($2, $5); }
    | "*" parameter "," defparameter_list "," "**" parameter comma_opt {
        $$ = PARAMETER_LIST_11($2, $4, $7); }
    | "**" parameter comma_opt { $$ = PARAMETER_LIST_06($2); }
    ;

parameter_list_opt
    : parameter_list { $$ = FUNC_ARGS_01(p.m_a, @$, $1); }
    | %empty { $$ = PARAMETER_LIST_12(@$); }
    ;

comma_opt
    : ","
    | %empty
    ;

function_def
    : decorators_opt KW_DEF id "(" parameter_list_opt ")" ":"
        body_stmts { FLOC(@$, @2); $$ = FUNCTION_01($1, $3, $5, $8, @$); }
    | decorators_opt KW_DEF id "(" parameter_list_opt ")" "->" expr ":"
        body_stmts { FLOC(@$, @2); $$ = FUNCTION_02($1, $3, $5, $8, $10, @$); }
    | decorators_opt KW_DEF id "(" parameter_list_opt ")" ":"
        TK_TYPE_COMMENT sep statements {
        FLOC(@$, @2); $$ = FUNCTION_03($1, $3, $5, $10, $8, @$); }
    | decorators_opt KW_DEF id "(" parameter_list_opt ")" "->" expr ":"
        TK_TYPE_COMMENT sep statements {
            FLOC(@$, @2); $$ = FUNCTION_04($1, $3, $5, $8, $12, $10, @$); }
    | decorators_opt KW_DEF id "(" parameter_list_opt ")" ":"
        sep TK_TYPE_COMMENT sep statements {
        FLOC(@$, @2); $$ = FUNCTION_03($1, $3, $5, $11, $9, @$); }
    | decorators_opt KW_DEF id "(" parameter_list_opt ")" "->" expr ":"
        sep TK_TYPE_COMMENT sep statements {
            FLOC(@$, @2); $$ = FUNCTION_04($1, $3, $5, $8, $13, $11, @$); }
    ;

class_def
    : decorators_opt KW_CLASS id ":" body_stmts {
       FLOC(@$, @2); $$ = CLASS_01($1, $3, $5, @$); }
    | decorators_opt KW_CLASS id "(" call_arguement_list ")" ":" body_stmts {
       FLOC(@$, @2); $$ = CLASS_02($1, $3, $5, $8, @$); }
    ;

async_func_def
    : decorators KW_ASYNC KW_DEF id "(" parameter_list_opt ")" ":"
        body_stmts { $$ = ASYNC_FUNCTION_01($1, $4, $6, $9, @$); }
    | decorators KW_ASYNC KW_DEF id "(" parameter_list_opt ")" "->" expr ":"
        body_stmts { $$ = ASYNC_FUNCTION_02($1, $4, $6, $9, $11, @$); }
    | KW_ASYNC KW_DEF id "(" parameter_list_opt ")" ":"
        body_stmts { $$ = ASYNC_FUNCTION_03($3, $5, $8, @$); }
    | KW_ASYNC KW_DEF id "(" parameter_list_opt ")" "->" expr ":"
        body_stmts { $$ = ASYNC_FUNCTION_04($3, $5, $8, $10, @$); }
    | decorators KW_ASYNC KW_DEF id "(" parameter_list_opt ")" ":"
        TK_TYPE_COMMENT sep statements {
        $$ = ASYNC_FUNCTION_05($1, $4, $6, $11, $9, @$); }
    | decorators KW_ASYNC KW_DEF id "(" parameter_list_opt ")" "->" expr ":"
        TK_TYPE_COMMENT sep statements {
        $$ = ASYNC_FUNCTION_06($1, $4, $6, $9, $13, $11, @$); }
    | KW_ASYNC KW_DEF id "(" parameter_list_opt ")" ":"
        TK_TYPE_COMMENT sep statements {
        $$ = ASYNC_FUNCTION_07($3, $5, $10, $8, @$); }
    | KW_ASYNC KW_DEF id "(" parameter_list_opt ")" "->" expr ":"
        TK_TYPE_COMMENT sep statements {
        $$ = ASYNC_FUNCTION_08($3, $5, $8, $12, $10, @$); }
    ;

async_for_stmt
    : KW_ASYNC KW_FOR for_target_list KW_IN tuple_list ":" body_stmts {
        $$ = ASYNC_FOR_01($3, $5, $7, @$); }
    | KW_ASYNC KW_FOR for_target_list KW_IN tuple_list ":" body_stmts
        KW_ELSE ":" body_stmts {
        $$ = ASYNC_FOR_02($3, $5, $7, $10, @$); }
    | KW_ASYNC KW_FOR for_target_list KW_IN tuple_list ":"
        TK_TYPE_COMMENT sep statements {
        $$ = ASYNC_FOR_03($3, $5, $9, $7, @$); }
    | KW_ASYNC KW_FOR for_target_list KW_IN tuple_list ":"
        TK_TYPE_COMMENT sep statements KW_ELSE ":" body_stmts {
        $$ = ASYNC_FOR_04($3, $5, $9, $12, $7, @$); }
    ;

async_with_stmt
    : KW_ASYNC KW_WITH with_as_items ":" body_stmts {
        $$ = ASYNC_WITH_01($3, $5, @$); }
    | KW_ASYNC KW_WITH with_as_items ":" TK_TYPE_COMMENT sep statements {
        $$ = ASYNC_WITH_02($3, $7, $5, @$); }
    ;

while_statement
    : KW_WHILE expr ":" body_stmts { $$ = WHILE_01($2, $4, @$); }
    | KW_WHILE expr ":" body_stmts KW_ELSE ":" body_stmts {
        $$ = WHILE_02($2, $4, $7, @$); }
    ;

expr_list_opt
    : expr_list comma_opt { $$ = $1; }
    | %empty { LIST_NEW($$); }
    ;

expr_list
    : expr_list "," expr %prec "not" { $$ = $1; LIST_ADD($$, $3); }
    | expr %prec "not" { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

dict
    : expr ":" expr { $$ = DICT_EXPR_01($1, $3, @$); }
    | expr ":" type_comment expr { $$ = DICT_EXPR_02($1, $3, $4, @$); }
    | "**" expr { $$ = DICT_EXPR_01(NONE(@$), $2, @$); }
    ;

dict_list
    : dict_list "," dict { $$ = $1; LIST_ADD($$, $3); }
    | dict { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

id_list_opt
    : id_list { $$ = $1; }
    | %empty { LIST_NEW($$); }
    ;

id_list
    : id_list "," id_item { $$ = $1; LIST_ADD($$, $3); }
    | id_item { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

id_item
    : id { $$ = $1; }
    | id "[" slice_item "]" { $$ = SUBSCRIPT_01($1, $3, @$); }
    | "*" id { $$ = STARRED_ARG_STORE($2, @$); }
    | "(" id ")" { $$ = $2; }
    | "(" id_list "," ")" { $$ = ID_TUPLE_03($2, @$); }
    | "(" id_list ","  id_item ")" { $$ = ID_TUPLE_01(TUPLE_($2, $4), @$); }
    | "[" id_list_opt "]" { $$ = LIST(SET_EXPR_CTX_02($2, Store), @$); }
    | "[" id_list "," "]" { $$ = LIST(SET_EXPR_CTX_02($2, Store), @$); }
    ;

comp_if_items
    : comp_if_items KW_IF expr { $$ = $1; LIST_ADD($$, $3); }
    | KW_IF expr { LIST_NEW($$); LIST_ADD($$, $2); }
    ;

comp_for
    : KW_FOR id_list KW_IN expr {
        $$ = COMP_FOR_01(ID_TUPLE_01($2, @$), $4, @$); }
    | KW_FOR id_list "," KW_IN expr {
        $$ = COMP_FOR_01(ID_TUPLE_03($2, @$), $5, @$); }
    | KW_FOR id_list KW_IN expr comp_if_items {
        $$ = COMP_FOR_02(ID_TUPLE_01($2, @$), $4, $5, @$); }
    | KW_FOR id_list "," KW_IN expr comp_if_items {
        $$ = COMP_FOR_02(ID_TUPLE_03($2, @$), $5, $6, @$); }
    | KW_ASYNC KW_FOR id_list KW_IN expr {
        $$ = COMP_FOR_03(ID_TUPLE_01($3, @$), $5, @$); }
    | KW_ASYNC KW_FOR id_list "," KW_IN expr {
        $$ = COMP_FOR_03(ID_TUPLE_03($3, @$), $6, @$); }
    | KW_ASYNC KW_FOR id_list KW_IN expr comp_if_items {
        $$ = COMP_FOR_04(ID_TUPLE_01($3, @$), $5, $6, @$); }
    | KW_ASYNC KW_FOR id_list "," KW_IN expr comp_if_items {
        $$ = COMP_FOR_04(ID_TUPLE_03($3, @$), $6, $7, @$); }
    ;

comp_for_items
    : comp_for_items comp_for { $$ = $1; PLIST_ADD($$, $2); }
    | comp_for { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

keywords_arguments
    : keywords_arguments "," "**" expr { $$ = $1;
        PLIST_ADD($$, CALL_EXPR_01($4, @$)); }
    | keywords_arguments "," id "=" expr { $$ = $1;
        PLIST_ADD($$, CALL_KW_01($3, $5, @$)); }
    | "**" expr { LIST_NEW($$); PLIST_ADD($$, CALL_EXPR_01($2, @$));}
    ;

starred_and_keyword
    : expr { $$ = CALL_EXPR_02($1, @$); }
    | id "=" expr { $$ = CALL_KW_02($1, $3, @$);}
    ;

positional_items
    : positional_items "," starred_and_keyword { $$ = $1; PLIST_ADD($$, $3); }
    | positional_items "," TK_TYPE_IGNORE starred_and_keyword { $$ = $1;
        PLIST_ADD($$, $4); extract_type_comment(p, @$, $3); }
    | starred_and_keyword { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

call_arguement_list
    : %empty { $$ = CALL_ARG_00(); }
    | positional_items type_ignore_opt { $$ = CALL_ARG_01($1); }
    | keywords_arguments type_ignore_opt { $$ = CALL_ARG_02($1); }
    | positional_items "," keywords_arguments type_ignore_opt {
        $$ = CALL_ARG_03($1, $3); }
    ;

primary
    : id { $$ = $1; }
    | string { $$ = $1; }
    | expr "." id { $$ = ATTRIBUTE_REF($1, $3, @$); }
    ;

function_call
    : primary "(" call_arguement_list ")" { $$ = CALL_01($1, $3, @$); }
    | primary "(" TK_TYPE_IGNORE call_arguement_list ")" {
        $$ = CALL_01($1, $4, @$); extract_type_comment(p, @$, $3); }
    | primary "(" expr comp_for_items ")" {
        $$ = CALL_02($1, A2LIST(p.m_a, GENERATOR_EXPR($3, $4, @$)), @$); }
    | function_call "(" call_arguement_list ")" { $$ = CALL_01($1, $3, @$); }
    | function_call "(" expr comp_for_items ")" {
        $$ = CALL_02($1, A2LIST(p.m_a, GENERATOR_EXPR($3, $4, @$)), @$); }
    | subscript "(" call_arguement_list ")" { $$ = CALL_01($1, $3, @$); }
    | subscript "(" expr comp_for_items ")" {
        $$ = CALL_02($1, A2LIST(p.m_a, GENERATOR_EXPR($3, $4, @$)), @$); }
    | "(" expr ")" "(" call_arguement_list ")" { $$ = CALL_01($2, $5, @$); }
    ;

slice_item_list
    : slice_item_list "," slice_items { $$ = $1; LIST_ADD($$, $3); }
    | slice_items { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

slice_items
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

slice_item
    : slice_item_list { $$ = TUPLE_01($1, @$); }
    | slice_item_list "," { $$ = TUPLE_03($1, @$); }
    ;

subscript
    : primary "[" slice_item "]" { $$ = SUBSCRIPT_01($1, $3, @$); }
    | function_call "[" slice_item "]" { $$ = SUBSCRIPT_01($1, $3, @$); }
    | "[" expr_list_opt "]" "[" slice_item "]" {
        $$ = SUBSCRIPT_01(LIST($2, @$), $5, @$); }
    | "{" expr_list "}" "[" slice_item "]" {
        $$ = SUBSCRIPT_01(SET($2, @$), $5, @$); }
    | "(" expr ")" "[" slice_item "]" { $$ = SUBSCRIPT_01($2, $5, @$); }
    | "(" expr_list "," ")" "[" slice_item "]" {
        $$ = SUBSCRIPT_01(TUPLE_03($2, @$), $6, @$); }
    | "(" expr_list "," expr ")" "[" slice_item "]" {
        $$ = SUBSCRIPT_01(TUPLE_01(TUPLE_($2, $4), @$), $7, @$); }
    | "{" dict_list comma_opt "}" "[" slice_item "]" {
        $$ = SUBSCRIPT_01(DICT_02($2, @$), $6, @$); }
    | "{" "}" "[" slice_item "]" { $$ = SUBSCRIPT_01(DICT_01(@$), $4, @$); }
    | subscript "[" slice_item "]" { $$ = SUBSCRIPT_01($1, $3, @$); }
    | comprehension "[" slice_item "]" { $$ = SUBSCRIPT_01($1, $3, @$); }
    ;

string
    : string TK_STRING { $$ = STRING2($1, $2, @$); } // TODO
    | string KW_STR_PREFIX TK_STRING { $$ = STRING4($1, STRING3($2, $3, @$), @$); }
    | string fstring { $$ = STRING4($1, $2, @$); }
    | TK_STRING { $$ = STRING1($1, @$); }
    | KW_STR_PREFIX TK_STRING { $$ = STRING3($1, $2, @$); }
    | fstring
    ;

fstring_middle 
    : fstring_middle TK_FSTRING_MIDDLE expr { $$ = FSTRING_MIDDLE($1, $2, $3, @$); }
    | expr { $$ = FSTRING_MIDDLE1($1, @$); }
    ;

fstring
    : TK_FSTRING_START fstring_middle TK_FSTRING_END { $$ = FSTRING($1, $2, $3, @$); }
    ;

lambda_parameter
    : id { $$ = ARGS_01($1, @$); }
    | id "=" expr { $$ = ARGS_03($1, $3, @$); }
    ;

lambda_defparameter_list
    : lambda_defparameter_list "," lambda_parameter { $$ = $1; LIST_ADD($$, $3); }
    | lambda_parameter { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

lambda_parameter_list
    : lambda_defparameter_list "," "/" comma_opt {
        $$ = PARAMETER_LIST_01($1, nullptr); }
    | lambda_defparameter_list "," "/" "," lambda_parameter_list_no_posonly {
        $$ = PARAMETER_LIST_01($1, $5); }
    | lambda_parameter_list_no_posonly { $$ = PARAMETER_LIST_02($1); }
    ;

lambda_parameter_list_no_posonly
    : lambda_defparameter_list comma_opt { $$ = PARAMETER_LIST_03($1, nullptr); }
    | lambda_defparameter_list "," lambda_parameter_list_starargs {
        $$ = PARAMETER_LIST_03($1, $3); }
    | lambda_parameter_list_starargs { $$ = PARAMETER_LIST_04($1); }
    ;

lambda_parameter_list_starargs
    : "*" "," lambda_defparameter_list comma_opt { $$ = PARAMETER_LIST_05($3); }
    | "*" "," "**" lambda_parameter comma_opt { $$ = PARAMETER_LIST_06($4); }
    | "*" "," lambda_defparameter_list "," "**" lambda_parameter comma_opt {
        $$ = PARAMETER_LIST_07($3, $6); }
    | "*" lambda_parameter comma_opt { $$ = PARAMETER_LIST_08($2); }
    | "*" lambda_parameter "," lambda_defparameter_list comma_opt {
        $$ = PARAMETER_LIST_09($2, $4); }
    | "*" lambda_parameter "," "**" lambda_parameter comma_opt {
        $$ = PARAMETER_LIST_10($2, $5); }
    | "*" lambda_parameter "," lambda_defparameter_list ","
        "**" lambda_parameter comma_opt { $$ = PARAMETER_LIST_11($2, $4, $7); }
    | "**" lambda_parameter comma_opt { $$ = PARAMETER_LIST_06($2); }
    ;

lambda_parameter_list_opt
    : lambda_parameter_list { $$ = FUNC_ARGS_01(p.m_a, @$, $1); }
    | %empty { $$ = PARAMETER_LIST_12(@$); }
    ;


lambda_expression
    : KW_LAMBDA lambda_parameter_list_opt ":" expr %prec LAMBDA {
        $$ = LAMBDA_01($2, $4, @$); }
    ;

comprehension
    : "[" expr comp_for_items "]" { $$ = LIST_COMP_1($2, $3, @$); }
    | "{" expr comp_for_items "}" { $$ = SET_COMP_1($2, $3, @$); }
    | "{" expr ":" expr comp_for_items "}" {
        $$ = DICT_COMP_1($2, $4, $5, @$); }
    | "(" expr comp_for_items ")" { $$ = COMP_EXPR_1($2, $3, @$); }
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
    | KW_AWAIT expr %prec AWAIT { $$ = AWAIT($2, @$); }
    | "(" yield_expr ")" { $$ = $2; }
    | "(" TK_TYPE_IGNORE expr ")" { $$ = $3; extract_type_comment(p, @$, $2); }
    | "(" ")" { $$ = TUPLE_EMPTY(@$); }
    | "(" expr_list "," ")" { $$ = TUPLE_03($2, @$); }
    | "(" TK_TYPE_IGNORE expr_list "," ")" { $$ = TUPLE_03($3, @$);
        extract_type_comment(p, @$, $2); }
    | "(" expr_list "," expr ")" { $$ = TUPLE_01(TUPLE_($2, $4), @$); }
    | "(" TK_TYPE_IGNORE expr_list "," expr ")" {
        $$ = TUPLE_01(TUPLE_($3, $5), @$); extract_type_comment(p, @$, $2);}
    | function_call { $$ = $1; }
    | subscript { $$ = $1; }
    | "[" expr_list_opt "]" { $$ = LIST($2, @$); }
    | "{" expr_list "}" { $$ = SET($2, @$); }
    | "{" expr_list "," "}" { $$ = SET($2, @$); }
    | expr "." id { $$ = ATTRIBUTE_REF($1, $3, @$); }

    | "{" "}" { $$ = DICT_01(@$); }
    | "{" dict_list comma_opt "}" { $$ = DICT_02($2, @$); }
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

    | comprehension { $$ = $1; }
    | ternary_if_statement { $$ = $1; }
    | lambda_expression { $$ = $1; }
    ;

id
    : TK_NAME { $$ = SYMBOL($1, @$); }
    ;

type_comment
    : TK_TYPE_COMMENT { $$ = $1; }
    | TK_TYPE_IGNORE { $$ = $1; }
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
