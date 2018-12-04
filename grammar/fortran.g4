/*
This is a grammar for a subset of modern Fortran.

The file is organized in several sections, rules in each section only depend on
the same or further sections, never on the previous sections. As such any
section together with all the subsequent sections form a self-contained
grammar.
*/

grammar fortran;

// ----------------------------------------------------------------------------
// Top level rules to be used for parsing.
//
// * For compiling files use the 'root' rule:
// * For interactive usage use the 'unit' rule, which allows to parse shorter
//   pieces of code, much like Python
//

root
    : (module | program) EOF
    ;

units
    : ( NEWLINE* script_unit NEWLINE* (EOF | NEWLINE+ | ';' NEWLINE*))+ EOF
    ;

script_unit
    : module
    | program
    | subroutine
    | function
    | use_statement
    | var_decl
    | statement
    | expr
    ;

// ----------------------------------------------------------------------------
// Module definitions
//
// * private/public blocks
// * interface blocks
//

module
    : NEWLINE* KW_MODULE ID NEWLINE+ (use_statement NEWLINE+)* implicit_statement
        NEWLINE+ module_decl* contains_block? KW_END KW_MODULE ID?
        (EOF | NEWLINE+)
    ;

module_decl
    : private_decl
    | public_decl
    | var_decl
    | interface_decl
    ;

private_decl
    : KW_PRIVATE '::'? id_list? NEWLINE+
    ;

public_decl
    : KW_PUBLIC '::'? id_list? NEWLINE+
    ;

interface_decl
    : KW_INTERFACE ID NEWLINE+ (KW_MODULE KW_PROCEDURE id_list NEWLINE+)* KW_END KW_INTERFACE ID? NEWLINE+
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
    : NEWLINE* KW_PROGRAM ID NEWLINE+ sub_block
        KW_PROGRAM? ID? (EOF | NEWLINE+)
    ;

subroutine
    : KW_SUBROUTINE ID ('(' id_list? ')')? NEWLINE+ sub_block KW_SUBROUTINE ID?
        (EOF | NEWLINE+)
    ;

function
    : (var_type ('(' ID ')')?)? KW_PURE? KW_RECURSIVE?
        KW_FUNCTION ID ('(' id_list? ')')? (KW_RESULT '(' ID ')')? NEWLINE+
        sub_block KW_FUNCTION ID? (EOF | NEWLINE+)
    ;

sub_block
    : (use_statement NEWLINE+)* (implicit_statement NEWLINE+)? var_decl* statements contains_block? KW_END
    ;

contains_block
    : KW_CONTAINS NEWLINE+ sub_or_func+
    ;

sub_or_func
    : subroutine | function
    ;

implicit_statement
    : KW_IMPLICIT KW_NONE
    ;

use_statement
    : KW_USE use_symbol (',' KW_ONLY ':' use_symbol_list)?
    ;

use_symbol_list : use_symbol (',' use_symbol)* ;
use_symbol : ID | ID '=>' ID ;

id_list : ID (',' ID)* ;

var_decl
    : var_type ('(' (ID '=')? ('*' | ID) ')')? (',' var_modifier)* '::'?  var_sym_decl (',' var_sym_decl)* ';'* NEWLINE*
    ;

var_type
    : KW_INTEGER | KW_CHAR | KW_REAL | KW_COMPLEX | KW_LOGICAL | KW_TYPE
    | KW_CHARACTER
    ;

var_modifier
    : KW_PARAMETER | KW_INTENT | KW_DIMENSION array_decl? | KW_ALLOCATABLE | KW_POINTER
    | KW_PROTECTED | KW_SAVE | KW_CONTIGUOUS
    | KW_INTENT '(' (KW_IN | KW_OUT | KW_INOUT ) ')'
    ;

var_sym_decl
    : ident array_decl? ('=' expr)?
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
    : ';'* ( statement (NEWLINE+ | ';' NEWLINE*))*
    ;

statement
    : assignment_statement
    | exit_statement
    | cycle_statement
    | return_statement
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
    | error_stop_statement
    | ';'
    ;

assignment_statement
    : expr op=('='|'=>') expr
    ;

exit_statement
    : KW_EXIT
    ;

