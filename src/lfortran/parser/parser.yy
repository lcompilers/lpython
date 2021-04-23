%require "3.0"
%define api.pure
%define api.value.type {LFortran::YYSTYPE}
%param {LFortran::Parser &p}
%locations
%glr-parser
%expect    468 // shift/reduce conflicts
%expect-rr 81  // reduce/reduce conflicts

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
%token <n> TK_LABEL
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
%token TK_RBRACKET_OLD "/)"
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
%token <string> KW_END_INTERFACE
%token <string> KW_ENDINTERFACE
%token <string> KW_END_FORALL
%token <string> KW_ENDFORALL
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
%token <string> KW_IN_OUT
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
%type <vec_ast> expr_list_opt
%type <ast> id
%type <vec_ast> id_list
%type <vec_ast> id_list_opt
%type <ast> script_unit
%type <ast> module
%type <ast> submodule
%type <ast> decl
%type <vec_ast> decl_star
%type <ast> interface_decl
%type <ast> interface_stmt
%type <ast> derived_type_decl
%type <ast> enum_decl
%type <ast> program
%type <ast> subroutine
%type <ast> procedure
%type <ast> sub_or_func
%type <vec_ast> sub_args
%type <ast> function
%type <ast> use_statement
%type <vec_ast> use_statement_star
%type <ast> use_symbol
%type <vec_ast> use_symbol_list
%type <vec_ast> var_decl_star
%type <vec_var_sym> var_sym_decl_list
%type <ast> var_decl
%type <var_sym> var_sym_decl
%type <vec_dim> array_comp_decl_list
%type <fnarg> fnarray_arg
%type <vec_fnarg> fnarray_arg_list_opt
%type <dim> array_comp_decl
%type <ast> var_type
%type <ast> fn_mod
%type <vec_ast> fn_mod_plus
%type <vec_ast> var_modifiers
%type <vec_ast> enum_var_modifiers
%type <vec_ast> var_modifier_list
%type <ast> var_modifier
%type <ast> statement
%type <ast> statement1
%type <ast> single_line_statement
%type <ast> multi_line_statement
%type <ast> multi_line_statement0
%type <ast> assignment_statement
%type <ast> goto_statement
%type <ast> associate_statement
%type <ast> associate_block
%type <ast> block_statement
%type <ast> subroutine_call
%type <ast> allocate_statement
%type <ast> deallocate_statement
%type <ast> nullify_statement
%type <ast> print_statement
%type <ast> open_statement
%type <ast> close_statement
%type <ast> write_statement
%type <ast> read_statement
%type <ast> inquire_statement
%type <ast> rewind_statement
%type <ast> backspace_statement
%type <ast> if_statement
%type <ast> if_statement_single
%type <ast> if_block
%type <ast> elseif_block
%type <ast> where_statement
%type <ast> where_statement_single
%type <ast> where_block
%type <ast> select_statement
%type <ast> select_type_statement
%type <vec_ast> case_statements
%type <ast> case_statement
%type <vec_ast> select_default_statement_opt
%type <vec_ast> select_default_statement
%type <ast> while_statement
%type <ast> do_statement
%type <ast> forall_statement
%type <ast> forall_statement_single
%type <vec_ast> concurrent_locality_star
%type <ast> concurrent_locality
%type <reduce_op_type> reduce_op
%type <ast> exit_statement
%type <ast> return_statement
%type <ast> cycle_statement
%type <ast> continue_statement
%type <ast> stop_statement
%type <ast> error_stop_statement
%type <ast> format_statement
%type <vec_ast> statements
%type <vec_ast> contains_block_opt
%type <vec_ast> sub_or_func_plus
%type <ast> result_opt
%type <ast> result
%type <string> inout
%type <vec_ast> concurrent_control_list
%type <ast> concurrent_control
%type <vec_var_sym> named_constant_def_list
%type <var_sym> named_constant_def
%type <vec_kind_arg> kind_arg_list
%type <kind_arg> kind_arg2
%type <vec_ast> interface_body
%type <ast> interface_item
%type <ast> write_arg
%type <argstarkw> write_arg2
%type <vec_argstarkw> write_arg_list
%type <struct_member> struct_member
%type <vec_struct_member> struct_member_star
%type <ast> bind
%type <ast> bind_opt
%type <vec_ast> import_statement_star
%type <ast> import_statement
%type <ast> implicit_statement
%type <vec_ast> implicit_statement_star
%type <ast> implicit_none_spec
%type <vec_ast> implicit_none_spec_list
%type <ast> letter_spec
%type <vec_ast> letter_spec_list

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
    | submodule
    | program
    | subroutine
    | procedure
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
    : KW_MODULE id sep use_statement_star implicit_statement_star
        decl_star contains_block_opt KW_END end_module_opt sep {
            $$ = MODULE($2, $4, $5, $6, $7, @$); }
    ;

submodule
    : KW_SUBMODULE "(" id ")" id sep use_statement_star implicit_statement_star
        decl_star contains_block_opt KW_END end_submodule_opt sep {
            $$ = SUBMODULE($3, $5, $7, $8, $9, $10, @$); }
    ;

interface_decl
    : interface_stmt sep interface_body endinterface sep {
            $$ = INTERFACE($1, $3, @$); }
    ;

interface_stmt
    : KW_INTERFACE { $$ = INTERFACE_HEADER1(@$); }
    | KW_INTERFACE id { $$ = INTERFACE_HEADER2($2, @$); }
    | KW_INTERFACE KW_ASSIGNMENT "(" "=" ")" { $$ = INTERFACE_HEADER3(@$); }
    | KW_INTERFACE KW_OPERATOR "(" operator_type ")" {
        $$ = INTERFACE_HEADER4(@$); }
    | KW_ABSTRACT KW_INTERFACE { $$ = INTERFACE_HEADER5(@$); }
    ;

