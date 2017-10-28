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
    : NEWLINE* 'program' ID NEWLINE+ sub_block 'program' ID? NEWLINE+ EOF
    ;

subroutine
    : 'subroutine' ID ('(' id_list? ')')? NEWLINE+ sub_block 'subroutine' ID? NEWLINE+
    ;

function
    : 'pure'? 'recursive'? (var_type ('(' ID ')')?)? 'function' ID ('(' id_list? ')')? ('result' '(' ID ')')? NEWLINE+ sub_block 'function' ID? NEWLINE+
    ;

sub_block
    : use_statement* implicit_statement? var_decl* statements? ('contains' NEWLINE+ (subroutine|function)+ )? 'end'
    ;

implicit_statement
    : 'implicit none' NEWLINE+
    ;

use_statement
    : 'use' ID (',' 'only' ':' only_symbol (',' only_symbol)*)? NEWLINE+
    ;

only_symbol : ID | ID '=>' ID ;

id_list : ID (',' ID)* ;

var_decl
    : var_type ('(' ID ')')? (',' var_modifier)* '::'? var_sym_decl (',' var_sym_decl)* NEWLINE+
    ;

var_type
    : 'integer' | 'char' | 'real' | 'complex' | 'logical' | 'type'
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
    | ':'
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
    : struct_member* ID ('(' array_index_list ')')? ('='|'=>') expr
    | 'exit' | 'cycle'
    | subroutine_call
    | if_statement
    | do_statement
    | while_statement
    | where_statement
    | print_statement
    | write_statement
    | stop_statement
    | ';'
    ;

subroutine_call
    : 'call' struct_member* ID '(' expr_list? ')'
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

print_statement
    : 'print' ('*' | STRING) ',' expr_list?
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

expr
    : struct_member* ID '(' expr_list? ')'       // func call like f(), f(x), f(1,2)
    | struct_member* ID '(' array_index_list ')' // array index like a(i), a(i, :, 3:)
    | '[' expr_list ']'                          // arrays like [1, 2, 3, x]
    | <assoc=right> expr '**' expr
    | ('+'|'-') expr
    | '.not.' expr
    | expr ('*'|'/') expr
    | expr ('+'|'-') expr
    | expr ('<'|'<='|'=='|'/='|'>='|'>') expr
    | expr ('.and.'|'.or.') expr
    | (ID '%')* ID
    | number
    | '.true.' | '.false.'
    | STRING
    | '(' expr ')'  // E.g. (1+2)*3
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
    : NUMBER                    // Real number
    | '(' NUMBER ',' NUMBER ')' // Complex number
    ;


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
