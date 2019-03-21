# Developer Tutorial

This is a tutorial for anybody who wants to either develop LFortran or build
tools on top.

## Introduction

LFortran is structured around two independent modules, AST and ASR, both of
which are standalone (completely independent of the rest of LFortran) and users
are encouraged to use them independently for other applications and build tools
on top:

* Abstract Syntax Tree (AST), module `lfortran.ast`: Represents any Fortran
  source code, strictly based on syntax, no semantic is included. The AST
  module can convert itself to Fortran source code.

* Abstract Semantic Representation (ASR), module `lfortran.asr`: Represents a
  valid Fortran source code, all semantic is included. Invalid Fortran code is
  not allowed (an error will be given). The ASR module can convert itself to an
  AST.

## Abstract Syntax Tree (AST)

Fortran source code can be parsed into an AST using the `src_to_ast()`
function:
```python
from lfortran.ast import src_to_ast, print_tree
from lfortran.ast.ast_to_src import ast_to_src
src = """\
integer function f(a, b) result(r)
integer, intent(in) :: a, b
r = a + b
end function
"""
ast = src_to_ast(src, translation_unit=False)
```
We can pretty print it using `print_tree()` ([#59](https://gitlab.com/lfortran/lfortran/issues/59)):
```python
print_tree(ast)
```
This will print (in color):
```
Legend: Node, Field, Token
program_unit.Function
├─name='f'
├─args=↓
│ ├─AST.arg
│ │ ╰─arg='a'
│ ╰─AST.arg
│   ╰─arg='b'
├─return_type=None
├─return_var=expr.Name
│ ╰─id='r'
├─bind=None
├─use=[]
├─decl=↓
│ ╰─unit_decl2.Declaration
│   ╰─vars=↓
│     ├─AST.decl
│     │ ├─sym='a'
│     │ ├─sym_type='integer'
│     │ ├─dims=[]
│     │ ╰─attrs=↓
│     │   ╰─attribute.Attribute
│     │     ├─name='intent'
│     │     ╰─args=↓
│     │       ╰─AST.attribute_arg
│     │         ╰─arg='in'
│     ╰─AST.decl
│       ├─sym='b'
│       ├─sym_type='integer'
│       ├─dims=[]
│       ╰─attrs=↓
│         ╰─attribute.Attribute
│           ├─name='intent'
│           ╰─args=↓
│             ╰─AST.attribute_arg
│               ╰─arg='in'
├─body=↓
│ ╰─stmt.Assignment
│   ├─target=expr.Name
│   │ ╰─id='r'
│   ╰─value=expr.BinOp
│     ├─left=expr.Name
│     │ ╰─id='a'
│     ├─op=operator.Add
│     ╰─right=expr.Name
│       ╰─id='b'
╰─contains=[]
```
We can convert AST to Fortran source code using `ast_to_src()`:
```python
print(ast_to_src(ast))
```
This will print:
```fortran
function f(a, b) result(r)
integer, intent(in) :: a
integer, intent(in) :: b
r = a + b
end function
```
All AST nodes and their arguments are described in
[AST.asdl](https://gitlab.com/lfortran/lfortran/blob/master/grammar/AST.asdl).

## Abstract Semantic Representation (ASR)

We can convert AST to ASR using the `ast_to_asr()` function:
```python
from lfortran.semantic.ast_to_asr import ast_to_asr
asr = ast_to_asr(ast)
```
For example to print the variables defined in the function scope:
```python
print(asr.global_scope.symbols["f"].symtab.symbols.keys())
```
it prints:
```
dict_keys(['a', 'b', 'r'])
```
We can convert ASR to AST and to source code:
```python
from lfortran.asr.asr_to_ast import asr_to_ast
print(ast_to_src(asr_to_ast(asr)))
```
it prints:
```fortran
integer function f(a, b) result(r)
integer, intent(in) :: a
integer, intent(in) :: b
r = a + b
end function
```
All ASR nodes and their arguments are described in
[ASR.asdl](https://gitlab.com/lfortran/lfortran/blob/master/grammar/ASR.asdl).
