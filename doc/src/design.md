# LFortran Design

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