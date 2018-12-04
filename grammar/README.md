# Fortran Grammar Tools

This directory contains the Fortran parse tree grammar (fortran.g4) and the
Fortran AST description (Fortran.asdl). The first is used by ANTLR4 to generate
a Python parser, the second is used by `asdl_py.py` to generate Python AST
classes and visitors. All generates files are written into the `lfortran`
module as Python files and once they are generated, this `grammar` directory is
not needed anymore and `lfortran` becomes just a regular Python library that
has all the Python files it needs.
