grammar fortran;

module
    : 'module' IDENT NEWLINE+ 'implicit none' NEWLINE+ decl* ('contains' NEWLINE+ subroutine+ )?  'end module' IDENT? NEWLINE+
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
    : var_type (',' modifier)* '::' var_sym_decl (',' var_sym_decl)* NEWLINE+
    ;

var_sym_decl
    : IDENT ('=' expr)?
    ;

expr
	: expr ('*'|'/') expr
    | expr ('+'|'-') expr
    | NUMBER
    | IDENT
    | fn_call
    | '(' expr ')'
	;

fn_call
    : IDENT '(' expr? ')'
    ;

var_type
    : 'integer' | 'char' | 'real' | 'logical'
    ;

modifier
    : 'parameter' | 'intent' | 'dimension' | 'allocatable' | 'pointer'
    ;

subroutine
    : 'subroutine' IDENT NEWLINE decl* statement* 'end subroutine' NEWLINE+
    ;

statement
    : IDENT '=' expr NEWLINE+
    | fn_call NEWLINE+
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
    : '&' WS* COMMENT? NEWLINE -> skip
    ;

NEWLINE
    : '\r'? '\n'
    ;

WS
    : [ \t] -> skip
    ;
