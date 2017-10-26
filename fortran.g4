grammar fortran;

module
    : 'module' IDENT 'implicit none' decl*  'end' IDENT?
    ;

decl
    : private_decl
    | public_decl
    | var_decl
    ;

private_decl
    : 'private'
    ;

public_decl
    : 'public' IDENT (',' IDENT)*
    ;

var_decl
    : type (',' modifier)* '::' var_sym_decl (',' var_sym_decl)*
    ;

var_sym_decl
    : IDENT ('=' expr)?
    ;

expr
	: IDENT | NUMBER | fn_call
	;

fn_call
    : IDENT '(' expr ')'
    ;

type
    : 'integer' | 'char' | 'real' | 'logical'
    ;

modifier
    : 'parameter' | 'intent' | 'dimension' | 'allocatable' | 'pointer'
    ;

NUMBER
    : ([0-9]+ '.' [0-9]* | '.' [0-9]+) ([eEdD] [+-]? [0-9]+)?
    |  [0-9]+                          ([eEdD] [+-]? [0-9]+)?
    ;

IDENT
    : ('a' .. 'z' | 'A' .. 'Z') ('a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_')*
    ;

COMMENT
    : '!' ~[\r\n]* -> skip;

LINE_CONTINUATION
    : '&' -> skip
    ;

WS
    : [ \t\r\n] -> skip
    ;
