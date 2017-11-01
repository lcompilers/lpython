/*
This is a grammar for a subset of modern Fortran.

The file is organized in several sections, rules in each section only depend on
the same or further sections, never on the previous sections. As such any
section together with all the subsequent sections form a self-contained
grammar.
*/

grammar fortran;

// ----------------------------------------------------------------------------
// Module definitions
//
// * private/public blocks
// * interface blocks
//

root
    : module | program
    ;

module
    : NEWLINE* 'module' ID NEWLINE+ use_statement* 'implicit none' NEWLINE+ module_decl* ('contains' NEWLINE+ (subroutine|function)+ )? 'end' 'module' ID? NEWLINE+ EOF
    ;

module_decl
    : private_decl
    | public_decl
    | var_decl
    | interface_decl
    ;

private_decl
    : 'private' '::'? id_list? NEWLINE+
    ;

public_decl
    : 'public' '::'? id_list? NEWLINE+
    ;

interface_decl
    : 'interface' ID NEWLINE+ ('module' 'procedure' id_list NEWLINE+)* 'end' 'interface' ID? NEWLINE+
    ;

// ----------------------------------------------------------------------------
// Subroutine/functions/program definitions
//
// * argument lists
// * use statement
// * variable (and arrays) declarations
//
// It turns out that all subroutines, functions and programs have a very similar
// structure.
//

program
    : NEWLINE* ('program'|'PROGRAM') ID NEWLINE+ sub_block 'program'? ID? NEWLINE+ EOF
    ;

subroutine
    : 'subroutine' ID ('(' id_list? ')')? NEWLINE+ sub_block 'subroutine' ID? NEWLINE+
    ;

function
    : (var_type ('(' ID ')')?)? 'pure'? 'recursive'? 'function' ID ('(' id_list? ')')? ('result' '(' ID ')')? NEWLINE+ sub_block 'function' ID? NEWLINE+
    ;

sub_block
    : use_statement* implicit_statement? var_decl* statements? ('contains' NEWLINE+ (subroutine|function)+ )? ('end'|'END')
    ;

implicit_statement
    : 'implicit none' NEWLINE+
    ;

use_statement
    : 'use' use_symbol_list (',' 'only' ':' use_symbol_list)? NEWLINE+
    ;

use_symbol_list : use_symbol (',' use_symbol)* ;
use_symbol : ID | ID '=>' ID ;

id_list : ID (',' ID)* ;

var_decl
    : var_type ('(' (ID '=')? ('*' | ID) ')')? (',' var_modifier)* '::'? var_sym_decl (',' var_sym_decl)* NEWLINE+
    ;

var_type
    : 'integer' | 'char' | 'real' | 'complex' | 'logical' | 'type'
    | 'character'
    ;

var_modifier
    : 'parameter' | 'intent' | 'dimension' array_decl? | 'allocatable' | 'pointer'
    | 'protected' | 'save' | 'contiguous'
    | 'intent' '(' ('in' | 'out' | 'inout' ) ')'
    ;

var_sym_decl
    : ID array_decl? ('=' expr)?
    ;

array_decl
    : '(' array_comp_decl (',' array_comp_decl)* ')'
    ;

array_comp_decl
    : expr
    | expr? ':' expr?
    ;


// ----------------------------------------------------------------------------
// Control flow:
//
// * statements
//     * assignment
//     * if/do/where/print/exit statements
//     * subroutine calls
//

statements
    : (statement (NEWLINE+ | ';' NEWLINE*))+
    ;

statement
    : assignment_statement
    | 'exit' | 'cycle' | 'return'
    | subroutine_call
    | builtin_statement
    | if_statement
    | do_statement
    | while_statement
    | select_statement
    | where_statement
    | print_statement
    | write_statement
    | stop_statement
    | ';'
    ;

assignment_statement
    : struct_member* ID ('(' array_index_list ')')? op=('='|'=>') expr
    ;

subroutine_call
    : 'call' struct_member* ID '(' arg_list? ')'
    ;

builtin_statement
    : ('allocate' | 'open' | 'close') '(' arg_list? ')'
    ;


if_statement
    : if_cond statement
    | if_block 'end' 'if'
    ;

if_cond: 'if' '(' expr ')' ;

if_block
    : if_cond 'then' NEWLINE+ statements ('else' (if_block | (NEWLINE+ statements)))?
    ;

where_statement
    : where_cond statement
    | where_block 'end' 'where'
    ;

where_cond: 'where' '(' expr ')' ;

