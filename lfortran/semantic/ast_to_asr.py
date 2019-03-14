"""
Converts AST to ASR.

This is a work in progress module, which will eventually replace analyze.py
once it reaches feature parity with it.
"""

from ..ast import ast
from ..asr import asr
from ..asr.builder import (make_translation_unit,
        translation_unit_make_module, make_function, make_type_integer)

class SymbolTableVisitor(ast.GenericASTVisitor):

    def __init__(self):
        self._unit = make_translation_unit()
        self._current_module = translation_unit_make_module(self._unit, "modx")
        self._current_scope = self._current_module.symtab

    def visit_Function(self, node):
        args = [arg.arg for arg in node.args]
        if node.return_var:
            name = node.return_var # Name of the result(...) var
        else:
            name = node.name # Name of the function
        if node.return_type:
            if node.return_type == "integer":
                type = make_type_integer()
            else:
                raise NotImplementedError("Type not implemented")
        else:
            # FIXME: should return None here
            #type = None
            type = make_type_integer()
        return_var = asr.Variable(name=name, type=type)
        f = make_function(self._current_module, node.name,
                args=args, return_var=return_var)


def ast_to_asr(ast):
    v = SymbolTableVisitor()
    v.visit(ast)
    return v._unit
