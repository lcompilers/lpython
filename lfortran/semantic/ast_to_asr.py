"""
Converts AST to ASR.

This is a work in progress module, which will eventually replace analyze.py
once it reaches feature parity with it.
"""

from ..ast import ast
from ..asr import asr
from ..asr.builder import (make_translation_unit,
    translation_unit_make_module, make_function, make_type_integer,
    make_type_real)

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


class SymbolTableVisitor(ast.GenericASTVisitor):

    def __init__(self):
        self._unit = make_translation_unit()
        self._current_module = translation_unit_make_module(self._unit, "modx")
        self._current_scope = self._current_module.symtab

    def visit_Function(self, node):
        args = [arg.arg for arg in node.args]
        if node.return_var:
            name = node.return_var.id # Name of the result(...) var
        else:
            name = node.name # Name of the function
        if node.return_type:
            type = string_to_type(node.return_type)
        else:
            # FIXME: should return None here
            #type = None
            type = make_type_integer()
        return_var = asr.Variable(name=name, type=type)
        f = make_function(self._current_module, node.name,
                args=args, return_var=return_var)

        old_scope = self._current_scope
        self._current_scope = f.symtab
        self.visit_sequence(node.decl)
        self._current_scope = old_scope

    def visit_Declaration(self, node):
        for v in node.vars:
            name = v.sym
            type = string_to_type(v.sym_type)
            dims = []
            for dim in v.dims:
                if isinstance(dim.end, ast.Num):
                    dims.append(int(dim.end.n))
                else:
                    raise NotImplementedError("Index not a number")
                if dim.start:
                    raise NotImplementedError("start index")
            #if len(dims) > 0:
            #    type = Array(type, dims)
            if name in self._current_scope.symbols:
                sym = self._current_scope.symbols[name]
            else:
                sym = asr.Variable(name=name, type=type, dummy=False)
                scope_add_symbol(self._current_scope, sym)
            assert sym.name == name
            for a in v.attrs:
                if a.name == "intent":
                    sym.intent = a.args[0].arg


def ast_to_asr(ast):
    v = SymbolTableVisitor()
    v.visit(ast)
    return v._unit
