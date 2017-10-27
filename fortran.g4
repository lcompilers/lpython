grammar fortran;

root
    : module | program
    ;

program
    : 'program' IDENT NEWLINE+ use_statement* 'implicit none' NEWLINE+ decl* statements? ('contains' NEWLINE+ (subroutine|function)+ )?  'end program' IDENT? NEWLINE+ EOF
    ;

module
    : 'module' IDENT NEWLINE+ use_statement* 'implicit none' NEWLINE+ decl* ('contains' NEWLINE+ (subroutine|function)+ )?  'end module' IDENT? NEWLINE+ EOF
    ;

use_statement
    : 'use' IDENT (',' 'only' ':' param_list)? NEWLINE+
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
    : 'public' IDENT (',' IDENT)* NEWLINE+
    ;

var_decl
    : var_type ('(' IDENT ')')? (',' var_modifier)* '::' var_sym_decl (',' var_sym_decl)* NEWLINE+
    ;

var_sym_decl
    : IDENT array_decl? ('=' expr)?
    ;

array_decl
    : '(' array_comp_decl (',' array_comp_decl)* ')'
    ;

array_comp_decl
    : expr
    | ':'
    ;

interface_decl
    : 'interface' IDENT NEWLINE+ ('module' 'procedure' IDENT NEWLINE+)* 'end' 'interface' IDENT? NEWLINE+
    ;

expr
    : <assoc=right> expr '**' expr
    | expr ('*'|'/') expr
    | '-' expr
    | expr ('+'|'-') expr
    | number
    | '.not.' expr
    | expr ('<'|'<='|'=='|'/='|'>='|'>') expr
    | expr ('.and.'|'.or.') expr
    | logical_value
    | IDENT
    | STRING
    | fn_call       // E.g. f(5), can be a function call or array access
    | '(' expr ')'  // E.g. (1+2)*3
	;

// This can be a function call, or array access. Must be determined later based
// on the 'IDENT' definition.
fn_call
    : IDENT '(' call_args? ')'
    ;

subroutine_call
    : 'call' IDENT '(' call_args? ')'
    ;

call_args
    : call_arg (',' call_arg)*
    ;

call_arg
    : expr
    | ':'           // This is for arrays only, not function/subroutine calls
    ;

var_type
    : 'integer' | 'char' | 'real' | 'complex' | 'logical'
    ;

var_modifier
    : 'parameter' | 'intent' | 'dimension' | 'allocatable' | 'pointer'
    | 'intent' '(' ('in' | 'out' | 'inout' ) ')'
    | 'save'
    ;

subroutine
    : 'subroutine' IDENT ('(' param_list? ')')? NEWLINE+ decl* statements? 'end subroutine' NEWLINE+
    ;

function
    : 'pure'? 'recursive'? 'function' IDENT ('(' param_list? ')')? ('result' '(' IDENT ')')? NEWLINE+ decl* statements? 'end function' NEWLINE+
    ;

param_list
    : IDENT (',' IDENT)*
    ;

statements
    : (statement (NEWLINE+ | ';' NEWLINE*))+
    ;

statement
    : IDENT '=' expr
    | 'exit'
    | subroutine_call
    | if_statement
    | do_statement
    | where_statement
    | print_statement
    | ';'
    ;

if_statement
    : 'if' '(' expr ')' statement
    | if_block 'end' 'if'
    ;

if_block
    : 'if' '(' expr ')' 'then' NEWLINE+ statements else_block?
    ;

else_block
    : 'else' (if_block | (NEWLINE+ statements))
    ;

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
    : 'do' (IDENT '=' expr ',' expr (',' expr)?)? NEWLINE+ statements 'end' 'do'
    ;

print_statement
    : 'print' '*' (',' expr)*
    ;

number
    : NUMBER                    // Real number
    | '(' NUMBER ',' NUMBER ')' // Complex number
    ;

logical_value
    : '.true.'
    | '.false.'
    ;

NUMBER
    : ([0-9]+ '.' [0-9]* | '.' [0-9]+) ([eEdD] [+-]? [0-9]+)? ('_' IDENT)?
    |  [0-9]+                          ([eEdD] [+-]? [0-9]+)? ('_' IDENT)?
    ;

IDENT
    : ('a' .. 'z' | 'A' .. 'Z') ('a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_')*
    ;

STRING
    : '"' .*? '"'
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