cycle_statement
    : KW_CYCLE
    ;

return_statement
    : KW_RETURN
    ;

subroutine_call
    : KW_CALL struct_member* ID '(' arg_list? ')'
    ;

builtin_statement
    : name=(KW_ALLOCATE | KW_OPEN | KW_CLOSE) '(' arg_list? ')'
    ;


if_statement
    : if_cond statement    # if_single_line
    | if_block KW_END KW_IF  # if_multi_line
    ;

if_cond: KW_IF '(' expr ')' ;

if_block
    : if_cond KW_THEN NEWLINE+ statements if_else_block?
    ;

if_else_block
    : KW_ELSE (if_block | (NEWLINE+ statements))
    ;

where_statement
    : where_cond statement        # where_single_line
    | where_block KW_END KW_WHERE # where_multi_line
    ;

where_cond: KW_WHERE '(' expr ')' ;

where_block
    : where_cond NEWLINE+ statements where_else_block?
    ;

where_else_block
    : KW_ELSE KW_WHERE? (where_block | (NEWLINE+ statements))
    ;

do_statement
    : KW_DO (ID '=' expr ',' expr (',' expr)?)? NEWLINE+ statements KW_END KW_DO
    ;

while_statement
    : KW_DO KW_WHILE '(' expr ')' NEWLINE* statements KW_END KW_DO
    ;

select_statement
    : KW_SELECT KW_CASE '(' expr ')' NEWLINE+ case_statement* select_default_statement? KW_END KW_SELECT
    ;

case_statement
    : KW_CASE '(' expr ')' NEWLINE+ statements
    ;

select_default_statement
    : KW_CASE KW_DEFAULT NEWLINE+ statements
    ;

print_statement
    : KW_PRINT ('*' | STRING) ',' expr_list?
    ;

write_statement
    : KW_WRITE '(' ('*' | expr) ',' ('*' | STRING) ')' expr_list?
    ;

stop_statement
    : KW_STOP STRING? NUMBER?
    ;

error_stop_statement
    : KW_ERROR KW_STOP STRING? NUMBER?
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
// ### primary
    : struct_member* fn_names '(' arg_list? ')'  # expr_fn_call
    | struct_member* ID '(' array_index_list ')' # expr_array_call
    | '[' expr_list ']'                          # expr_array_const
    | struct_member* ident                       # expr_id
    | number                                     # expr_number
    | op=('.true.' | '.false.')                  # expr_truefalse
    | STRING                                     # expr_string
    | '(' expr ')'                               # expr_nest // E.g. (1+2)*3

// ### level-1

// ### level-2
    | <assoc=right> expr '**' expr               # expr_pow
    | expr op=('*'|'/') expr                     # expr_muldiv
    | op=('+'|'-') expr                          # expr_unary
    | expr op=('+'|'-') expr                     # expr_addsub

// ### level-3
    | expr '//' expr                             # expr_string_conc

// ### level-4
    | expr op=('<'|'<='|'=='|'/='|'>='|'>' | '.lt.'|'.le.'|'.eq.'|'.neq.'|'.ge.'|'.gt.') expr # expr_rel

// ### level-5
    | '.not.' expr                               # expr_not
    | expr '.and.' expr                          # expr_and
    | expr '.or.' expr                           # expr_or
    | expr op=('.eqv.'|'.neqv') expr             # expr_eqv
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
    : expr              # array_index_simple
    | expr? ':' expr?   # array_index_slice
    ;

struct_member: ID '%' ;

number
    : NUMBER                    # number_real    // Real number
    | '(' NUMBER ',' NUMBER ')' # number_complex // Complex number
    ;

fn_names: ID | KW_REAL ; // real is both a type and a function name

