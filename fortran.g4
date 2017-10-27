/*
This is a grammar for a subset of modern Fortran.

The file is organized in several sections, rules in each section only depend on
that or below sections, never on sections above. As such any section together
with all the subsequent sections form a self-contained grammar.
*/

grammar fortran;

root
    : module | program
    ;

program
    : 'program' ID NEWLINE+ use_statement* 'implicit none' NEWLINE+ decl* statements? ('contains' NEWLINE+ (subroutine|function)+ )?  'end program' ID? NEWLINE+ EOF
    ;

module
    : 'module' ID NEWLINE+ use_statement* 'implicit none' NEWLINE+ decl* ('contains' NEWLINE+ (subroutine|function)+ )?  'end module' ID? NEWLINE+ EOF
    ;

use_statement
    : 'use' ID (',' 'only' ':' param_list)? NEWLINE+
    ;

decl
    : private_decl
    | public_decl
    | var_decl
    | interface_decl
    ;

private_decl
    : 'private' NEWLINE+
    ;

public_decl
    : 'public' ID (',' ID)* NEWLINE+
    ;

var_decl
    : var_type ('(' ID ')')? (',' var_modifier)* '::' var_sym_decl (',' var_sym_decl)* NEWLINE+
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

interface_decl
    : 'interface' ID NEWLINE+ ('module' 'procedure' ID NEWLINE+)* 'end' 'interface' ID? NEWLINE+
    ;

subroutine
    : 'subroutine' ID ('(' param_list? ')')? NEWLINE+ decl* statements? 'end subroutine' NEWLINE+
    ;

function
    : 'pure'? 'recursive'? 'function' ID ('(' param_list? ')')? ('result' '(' ID ')')? NEWLINE+ decl* statements? 'end function' NEWLINE+
    ;


var_type
    : 'integer' | 'char' | 'real' | 'complex' | 'logical'
    ;

var_modifier
    : 'parameter' | 'intent' | 'dimension' | 'allocatable' | 'pointer'
    | 'intent' '(' ('in' | 'out' | 'inout' ) ')'
    | 'save'
    ;

param_list
    : ID (',' ID)*
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
    : ID '=' expr
    | 'exit'
    | subroutine_call
    | if_statement
    | do_statement
    | where_statement
    | print_statement
    | ';'
    ;

subroutine_call
    : 'call' ID '(' expr_list? ')'
    ;


// TODO: if_block -> multiline_if, merge else_block
if_statement
    : 'if' '(' expr ')' statement
    | if_block 'end' 'if'
    ;

if_block
    : 'if' '(' expr ')' 'then' NEWLINE+ statements ('else' (if_block | (NEWLINE+ statements)))?
    ;

// TODO: the same here as with if
where_statement
    : 'where' '(' expr ')' statement
    | 'where' where_block 'end' 'where'
    ;

where_block
    : '(' expr ')' NEWLINE+ statements else_where_block?
    ;

else_where_block
    : 'else' 'where' (where_block | (NEWLINE+ statements))
    ;

do_statement
    : 'do' (ID '=' expr ',' expr (',' expr)?)? NEWLINE+ statements 'end' 'do'
    ;

print_statement
    : 'print' '*' (',' expr)*
    ;


// ----------------------------------------------------------------------------
// Fortran expression
//
// * function calls
// * array operations
// * arithmetics
// * numbers/variables/strings/etc.
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
    : ID '(' expr_list? ')'            // func call like f(), f(x), f(1,2)
    | ID '(' array_index_list ')'      // array index like a(i), a(i, :, 3:)
    | <assoc=right> expr '**' expr
    | ('+'|'-') expr
    | '.not.' expr
    | expr ('*'|'/') expr
    | expr ('+'|'-') expr
    | expr ('<'|'<='|'=='|'/='|'>='|'>') expr
    | expr ('.and.'|'.or.') expr
    | ID
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
