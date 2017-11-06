"""
Semantic analysis module
"""

from ..ast import ast

class SymbolTableVisitor(ast.GenericASTVisitor):

    def __init__(self):
        self.types = ["integer", "real", "complex", "character", "logical"]
        self.symbol_table = {}


    def visit_Declaration(self, node):
        for v in node.vars:
            sym = v.sym
            type_f = v.sym_type
            if type_f not in self.types:
                raise Exception("Type not implemented.")
            self.symbol_table[sym] = {"name": sym, "type": type_f}