ident
    : ID
    | KW_ALLOCATABLE
    | KW_ALLOCATE
    | KW_CALL
    | KW_CASE
    | KW_CHAR
    | KW_CHARACTER
    | KW_CLOSE
    | KW_COMPLEX
    | KW_CONTAINS
    | KW_CONTIGUOUS
    | KW_CYCLE
    | KW_DEFAULT
    | KW_DIMENSION
    | KW_DO
    | KW_ELSE
    | KW_END
    | KW_ERROR
    | KW_EXIT
    | KW_FUNCTION
    | KW_IF
    | KW_IMPLICIT
    | KW_IN
    | KW_INOUT
    | KW_INTEGER
    | KW_INTERFACE
    | KW_INTENT
    | KW_LOGICAL
    | KW_MODULE
    | KW_NONE
    | KW_ONLY
    | KW_OPEN
    | KW_OUT
    | KW_PARAMETER
    | KW_POINTER
    | KW_PRINT
    | KW_PRIVATE
    | KW_PROCEDURE
    | KW_PROGRAM
    | KW_PROTECTED
    | KW_PUBLIC
    | KW_PURE
    | KW_REAL
    | KW_RECURSIVE
    | KW_RESULT
    | KW_RETURN
    | KW_SAVE
    | KW_SELECT
    | KW_STOP
    | KW_SUBROUTINE
    | KW_THEN
    | KW_TYPE
    | KW_USE
    | KW_WHERE
    | KW_WHILE
    | KW_WRITE
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

// Keywords

fragment A : [aA] ;
fragment B : [bB] ;
fragment C : [cC] ;
fragment D : [dD] ;
fragment E : [eE] ;
fragment F : [fF] ;
fragment G : [gG] ;
fragment H : [hH] ;
fragment I : [iI] ;
fragment J : [jJ] ;
fragment K : [kK] ;
fragment L : [lL] ;
fragment M : [mM] ;
fragment N : [nN] ;
fragment O : [oO] ;
fragment P : [pP] ;
fragment Q : [qQ] ;
fragment R : [rR] ;
fragment S : [sS] ;
fragment T : [tT] ;
fragment U : [uU] ;
fragment V : [vV] ;
fragment W : [wW] ;
fragment X : [xX] ;
fragment Y : [yY] ;
fragment Z : [zZ] ;

KW_ALLOCATABLE: A L L O C A T A B L E;
KW_ALLOCATE: A L L O C A T E;
KW_CALL: C A L L;
KW_CASE: C A S E;
KW_CHAR: C H A R;
KW_CHARACTER: C H A R A C T E R;
KW_CLOSE: C L O S E;
KW_COMPLEX: C O M P L E X;
KW_CONTAINS: C O N T A I N S;
KW_CONTIGUOUS: C O N T I G U O U S;
KW_CYCLE: C Y C L E;
KW_DEFAULT: D E F A U L T;
KW_DIMENSION: D I M E N S I O N;
KW_DO: D O;
KW_ELSE: E L S E;
KW_END: E N D;
KW_ERROR: E R R O R;
KW_EXIT: E X I T;
KW_FUNCTION: F U N C T I O N;
KW_IF: I F;
KW_IMPLICIT: I M P L I C I T;
KW_IN: I N;
KW_INOUT: I N O U T;
KW_INTEGER: I N T E G E R;
KW_INTERFACE: I N T E R F A C E;
KW_INTENT: I N T E N T;
KW_LOGICAL: L O G I C A L;
KW_MODULE: M O D U L E;
KW_NONE: N O N E;
KW_ONLY: O N L Y;
KW_OPEN: O P E N;
KW_OUT: O U T;
KW_PARAMETER: P A R A M E T E R;
KW_POINTER: P O I N T E R;
KW_PRINT: P R I N T;
KW_PRIVATE: P R I V A T E;
KW_PROCEDURE: P R O C E D U R E;
KW_PROGRAM: P R O G R A M ;
KW_PROTECTED: P R O T E C T E D;
KW_PUBLIC: P U B L I C;
KW_PURE: P U R E;
KW_REAL: R E A L;
KW_RECURSIVE: R E C U R S I V E;
KW_RESULT: R E S U L T;
KW_RETURN: R E T U R N;
KW_SAVE: S A V E;
KW_SELECT: S E L E C T;
KW_STOP: S T O P;
KW_SUBROUTINE: S U B R O U T I N E;
KW_THEN: T H E N;
KW_TYPE: T Y P E;
KW_USE: U S E;
KW_WHERE: W H E R E;
KW_WHILE: W H I L E;
KW_WRITE: W R I T E;


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