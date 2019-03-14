"""
Converts AST to ASR.

This is a work in progress module, which will eventually replace analyze.py
once it reaches feature parity with it.
"""

from ..ast import ast
from ..asr import asr
from ..asr.builder import (make_translation_unit,
        translation_unit_make_module, make_function)

class SymbolTableVisitor(ast.GenericASTVisitor):

    def __init__(self):
        self._unit = make_translation_unit()
        self._current_module = translation_unit_make_module(self._unit, "modx")
        self._current_scope = self._current_module.symtab

    def visit_Function(self, node):
        args = [arg.arg for arg in node.args]
        return_var = "FIXME"
        f = make_function(self._current_module, node.name,
                args=args, return_var=return_var)


def ast_to_asr(ast):
    v = SymbolTableVisitor()
    v.visit(ast)
    return v._unit
