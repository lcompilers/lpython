grammar fortran;

module
    : 'module' IDENT 'implicit none' private_decl? public_decl? 'end' IDENT?
    ;

private_decl
    : 'private'
    ;

public_decl
    : 'public' IDENT (',' IDENT)*
    ;

IDENT
    : ('a' .. 'z' | 'A' .. 'Z') ('a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_')*
    ;

WS
    : [ \t\r\n] -> skip
    ;
