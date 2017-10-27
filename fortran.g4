grammar fortran;

module
    : 'module' IDENT NEWLINE+ use_statement* 'implicit none' NEWLINE+ decl* ('contains' NEWLINE+ subroutine+ )?  'end module' IDENT? NEWLINE+ EOF
    ;

use_statement
    : 'use' IDENT (',' 'only' ':' param_list)? NEWLINE+
    ;

decl
    : private_decl
    | public_decl
    | var_decl
    ;

private_decl
    : 'private' NEWLINE+
    ;

public_decl
    : 'public' IDENT (',' IDENT)* NEWLINE+
    ;

var_decl
    : var_type ('(' IDENT ')')? (',' modifier)* '::' var_sym_decl (',' var_sym_decl)* NEWLINE+
    ;

var_sym_decl
    : IDENT ('=' expr)?
    ;

expr
    : expr '**' expr
    | expr ('*'|'/') expr
    | expr ('+'|'-') expr
    | number
    | IDENT
    | fn_call
    | '(' expr ')'
	;

fn_call
    : IDENT '(' expr? ')'
    ;

var_type
    : 'integer' | 'char' | 'real' | 'complex' | 'logical'
    ;

modifier
    : 'parameter' | 'intent' | 'dimension' | 'allocatable' | 'pointer'
    | 'intent' '(' ('in' | 'out' | 'inout' ) ')'
    ;

subroutine
    : 'subroutine' IDENT ('(' param_list? ')')? NEWLINE+ decl* statement* 'end subroutine' NEWLINE+
    ;

param_list
    : IDENT (',' IDENT)*
    ;

statement
    : IDENT '=' expr NEWLINE+
    | fn_call NEWLINE+
    ;

number
    : NUMBER                    // Real number
    | '(' NUMBER ',' NUMBER ')' // Complex number
    ;

NUMBER
    : ([0-9]+ '.' [0-9]* | '.' [0-9]+) ([eEdD] [+-]? [0-9]+)? ('_' IDENT)?
    |  [0-9]+                          ([eEdD] [+-]? [0-9]+)? ('_' IDENT)?
    ;

IDENT
    : ('a' .. 'z' | 'A' .. 'Z') ('a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_')*
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