endinterface
    : endinterface0
    | endinterface0 id
    | endinterface0 KW_ASSIGNMENT "(" "=" ")"
    | endinterface0 KW_OPERATOR "(" operator_type ")"
    ;

endinterface0
    : KW_END_INTERFACE
    | KW_ENDINTERFACE
    ;


interface_body
    : interface_body interface_item { $$ = $1; LIST_ADD($$, $2); }
    | %empty { LIST_NEW($$); }
    ;

interface_item
    : fn_mod_plus KW_PROCEDURE id_list sep {
        $$ = INTERFACE_MODULE_PROC($3, @$); }
    | fn_mod_plus KW_PROCEDURE "::" id_list sep {
        $$ = INTERFACE_MODULE_PROC($4, @$); }
    | KW_PROCEDURE id_list sep {
        $$ = INTERFACE_MODULE_PROC($2, @$); }
    | KW_PROCEDURE "::" id_list sep {
        $$ = INTERFACE_MODULE_PROC($3, @$); }
    | subroutine {
        $$ = INTERFACE_PROC($1, @$); }
    | function {
        $$ = INTERFACE_PROC($1, @$); }
    ;

enum_decl
    : KW_ENUM enum_var_modifiers sep var_decl_star KW_END KW_ENUM sep {
        $$ = ENUM(@$); }
    ;

enum_var_modifiers
    : %empty { LIST_NEW($$); }
    | var_modifier_list { $$ = $1; }
    ;

derived_type_decl
    : KW_TYPE var_modifiers id sep var_decl_star derived_type_contains_opt KW_END KW_TYPE id_opt sep {
        $$ = DERIVED_TYPE($3, $5, @$); }
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
    : KW_PROCEDURE proc_paren proc_modifiers use_symbol_list sep
    | KW_GENERIC "::" KW_OPERATOR "(" operator_type ")" "=>" id_list sep
    | KW_GENERIC "::" KW_ASSIGNMENT "(" "=" ")" "=>" id_list sep
    | KW_GENERIC "::" id "=>" id_list sep
    | KW_FINAL "::" id sep
    ;

operator_type
    : "+"
    | "-"
    | "=="
    | "/="
    | ">"
    | ">="
    | "<"
    | "<="
    ;

proc_paren
    : %empty
    | "(" id ")"
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
    | KW_PUBLIC
    | KW_PASS "(" id ")"
    | KW_NOPASS
    | KW_DEFERRED
    | KW_NON_OVERRIDABLE
    ;


// ----------------------------------------------------------------------------
// Subroutine/Procedure/functions/program definitions


program
    : KW_PROGRAM id sep use_statement_star implicit_statement_star decl_star statements
        contains_block_opt KW_END end_program_opt sep {
            LLOC(@$, @10); $$ = PROGRAM($2, $4, $5, $6, $7, $8, @$); }
    ;

end_program_opt
    : KW_PROGRAM id_opt
    | %empty
    ;

end_module_opt
    : KW_MODULE id_opt
    | %empty
    ;

end_submodule_opt
    : KW_SUBMODULE id_opt
    | %empty
    ;

end_subroutine_opt
    : KW_SUBROUTINE id_opt
    | %empty
    ;

end_procedure_opt
    : KW_PROCEDURE id_opt
    | %empty
    ;

end_function_opt
    : KW_FUNCTION id_opt
    | %empty
    ;

subroutine
    : KW_SUBROUTINE id sub_args bind_opt sep use_statement_star
    import_statement_star implicit_statement_star decl_star statements
        contains_block_opt
        KW_END end_subroutine_opt sep {
            LLOC(@$, @13); $$ = SUBROUTINE($2, $3, $4, $6, $7, $8, $9, $10, $11, @$); }
    | fn_mod_plus KW_SUBROUTINE id sub_args bind_opt sep use_statement_star
    import_statement_star implicit_statement_star decl_star statements
        contains_block_opt
        KW_END end_subroutine_opt sep {
            LLOC(@$, @14); $$ = SUBROUTINE($3, $4, $5, $7, $8, $9, $10, $11, $12, @$); }
    ;

procedure
    : fn_mod_plus KW_PROCEDURE id sub_args sep use_statement_star
    import_statement_star implicit_statement_star decl_star statements
        contains_block_opt
        KW_END end_procedure_opt sep {
            LLOC(@$, @14); $$ = PROCEDURE($3, $4, $6, $9, $10, $11, @$); }
    ;

