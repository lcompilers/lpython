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
from lfortran.asr.builder import (make_translation_unit,
    translation_unit_make_module, scope_add_function, make_type_integer,
    make_type_real, type_eq, make_binop, scope_add_symbol)
from lfortran.asr.asr_to_ast import asr_to_ast
from lfortran.ast.ast_to_src import ast_to_src
import lfortran.adapters.gfortran.mod as gp

def string_to_type(stype, kind=None):
    """
    Converts a string `stype` and a numerical `kind` to an ASR type.
    """
    if stype == "integer":
        return make_type_integer(kind)
    elif stype == "real":
        return make_type_real(kind)
    elif stype == "complex":
        raise NotImplementedError("Complex not implemented")
    elif stype == "character":
        raise NotImplementedError("Complex not implemented")
    elif stype == "logical":
        raise NotImplementedError("Complex not implemented")
    raise Exception("Unknown type")

def tofortran_bound(symtab, b):
    if isinstance(b, gp.Integer):
        return asr.Num(n=b.i, type=make_type_integer())
    elif isinstance(b, gp.VarIdx):
        # TODO: maintain a lookup table of GFortran symbols and ASR symbols
        # and lookup the symbol first in ASR symbols here. For now, we
        # always construct a new variable, which means that two arrays
        # A(n), B(n) will have two distinct variables called "n".
        v = symtab[b.idx]
        w = asr.Variable(name=v.name, type=string_to_type(v.type))
        return w
    raise NotImplementedError("Unsupported bound type")

def convert_arg(table, idx):
    arg = table[idx]
    assert isinstance(arg.name, str)
    assert isinstance(arg.type, str)
    a = asr.Variable(name=arg.name, type=string_to_type(arg.type))
    assert isinstance(arg.intent, str)
    a.intent = arg.intent

    if arg.bounds:
        dims = []
        for bound in arg.bounds:
            lb, ub = bound
            if lb:
                lb = tofortran_bound(table, lb)
            if ub:
                ub = tofortran_bound(table, ub)
            dims.append(asr.dimension(lb, ub))
        a.type.dims = dims
    return a

def convert_function(symtab, table, f):
    assert isinstance(f, gp.Procedure)
    assert isinstance(f.name, str)
    assert isinstance(f.type, str)
    return_var = asr.Variable(name=f.name, type=string_to_type(f.type))
    args = []
    for arg in f.args:
        assert isinstance(arg, gp.VarIdx)
        args.append(convert_arg(table, arg.idx))
    scope_add_function(symtab, f.name, args=args, return_var=return_var)

def convert_module(table, public_symbols):
    # Determine the module name
    module_name = None
    for sym in public_symbols:
        s = table[sym.idx.idx]
        if isinstance(s, gp.Module):
            # Skip modules if they are listed in public symbols
            continue
        assert isinstance(s, gp.Procedure)
        if module_name:
            assert module_name == s.mod
        else:
            module_name = s.mod


    u = make_translation_unit()
    m = translation_unit_make_module(u, module_name)

    # Convert functions
    for sym in public_symbols:
        s = table[sym.idx.idx]
        if isinstance(s, gp.Module):
            continue
        assert isinstance(s, gp.Procedure)
        convert_function(m.symtab, table, s)

    return u

version, orig_file, table, public_symbols = gp.load_module("mod1.mod")
u = convert_module(table, public_symbols)
verify_asr(u)
a = asr_to_ast(u)
s = ast_to_src(a)
print(s)
