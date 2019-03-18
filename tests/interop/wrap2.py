"""
# Design

This file converts from a GFortran module file representation (documented in
the `lfortran.adapters.gfortran.mod` module) to an Abstract Semantic Representation (ASR).
"""

# TODO: move this into the lfortran package itself
import sys
sys.path.append("../..")

from lfortran.asr import asr
from lfortran.asr.asr_check import verify_asr
from lfortran.asr.asr_to_ast import asr_to_ast
from lfortran.ast.ast_to_src import ast_to_src
from lfortran.adapters.gfortran.mod import mod_to_asr
from lfortran.asr.builder import (make_translation_unit,
    translation_unit_make_module, scope_add_function, make_type_integer,
    make_type_real, type_eq, make_binop, scope_add_symbol)

u = mod_to_asr("mod1.mod")
a = asr_to_ast(u)
s = ast_to_src(a)
print(s)