function
    : KW_FUNCTION id "(" id_list_opt ")"
        sep use_statement_star import_statement_star implicit_statement_star decl_star statements
        contains_block_opt
        KW_END end_function_opt sep {
            LLOC(@$, @14); $$ = FUNCTION0($2, $4, nullptr, nullptr, $7, $8, $9, $10, $11, $12, @$); }
    | KW_FUNCTION id "(" id_list_opt ")"
        bind
        result_opt
        sep use_statement_star import_statement_star implicit_statement_star decl_star statements
        contains_block_opt
        KW_END end_function_opt sep {
            LLOC(@$, @16); $$ = FUNCTION0($2, $4, $7, $6, $9, $10, $11, $12, $13, $14, @$); }
    | KW_FUNCTION id "(" id_list_opt ")"
        result
        bind_opt
        sep use_statement_star import_statement_star implicit_statement_star decl_star statements
        contains_block_opt
        KW_END end_function_opt sep {
            LLOC(@$, @16); $$ = FUNCTION0($2, $4, $6, $7, $9, $10, $11, $12, $13, $14, @$); }
    | fn_mod_plus KW_FUNCTION id "(" id_list_opt ")"
        sep use_statement_star import_statement_star implicit_statement_star decl_star statements
        contains_block_opt
        KW_END end_function_opt sep {
            LLOC(@$, @15); $$ = FUNCTION($1, $3, $5, nullptr, nullptr, $8, $9, $10, $11, $12, $13, @$); }
    | fn_mod_plus KW_FUNCTION id "(" id_list_opt ")"
        bind
        result_opt
        sep use_statement_star import_statement_star implicit_statement_star decl_star statements
        contains_block_opt
        KW_END end_function_opt sep {
            LLOC(@$, @17); $$ = FUNCTION($1, $3, $5, $8, $7, $10, $11, $12, $13, $14, $15, @$); }
    | fn_mod_plus KW_FUNCTION id "(" id_list_opt ")"
        result
        bind_opt
        sep use_statement_star import_statement_star implicit_statement_star decl_star statements
        contains_block_opt
        KW_END end_function_opt sep {
            LLOC(@$, @17); $$ = FUNCTION($1, $3, $5, $7, $8, $10, $11, $12, $13, $14, $15, @$); }
    ;