where_block
    : where_cond NEWLINE+ statements ('else' 'where'? (where_block | (NEWLINE+ statements)))?
    ;

do_statement
    : 'do' (ID '=' expr ',' expr (',' expr)?)? NEWLINE+ statements 'end' 'do'
    ;

while_statement
    : 'do' 'while' '(' expr ')' NEWLINE+ statements 'end' 'do'
    ;

select_statement
    : 'select' 'case' '(' expr ')' NEWLINE+ case_statement* select_default_statement? 'end' 'select'
    ;

case_statement
    : 'case' '(' expr ')' NEWLINE+ statements
    ;

select_default_statement
    : 'case' 'default' NEWLINE+ statements
    ;

print_statement
    : ('print'|'PRINT') ('*' | STRING) ',' expr_list?
    ;

write_statement
    : 'write' '(' ('*' | expr) ',' ('*' | STRING) ')' expr_list?
    ;

stop_statement
    : 'stop' STRING?
    ;


// ----------------------------------------------------------------------------
// Fortran expression
//
// * function calls
// * array operations
// * arithmetics
// * numbers/variables/strings/etc.
// * derived types access (e.g. x%a(3))
//
// Expressions are used in previous sections in the following situations:
//
// * assignments, possibly during declaration (x=1+2)
// * subroutine/function calls (f(1+2, 2*a))
// * in array dimension declarations (integer :: a(2*n))
// * in if statement conditions (if (x == y+1) then)
//
// An expression can have any type (e.g. logical, integer, real, string), so
// the allowed usage depends on the actual type (e.g. a dimension of an array
// must be an integer, an if statement condition must be a logical, etc.).
//

expr_list
    : expr (',' expr)*
    ;

/*
expr_fn_call: func call like f(), f(x), f(1,2), but it can also be an array
             access
expr_array_call: array index like a(i), a(i, :, 3:)
expr_array_const: arrays like [1, 2, 3, x]
*/
expr
    : struct_member* fn_names '(' arg_list? ')'  # expr_fn_call
    | struct_member* ID '(' array_index_list ')' # expr_array_call
    | '[' expr_list ']'                          # expr_array_const
    | <assoc=right> expr '**' expr               # expr_pow
    | op=('+'|'-') expr                          # expr_unary
    | '.not.' expr                               # expr_not
    | expr op=('*'|'/') expr                     # expr_muldiv
    | expr op=('+'|'-') expr                     # expr_addsub
    | expr op=('<'|'<='|'=='|'/='|'>='|'>') expr # expr_rel
    | expr op=('.and.'|'.or.') expr              # expr_andor
    | (ID '%')* ID                               # expr_id
    | number                                     # expr_number
    | op=('.true.' | '.false.')                  # expr_truefalse
    | expr '//' expr                             # expr_string_conc
    | STRING                                     # expr_string
    | '(' expr ')'                               # expr_nest // E.g. (1+2)*3
	;

arg_list
    : arg (',' arg)*
    ;

arg
    : expr
    | ID '=' expr
    ;

array_index_list
    : array_index (',' array_index)*
    ;

array_index
    : expr
    | expr? ':' expr?
    ;

struct_member: ID '%' ;

number
    : NUMBER                    # number_real    // Real number
    | '(' NUMBER ',' NUMBER ')' # number_complex // Complex number
    ;

fn_names: ID | 'real' ; // real is both a type and a function name


// ----------------------------------------------------------------------------
// Lexer
//
// * numbers
// * identifiers (variables/arguments/function calls)
// * strings
// * comments
// * white space / end of lines
//
// Besides the explicit tokens below (in uppercase), there are implicit tokens
// above in quotes, such as '.true.', 'exit', '*', '(', etc. Those all together
// form the tokens and are saved into `fortranLexer.tokens`.
//

NUMBER
    : ([0-9]+ '.' [0-9]* | '.' [0-9]+) EXP? NTYP?
    |  [0-9]+                          EXP? NTYP?
    ;

fragment EXP : [eEdD] [+-]? [0-9]+ ;
fragment NTYP : '_' ID ;

ID
    : ('a' .. 'z' | 'A' .. 'Z') ('a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_')*
    ;

STRING
    : '"' ('""'|~'"')* '"'
    | '\'' ('\'\''|~'\'')* '\''
    ;

COMMENT
    : '!' ~[\r\n]* -> skip;

LINE_CONTINUATION
    : '&' WS* COMMENT? NEWLINE -> skip
    ;

NEWLINE
    : '\r'? '\n'
    ;

WS
    : [ \t] -> skip
    ;
