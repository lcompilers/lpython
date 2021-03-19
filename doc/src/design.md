# LFortran Design

## High Level Overview

LFortran is structured around two independent modules, AST and ASR, both of
which are standalone (completely independent of the rest of LFortran) and users
are encouraged to use them independently for other applications and build tools
on top:

* Abstract Syntax Tree (AST), module `lfortran.ast`: Represents any Fortran
  source code, strictly based on syntax, no semantic is included. The AST
  module can convert itself to Fortran source code.

* Abstract Semantic Representation (ASR), module `lfortran.asr`: Represents a valid
  Fortran source code, all semantic is included. Invalid Fortran code is not
  allowed (an error will be given). The ASR module can convert itself to an
  AST.

The LFortran compiler is then composed of the following independent stages:

* Parsing: converts Fortran source code to an AST
* Semantic: converts an AST to an ASR
* High level optimizations: optimize ASR to a possibly faster/simpler ASR
  (things like inlining functions, eliminating redundant expressions or
  statements, etc.)
* LLVM IR code generation and lower level optimizations: converts an ASR to an
  LLVM IR. This stage also does all other optimizations that do not produce an
  ASR, but still make sense to do before passing to LLVM IR.
* Machine code generation: LLVM then does all its optimizations and generates
  machine code (such as a binary executable, a library, an object file, or it
  is loaded and executed using JIT as part of the interactive LFortran session
  or in a Jupyter kernel).

LFortran is structured as a library, and so one can for example use the parser
to obtain an AST and do something with it, or one can then use the semantic
analyzer to obtain ASR and do something with it. One can generate the ASR
directly (e.g., from SymPy) and then either convert to AST and to a Fortran
source code, or use LFortran to compile it to machine code directly. In other
words, one can use LFortran to easily convert between the three equivalent
representations:

* Fortran source code
* Abstract Syntax Tree (AST)
* Abstract Semantic Representation (ASR)

They are all equivalent in the following sense:

* Any ASR can always be converted to an equivalent AST
* Any AST can always be converted to an equivalent Fortran source code
* Any Fortran source code can always be either converted to an equivalent AST
  or one gets a syntax error
* Any AST can always be either converted to an equivalent ASR or one gets a
  semantic error

So when a conversion can be done, they are equivalent, and the conversion can
always be done unless the code is invalid.

## ASR Design Details

The ASR is designed to have the following features:

* ASR is still semantically equivalent to the original Fortran code (it did not
  lose any semantic information). ASR can be converted to AST, and AST to
  Fortran source code which is functionally equivalent to the original.

* ASR is as simple as possible: it does not contain any information that could
  not be inferred from ASR.

* The ASR C++ classes (down the road) are designed similarly to SymEngine: they
  are constructed once and after that they are immutable. The constructor
  checks in Debug more that all the requirements are met (e.g., that all
  Variables in a Function have a dummy argument set, that explicit-shape arrays
  are not allocatable and all other Fortran requirements to make it a valid
  code), but in Release mode it quickly constructs the class without checks.
  Then there are builder classes that construct the ASR C++ classes to meet
  requirements (checked in Debug mode) and the builder gives an error message
  if a code is not a valid Fortran code, and if it doesn't give an error
  message, then the ASR C++ classes are constructed correctly. Thus by
  construction, the ASR classes always contain valid Fortran code and the rest
  of LFortran can depend on it.

## Notes:

Information that is lost when parsing source to AST:
whitespace, multiline/single line if statement distinction, case sensitivity of keywords.

Information that is lost when going from AST to ASR:
detailed syntax how variables were defined and the order of type attributes (whether array dimension is using the `dimension` attribute, or parentheses at the variable; or how many variables there are per declaration line or their order), as ASR only represents the aggregated type information in the symbol table.

