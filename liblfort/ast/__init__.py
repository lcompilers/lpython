"""
# AST module

This module provides classes to represent an AST, a `parse()` function that can
parse Fortran code and returns an AST and some other utility functions.

The AST classes are generated from `Fortran.asdl`. The `parse()` function
internally calls a particular parser implementation from the `parser` module,
however, the result of `parse()` is an AST that is independent of any
particular parser implementation. As such, this `ast` module serves as a
starting point for the compiler and other tools. The only thing one has to
understand is the AST, which is fully described by `Fortran.asdl`.
"""

from .utils import parse, parse_file, dump
