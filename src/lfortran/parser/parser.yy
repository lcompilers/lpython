%require "3.0"
%define api.pure
%define api.value.type {LFortran::YYSTYPE}
%param {LFortran::Parser &p}
%locations
%glr-parser
%expect    111 // shift/reduce conflicts
%expect-rr 15  // reduce/reduce conflicts

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
#include <lfortran/parser/parser.h>
}

%code // *.cpp
{

#include <lfortran/parser/parser.h>
#include <lfortran/parser/tokenizer.h>
#include <lfortran/parser/semantics.h>

int yylex(LFortran::YYSTYPE *yylval, YYLTYPE *yyloc, LFortran::Parser &p)
{
    return p.m_tokenizer.lex(*yylval, *yyloc);
} // ylex

void yyerror(YYLTYPE *yyloc, LFortran::Parser &p, const std::string &msg)
{
    LFortran::YYSTYPE yylval_;
    YYLTYPE yyloc_;
    p.m_tokenizer.cur = p.m_tokenizer.tok;
    int token = p.m_tokenizer.lex(yylval_, yyloc_);
    throw LFortran::ParserError(msg, *yyloc, token);
}

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
%token <string> TK_NAME
%token <string> TK_DEF_OP
%token <n> TK_INTEGER
%token <string> TK_REAL
%token <string> TK_BOZ_CONSTANT

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
%token TK_PERCENT "%"
%token TK_VBAR "|"

%token <string> TK_STRING
%token <string> TK_COMMENT

%token TK_DBL_DOT ".."
%token TK_DBL_COLON "::"
%token TK_POW "**"
%token TK_CONCAT "//"
%token TK_ARROW "=>"

%token TK_EQ "=="
%token TK_NE "/="
%token TK_LT "<"
%token TK_LE "<="
%token TK_GT ">"
%token TK_GE ">="

%token TK_NOT ".not."
%token TK_AND ".and."
%token TK_OR ".or."
%token TK_EQV ".eqv."
%token TK_NEQV ".neqv."

%token TK_TRUE ".true."
%token TK_FALSE ".false."

// Terminal tokens: semi-reserved keywords

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
%token <string> KW_ELSEIF
%token <string> KW_ELSEWHERE
%token <string> KW_END
%token <string> KW_END_IF
%token <string> KW_ENDIF
%token <string> KW_END_DO
%token <string> KW_ENDDO
%token <string> KW_END_WHERE
%token <string> KW_ENDWHERE
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
%token <string> KW_REDUCE
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

// Nonterminal tokens

%type <ast> expr
%type <vec_ast> expr_list
%type <ast> id
%type <vec_ast> id_list
%type <vec_ast> id_list_opt
%type <ast> script_unit
%type <ast> module
%type <ast> module_decl
%type <vec_ast> module_decl_star
%type <ast> private_decl
%type <ast> public_decl
%type <ast> interface_decl
%type <ast> derived_type_decl
%type <ast> program
%type <ast> subroutine
%type <ast> sub_or_func
%type <vec_ast> sub_args
%type <ast> function
%type <ast> use_statement
%type <vec_ast> use_statement_star
%type <ast> use_symbol
%type <vec_ast> use_symbol_list
%type <vec_ast> var_decl_star
%type <vec_decl> var_sym_decl_list
%type <ast> var_decl
%type <decl> var_sym_decl
%type <vec_dim> array_comp_decl_list
%type <dim> fnarray_arg
%type <vec_dim> fnarray_arg_list
%type <vec_dim> fnarray_arg_list_opt
%type <dim> array_comp_decl
%type <string> var_type
%type <string> fn_type
%type <vec_ast> var_modifiers
%type <vec_ast> var_modifier_list
%type <ast> var_modifier
%type <ast> statement
%type <ast> assignment_statement
%type <ast> associate_statement
%type <ast> associate_block
%type <ast> block_statement
%type <ast> subroutine_call
%type <ast> allocate_statement
%type <ast> deallocate_statement
%type <ast> print_statement
%type <ast> write_statement
%type <ast> if_statement
%type <ast> if_block
%type <ast> where_statement
%type <ast> where_block
%type <ast> select_statement
%type <vec_ast> case_statements
%type <ast> case_statement
%type <vec_ast> select_default_statement_opt
%type <vec_ast> select_default_statement
%type <ast> while_statement
%type <ast> do_statement
%type <ast> reduce
%type <reduce_op_type> reduce_op
%type <ast> exit_statement
%type <ast> return_statement
%type <ast> cycle_statement
%type <ast> stop_statement
%type <ast> error_stop_statement
%type <vec_ast> statements
%type <vec_ast> contains_block_opt
%type <vec_ast> sub_or_func_plus
%type <ast> result_opt

// Precedence

%left ".not." ".and." ".or." ".eqv." ".neqv."
%left "==" "/=" "<" "<=" ">" ">="
%left "//"
%left "-" "+"
%left "*" "/"
%precedence UMINUS
%right "**"

%start units

%%

// The order of rules does not matter in Bison (unlike in ANTLR). The
// precedence is specified not by the order but by %left and %right directives
// as well as with %dprec.

// ----------------------------------------------------------------------------
// Top level rules to be used for parsing.

// Higher %dprec means higher precedence

units
    : units script_unit  %dprec 9  { RESULT($2); }
    | script_unit        %dprec 10 { RESULT($1); }
    | sep
    ;

script_unit
    : module
    | program
    | subroutine
    | function
    | use_statement
    | var_decl
    | statement          %dprec 7
    | expr sep           %dprec 8
    ;

// ----------------------------------------------------------------------------
// Module definitions
//
// * private/public blocks
// * interface blocks
//

module
    : KW_MODULE id sep use_statement_star implicit_statement_opt
        module_decl_star contains_block_opt KW_END KW_MODULE id_opt sep {
            $$ = MODULE($2, $6, $7, @$); }
    ;

module_decl_star
    : module_decl_star module_decl { $$ = $1; LIST_ADD($$, $2); }
    | %empty { LIST_NEW($$); }

module_decl
    : private_decl
    | public_decl
    | var_decl
    | interface_decl
    | derived_type_decl
    ;

private_decl
    : KW_PRIVATE id_list_opt sep { $$ = PRIVATE($2, @$); }
    | KW_PRIVATE "::" id_list_opt sep { $$ = PRIVATE($3, @$); }
    ;

public_decl
    : KW_PUBLIC id_list_opt sep { $$ = PUBLIC($2, @$); }
    | KW_PUBLIC "::" id_list_opt sep { $$ = PUBLIC($3, @$); }
    ;

interface_decl
    : KW_INTERFACE id sep proc_list KW_END KW_INTERFACE id_opt sep {
            $$ = INTERFACE($2, @$); }
    ;

proc_list
    : proc_list proc
    | %empty
    ;

proc
    : KW_MODULE KW_PROCEDURE id_list sep

derived_type_decl
    : KW_TYPE derived_type_modifiers id sep derived_type_private_opt var_decl_star derived_type_contains_opt KW_END KW_TYPE id_opt sep {
        $$ = DERIVED_TYPE($3, @$); }
    ;

derived_type_private_opt
    : KW_PRIVATE sep
    | %empty
    ;

derived_type_modifiers
    : %empty
    | "::"
    | derived_type_modifier_list "::"
    ;

derived_type_modifier_list
    : derived_type_modifier_list "," derived_type_modifier
    | "," derived_type_modifier
    ;

derived_type_modifier
    : KW_PUBLIC
    | KW_EXTENDS "(" id ")"
    ;

derived_type_contains_opt
    : KW_CONTAINS sep procedure_list
    | %empty
    ;

procedure_list
    : procedure_list procedure_decl
    | procedure_decl
    ;

procedure_decl
    : KW_PROCEDURE proc_modifiers id sep
    ;

proc_modifiers
    : %empty
    | "::"
    | proc_modifier_list "::"
    ;

proc_modifier_list
    : proc_modifier_list "," proc_modifier
    | "," proc_modifier
    ;

proc_modifier
    : KW_PRIVATE
    ;


// ----------------------------------------------------------------------------
// Subroutine/functions/program definitions


program
    : KW_PROGRAM id sep use_statement_star implicit_statement_opt var_decl_star statements
        contains_block_opt KW_END end_program_opt sep {
            LLOC(@$, @10); $$ = PROGRAM($2, $6, $7, $8, @$); }
    ;

end_program_opt
    : KW_PROGRAM id_opt
    | %empty
    ;

subroutine
    : KW_SUBROUTINE id sub_args sep use_statement_star implicit_statement_opt var_decl_star statements
        KW_END KW_SUBROUTINE id_opt sep {
            LLOC(@$, @11); $$ = SUBROUTINE($2, $3, $7, $8, @$); }
    ;

function
    : fn_type pure_opt recursive_opt KW_FUNCTION id "(" id_list_opt ")"
        result_opt sep use_statement_star implicit_statement_opt var_decl_star statements KW_END KW_FUNCTION id_opt sep {
            LLOC(@$, @17); $$ = FUNCTION($1, $5, $7, $9, $13, $14, @$); }
    ;

contains_block_opt
    : KW_CONTAINS sep sub_or_func_plus { $$ = $3; }
    | %empty { LIST_NEW($$); }
    ;

sub_or_func_plus
    : sub_or_func_plus sub_or_func { LIST_ADD($$, $2); }
    | sub_or_func { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

sub_or_func
    : subroutine
    | function
    ;

sub_args
    : "(" id_list_opt ")" { $$ = $2; }
    | %empty { LIST_NEW($$); }
    ;

pure_opt
    : KW_PURE
    | %empty
    ;

recursive_opt
    : KW_RECURSIVE
    | %empty
    ;

fn_type
    : var_type { $$ = $1;}
    | %empty { $$.p = nullptr; $$.n = 0; }
    ;

result_opt
    : KW_RESULT "(" id ")" { $$ = $3; }
    | %empty { $$ = nullptr; }
    ;

implicit_statement_opt
    : implicit_statement
    | %empty
    ;

implicit_statement
    : KW_IMPLICIT KW_NONE sep
    ;

use_statement_star
    : use_statement_star use_statement { $$ = $1; LIST_ADD($$, $2); }
    | %empty { LIST_NEW($$); }
    ;

use_statement
    : KW_USE use_modifiers id sep { $$ = USE1($3, @$); }
    | KW_USE use_modifiers id "," KW_ONLY ":" use_symbol_list sep {
            $$ = USE2($3, $7, @$); }
    ;

use_symbol_list
    : use_symbol_list "," use_symbol { $$ = $1; LIST_ADD($$, $3); }
    | use_symbol { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

use_symbol
    : id          { $$ = USE_SYMBOL1($1, @$); }
    | id "=>" id  { $$ = USE_SYMBOL2($1, $3, @$); }
    ;

use_modifiers
    : %empty
    | "::"
    | use_modifier_list "::"
    ;

use_modifier_list
    : use_modifier_list "," use_modifier
    | "," use_modifier
    ;

use_modifier
    : KW_INTRINSIC
    ;

// var_decl*
var_decl_star
    : var_decl_star var_decl { $$ = $1; LIST_ADD($$, $2); }
    | %empty { LIST_NEW($$); }
    ;

var_decl
    : var_type var_modifiers var_sym_decl_list sep {
            $$ = VAR_DECL($1, $2, $3, @$); }
    | var_modifier var_sym_decl_list sep {
            $$ = VAR_DECL2($1, $2, @$); }
    | var_modifier "::" var_sym_decl_list sep {
            $$ = VAR_DECL2($1, $3, @$); }
    ;

kind_arg_list
    : kind_arg_list "," kind_arg2
    | kind_arg2
    ;

kind_arg2
    : kind_arg
    | id "=" kind_arg
    ;

kind_arg
    : expr
    | "*"
    | ":"
    ;

kind_selector
    : "(" kind_arg_list ")"
    | %empty
    ;

var_modifiers
    : %empty { LIST_NEW($$); }
    | "::" { LIST_NEW($$); }
    | var_modifier_list "::" { $$ = $1; }
    ;

var_modifier_list
    : var_modifier_list "," var_modifier { $$=$1; LIST_ADD($$, $3); }
    | "," var_modifier { LIST_NEW($$); LIST_ADD($$, $2); }
    ;

var_modifier
    : KW_PARAMETER { $$ = VARMOD($1, @$); }
    | KW_DIMENSION "(" array_comp_decl_list ")" { $$ = VARMOD_DIM($1, $3, @$); }
    | KW_ALLOCATABLE { $$ = VARMOD($1, @$); }
    | KW_POINTER { $$ = VARMOD($1, @$); }
    | KW_TARGET { $$ = VARMOD($1, @$); }
    | KW_OPTIONAL { $$ = VARMOD($1, @$); }
    | KW_PROTECTED { $$ = VARMOD($1, @$); }
    | KW_SAVE { $$ = VARMOD($1, @$); }
    | KW_CONTIGUOUS { $$ = VARMOD($1, @$); }
    | KW_INTENT "(" KW_IN ")" { $$ = VARMOD2($1, $3, @$); }
    | KW_INTENT "(" KW_OUT ")" { $$ = VARMOD2($1, $3, @$); }
    | KW_INTENT "(" KW_INOUT ")" { $$ = VARMOD2($1, $3, @$); }
    ;


var_type
    : KW_INTEGER kind_selector
    | KW_CHARACTER kind_selector
    | KW_REAL kind_selector
    | KW_COMPLEX kind_selector
    | KW_LOGICAL kind_selector
    | KW_TYPE "(" id ")"
    | KW_CLASS "(" id ")"
    ;

var_sym_decl_list
    : var_sym_decl_list "," var_sym_decl { $$=$1; PLIST_ADD($$, $3); }
    | var_sym_decl { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

var_sym_decl
    : id                                     { $$ = VAR_SYM_DECL1($1, @$); }
    | id "=" expr                            { $$ = VAR_SYM_DECL2($1, $3, @$); }
    | id "=>" expr                           { $$ = VAR_SYM_DECL5($1, $3, @$); }
    | id "(" array_comp_decl_list ")"        { $$ = VAR_SYM_DECL3($1, $3, @$); }
    | id "(" array_comp_decl_list ")" "=" expr {
            $$ = VAR_SYM_DECL4($1, $3, $6, @$); }
    | id "(" array_comp_decl_list ")" "=>" expr {
            $$ = VAR_SYM_DECL6($1, $3, $6, @$); }
    ;

array_comp_decl_list
    : array_comp_decl_list "," array_comp_decl { $$ = $1; LIST_ADD($$, $3); }
    | array_comp_decl { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

array_comp_decl
    : expr           { $$ = ARRAY_COMP_DECL1($1, @$); }
    | expr ":" expr  { $$ = ARRAY_COMP_DECL2($1, $3, @$); }
    | expr ":"       { $$ = ARRAY_COMP_DECL3($1, @$); }
    | ":" expr       { $$ = ARRAY_COMP_DECL4($2, @$); }
    | ":"            { $$ = ARRAY_COMP_DECL5(@$); }
    ;


// -----------------------------------------------------------------------------
// Control flow

statements
    : statements statement { $$ = $1; LIST_ADD($$, $2); }
    | %empty { LIST_NEW($$); }
    ;

sep
    : sep sep_one
    | sep_one
    ;

sep_one
    : TK_NEWLINE
    | TK_COMMENT
    | ";"
    ;

statement
    : assignment_statement sep
    | associate_statement sep
    | associate_block sep
    | block_statement sep
    | allocate_statement sep
    | deallocate_statement sep
    | subroutine_call sep
    | print_statement sep
    | write_statement sep
    | exit_statement sep
    | return_statement sep
    | cycle_statement sep
    | stop_statement sep
    | error_stop_statement sep
    | if_statement
    | where_statement
    | select_statement sep
    | while_statement sep
    | do_statement sep
    ;

assignment_statement
    : expr "=" expr { $$ = ASSIGNMENT($1, $3, @$); }
    ;

associate_statement
    : expr "=>" expr { $$ = ASSOCIATE($1, $3, @$); }
    ;

associate_block
    : KW_ASSOCIATE "(" var_sym_decl_list ")" sep statements KW_END KW_ASSOCIATE {
        $$ = PRINT0(@$); }
    ;

block_statement
    : KW_BLOCK sep var_decl_star statements KW_END KW_BLOCK {
        $$ = PRINT0(@$); }
    ;

allocate_statement
    : KW_ALLOCATE "(" fnarray_arg_list_opt ")" {
            $$ = PRINT0(@$); }

deallocate_statement
    : KW_DEALLOCATE "(" fnarray_arg_list_opt ")" {
            $$ = PRINT0(@$); }

subroutine_call
    : KW_CALL id "(" fnarray_arg_list_opt ")" {
            $$ = SUBROUTINE_CALL($2, $4, @$); }
    | KW_CALL struct_member_star id "(" fnarray_arg_list_opt ")" {
            $$ = SUBROUTINE_CALL($3, $5, @$); }
    | KW_CALL id {
            $$ = SUBROUTINE_CALL2($2, @$); }
    | KW_CALL struct_member_star id {
            $$ = SUBROUTINE_CALL2($3, @$); }
    ;

print_statement
    : KW_PRINT    "*"                  { $$ = PRINT0(        @$); }
    | KW_PRINT    "*"    ","           { $$ = PRINT0(        @$); }
    | KW_PRINT    "*"    "," expr_list { $$ = PRINT(     $4, @$); }
    | KW_PRINT TK_STRING               { $$ = PRINTF0($2,    @$); }
    | KW_PRINT TK_STRING ","           { $$ = PRINTF0($2,    @$); }
    | KW_PRINT TK_STRING "," expr_list { $$ = PRINTF($2, $4, @$); }
    ;

write_arg_list
    : write_arg_list "," write_arg2
    | write_arg2
    ;

write_arg2
    : write_arg
    | id "=" write_arg
    ;

write_arg
    : expr
    | "*"
    ;

write_statement
    : KW_WRITE "(" write_arg_list ")" expr_list { $$ = PRINT($5, @$); }
    | KW_WRITE "(" write_arg_list ")" { $$ = PRINT0(@$); }
    ;

// sr-conflict (2x): KW_ENDIF can be an "id" or end of "if_statement"
if_statement
    : if_block endif sep {}
    | KW_IF "(" expr ")" statement { $$ = IFSINGLE($3, $5, @$); }
    ;

if_block
    : KW_IF "(" expr ")" KW_THEN sep statements {
            $$ = IF1($3, $7, @$); }
    | KW_IF "(" expr ")" KW_THEN sep statements KW_ELSE sep statements {
            $$ = IF2($3, $7, $10, @$); }
    | KW_IF "(" expr ")" KW_THEN sep statements KW_ELSE if_block {
            $$ = IF3($3, $7, $9, @$); }
    ;

where_statement
    : where_block endwhere sep {}
    | KW_WHERE "(" expr ")" statement { $$ = WHERESINGLE($3, $5, @$); }
    ;

where_block
    : KW_WHERE "(" expr ")" sep statements {
            $$ = WHERE1($3, $6, @$); }
    | KW_WHERE "(" expr ")" sep statements KW_ELSE sep statements {
            $$ = WHERE2($3, $6, $9, @$); }
    | KW_WHERE "(" expr ")" sep statements KW_ELSE KW_WHERE sep statements {
            $$ = WHERE2($3, $6, $10, @$); }
    | KW_WHERE "(" expr ")" sep statements KW_ELSE where_block {
            $$ = WHERE3($3, $6, $8, @$); }
    ;

select_statement
    : KW_SELECT KW_CASE "(" expr ")" sep case_statements
        select_default_statement_opt KW_END KW_SELECT {
                $$ = SELECT($4, $7, $8, @$); }
    ;

case_statements
    : case_statements case_statement { $$ = $1; LIST_ADD($$, $2); }
    | %empty { LIST_NEW($$); }
    ;

case_statement
    : KW_CASE "(" expr ")" sep statements { $$ = CASE_STMT($3, $6, @$); }
    ;

select_default_statement_opt
    : select_default_statement { $$ = $1; }
    | %empty { LIST_NEW($$); }
    ;

select_default_statement
    : KW_CASE KW_DEFAULT sep statements { $$ = $4; }
    ;


while_statement
    : KW_DO KW_WHILE "(" expr ")" sep statements enddo {
            $$ = WHILE($4, $7, @$); }

// sr-conflict (2x): "KW_DO sep" being either a do_statement or an expr
do_statement
    : KW_DO sep statements enddo id_opt {
            $$ = DO1($3, @$); }
    | id ":" KW_DO sep statements enddo id_opt {
            $$ = DO1($5, @$); }
    | KW_DO id "=" expr "," expr sep statements enddo id_opt {
            $$ = DO2($2, $4, $6, $8, @$); }
    | id ":" KW_DO id "=" expr "," expr sep statements enddo id_opt {
            $$ = DO2($4, $6, $8, $10, @$); }
    | KW_DO id "=" expr "," expr "," expr sep statements enddo id_opt {
            $$ = DO3($2, $4, $6, $8, $10, @$); }
    | id ":" KW_DO id "=" expr "," expr "," expr sep statements enddo id_opt {
            $$ = DO3($4, $6, $8, $10, $12, @$); }
    | KW_DO KW_CONCURRENT "(" id "=" expr ":" expr ")" sep statements enddo {
            $$ = DO_CONCURRENT($4, $6, $8, $11, @$); }
    | KW_DO KW_CONCURRENT "(" id "=" expr ":" expr ")" reduce sep statements enddo {
            $$ = DO_CONCURRENT_REDUCE($4, $6, $8, $10, $12, @$); }
    ;

reduce
    : KW_REDUCE "(" reduce_op ":" id_list ")" { $$ = REDUCE($3, $5, @$); }
    ;

reduce_op
    : "+" { $$ = REDUCE_OP_TYPE_ADD(@$); }
    | "*" { $$ = REDUCE_OP_TYPE_MUL(@$); }
    | id  { $$ = REDUCE_OP_TYPE_ID($1, @$); }
    ;

enddo
    : KW_END_DO
    | KW_ENDDO
    ;

endif
    : KW_END_IF
    | KW_ENDIF
    ;

endwhere
    : KW_END_WHERE
    | KW_ENDWHERE
    ;

exit_statement
    : KW_EXIT id_opt { $$ = EXIT(@$); }
    ;

return_statement
    : KW_RETURN { $$ = RETURN(@$); }
    ;

cycle_statement
    : KW_CYCLE { $$ = CYCLE(@$); }
    ;

stop_statement
    : KW_STOP { $$ = STOP(@$); }
    | KW_STOP expr { $$ = STOP1($2, @$); }
    ;

error_stop_statement
    : KW_ERROR KW_STOP { $$ = ERROR_STOP(@$); }
    | KW_ERROR KW_STOP expr { $$ = ERROR_STOP1($3, @$); }
    ;

// -----------------------------------------------------------------------------
// Fortran expression

expr_list
    : expr_list "," expr { $$ = $1; LIST_ADD($$, $3); }
    | expr { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

expr
// ### primary
    : id { $$ = $1; }
    | struct_member_star id { $$ = $2; }
    | id "(" fnarray_arg_list_opt ")" { $$ = FUNCCALLORARRAY($1, $3, @$); }
    | struct_member_star id "(" fnarray_arg_list_opt ")" {
            $$ = FUNCCALLORARRAY($2, $4, @$); }
    | "[" expr_list "]" { $$ = ARRAY_IN($2, @$); }
    | TK_INTEGER { $$ = INTEGER($1, @$); }
    | TK_REAL { $$ = REAL($1, @$); }
    | TK_STRING { $$ = STRING($1, @$); }
    | TK_BOZ_CONSTANT { $$ = STRING($1, @$); } // TODO: add BOZ AST node
    | ".true."  { $$ = TRUE(@$); }
    | ".false." { $$ = FALSE(@$); }
    | "(" expr ")" { $$ = $2; }
    | "(" expr "," expr ")" { $$ = $2; } // TODO: return a complex here

// ### level-1

// ### level-2
    | expr "+" expr { $$ = ADD($1, $3, @$); }
    | expr "-" expr { $$ = SUB($1, $3, @$); }
    | expr "*" expr { $$ = MUL($1, $3, @$); }
    | expr "/" expr { $$ = DIV($1, $3, @$); }
    | "-" expr %prec UMINUS { $$ = UNARY_MINUS($2, @$); }
    | "+" expr %prec UMINUS { $$ = UNARY_PLUS ($2, @$); }
    | expr "**" expr { $$ = POW($1, $3, @$); }

// ### level-3
    | expr "//" expr { $$ = STRCONCAT($1, $3, @$); }

// ### level-4
    | expr "==" expr { $$ = EQ($1, $3, @$); }
    | expr "/=" expr { $$ = NE($1, $3, @$); }
    | expr "<" expr { $$ = LT($1, $3, @$); }
    | expr "<=" expr { $$ = LE($1, $3, @$); }
    | expr ">" expr { $$ = GT($1, $3, @$); }
    | expr ">=" expr { $$ = GE($1, $3, @$); }

// ### level-5
    | ".not." expr { $$ = NOT($2, @$); }
    | expr ".and." expr { $$ = AND($1, $3, @$); }
    | expr ".or." expr { $$ = OR($1, $3, @$); }
    | expr ".eqv." expr { $$ = EQV($1, $3, @$); }
    | expr ".neqv." expr { $$ = NEQV($1, $3, @$); }
    ;

struct_member_star
    : struct_member_star struct_member
    | struct_member
    ;

struct_member
    : id "%"
    ;

fnarray_arg_list_opt
    : fnarray_arg_list
    | %empty { LIST_NEW($$); }
    ;

fnarray_arg_list
    : fnarray_arg_list "," fnarray_arg {$$ = $1; LIST_ADD($$, $3); }
    | fnarray_arg { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

fnarray_arg
    : array_comp_decl
    // TODO: extend "dim" to also include the keyword argument "id"
    // This can be done by adding a flag "keyword",
    // and encoding start=id, end=expr
    | id "=" expr { $$ = ARRAY_COMP_DECL1($3, @$); }
    ;

id_list_opt
    : id_list
    | %empty { LIST_NEW($$); }
    ;

id_list
    : id_list "," id { $$ = $1; LIST_ADD($$, $3); }
    | id { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

// id?
id_opt
    : id
    | %empty
    ;


id
    : TK_NAME { $$ = SYMBOL($1, @$); }
    | KW_ABSTRACT { $$ = SYMBOL($1, @$); }
    | KW_ALL { $$ = SYMBOL($1, @$); }
    | KW_ALLOCATABLE { $$ = SYMBOL($1, @$); }
    | KW_ALLOCATE { $$ = SYMBOL($1, @$); }
    | KW_ASSIGNMENT { $$ = SYMBOL($1, @$); }
    | KW_ASSOCIATE { $$ = SYMBOL($1, @$); }
    | KW_ASYNCHRONOUS { $$ = SYMBOL($1, @$); }
    | KW_BACKSPACE { $$ = SYMBOL($1, @$); }
    | KW_BIND { $$ = SYMBOL($1, @$); }
    | KW_BLOCK { $$ = SYMBOL($1, @$); }
    | KW_CALL { $$ = SYMBOL($1, @$); }
    | KW_CASE { $$ = SYMBOL($1, @$); }
    | KW_CHARACTER { $$ = SYMBOL($1, @$); }
    | KW_CLASS { $$ = SYMBOL($1, @$); }
    | KW_CLOSE { $$ = SYMBOL($1, @$); }
    | KW_CODIMENSION { $$ = SYMBOL($1, @$); }
    | KW_COMMON { $$ = SYMBOL($1, @$); }
    | KW_COMPLEX { $$ = SYMBOL($1, @$); }
    | KW_CONCURRENT { $$ = SYMBOL($1, @$); }
    | KW_CONTAINS { $$ = SYMBOL($1, @$); }
    | KW_CONTIGUOUS { $$ = SYMBOL($1, @$); }
    | KW_CONTINUE { $$ = SYMBOL($1, @$); }
    | KW_CRITICAL { $$ = SYMBOL($1, @$); }
    | KW_CYCLE { $$ = SYMBOL($1, @$); }
    | KW_DATA { $$ = SYMBOL($1, @$); }
    | KW_DEALLOCATE { $$ = SYMBOL($1, @$); }
    | KW_DEFAULT { $$ = SYMBOL($1, @$); }
    | KW_DEFERRED { $$ = SYMBOL($1, @$); }
    | KW_DIMENSION { $$ = SYMBOL($1, @$); }
    | KW_DO { $$ = SYMBOL($1, @$); }
    | KW_DOWHILE { $$ = SYMBOL($1, @$); }
    | KW_DOUBLE { $$ = SYMBOL($1, @$); }
    | KW_ELEMENTAL { $$ = SYMBOL($1, @$); }
    | KW_ELSE { $$ = SYMBOL($1, @$); }
    | KW_ELSEIF { $$ = SYMBOL($1, @$); }
    | KW_ELSEWHERE { $$ = SYMBOL($1, @$); }
    | KW_END { $$ = SYMBOL($1, @$); }
    | KW_ENDDO { $$ = SYMBOL($1, @$); }
    | KW_ENDIF { $$ = SYMBOL($1, @$); }
    | KW_ENTRY { $$ = SYMBOL($1, @$); }
    | KW_ENUM { $$ = SYMBOL($1, @$); }
    | KW_ENUMERATOR { $$ = SYMBOL($1, @$); }
    | KW_EQUIVALENCE { $$ = SYMBOL($1, @$); }
    | KW_ERRMSG { $$ = SYMBOL($1, @$); }
    | KW_ERROR { $$ = SYMBOL($1, @$); }
    | KW_EXIT { $$ = SYMBOL($1, @$); }
    | KW_EXTENDS { $$ = SYMBOL($1, @$); }
    | KW_EXTERNAL { $$ = SYMBOL($1, @$); }
    | KW_FILE { $$ = SYMBOL($1, @$); }
    | KW_FINAL { $$ = SYMBOL($1, @$); }
    | KW_FLUSH { $$ = SYMBOL($1, @$); }
    | KW_FORALL { $$ = SYMBOL($1, @$); }
    | KW_FORMAT { $$ = SYMBOL($1, @$); }
    | KW_FORMATTED { $$ = SYMBOL($1, @$); }
    | KW_FUNCTION { $$ = SYMBOL($1, @$); }
    | KW_GENERIC { $$ = SYMBOL($1, @$); }
    | KW_GO { $$ = SYMBOL($1, @$); }
    | KW_IF { $$ = SYMBOL($1, @$); }
    | KW_IMPLICIT { $$ = SYMBOL($1, @$); }
    | KW_IMPORT { $$ = SYMBOL($1, @$); }
    | KW_IMPURE { $$ = SYMBOL($1, @$); }
    | KW_IN { $$ = SYMBOL($1, @$); }
    | KW_INCLUDE { $$ = SYMBOL($1, @$); }
    | KW_INOUT { $$ = SYMBOL($1, @$); }
    | KW_INQUIRE { $$ = SYMBOL($1, @$); }
    | KW_INTEGER { $$ = SYMBOL($1, @$); }
    | KW_INTENT { $$ = SYMBOL($1, @$); }
    | KW_INTERFACE { $$ = SYMBOL($1, @$); }
    | KW_INTRINSIC { $$ = SYMBOL($1, @$); }
    | KW_IS { $$ = SYMBOL($1, @$); }
    | KW_KIND { $$ = SYMBOL($1, @$); }
    | KW_LEN { $$ = SYMBOL($1, @$); }
    | KW_LOCAL { $$ = SYMBOL($1, @$); }
    | KW_LOCAL_INIT { $$ = SYMBOL($1, @$); }
    | KW_LOGICAL { $$ = SYMBOL($1, @$); }
    | KW_MODULE { $$ = SYMBOL($1, @$); }
    | KW_MOLD { $$ = SYMBOL($1, @$); }
    | KW_NAME { $$ = SYMBOL($1, @$); }
    | KW_NAMELIST { $$ = SYMBOL($1, @$); }
    | KW_NOPASS { $$ = SYMBOL($1, @$); }
    | KW_NON_INTRINSIC { $$ = SYMBOL($1, @$); }
    | KW_NON_OVERRIDABLE { $$ = SYMBOL($1, @$); }
    | KW_NON_RECURSIVE { $$ = SYMBOL($1, @$); }
    | KW_NONE { $$ = SYMBOL($1, @$); }
    | KW_NULLIFY { $$ = SYMBOL($1, @$); }
    | KW_ONLY { $$ = SYMBOL($1, @$); }
    | KW_OPEN { $$ = SYMBOL($1, @$); }
    | KW_OPERATOR { $$ = SYMBOL($1, @$); }
    | KW_OPTIONAL { $$ = SYMBOL($1, @$); }
    | KW_OUT { $$ = SYMBOL($1, @$); }
    | KW_PARAMETER { $$ = SYMBOL($1, @$); }
    | KW_PASS { $$ = SYMBOL($1, @$); }
    | KW_POINTER { $$ = SYMBOL($1, @$); }
    | KW_PRECISION { $$ = SYMBOL($1, @$); }
    | KW_PRINT { $$ = SYMBOL($1, @$); }
    | KW_PRIVATE { $$ = SYMBOL($1, @$); }
    | KW_PROCEDURE { $$ = SYMBOL($1, @$); }
    | KW_PROGRAM { $$ = SYMBOL($1, @$); }
    | KW_PROTECTED { $$ = SYMBOL($1, @$); }
    | KW_PUBLIC { $$ = SYMBOL($1, @$); }
    | KW_PURE { $$ = SYMBOL($1, @$); }
    | KW_QUIET { $$ = SYMBOL($1, @$); }
    | KW_RANK { $$ = SYMBOL($1, @$); }
    | KW_READ { $$ = SYMBOL($1, @$); }
    | KW_REAL { $$ = SYMBOL($1, @$); }
    | KW_RECURSIVE { $$ = SYMBOL($1, @$); }
    | KW_REDUCE { $$ = SYMBOL($1, @$); }
    | KW_RESULT { $$ = SYMBOL($1, @$); }
    | KW_RETURN { $$ = SYMBOL($1, @$); }
    | KW_REWIND { $$ = SYMBOL($1, @$); }
    | KW_SAVE { $$ = SYMBOL($1, @$); }
    | KW_SELECT { $$ = SYMBOL($1, @$); }
    | KW_SEQUENCE { $$ = SYMBOL($1, @$); }
    | KW_SHARED { $$ = SYMBOL($1, @$); }
    | KW_SOURCE { $$ = SYMBOL($1, @$); }
    | KW_STAT { $$ = SYMBOL($1, @$); }
    | KW_STOP { $$ = SYMBOL($1, @$); }
    | KW_SUBMODULE { $$ = SYMBOL($1, @$); }
    | KW_SUBROUTINE { $$ = SYMBOL($1, @$); }
    | KW_TARGET { $$ = SYMBOL($1, @$); }
    | KW_TEAM { $$ = SYMBOL($1, @$); }
    | KW_TEAM_NUMBER { $$ = SYMBOL($1, @$); }
    | KW_THEN { $$ = SYMBOL($1, @$); }
    | KW_TO { $$ = SYMBOL($1, @$); }
    | KW_TYPE { $$ = SYMBOL($1, @$); }
    | KW_UNFORMATTED { $$ = SYMBOL($1, @$); }
    | KW_USE { $$ = SYMBOL($1, @$); }
    | KW_VALUE { $$ = SYMBOL($1, @$); }
    | KW_VOLATILE { $$ = SYMBOL($1, @$); }
    | KW_WHERE { $$ = SYMBOL($1, @$); }
    | KW_WHILE { $$ = SYMBOL($1, @$); }
    | KW_WRITE { $$ = SYMBOL($1, @$); }
    ;