ASR is the simplest way to generate Fortran code, as one does not
have to worry about the detailed syntax (as in AST) about how and where
things are declared. One specifies the symbol table for a module, then for
each symbol (functions, global variables, types, ...) one specifies the local
variables and if this is an interface then one needs to specify where one can
find an implementation, otherwise a body is supplied with statements, those
nodes are almost the same as in AST, except that each variable is just a
reference to a symbol in the symbol table (so by construction one cannot have
undefined variables). The symbol table for each node such as Function or Module
also references its parent (for example a function references a module,
a module references the global scope).

The ASR can be directly converted to an AST without gathering any other
information. And the AST directly to Fortran source code.

The ASR is always representing a semantically valid Fortran code.  This is
enforced by checks in the ASR C++ constructors (in Debug build).
When an ASR is used, one can assume it is valid.

## Fortran 2008

Fortran 2008 [standard](https://j3-fortran.org/doc/year/10/10-007.pdf) chapter
2 "Fortran concepts" specifies that Fortran code is a collection of _program
units_ (either all in one file, or in separate files), where each _program
unit_ is one of:

* main program
* module or submodule
* function or subroutine

Note: It can also be a _block data_ program unit, that is used to provide
initial values for data objects in named _common blocks_, but we do not
recommend the use of _common blocks_ (use modules instead).

## LFortran Extension

We extend the Fortran language by introducing a _global scope_, which is not
only the list of _program units_ (as in F2008) but can also include statements,
declarations, use statements and expressions. We define _global scope_ as a
collection of the following items:

* main program
* module or submodule
* function or subroutine
* use statement
* declaration
* statement
* expression

In addition, if a variable is not defined in an assignment statement (such as
`x = 5+3`) then the type of the variable is inferred from the right hand side
(e.g., `x` in `x = 5+3` would be of type `integer`, and `y` in `y = 5._dp`
would be of type `real(dp)`). This rule only applies at the top level of
_global scope_. Types must be fully specified inside main programs, modules,
functions and subroutines, just like in F2008.

The _global scope_ has its own symbol table. The main program and
module/submodule do not see any symbols from this symbol table. But functions,
subroutines, statements and expressions at the top level of _global scope_ use
and operate on this symbol table.

The _global scope_ has the following symbols predefined in the symbol table:

* the usual standard set of Fortran functions (such as `size`,
  `sin`, `cos`, ...)
* the `dp` double precision symbol, so that one can use `5._dp` for double
  precision.

Each item in the _global scope_ is interpreted as follows: main program is
compiled into an executable with the same name and executed; modules,
functions and subroutines are compiled and loaded; use statement and
declaration adds those symbols with the proper type into the _global scope_
symbol table, but do not generate any code; statement is wrapped into an
anonymous subroutine with no arguments, compiled, loaded and executed;
expression is wrapped into an anonymous function with no arguments returning
the expression, compiled, loaded, executed and the return value is returned
to the user.

The _global scope_ is always interpreted, item by item, per the previous
paragraph. It is meant to allow interactive usage, experimentations and
writing simple scripts. Code in _global scope_ must be interpreted using
`lfortran`. For more complex (production) code it is recommended to turn it
into modules and programs (by wrapping loose statements into subroutines or
functions and by adding type declarations) and compile it with `lfortran` or
any other Fortran compiler.

Here are some examples of valid code in _global scope_:

### Example 1

```fortran
a = 5
print *, a
```

### Example 2

```fortran
a = 5

subroutine p()
print *, a
end subroutine

call p()
```

### Example 3

```fortran
module a
implicit none
integer :: i
end module

use a, only: i
i = 5
```

### Example 4

```fortran
x = [1, 2, 3]
y = [1, 2, 1]
call plot(x, y, "o-")
```

## Design Considerations

The LFortran extension of Fortran was chosen in a way so as to minimize the
number of changes. In particular, only the top level of the _global scope_
has relaxed some of the Fortran rules (such as making specifying types
optional) so as to allow simple and quick interactive usage, but inside
functions, subroutines, modules or programs this relaxation does not apply.

The number of changes were kept to minimum in order to make it
straightforward to turn code at _global scope_ into standard compliant
Fortran code using programs and modules, so that it can be compiled by any
Fortran compiler.