fn_mod_plus
    : fn_mod_plus fn_mod { $$ = $1; LIST_ADD($$, $2); }
    | fn_mod { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

fn_mod
    : var_type { $$ = $1; }
    | KW_ELEMENTAL { $$ = SIMPLE_ATTR(Elemental, @$); }
    | KW_IMPURE { $$ = SIMPLE_ATTR(Impure, @$); }
    | KW_MODULE { $$ = SIMPLE_ATTR(Module, @$); }
    | KW_PURE { $$ = SIMPLE_ATTR(Pure, @$); }
    | KW_RECURSIVE {  $$ = SIMPLE_ATTR(Recursive, @$); }
    ;

decl_star
    : decl_star decl { $$ = $1; LIST_ADD($$, $2); }
    | %empty { LIST_NEW($$); }

decl
    : var_decl
    | interface_decl
    | derived_type_decl
    | enum_decl
    ;

contains_block_opt
    : KW_CONTAINS sep sub_or_func_plus { $$ = $3; }
    | KW_CONTAINS sep { LIST_NEW($$); }
    | %empty { LIST_NEW($$); }
    ;

sub_or_func_plus
    : sub_or_func_plus sub_or_func { LIST_ADD($$, $2); }
    | sub_or_func { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

sub_or_func
    : subroutine
    | function
    | procedure
    ;

sub_args
    : "(" id_list_opt ")" { $$ = $2; }
    | %empty { LIST_NEW($$); }
    ;

bind_opt
    : bind { $$ = $1; }
    | %empty { $$ = nullptr; }
    ;

bind
    : KW_BIND "(" write_arg_list ")" { $$ = BIND2($3, @$); }
    ;

result_opt
    : result { $$ = $1; }
    | %empty { $$ = nullptr; }
    ;

result
    : KW_RESULT "(" id ")" { $$ = $3; }
    ;

implicit_statement_star
    : implicit_statement_star implicit_statement { $$ = $1; LIST_ADD($$, $2); }
    | %empty { LIST_NEW($$); }
    ;

implicit_statement
    : KW_IMPLICIT KW_NONE sep { $$ = IMPLICIT_NONE(@$); }
    | KW_IMPLICIT KW_NONE "(" implicit_none_spec_list ")" sep {
            $$ = IMPLICIT_NONE2($4, @$); }
    | KW_IMPLICIT KW_INTEGER "(" letter_spec_list ")" sep {
            $$ = IMPLICIT(ATTR_TYPE(Integer, @$), $4, @$); }
    | KW_IMPLICIT KW_INTEGER "*" TK_INTEGER "(" letter_spec_list ")" sep {
            $$ = IMPLICIT(ATTR_TYPE_INT(Integer, $4, @$), $6, @$); }
    | KW_IMPLICIT KW_CHARACTER "(" letter_spec_list ")" sep {
            $$ = IMPLICIT(ATTR_TYPE(Character, @$), $4, @$); }
    | KW_IMPLICIT KW_CHARACTER "*" TK_INTEGER "(" letter_spec_list ")" sep {
            $$ = IMPLICIT(ATTR_TYPE_INT(Character, $4, @$), $6, @$); }
    | KW_IMPLICIT KW_REAL "(" letter_spec_list ")" sep {
            $$ = IMPLICIT(ATTR_TYPE(Real, @$), $4, @$); }
    | KW_IMPLICIT KW_REAL "*" TK_INTEGER "(" letter_spec_list ")" sep {
            $$ = IMPLICIT(ATTR_TYPE_INT(Real, $4, @$), $6, @$); }
    | KW_IMPLICIT KW_COMPLEX "(" letter_spec_list ")" sep {
            $$ = IMPLICIT(ATTR_TYPE(Complex, @$), $4, @$); }
    | KW_IMPLICIT KW_COMPLEX "*" TK_INTEGER "(" letter_spec_list ")" sep {
            $$ = IMPLICIT(ATTR_TYPE_INT(Complex, $4, @$), $6, @$); }
    | KW_IMPLICIT KW_LOGICAL "(" letter_spec_list ")" sep {
            $$ = IMPLICIT(ATTR_TYPE(Logical, @$), $4, @$); }
    | KW_IMPLICIT KW_LOGICAL "*" TK_INTEGER "(" letter_spec_list ")" sep {
            $$ = IMPLICIT(ATTR_TYPE_INT(Logical, $4, @$), $6, @$); }
    | KW_IMPLICIT KW_DOUBLE KW_PRECISION "(" letter_spec_list ")" sep {
            $$ = IMPLICIT(ATTR_TYPE(DoublePrecision, @$), $5, @$); }
    | KW_IMPLICIT KW_TYPE "(" id ")" "(" letter_spec_list ")" sep {
            $$ = IMPLICIT(ATTR_TYPE_NAME(Type, $4, @$), $7, @$); }
    | KW_IMPLICIT KW_PROCEDURE "(" id ")" "(" letter_spec_list ")" sep {
            $$ = IMPLICIT(ATTR_TYPE_NAME(Procedure, $4, @$), $7, @$); }
    | KW_IMPLICIT KW_CLASS "(" id ")" "(" letter_spec_list ")" sep {
            $$ = IMPLICIT(ATTR_TYPE_NAME(Class, $4, @$), $7, @$); }
    ;

implicit_none_spec_list
    : implicit_none_spec_list "," implicit_none_spec { $$ = $1; LIST_ADD($$, $3); }
    | implicit_none_spec { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

implicit_none_spec
    : KW_EXTERNAL { $$ = IMPLICIT_NONE_EXTERNAL(@$); }
    | KW_TYPE { $$ = IMPLICIT_NONE_TYPE(@$); }
    ;

letter_spec_list
    : letter_spec_list "," letter_spec { $$ = $1; LIST_ADD($$, $3); }
    | letter_spec { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

letter_spec
    : id           { $$ = LETTER_SPEC1($1, @$); }
    | id "-" id    { $$ = LETTER_SPEC2($1, $3, @$); }
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

import_statement_star
    : import_statement_star import_statement { $$ = $1; LIST_ADD($$, $2); }
    | %empty { LIST_NEW($$); }
    ;

import_statement
    : KW_IMPORT sep { $$ = IMPORT0(Default, @$); }
    | KW_IMPORT id_list sep { $$ = IMPORT1($2, Default, @$); }
    | KW_IMPORT "::" id_list sep { $$ = IMPORT1($3, Default, @$); }
    | KW_IMPORT "," KW_ONLY ":" id_list sep { $$ = IMPORT1($5, Only, @$); }
    | KW_IMPORT "," KW_NONE sep { $$ = IMPORT0(None, @$); }
    | KW_IMPORT "," KW_ALL sep { $$ = IMPORT0(All, @$); }
    ;

use_symbol_list
    : use_symbol_list "," use_symbol { $$ = $1; LIST_ADD($$, $3); }
    | use_symbol { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

use_symbol
    : id          { $$ = USE_SYMBOL1($1, @$); }
    | id "=>" id  { $$ = USE_SYMBOL2($1, $3, @$); }
    | KW_ASSIGNMENT "(" "=" ")"  { $$ = USE_SYMBOL3(@$); }
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
            $$ = VAR_DECL1($1, $2, $3, @$); }
    | var_modifier sep {
            $$ = VAR_DECL2($1, @$); }
    | var_modifier var_sym_decl_list sep {
            $$ = VAR_DECL3($1, $2, @$); }
    | var_modifier "::" var_sym_decl_list sep {
            $$ = VAR_DECL3($1, $3, @$); }
    | KW_PARAMETER "(" named_constant_def_list ")" sep {
            $$ = VAR_DECL_PARAMETER($3, @$); }
    | KW_NAMELIST "/" id "/" id_list sep {
            $$ = VAR_DECL_NAMELIST($3, $5, @$); }
    ;

named_constant_def_list
    : named_constant_def_list "," named_constant_def {
            $$ = $1; PLIST_ADD($$, $3); }
    | named_constant_def { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

named_constant_def
    : id "=" expr { VAR_SYM($$, $1, nullptr, 0, $3, @$); }
    ;


kind_arg_list
    : kind_arg_list "," kind_arg2 { $$ = $1; LIST_ADD($$, *$3); }
    | kind_arg2 { LIST_NEW($$); LIST_ADD($$, *$1); }
    ;

kind_arg2
    : expr { $$ = KIND_ARG1($1, @$); }
    | "*" { $$ = KIND_ARG1S(@$); }
    | ":" { $$ = KIND_ARG1C(@$); }
    | id "=" expr { $$ = KIND_ARG2($1, $3, @$); }
    | id "=" "*" { $$ = KIND_ARG2S($1, @$); }
    | id "=" ":" { $$ = KIND_ARG2C($1, @$); }
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
    : KW_PARAMETER { $$ = SIMPLE_ATTR(Parameter, @$); }
    | KW_DIMENSION "(" array_comp_decl_list ")" { $$ = DIMENSION($3, @$); }
    | KW_DIMENSION { $$ = DIMENSION0(@$); }
    | KW_ALLOCATABLE { $$ = SIMPLE_ATTR(Allocatable, @$); }
    | KW_POINTER { $$ = SIMPLE_ATTR(Pointer, @$); }
    | KW_TARGET { $$ = SIMPLE_ATTR(Target, @$); }
    | KW_OPTIONAL { $$ = SIMPLE_ATTR(Optional, @$); }
    | KW_PROTECTED { $$ = SIMPLE_ATTR(Protected, @$); }
    | KW_SAVE { $$ = SIMPLE_ATTR(Save, @$); }
    | KW_CONTIGUOUS { $$ = SIMPLE_ATTR(Contiguous, @$); }
    | KW_NOPASS { $$ = SIMPLE_ATTR(NoPass, @$); }
    | KW_PRIVATE { $$ = SIMPLE_ATTR(Private, @$); }
    | KW_PUBLIC { $$ = SIMPLE_ATTR(Public, @$); }
    | KW_ABSTRACT { $$ = SIMPLE_ATTR(Abstract, @$); }
    | KW_ENUMERATOR { $$ = SIMPLE_ATTR(Enumerator, @$); }
    | KW_INTENT "(" KW_IN ")" { $$ = INTENT(In, @$); }
    | KW_INTENT "(" KW_OUT ")" { $$ = INTENT(Out, @$); }
    | KW_INTENT "(" inout ")" { $$ = INTENT(InOut, @$); }
    | KW_VALUE { $$ = SIMPLE_ATTR(Value, @$); }
    | KW_EXTENDS "(" id ")" { $$ = EXTENDS($3, @$); }
    | KW_BIND "(" id ")" { $$ = BIND($3, @$); }
    ;


var_type
    : KW_INTEGER { $$ = ATTR_TYPE(Integer, @$); }
    | KW_INTEGER "(" kind_arg_list ")" { $$ = ATTR_TYPE_KIND(Integer, $3, @$); }
    | KW_INTEGER "*" TK_INTEGER { $$ = ATTR_TYPE_INT(Integer, $3, @$); }
    | KW_CHARACTER { $$ = ATTR_TYPE(Character, @$); }
    | KW_CHARACTER "(" kind_arg_list ")" { $$ = ATTR_TYPE_KIND(Character, $3, @$); }
    | KW_CHARACTER "*" TK_INTEGER { $$ = ATTR_TYPE_INT(Character, $3, @$); }
    | KW_CHARACTER "*" "(" "*" ")" { $$ = ATTR_TYPE(Character, @$); } // TODO
    | KW_REAL { $$ = ATTR_TYPE(Real, @$); }
    | KW_REAL "(" kind_arg_list ")" { $$ = ATTR_TYPE_KIND(Real, $3, @$); }
    | KW_REAL "*" TK_INTEGER { $$ = ATTR_TYPE_INT(Real, $3, @$); }
    | KW_COMPLEX { $$ = ATTR_TYPE(Complex, @$); }
    | KW_COMPLEX "(" kind_arg_list ")" { $$ = ATTR_TYPE_KIND(Complex, $3, @$); }
    | KW_COMPLEX "*" TK_INTEGER { $$ = ATTR_TYPE_INT(Complex, $3, @$); }
    | KW_LOGICAL { $$ = ATTR_TYPE(Logical, @$); }
    | KW_LOGICAL "(" kind_arg_list ")" { $$ = ATTR_TYPE_KIND(Logical, $3, @$); }
    | KW_LOGICAL "*" TK_INTEGER { $$ = ATTR_TYPE_INT(Logical, $3, @$); }
    | KW_DOUBLE KW_PRECISION { $$ = ATTR_TYPE(DoublePrecision, @$); }
    | KW_TYPE "(" id ")" { $$ = ATTR_TYPE_NAME(Type, $3, @$); }
    | KW_PROCEDURE "(" id ")" { $$ = ATTR_TYPE_NAME(Procedure, $3, @$); }
    | KW_CLASS "(" id ")" { $$ = ATTR_TYPE_NAME(Class, $3, @$); }
    | KW_CLASS "(" "*" ")" { $$ = ATTR_TYPE(Class, @$); } // TODO
    ;

var_sym_decl_list
    : var_sym_decl_list "," var_sym_decl { $$=$1; PLIST_ADD($$, $3); }
    | var_sym_decl { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

var_sym_decl
    : id { VAR_SYM2($$, $1, nullptr, 0, @$); }
    | id "=" expr { VAR_SYM($$, $1, nullptr, 0, $3, @$); }
    | id "=>" expr { VAR_SYM($$, $1, nullptr, 0, $3, @$); }
    | id "*" expr { VAR_SYM2($$, $1, nullptr, 0, @$); }
    | id "(" array_comp_decl_list ")" { VAR_SYM2($$, $1, $3.p, $3.n, @$); }
    | id "(" array_comp_decl_list ")" "=" expr {
            VAR_SYM($$, $1, $3.p, $3.n, $6, @$); }
    | id "(" array_comp_decl_list ")" "=>" expr {
            VAR_SYM($$, $1, $3.p, $3.n, $6, @$); }

// TODO: is this needed? It seems it should go somewheer else
/*
    | KW_ASSIGNMENT "(" "=" ")"              {
            $$ = FIXME(@$); }
*/
    ;

array_comp_decl_list
    : array_comp_decl_list "," array_comp_decl { $$ = $1; PLIST_ADD($$, $3); }
    | array_comp_decl { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

array_comp_decl
    : expr           { $$ = ARRAY_COMP_DECL1d($1, @$); }
    | expr ":" expr  { $$ = ARRAY_COMP_DECL2d($1, $3, @$); }
    | expr ":"       { $$ = ARRAY_COMP_DECL3d($1, @$); }
    | ":" expr       { $$ = ARRAY_COMP_DECL4d($2, @$); }
    | ":"            { $$ = ARRAY_COMP_DECL5d(@$); }
    | "*"            { $$ = ARRAY_COMP_DECL6d(@$); }
    | expr ":" "*"   { $$ = ARRAY_COMP_DECL7d($1, @$); }
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
    : statement1 sep { $$ = $1; }
    | TK_LABEL statement1 sep { $$ = $2; LABEL($$, $1); }
    ;

statement1
    : single_line_statement
    | multi_line_statement
    ;

single_line_statement
    : allocate_statement
    | assignment_statement
    | associate_statement
    | close_statement
    | continue_statement
    | cycle_statement
    | deallocate_statement
    | error_stop_statement
    | exit_statement
    | forall_statement_single
    | format_statement
    | goto_statement
    | if_statement_single
    | inquire_statement
    | nullify_statement
    | open_statement
    | print_statement
    | read_statement
    | return_statement
    | rewind_statement
    | backspace_statement
    | stop_statement
    | subroutine_call
    | where_statement_single
    | write_statement
    ;

multi_line_statement
    : multi_line_statement0 { $$ = $1; }
    | id ":" multi_line_statement0 id { $$ = STMT_NAME($1, $4, $3); }
    ;

multi_line_statement0
    : associate_block
    | block_statement
    | do_statement
    | forall_statement
    | if_statement
    | select_statement
    | select_type_statement
    | where_statement
    | while_statement
    ;

assignment_statement
    : expr "=" expr { $$ = ASSIGNMENT($1, $3, @$); }
    ;

goto_statement
    : KW_GO KW_TO TK_INTEGER { $$ = GOTO($3, @$); }
    ;

associate_statement
    : expr "=>" expr { $$ = ASSOCIATE($1, $3, @$); }
    ;

associate_block
    : KW_ASSOCIATE "(" var_sym_decl_list ")" sep statements KW_END KW_ASSOCIATE {
        $$ = ASSOCIATE_BLOCK($3, $6, @$); }
    ;

block_statement
    : KW_BLOCK sep var_decl_star statements KW_END KW_BLOCK {
        $$ = BLOCK($3, $4, @$); }
    ;

allocate_statement
    : KW_ALLOCATE "(" fnarray_arg_list_opt ")" {
            $$ = ALLOCATE_STMT($3, @$); }

deallocate_statement
    : KW_DEALLOCATE "(" fnarray_arg_list_opt ")" {
            $$ = DEALLOCATE_STMT($3, @$); }

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

open_statement
    : KW_OPEN "(" write_arg_list ")" { $$ = OPEN($3, @$); }

close_statement
    : KW_CLOSE "(" write_arg_list ")" { $$ = CLOSE($3, @$); }

write_arg_list
    : write_arg_list "," write_arg2 { $$ = $1; PLIST_ADD($$, $3); }
    | write_arg2 { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

write_arg2
    : write_arg { WRITE_ARG1($$, $1); }
    | id "=" write_arg { WRITE_ARG2($$, $1, $3); }
    ;

write_arg
    : expr { $$ = $1; }
    | "*" { $$ = nullptr; }
    ;

write_statement
    : KW_WRITE "(" write_arg_list ")" expr_list { $$ = WRITE($3, $5, @$); }
    | KW_WRITE "(" write_arg_list ")" "," expr_list { $$ = WRITE($3, $6, @$); }
    | KW_WRITE "(" write_arg_list ")" { $$ = WRITE0($3, @$); }
    ;

read_statement
    : KW_READ "(" write_arg_list ")" expr_list { $$ = READ($3, $5, @$); }
    | KW_READ "(" write_arg_list ")" "," expr_list { $$ = READ($3, $6, @$); }
    | KW_READ "(" write_arg_list ")" { $$ = READ0($3, @$); }
    ;

nullify_statement
    : KW_NULLIFY "(" write_arg_list ")" {
            $$ = NULLIFY($3, @$); }

inquire_statement
    : KW_INQUIRE "(" write_arg_list ")" expr_list { $$ = INQUIRE($3, $5, @$); }
    | KW_INQUIRE "(" write_arg_list ")" { $$ = INQUIRE0($3, @$); }
    ;

rewind_statement
    : KW_REWIND "(" write_arg_list ")" { $$ = REWIND($3, @$); }
    | KW_REWIND id { $$ = REWIND2($2, @$); }
    | KW_REWIND TK_INTEGER { $$ = REWIND3($2, @$); }
    ;

backspace_statement
    : KW_BACKSPACE "(" write_arg_list ")" { $$ = BACKSPACE($3, @$); }
    ;

// sr-conflict (2x): KW_ENDIF can be an "id" or end of "if_statement"
if_statement
    : if_block endif {}
    ;

if_statement_single
    : KW_IF "(" expr ")" single_line_statement { $$ = IFSINGLE($3, $5, @$); }
    ;

if_block
    : KW_IF "(" expr ")" KW_THEN sep statements {
            $$ = IF1($3, $7, @$); }
    | KW_IF "(" expr ")" KW_THEN sep statements KW_ELSE sep statements {
            $$ = IF2($3, $7, $10, @$); }
    | KW_IF "(" expr ")" KW_THEN sep statements KW_ELSE if_block {
            $$ = IF3($3, $7, $9, @$); }
    | KW_IF "(" expr ")" KW_THEN sep statements elseif_block {
            $$ = IF3($3, $7, $8, @$); }
    ;

elseif_block
    : KW_ELSEIF "(" expr ")" KW_THEN sep statements {
            $$ = IF1($3, $7, @$); }
    | KW_ELSEIF "(" expr ")" KW_THEN sep statements KW_ELSE sep statements {
            $$ = IF2($3, $7, $10, @$); }
    | KW_ELSEIF "(" expr ")" KW_THEN sep statements KW_ELSE if_block {
            $$ = IF3($3, $7, $9, @$); }
    | KW_ELSEIF "(" expr ")" KW_THEN sep statements elseif_block {
            $$ = IF3($3, $7, $8, @$); }
    ;

where_statement
    : where_block endwhere {}
    ;

where_statement_single
    : KW_WHERE "(" expr ")" assignment_statement { $$ = WHERESINGLE($3, $5, @$); }
    ;

where_block
    : KW_WHERE "(" expr ")" sep statements {
            $$ = WHERE1($3, $6, @$); }
    | KW_WHERE "(" expr ")" sep statements KW_ELSE sep statements {
            $$ = WHERE2($3, $6, $9, @$); }
    | KW_WHERE "(" expr ")" sep statements KW_ELSE KW_WHERE sep statements {
            $$ = WHERE2($3, $6, $10, @$); }
    | KW_WHERE "(" expr ")" sep statements KW_ELSEWHERE sep statements {
            $$ = WHERE2($3, $6, $9, @$); }
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
    : KW_CASE "(" expr_list ")" sep statements { $$ = CASE_STMT($3, $6, @$); }
    | KW_CASE "(" expr ":" ")" sep statements { $$ = CASE_STMT2($3, $7, @$); }
    | KW_CASE "(" ":" expr ")" sep statements { $$ = CASE_STMT3($4, $7, @$); }
    | KW_CASE "(" expr ":" expr ")" sep statements {
        $$ = CASE_STMT4($3, $5, $8, @$); }
    ;

select_default_statement_opt
    : select_default_statement { $$ = $1; }
    | %empty { LIST_NEW($$); }
    ;

select_default_statement
    : KW_CASE KW_DEFAULT sep statements { $$ = $4; }
    ;

select_type_statement
    : KW_SELECT KW_TYPE "(" expr ")" sep select_type_body_statements
        KW_END KW_SELECT {
                $$ = PRINT0(@$); }
    | KW_SELECT KW_TYPE "(" id "=>" expr ")" sep select_type_body_statements
        KW_END KW_SELECT {
                $$ = PRINT0(@$); }
    ;

select_type_body_statements
    : select_type_body_statements select_type_body_statement
    | %empty
    ;

select_type_body_statement
    : KW_TYPE KW_IS "(" TK_NAME ")" sep statements
    | KW_TYPE KW_IS "(" var_type ")" sep statements
    | KW_CLASS KW_IS "(" id ")" sep statements
    | KW_CLASS KW_DEFAULT sep statements
    ;


while_statement
    : KW_DO KW_WHILE "(" expr ")" sep statements enddo {
            $$ = WHILE($4, $7, @$); }
    ;

// sr-conflict (2x): "KW_DO sep" being either a do_statement or an expr
do_statement
    : KW_DO sep statements enddo {
            $$ = DO1($3, @$); }
    | KW_DO id "=" expr "," expr sep statements enddo {
            $$ = DO2($2, $4, $6, $8, @$); }
    | KW_DO id "=" expr "," expr "," expr sep statements enddo {
            $$ = DO3($2, $4, $6, $8, $10, @$); }
    | KW_DO TK_INTEGER id "=" expr "," expr sep statements enddo {
            $$ = DO2($3, $5, $7, $9, @$); }
    | KW_DO TK_INTEGER id "=" expr "," expr "," expr sep statements enddo {
            $$ = DO3($3, $5, $7, $9, $11, @$); }
    | KW_DO KW_CONCURRENT "(" concurrent_control_list ")"
        concurrent_locality_star sep statements enddo {
            $$ = DO_CONCURRENT1($4, $6, $8, @$); }
    | KW_DO KW_CONCURRENT "(" concurrent_control_list "," expr ")"
        concurrent_locality_star sep statements enddo {
            $$ = DO_CONCURRENT2($4, $6, $8, $10, @$); }
    ;

concurrent_control_list
    : concurrent_control_list "," concurrent_control {
        $$ = $1; LIST_ADD($$, $3); }
    | concurrent_control { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

concurrent_control
    : id "=" expr ":" expr {
            $$ = CONCURRENT_CONTROL1($1, $3, $5,     @$); }
    | id "=" expr ":" expr ":" expr {
            $$ = CONCURRENT_CONTROL2($1, $3, $5, $7, @$); }
    ;

concurrent_locality_star
    : concurrent_locality_star concurrent_locality {
        $$ = $1; LIST_ADD($$, $2); }
    | %empty { LIST_NEW($$); }
    ;

concurrent_locality
    : KW_LOCAL "(" id_list ")" { $$ = CONCURRENT_LOCAL($3, @$); }
    | KW_LOCAL_INIT "(" id_list ")" { $$ = CONCURRENT_LOCAL_INIT($3, @$); }
    | KW_SHARED "(" id_list ")" { $$ = CONCURRENT_SHARED($3, @$); }
    | KW_DEFAULT "(" KW_NONE ")" { $$ = CONCURRENT_DEFAULT(@$); }
    | KW_REDUCE "(" reduce_op ":" id_list ")" {
        $$ = CONCURRENT_REDUCE($3, $5, @$); }
    ;

forall_statement
    : KW_FORALL "(" concurrent_control_list ")"
        concurrent_locality_star sep statements endforall {
            $$ = DO_CONCURRENT1($3, $5, $7, @$); }
    | KW_FORALL "(" concurrent_control_list "," expr ")"
        concurrent_locality_star sep statements endforall {
            $$ = DO_CONCURRENT2($3, $5, $7, $9, @$); }
    ;

forall_statement_single
    : KW_FORALL "(" concurrent_control_list ")"
        assignment_statement { $$ = PRINT0(@$); }
    | KW_FORALL "(" concurrent_control_list "," expr ")"
        assignment_statement { $$ = PRINT0(@$); }
    ;

format_statement
    : KW_FORMAT "(" format_items ")" { $$ = FORMAT(@$); }
    | KW_FORMAT "(" format_items "," "*" "(" format_items ")" ")" {
            $$ = FORMAT(@$); }
    | KW_FORMAT "(" "*" "(" format_items ")" ")" {
            $$ = FORMAT(@$); }
    | KW_FORMAT "(" "/)" { $$ = FORMAT(@$); }
    | KW_FORMAT "(" TK_INTEGER "/)" { $$ = FORMAT(@$); }
    | KW_FORMAT "(" format_items "," "/)" { $$ = FORMAT(@$); }
    ;

format_items
    : format_items "," format_item
    | format_item
    ;


format_item
    : format_item1
    | format_item_slash
    | format_item_slash format_item1
    | format_item1 format_item_slash format_item1
    | ":"
    ;

format_item_slash
    : "/"
    | TK_INTEGER "/"
    | "//"
    ;

format_item1
    : format_item0
    | TK_INTEGER format_item0
    ;

format_item0
    : TK_NAME
    | TK_NAME TK_REAL
    | TK_REAL TK_REAL
    | TK_STRING
    | "(" format_items ")"
    ;

reduce_op
    : "+" { $$ = REDUCE_OP_TYPE_ADD(@$); }
    | "*" { $$ = REDUCE_OP_TYPE_MUL(@$); }
    | id  { $$ = REDUCE_OP_TYPE_ID($1, @$); }
    ;

inout
    : KW_IN_OUT
    | KW_INOUT
    ;

enddo
    : KW_END_DO
    | TK_LABEL KW_END_DO
    | KW_ENDDO
    | TK_LABEL KW_ENDDO
    ;

endforall
    : KW_END_FORALL
    | KW_ENDFORALL
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
    : KW_EXIT { $$ = EXIT(@$); }
    | KW_EXIT id { $$ = EXIT2($2, @$); }
    ;

return_statement
    : KW_RETURN { $$ = RETURN(@$); }
    ;

cycle_statement
    : KW_CYCLE { $$ = CYCLE(@$); }
    | KW_CYCLE id { $$ = CYCLE2($2, @$); }
    ;

continue_statement
    : KW_CONTINUE { $$ = CONTINUE(@$); }
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

expr_list_opt
    : expr_list { $$ = $1; }
    | %empty { LIST_NEW($$); }
    ;

expr_list
    : expr_list "," expr { $$ = $1; LIST_ADD($$, $3); }
    | expr { LIST_NEW($$); LIST_ADD($$, $1); }
    ;

rbracket
    : "]"
    | "/)"
    ;

expr
// ### primary
    : id { $$ = $1; }
    | struct_member_star id { NAME1($$, $2, $1, @$); }
    | id "(" fnarray_arg_list_opt ")" { $$ = FUNCCALLORARRAY($1, $3, @$); }
    | struct_member_star id "(" fnarray_arg_list_opt ")" {
            $$ = FUNCCALLORARRAY2($1, $2, $4, @$); }
    | "[" expr_list_opt rbracket { $$ = ARRAY_IN($2, @$); }
    | "[" var_type "::" expr_list_opt rbracket { $$ = ARRAY_IN($4, @$); }
    | TK_INTEGER { $$ = INTEGER($1, @$); }
    | TK_REAL { $$ = REAL($1, @$); }
    | TK_STRING { $$ = STRING($1, @$); }
    | TK_BOZ_CONSTANT { $$ = STRING($1, @$); } // TODO: add BOZ AST node
    | ".true."  { $$ = TRUE(@$); }
    | ".false." { $$ = FALSE(@$); }
    | "(" expr ")" { $$ = $2; }
    | "(" expr "," expr ")" { $$ = COMPLEX($2, $4, @$); }
    | "(" expr "," id "=" expr "," expr ")" {
            $$ = IMPLIED_DO_LOOP1($2, $4, $6, $8, @$); }
    | "(" expr "," expr "," id "=" expr "," expr ")" {
            $$ = IMPLIED_DO_LOOP2($2, $4, $6, $8, $10, @$); }
    | "(" expr "," expr "," expr_list "," id "=" expr "," expr ")" {
            $$ = IMPLIED_DO_LOOP3($2, $4, $6, $8, $10, $12, @$); }

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
    : struct_member_star struct_member { $$ = $1; PLIST_ADD($$, $2); }
    | struct_member { LIST_NEW($$); PLIST_ADD($$, $1); }
    ;

struct_member
    : id "%" { STRUCT_MEMBER1($$, $1); }
    | id "(" fnarray_arg_list_opt ")" "%" { STRUCT_MEMBER2($$, $1, $3); }
    ;

fnarray_arg_list_opt
    : fnarray_arg_list_opt "," fnarray_arg { $$ = $1; PLIST_ADD($$, $3); }
    | fnarray_arg { LIST_NEW($$); PLIST_ADD($$, $1); }
    | %empty { LIST_NEW($$); }
    ;

fnarray_arg
// array element / function argument
    : expr                   { $$ = ARRAY_COMP_DECL_0i0($1, @$); }
// array section
    | ":"                    { $$ = ARRAY_COMP_DECL_001(@$); }
    | expr ":"               { $$ = ARRAY_COMP_DECL_a01($1, @$); }
    | ":" expr               { $$ = ARRAY_COMP_DECL_0b1($2, @$); }
    | expr ":" expr          { $$ = ARRAY_COMP_DECL_ab1($1, $3, @$); }
    | "::" expr              { $$ = ARRAY_COMP_DECL_00c($2, @$); }
    | ":" ":" expr           { $$ = ARRAY_COMP_DECL_00c($3, @$); }
    | expr "::" expr         { $$ = ARRAY_COMP_DECL_a0c($1, $3, @$); }
    | expr ":" ":" expr      { $$ = ARRAY_COMP_DECL_a0c($1, $4, @$); }
    | ":" expr ":" expr      { $$ = ARRAY_COMP_DECL_0bc($2, $4, @$); }
    | expr ":" expr ":" expr { $$ = ARRAY_COMP_DECL_abc($1, $3, $5, @$); }
// keyword function argument
    | id "=" expr            { $$ = ARRAY_COMP_DECL1k($1, $3, @$); }
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
    | KW_ENDINTERFACE { $$ = SYMBOL($1, @$); }
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
