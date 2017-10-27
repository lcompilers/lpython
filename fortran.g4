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

expr
    : ID '(' call_args? ')'
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

subroutine_call
    : 'call' ID '(' call_args? ')'
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
    : 'subroutine' ID ('(' param_list? ')')? NEWLINE+ decl* statements? 'end subroutine' NEWLINE+
    ;

function
    : 'pure'? 'recursive'? 'function' ID ('(' param_list? ')')? ('result' '(' ID ')')? NEWLINE+ decl* statements? 'end function' NEWLINE+
    ;

param_list
    : ID (',' ID)*
    ;

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
    : 'do' (ID '=' expr ',' expr (',' expr)?)? NEWLINE+ statements 'end' 'do'
    ;

print_statement
    : 'print' '*' (',' expr)*
    ;

number
    : NUMBER                    // Real number
    | '(' NUMBER ',' NUMBER ')' // Complex number
    ;

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
